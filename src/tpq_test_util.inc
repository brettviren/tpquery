

#include "ptmp/data.h"

static void
s_tpset_send(zsock_t* sock, uint64_t tstart, uint32_t tspan)
{
    static int count = 0;
    ++count;
    ptmp::data::TPSet tpset;
    tpset.set_count(count);
    tpset.set_created(ptmp::data::now());
    tpset.set_detid(0xdeadbeaf);
    tpset.set_tstart(tstart);
    tpset.set_tspan(tspan);
    ptmp::internals::send(sock, tpset);
}

static 
ptmp::data::TPSet s_tpset_recv(zframe_t* frame)
{
    zmsg_t* msg = zmsg_decode(frame);
    assert(msg);
    ptmp::data::TPSet tpset;
    ptmp::internals::recv(&msg, tpset);
    assert(tpset.detid() == 0xdeadbeaf);
    return tpset;
}

static
zactor_t * s_setup_server(const char* server_address,
                          const char* spigot_address,
                          uint64_t detmask, bool verbose)
{
    zactor_t *server = zactor_new (tpq_server, (char*)"server");
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "BIND", server_address, NULL);

    json server_config = {
        { "input", {
                { "socket", {
                        { "connect", spigot_address },
                        { "type", "PULL" } } } } } };

    std::string tmp = server_config.dump();
    zstr_sendx(server, "INGEST", tmp.c_str(), NULL);
    char* answer = zstr_recv(server);
    if (verbose)
        zsys_debug(answer);
    assert(streq(answer, "INGEST OK"));
    free (answer);

    zsock_send(server, "s8", "DETMASK", detmask);
    answer = zstr_recv(server);
    if (verbose)
        zsys_debug(answer);    
    assert(streq(answer, "DETMASK OK"));
    free (answer);

    zsock_send(server, "s88", "QUEUE", 2100, 4100);
    answer = zstr_recv(server);
    if (verbose)
        zsys_debug(answer);    
    assert(streq(answer, "QUEUE OK"));
    free (answer);
    return server;
}
