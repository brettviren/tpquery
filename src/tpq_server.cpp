/*  =========================================================================
    tpq_server - tpq_server

    Copyright 2019.  May use and distribute according to LGPL v3 or later.
    =========================================================================
*/

/*
@header
    Description of class for man page.
@discuss
    Detailed discussion of the class, if any.
@end
*/

#include "tpquery_classes.hpp"
//  TODO: Change these to match your project's needs
#include "../include/tpq_codec.hpp"
#include "../include/tpq_server.hpp"

#include "ptmp/internals.h"
#include "icl/interval.hpp"
#include "icl/interval_map.hpp"

#include "json.hpp"
using json = nlohmann::json;

#include <map>
#include <vector>
#include <algorithm>
#include <unordered_set>

typedef int64_t imkey_t;
typedef boost::icl::interval<imkey_t>::interval_type iminterval_t;

// this holds a TPSet object as a zmsg.  It is kept in the interval
// map via a shared_ptr.
struct payload_t {
    zmsg_t* _msg;
    int64_t tstart;
    int64_t tspan;
    uint64_t detid;             // actually 32bit in msg
    uint32_t count;
    payload_t(zmsg_t* m) : _msg(m) {
        // deserialize to get detid and time interval
        ptmp::data::TPSet tpset;
        ptmp::internals::recv_keep(_msg, tpset);
        tstart = tpset.tstart();
        tspan = tpset.tspan();
        detid = tpset.detid();
        count = tpset.count();
    }
    ~payload_t() {
        zmsg_destroy(&_msg);
    }

    iminterval_t interval() const {
        return iminterval_t::right_open(tstart, tstart+tspan);
    }

    // Return original message as encoded frame ready to append to
    // another message.  Caller takes ownership of returned frame
    zframe_t* encode() const {
        return zmsg_encode(_msg);
    }
};

typedef std::shared_ptr<payload_t> imvalue_t;
typedef std::set<imvalue_t> imset_t;
typedef boost::icl::interval_map<imkey_t, imset_t> immap_t;


//  ---------------------------------------------------------------------------
//  Forward declarations for the two main classes we use here

typedef struct _server_t server_t;
typedef struct _client_t client_t;

struct request_t {
    client_t* client;
    uint64_t detmask;
    int64_t tstart;             // tend is the key
    uint32_t seqno;
};
// Keyed by END of requested interval.
// Any pending requests with keys after queue end may be serviced.
// Multiple requests may have intervals with coinciding ends.
typedef std::map<imkey_t, std::vector<request_t> > request_queue_t;
typedef std::unordered_map<uint64_t, uint64_t> count_check_t;

//  This structure defines the context for each running server. Store
//  whatever properties and structures you need for the server.

struct _server_t {
    //  These properties must always be present in the server_t
    //  and are set by the generated engine; do not modify them!
    zsock_t *pipe;              //  Actor pipe back to caller
    zconfig_t *config;          //  Current loaded configuration

    //  server properties
    zsock_t* ingest;            // PTMP message input
    uint64_t detmask;           // the detector ID mask supported by server

    immap_t *iq;                // interval queue
    uint64_t iq_lwm;              // low water mark time span
    uint64_t iq_hwm;              // high water mark time span

    request_queue_t *pending;    // pending client requests

    int64_t first_seen;
    count_check_t* count_check;
};

//  ---------------------------------------------------------------------------
//  This structure defines the state for each client connection. It will
//  be passed to each action in the 'self' argument.

struct _client_t {
    //  These properties must always be present in the client_t
    //  and are set by the generated engine; do not modify them!
    server_t *server;           //  Reference to parent server
    tpq_codec_t *message;       //  Message in and out
    uint  unique_id;            //  Client identifier (for correlation purpose with the engine)

    //  TODO: Add specific properties for your application
};

//  Include the generated server engine
#include "tpq_server_engine.inc"

//  Allocate properties and structures for a new server instance.
//  Return 0 if OK, or -1 if there was an error.

static int
server_initialize (server_t *self)
{
    ZPROTO_UNUSED(self);
    //  Construct properties here

    ptmp::internals::set_thread_name("tpq_server");

    // accept all by default
    self->detmask = 0xffffffffffffffff; 
    // some default
    self->iq_lwm = 100000000;
    self->iq_hwm = 200000000;
    // need to explicitly construct C++ bits as server_t is simply malloc'ed
    self->iq = new immap_t;
    self->pending = new request_queue_t;
    self->first_seen = 0;
    self->count_check = new count_check_t;
    self->ingest = NULL;
    return 0;
}

//  Free properties and structures for a server instance

