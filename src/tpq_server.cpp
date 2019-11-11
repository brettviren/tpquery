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
    payload_t(zmsg_t* m) : _msg(m) {
        // deserialize to get detid and time interval
        ptmp::data::TPSet tpset;
        ptmp::internals::recv_keep(_msg, tpset);
        tstart = tpset.tstart();
        tspan = tpset.tspan();
        detid = tpset.detid();
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

    immap_t iq;                 // interval queue
    size_t iq_lwm{10000};       // low water mark iterative size
    size_t iq_hwm{20000};       // high water mark iterative size

    request_queue_t pending;    // pending client requests

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

    self->ingest = NULL;
    return 0;
}

//  Free properties and structures for a server instance

static void
server_terminate (server_t *self)
{
    ZPROTO_UNUSED(self);
    //  Destroy properties here
}

static int
s_server_handle_ingest(zloop_t* loop, zsock_t* reader, void* varg);

static
int64_t poplong(zmsg_t *msg)
{
    char *value = zmsg_popstr (msg);
    const int64_t ret = atol(value);
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
        char* cfg = zmsg_popstr(msg);
        self->ingest = ptmp::internals::endpoint(cfg);
        if (self->ingest) {
            engine_handle_socket(self, self->ingest, s_server_handle_ingest);
        }
        else {
            zsys_error("tpq_server: failed to create ingest socket\n%s", cfg);
        }
        free(cfg);
        reply = zmsg_new();
        zmsg_addstrf(reply, "INGEST %s", self->ingest ? "OK" : "FAILED");
    }

    else if (streq (method, "DETMASK")) {
        self->detmask = poplong(msg);
        reply = zmsg_new();
        zmsg_addstr(reply, "DETMASK OK");
    }

    else if (streq (method, "QUEUE")) {
        self->iq_lwm = poplong(msg);
        self->iq_hwm = poplong(msg);
        reply = zmsg_new();
        zmsg_addstr(reply, "QUEUE OK");
    }

    else {
        zsys_error ("unknown server method '%s', fix calling code", method);
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

    // 1. slurp new
    while (!(zsock_events(ingest) & ZMQ_POLLIN)) {
        zmsg_t* msg = zmsg_recv(ingest); // ptmp message
        auto ptr = std::make_shared<payload_t>(msg);
        if (ptr->detid & self->detmask) {
            self->iq += std::make_pair(ptr->interval(), imset_t{ptr});
        }
    }
    // 2. check queued, execute "result available" on client
    int64_t queue_beg = boost::icl::lower(self->iq);
    int64_t queue_end = boost::icl::upper(self->iq);
    std::vector<request_queue_t::iterator> tokill;
    for (auto it = self->pending.begin(); it != self->pending.end(); ++it) {
        int64_t tend = it->first;
        if (tend >= queue_end) {
            break;
        }
        for (auto& req : it->second) {
            tpq_codec_set_seqno(req.client->message, req.seqno);
            tpq_codec_set_status(req.client->message, 200);
            engine_send_event(req.client, query_is_satisfied_event);
        }
        tokill.push_back(it);
    }
    for (auto dead : tokill) {
        self->pending.erase(dead);
    }

    // 3. maybe purge
    if (queue_end - queue_beg > self->iq_hwm) {
        self->iq -= iminterval_t::right_open(0, queue_end - self->iq_lwm);
    }

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
//  Selftest

void
tpq_server_test (bool verbose)
{
    printf (" * tpq_server: ");
    if (verbose)
        printf ("\n");

    //  @selftest
    zactor_t *server = zactor_new (tpq_server, (char*)"server");
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "BIND", "ipc://@/tpq_server", NULL);

    zsock_t *client = zsock_new (ZMQ_DEALER);
    assert (client);
    zsock_set_rcvtimeo (client, 2000);
    zsock_connect (client, "ipc://@/tpq_server");

    //  TODO: fill this out
    tpq_codec_t *request = tpq_codec_new ();
    tpq_codec_destroy (&request);

    zsock_destroy (&client);
    zactor_destroy (&server);
    //  @end
    printf ("OK\n");
}


//  ---------------------------------------------------------------------------
//  set_coverage
//

static void
set_coverage (client_t *self)
{

}


//  ---------------------------------------------------------------------------
//  check_query
//

static void
check_query (client_t *self)
{

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
    iminterval_t want = iminterval_t::right_open(tbeg, tend);
    auto got = server->iq & want;
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
    tpq_codec_set_payload(self->message, &plmsg);
}


//  ---------------------------------------------------------------------------
//  push_query
//

static void
push_query (client_t *self)
{
    const int64_t tbeg = tpq_codec_tstart(self->message);
    const int64_t tend = tpq_codec_tspan(self->message) + tbeg;
    const uint64_t detmask = tpq_codec_detmask(self->message);
    uint32_t seqno = tpq_codec_seqno(self->message);
    self->server->pending[tend].push_back({self, detmask, tbeg, seqno});
}


//  ---------------------------------------------------------------------------
//  signal_command_invalid
//

static void
signal_command_invalid (client_t *self)
{

}


//  ---------------------------------------------------------------------------
//  handle_query
//

static void
handle_query (client_t *self)
{
    uint64_t want_detmask = tpq_codec_detmask(self->message);
    if (! want_detmask & self->server->detmask) {
        // return 500
        engine_set_next_event(self, query_is_bogus_event);        
    }
    int64_t query_beg = tpq_codec_tstart(self->message);
    int64_t query_end = tpq_codec_tspan(self->message) + query_beg;

    int64_t queue_beg = boost::icl::lower(self->server->iq);
    int64_t queue_end = boost::icl::upper(self->server->iq);
    if (query_end <= queue_beg) {
        // return 404, not found
        engine_set_next_event(self, query_is_fully_late_event);        
    }

    else if (query_end <= queue_end) {
        if (query_beg < queue_end) {
            // return 206, partial content
            tpq_codec_set_status(self->message, 206);
            engine_set_next_event(self, query_is_partly_late_event);
        }
        else {
            // return 200, success
            tpq_codec_set_status(self->message, 206);
            engine_set_next_event(self, query_is_satisfied_event);
        }
    }        

    else {
        // add to pending
        engine_set_next_event(self, query_is_delayed_event);
    }
}
