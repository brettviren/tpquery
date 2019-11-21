// This file specifies layers in a ptmper graph to test out tpquery.
// It creates a somewhat silly graph where the TPSet streams split
// into two.  One going into a tpqsvc and one into a tpqclt.  The 
// latter then queries the former at some rate driven by its input.

local ptmp = import "ptmp.jsonnet";

// user serviceable, adapt to local file source
local links = std.range(1,10);
local apas = [4,5,6];
local datadir = "/data/fast/bviren/ptmp-dumps/2019-11-12/";
local filepat = "run10306-felix%(apa)d%(link)02d.dump";


// The rest can be left alone.

local datapat = datadir + filepat;

local map2 = function(f, arr1, arr2) std.map(function (a1)
                                             std.map(function (a2)
                                                     f(a1,a2),
                                                     arr2),
                                             arr1);
local patmapper = function(pat) function(a,l) pat%{apa:a,link:l};

local mapals = function(pat) map2(patmapper(pat), apas, links);


// URL patterns for the BIND ends.
local url_file = "inproc://file%(apa)d%(link)02d";
local url_replay  = "tcp://127.0.0.1:50%(apa)d%(link)02d";
local url_winbulk = "tcp://127.0.0.1:51%(apa)d00";
local url_wintrig = "tcp://127.0.0.1:52%(apa)d00";
local url_tpqsvc  = "tcp://127.0.0.1:53%(apa)d00";

local files = function(apa) std.map(function(link) {
    local al={apa:apa,link:link},
    local name="file%(apa)d%(link)02d"%al,
    res: ptmp.czmqat(name,
                     osocket = ptmp.socket('bind','pair', url_file%al),
                     cfg = {
                         name:name,
                         ifile: datapat%al
                     })}.res, links);


local replays = function(apa) std.map(function(link) {
    local al={apa:apa,link:link},
    res: ptmp.replay("replay%(apa)d%(link)02d"%al,
                     isocket = ptmp.socket('connect','pair', url_file%al),
                     osocket = ptmp.socket('bind','pub', url_replay%al)),
}.res, links);

local windows = function(npat, url, cfg) function(apa) std.map(function(link) {
    local al={apa:apa,link:link},    
    res: ptmp.window(npat%al,
                     isocket = ptmp.socket('connect','sub', url_replay%al),
                     osocket = ptmp.socket('connect','pub', url%al)),
}.res, links);

local servers = function(apa) {
    local al={apa:apa},
    res: ptmp.nodeconfig("tpqsvc", "tpqsvc%d"%apa,
                         isocket = ptmp.socket('bind','sub', url_winbulk%al),
                         osocket = null,
                         cfg = {
                             name: "tpqsvc%d"%apa,
                             verbose: 1,
                             service: url_tpqsvc%al,
                             detmask: "-1", // parsed as string by svc to uint64
                             queue: {lwm:5e8, hwm:10e8},
                         }),
}.res;

local clients = function(apa) {
    local al={apa:apa},
    res: ptmp.nodeconfig("tpqclt", "tpqclt%d"%apa,
                         isocket = ptmp.socket('bind','sub', url_wintrig%al),
                         osocket = null,
                         cfg = {
                             name: "tpqclt%d"%apa,
                             // fixme: replace with all APAs to up the challenge
                             service: url_tpqsvc%{apa:apa},
                             minbias: 0.01,
                         }),
}.res;

local generators2d = [
    files,
    replays, 
    windows("bulk%(apa)d%(link)02d", url_winbulk, {tspan:1000/0.02, tbuf:5000/0.02}),
    windows("trig%(apa)d%(link)02d", url_wintrig, {tspan:50/0.02, tbuf:5000/0.02}),
];
local generators1d = [
    servers,
    clients,
];

local cfgfilepat = "tpqsvc-test-apa%(apa)d.json";
local mapgens = function(gens, apa) [g(apa) for g in gens];
//[mapgens(generators, apa) for apa in apas]
//std.flattenArrays(mapgens(generators2d, 5))




{
    // jsonnet -m $outdir -e 'local p=import "tpqsvc-test.jsonnet"; p.cfggen'
    cfggen:: {
        [cfgfilepat%{apa:apa}] : {
            name: "apa%d"%apa,
            plugins: ["tpquery"],
            proxies: std.flattenArrays(mapgens(generators2d, apa))
                + mapgens(generators1d, apa),
        } for apa in apas
    },

    // jsonnet -S -e 'local p=import "tpqsvc-test.jsonnet"; p.procgen' > $outdir/Procfile
    procgen :: std.join('\n',[("apa%(apa)d: ptmper "+cfgfilepat)%{apa:apa} for apa in apas])
}

                    