static void
server_terminate (server_t *self)
{
    //  Destroy properties 
    if (self->ingest) {
        zsock_destroy(&self->ingest);
    }
    if (self->iq) {
        delete self->iq;
    }
    if (self->pending) {
        delete self->pending;
    }
    if (self->count_check) {
        delete self->count_check;
    }
}

static int
s_server_handle_ingest(zloop_t* loop, zsock_t* reader, void* varg);

static
uint64_t poplong(zmsg_t *msg)
{
    char *value = zmsg_popstr (msg);
    const uint64_t ret = strtoul(value, NULL, 10);
    free (value);
    return ret;
}

//  Process server API method, return reply message if any

static zmsg_t *
server_method (server_t *self, const char *method, zmsg_t *msg)
{
    zmsg_t* reply = NULL;

    if (streq (method, "INGEST")) {
        if (self->ingest) {
            // how to "unhandle_socket()" ?
            zsock_destroy(&self->ingest);
        }
        char* cfgstr = zmsg_popstr(msg);
        json jcfg = json::parse(cfgstr);
        std::string input = jcfg["input"].dump();
        self->ingest = ptmp::internals::endpoint(input);
        if (self->ingest) {
            engine_handle_socket(self, self->ingest, s_server_handle_ingest);
        }
        else {
            zsys_error("tpq_server: failed to create ingest socket\n%s",
                       input.c_str());
            assert(self->ingest);
        }
        free(cfgstr);
        reply = zmsg_new();
        zmsg_addstrf(reply, "INGEST %s", self->ingest ? "OK" : "FAILED");
    }

    else if (streq (method, "DETMASK")) {
        self->detmask = poplong(msg);
        if (engine_verbose(self))
            zsys_debug("tpq_server: set detmask to 0x%lx", self->detmask);
        assert(self->detmask > 0);
        reply = zmsg_new();
        zmsg_addstr(reply, "DETMASK OK");
    }

    else if (streq (method, "QUEUE")) {
        self->iq_lwm = poplong(msg);
        self->iq_hwm = poplong(msg);
        if (engine_verbose(self))
            zsys_debug("tpq_server: queue lwm=%ld hwm=%ld", self->iq_lwm, self->iq_hwm);
        reply = zmsg_new();
        zmsg_addstr(reply, "QUEUE OK");
    }

    else {
        zsys_error ("tqp_server: unknown server method '%s', fix calling code", method);
        assert (false);         // fix calling code!
    }
    return reply;
}

//  Apply new configuration.

static void
server_configuration (server_t *self, zconfig_t *config)
{
    ZPROTO_UNUSED(self);
    ZPROTO_UNUSED(config);
    //  Apply new configuration
}

static int
s_server_handle_ingest(zloop_t* loop, zsock_t* ingest, void* varg)
{
    server_t *self = (server_t *) varg;
    const bool super_verbose = false; // true here for super verbose
    const bool verbose = engine_verbose(self);
    const bool check_count = true; // make configurable

    auto t0 = ptmp::data::now();

    // 1. slurp new
    int nslurped = 0;
    while (zsock_events(ingest) & ZMQ_POLLIN) {
        zmsg_t* msg = zmsg_recv(ingest); // ptmp message
        auto ptr = std::make_shared<payload_t>(msg);
        // zsys_debug("tpq_server: slurp: %8d %lx %lx",
        //            nslurped, ptr->detid, ptr->detid & self->detmask);
        if ( ! (ptr->detid & self->detmask) ) {
            continue;
        }

        (*self->iq) += std::make_pair(ptr->interval(), imset_t{ptr});
        ++nslurped;
        if (! self->first_seen) {
            // keep this just to help debugging.
            self->first_seen = ptr->tstart;
        }
        
        // tpq doesn't actually care.
        if (check_count) {    
            const uint64_t last_count = (*self->count_check)[ptr->detid];
            if (verbose and last_count and ptr->count - last_count != 1) {
                zsys_debug("tpq_server: dropped: last: %d, this: %d, "
                           "detid: 0x%x, tstart: %.3f Mtick",
                           last_count, ptr->count,
                           ptr->detid, 1e-6*(ptr->tstart - self->first_seen));
            }
            (*self->count_check)[ptr->detid] = ptr->count;
        }

        if (super_verbose) {
            int64_t queue_end = boost::icl::upper((*self->iq));
            int64_t queue_beg = boost::icl::lower((*self->iq));
            zsys_debug("\t%8d: queue:%ld + %ld, entry: %ld + %ld @ %ld",
                       nslurped,queue_beg, queue_end-queue_beg,
                       ptr->tstart, ptr->tspan,
                       ptr->tstart - queue_beg);
        }
    }

    int64_t queue_end = boost::icl::upper((*self->iq));
    int64_t queue_beg = boost::icl::lower((*self->iq));
    if (super_verbose and (nslurped or self->pending->size())) {
        zsys_debug("tpq_server: ingested: %d, queue: %.3f + %.3f Mticks, pending: %ld", 
                   nslurped,
                   1e-6*(queue_beg-self->first_seen),
                   1e-6*(queue_end-queue_beg),
                   self->pending->size());
    }
    // 2. check queued, execute "result available" on client
    std::vector<request_queue_t::iterator> tokill;
    for (auto it = self->pending->begin(); it != self->pending->end(); ++it) {
        int64_t tend = it->first;
        if (tend > queue_end) {
            //zsys_debug("\tpending: %ld > %ld", tend, queue_end);
            break;
        }
        for (auto& req : it->second) {
            if (verbose) {
                zsys_debug("tpq_server: ingest: send #%d query %.3f + %.3f Mtick",
                           req.seqno,
                           1e-6*(req.tstart - self->first_seen),
                           1e-6*(tend - req.tstart));
            }
            tpq_codec_set_seqno(req.client->message, req.seqno);
            tpq_codec_set_status(req.client->message, 200);
            engine_send_event(req.client, query_is_satisfied_event);
        }
        tokill.push_back(it);
    }
    for (auto dead : tokill) {
        self->pending->erase(dead);
    }

    // 3. maybe purge.
    const int64_t qspan = queue_end - queue_beg;
    if (qspan > self->iq_hwm) {
        const int64_t new_beg = queue_end - self->iq_lwm;
        // remove anything prior to new beggining
        (*self->iq) -= iminterval_t::right_open(0, new_beg);

        int64_t nend = boost::icl::upper((*self->iq));
        int64_t nbeg = boost::icl::lower((*self->iq));
        if (verbose)
            zsys_debug("tpq_server: purge from queue: %.3f + %.3f -> %.3f + %.3f Mtick",
                       1e-6*(queue_beg-self->first_seen),
                       1e-6*(queue_end-queue_beg),
                       1e-6*(nbeg-self->first_seen),
                       1e-6*(nend-nbeg));
    }

    auto t1 = ptmp::data::now();

    /// With a 100ms artifical throttle, the server reduces CPU from
    /// 100% to about 50%.
    // const int64_t throttle = 100000;
    // if (throttle) { 
    //     if (t1-t0 < throttle) {
    //         auto tosleep = throttle - (t1-t0);
    //         ptmp::internals::microsleep(tosleep);
    //         t1 = ptmp::data::now();
    //     }
    // }

    if (verbose)
        zsys_debug("tpq_server: took %ld us", t1-t0);
    

    return 0;
}

//  Allocate properties and structures for a new client connection and
//  optionally engine_set_next_event (). Return 0 if OK, or -1 on error.

static int
client_initialize (client_t *self)
{
    ZPROTO_UNUSED(self);
    //  Construct properties here
    return 0;
}

//  Free properties and structures for a client connection

static void
client_terminate (client_t *self)
{
    ZPROTO_UNUSED(self);
    //  Destroy properties here
}


//  ---------------------------------------------------------------------------
//  set_payload
//

bool payload_lt(const imvalue_t& a, const imvalue_t& b)
{
    return a->tstart < b->tstart;
}

static void
set_payload (client_t *self)
{
    uint64_t tbeg = tpq_codec_tstart(self->message);
    uint64_t tend = tpq_codec_tspan(self->message) + tbeg;
    server_t* server = self->server;
    auto want = iminterval_t::right_open(tbeg, tend);
    auto got = (*server->iq) & want;
    std::set<imvalue_t> unique;
    for (auto one : got) {
        unique.insert(one.second.begin(), one.second.end());
    }
    std::vector<imvalue_t> ordered(unique.begin(), unique.end());
    std::sort(ordered.begin(), ordered.end(), payload_lt);
    zmsg_t* plmsg = zmsg_new();
    for (auto one : ordered) {
        zframe_t* frame = one->encode();
        zmsg_append(plmsg, &frame);
    }
    int64_t queue_end = boost::icl::upper((*server->iq));
    int64_t queue_beg = boost::icl::lower((*server->iq));
    if (engine_verbose(self->server))
        zsys_debug("tpq_server: payload %.3f + %.3f Mtick with %ld frames from queue %.3f + %.3f Mtick",
                   1e-6*(tbeg - server->first_seen),
                   1e-6*(tend-tbeg),
                   ordered.size(),
                   1e-6*(queue_beg - server->first_seen),
                   1e-6*(queue_end-queue_beg));
    tpq_codec_set_payload(self->message, &plmsg);
}

//  ---------------------------------------------------------------------------
//  handle_query
//

static void
handle_query (client_t *self)
{
    const uint32_t seqno = tpq_codec_seqno(self->message);
    const uint64_t want_detmask = tpq_codec_detmask(self->message);
    if (! want_detmask & self->server->detmask) {
        // return 500
        zsys_error("tpq_server: query: detmask mismatch");
        engine_set_next_event(self, query_is_bogus_event);        
        return;
    }
    const int64_t query_beg = tpq_codec_tstart(self->message);
    const int64_t query_end = tpq_codec_tspan(self->message) + query_beg;

    const int64_t queue_beg = boost::icl::lower(*self->server->iq);
    const int64_t queue_end = boost::icl::upper(*self->server->iq);
    // zsys_debug("tpq_server: query: %ld %ld", query_beg, query_end);
    // zsys_debug("tpq_server: queue: %ld %ld", queue_beg, queue_end);

    if (query_end <= queue_beg) {
        // return 404, not found
        if (engine_verbose(self->server))
            zsys_debug("tpq_server: query: fully late: "
                       "query: %.3f + %.3f,  queue: %.3f + %.3f Mtick",
                       1e-6*(query_beg-self->server->first_seen),
                       1e-6*(query_end - query_beg),
                       1e-6*(queue_beg-self->server->first_seen),
                       1e-6*(queue_end - queue_beg));
        engine_set_next_event(self, query_is_fully_late_event);        
        return;
    }

    else if (query_end <= queue_end) {
        if (query_beg < queue_beg) {
            // return 206, partial content
            if (engine_verbose(self->server))
                zsys_debug("tpq_server: query: partly late: "
                           "query: %.3f + %.3f,  queue: %.3f + %.3f Mtick",
                           1e-6*(query_beg-self->server->first_seen),
                           1e-6*(query_end - query_beg),
                           1e-6*(queue_beg-self->server->first_seen),
                           1e-6*(queue_end - queue_beg));

            tpq_codec_set_status(self->message, TPQ_CODEC_PARTIAL_CONTENT);
            engine_set_next_event(self, query_is_partly_late_event);
            return;
        }
        else {
            // return 200, success
            if (engine_verbose(self->server))
                zsys_debug("tpq_server: query: got one: "
                           "query: %.3f + %.3f,  queue: %.3f + %.3f Mtick",
                           1e-6*(query_beg-self->server->first_seen),
                           1e-6*(query_end - query_beg),
                           1e-6*(queue_beg-self->server->first_seen),
                           1e-6*(queue_end - queue_beg));
            tpq_codec_set_status(self->message, TPQ_CODEC_SUCCESS);
            engine_set_next_event(self, query_is_satisfied_event);
            return;
        }
    }        

    else {
        // add to pending
        if (engine_verbose(self->server))
            zsys_debug("tpq_server: query: pending: "
                       "query: %.3f + %.3f,  queue: %.3f + %.3f Mtick",
                           1e-6*(query_beg-self->server->first_seen),
                           1e-6*(query_end - query_beg),
                           1e-6*(queue_beg-self->server->first_seen),
                           1e-6*(queue_end - queue_beg));
        (*self->server->pending)[query_end].push_back({self, want_detmask, query_beg, seqno});
        return;
    }
}



//  ---------------------------------------------------------------------------
//  set_not_found
//

static void
set_not_found (client_t *self)
{
    zmsg_t* empty = zmsg_new();
    tpq_codec_set_payload(self->message, &empty);
    tpq_codec_set_status(self->message, TPQ_CODEC_NOT_FOUND);
}


//  ---------------------------------------------------------------------------
//  set_command_invalid
//

static void
set_command_invalid (client_t *self)
{
    zmsg_t* empty = zmsg_new();
    tpq_codec_set_payload(self->message, &empty);
    tpq_codec_set_status(self->message, TPQ_CODEC_COMMAND_INVALID);
}


//  ---------------------------------------------------------------------------
//  set_coverage
//

static void
set_coverage (client_t *self)
{
    tpq_codec_set_detmask(self->message, self->server->detmask);
}

//  ---------------------------------------------------------------------------
//  Selftest

#include "tpq_test_util.inc"
void
tpq_server_test (bool verbose)
{
    zsys_init();
    zsys_debug("tpq_server: ");

    const char* server_address = "ipc://@/tpq-server";
    const char* spigot_address = "ipc://@/tpq-spigot";

    zsock_t *spigot = zsock_new(ZMQ_PUSH);
    zsock_bind(spigot, spigot_address, NULL);

    //  @selftest
    uint64_t detmask = 0xFFFFFFFFFFFFFFFF;
    zactor_t* server = s_setup_server(server_address, spigot_address,
                                      detmask, verbose);

    zsock_t *client = zsock_new (ZMQ_DEALER);
    assert (client);
    zsock_set_rcvtimeo (client, 2000);
    zsock_connect (client, server_address, NULL);

    // Send some TPSets in to server
    s_tpset_send(spigot, 1000, 100);
    s_tpset_send(spigot, 2000, 100);
    s_tpset_send(spigot, 3000, 100);
    s_tpset_send(spigot, 4000, 100);
    s_tpset_send(spigot, 5000, 100);
    s_tpset_send(spigot, 6000, 100);
    
    tpq_codec_t *request = tpq_codec_new ();

    {                           // trigger valgrind if this leaks
        zmsg_t* empty = NULL;
        empty = zmsg_new();
        tpq_codec_set_payload(request, &empty);
        empty = zmsg_new();
        tpq_codec_set_payload(request, &empty);
        empty = zmsg_new();
        tpq_codec_set_payload(request, &empty);
        empty = zmsg_new();
        tpq_codec_set_payload(request, &empty);
    }


    ptmp::data::TPSet tpset;
    int rc = 0;
    
    // hello
    tpq_codec_set_id(request, TPQ_CODEC_HELLO);
    tpq_codec_set_nickname(request, "tpq_server_test");
    rc = tpq_codec_send(request, client);
    assert (rc == 0);

    rc = tpq_codec_recv(request, client);
    assert(rc == 0);
    assert(tpq_codec_id(request) == TPQ_CODEC_COVERAGE);
    assert(tpq_codec_detmask(request) == detmask);
    
    // query partly late.
    tpq_codec_set_id(request, TPQ_CODEC_QUERY);
    tpq_codec_set_seqno(request, 1);
    tpq_codec_set_tstart(request, 0);
    tpq_codec_set_tspan(request, 5001); // intervals are right-open
    tpq_codec_set_detmask(request, detmask);
    rc = tpq_codec_send(request, client);
    assert (rc == 0);
    
    tpq_codec_recv(request, client);
    assert(tpq_codec_id(request) == TPQ_CODEC_RESULT);    
    assert(tpq_codec_seqno(request) == 1);
    zmsg_t* msg = tpq_codec_payload(request);
    assert(msg);
    if (verbose)
        zsys_debug("payload message has %ld frames", zmsg_size(msg));
    assert(zmsg_size(msg) == 2); // should have 4000 and (just barely) 5000

    tpset = s_tpset_recv(zmsg_first(msg));
    assert(tpset.tstart() == 4000);
    tpset = s_tpset_recv(zmsg_next(msg));
    assert(tpset.tstart() == 5000);
    

    // query delayed.
    tpq_codec_set_id(request, TPQ_CODEC_QUERY);
    tpq_codec_set_seqno(request, 2);
    tpq_codec_set_tstart(request, 4000);
    tpq_codec_set_tspan(request, 3000); // intervals are right-open
    tpq_codec_set_detmask(request, detmask);
    rc = tpq_codec_send(request, client);
    assert (rc == 0);
    
    zsys_debug("Note: should see 'tpq_codec: interrupted' next");
    rc = tpq_codec_recv(request, client);
    assert(rc == -1);           // should be interrupted

    s_tpset_send(spigot, 7000, 100); // should trigger delayed send
    rc = tpq_codec_recv(request, client);
    assert(rc == 0);            // should NOT be interrupted

    if (verbose)
        zsys_debug("Got back id %d", tpq_codec_id(request));
    assert(tpq_codec_id(request) == TPQ_CODEC_RESULT);    
    assert(tpq_codec_seqno(request) == 2);
    msg = tpq_codec_payload(request);
    assert(msg);
    if (verbose)
        zsys_debug("payload message has %ld frames", zmsg_size(msg));
    assert(zmsg_size(msg) == 3); // should have 4000 and (just barely) 5000

    tpset = s_tpset_recv(zmsg_first(msg));
    assert(tpset.tstart() == 4000);
    tpset = s_tpset_recv(zmsg_next(msg));
    assert(tpset.tstart() == 5000);
    tpset = s_tpset_recv(zmsg_next(msg));
    assert(tpset.tstart() == 6000);

    tpq_codec_destroy (&request);


    zsock_destroy (&spigot);
    zsock_destroy (&client);
    zactor_destroy (&server);
    //  @end
    printf ("OK\n");
}


