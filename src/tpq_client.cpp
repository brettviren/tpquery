/*  =========================================================================
    tpq_client - TP Query Client

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
#include "../include/tpq_client.hpp"

//  Forward reference to method arguments structure
typedef struct _client_args_t client_args_t;

//  This structure defines the context for a client connection
typedef struct {
    //  These properties must always be present in the client_t
    //  and are set by the generated engine. The cmdpipe gets
    //  messages sent to the actor; the msgpipe may be used for
    //  faster asynchronous message flows.
    zsock_t *cmdpipe;           //  Command pipe to/from caller API
    zsock_t *msgpipe;           //  Message pipe to/from caller API
    zsock_t *dealer;            //  Socket to talk to server
    tpq_codec_t *message;       //  Message to/from server
    client_args_t *args;        //  Arguments from methods

    //  specific application properties
    int heartbeat_timer;        //  Timeout for heartbeats to server
    int retries;                //  How many heartbeats we've tried
    int seqno;
    uint64_t detmask;           // detmask of server

} client_t;

//  Include the generated client engine
#include "tpq_client_engine.inc"

//  Allocate properties and structures for a new client instance.
//  Return 0 if OK, -1 if failed

static int
client_initialize (client_t *self)
{
    self->heartbeat_timer = 1000;
    return 0;
}

//  Free properties and structures for a client instance

static void
client_terminate (client_t *self)
{
    //  Destroy properties here
}



//  ---------------------------------------------------------------------------
//  set_nickname
//

static void
set_nickname (client_t *self)
{
    //zsys_debug("client nickname is %s", self->args->nickname);
    tpq_codec_set_nickname(self->message, self->args->nickname);
}


//  ---------------------------------------------------------------------------
//  use_connect_timeout
//

static void
use_connect_timeout (client_t *self)
{
    engine_set_timeout (self, self->args->timeout);
}


//  ---------------------------------------------------------------------------
//  connect_to_server
//

static void
connect_to_server (client_t *self)
{
    if (zsock_connect (self->dealer, "%s", self->args->endpoint)) {
        engine_set_exception (self, bad_endpoint_event);
        zsys_warning ("could not connect to %s", self->args->endpoint);
    }
}


//  ---------------------------------------------------------------------------
//  signal_connected
//

static void
signal_connected (client_t *self)
{
    self->detmask = tpq_codec_detmask(self->message);
    // zsys_debug("tpq_client: connected to server with detmask 0x%lx",
    //            self->detmask);
    zsock_send (self->cmdpipe, "si8", "CONNECTED", 0, self->detmask);
}


//  ---------------------------------------------------------------------------
//  client_is_connected
//

static void
client_is_connected (client_t *self)
{
    self->retries = 0;
    engine_set_connected (self, true);
    engine_set_timeout (self, self->heartbeat_timer);
}


//  ---------------------------------------------------------------------------
//  set_query
//

static void
set_query (client_t *self)
{
    tpq_codec_set_seqno(self->message, ++self->seqno);
    tpq_codec_set_tstart(self->message, self->args->tstart);
    tpq_codec_set_tspan(self->message, self->args->tspan);
    tpq_codec_set_detmask(self->message, self->args->detmask);
    tpq_codec_set_timeout(self->message, self->args->timeout);
}


//  ---------------------------------------------------------------------------
//  signal_result
//

static void
signal_result (client_t *self)
{
    int seqno = tpq_codec_seqno(self->message);
    int status = tpq_codec_status(self->message);
    zmsg_t* msg = tpq_codec_payload(self->message);

    // zsys_debug("tpq_client: signal_result: seqno %d, status %d, %ld objs",
    //            seqno, status, zmsg_size(msg));
    assert(msg);
    msg = zmsg_dup(msg);
    assert(msg);    
    zsock_send(self->cmdpipe, "s42p", "RESULT",
               seqno, status, msg);
}


//  ---------------------------------------------------------------------------
//  signal_success
//

static void
signal_success (client_t *self)
{
}


//  ---------------------------------------------------------------------------
//  check_if_connection_is_dead
//

static void
check_if_connection_is_dead (client_t *self)
{
    if (++self->retries >= 3) {
        engine_set_timeout (self, 0);
        engine_set_connected (self, false);
        engine_set_exception (self, exception_event);
    }
}


//  ---------------------------------------------------------------------------
//  check_status_code
//

static void
check_status_code (client_t *self)
{
    if (tpq_codec_status (self->message) == TPQ_CODEC_COMMAND_INVALID) {
        engine_set_next_event (self, command_invalid_event);
    }
    else {
        engine_set_next_event (self, other_event);
    }
}


//  ---------------------------------------------------------------------------
//  signal_internal_error
//

static void
signal_internal_error (client_t *self)
{
    zsock_send (self->cmdpipe, "sis", "FAILURE", -1, "Internal server error");
    zsock_send (self->msgpipe, "sis", "FAILURE", -1, "Internal server error");
}


//  ---------------------------------------------------------------------------
//  signal_unhandled_error
//

static void
signal_unhandled_error (client_t *self)
{
}



//  ---------------------------------------------------------------------------
//  signal_bad_endpoint
//

static void
signal_bad_endpoint (client_t *self)
{
}


//  ---------------------------------------------------------------------------
//  Selftest

#include "json.hpp"
using json = nlohmann::json;

#include "tpq_test_util.inc"
void
tpq_client_test (bool verbose)
{
    zsys_init();
    if (verbose) 
        zsys_debug("tpq_client: ");

    const char* server_address = "ipc://@/tpq-server-client";
    const char* spigot_address = "ipc://@/tpq-spigot-server";
    
    zsock_t *spigot = zsock_new(ZMQ_PUSH);
    zsock_bind(spigot, spigot_address, NULL);

    uint64_t detmask = 0xFFFFFFFFFFFFFFFF;
    zactor_t* server = s_setup_server(server_address, spigot_address,
                                      detmask, verbose);

    // Send some TPSets in to server
    s_tpset_send(spigot, 1000, 100);
    s_tpset_send(spigot, 2000, 100);
    s_tpset_send(spigot, 3000, 100);
    s_tpset_send(spigot, 4000, 100);
    s_tpset_send(spigot, 5000, 100);
    s_tpset_send(spigot, 6000, 100);

    // TODO: fill this out
    tpq_client_t *client = tpq_client_new ();
    tpq_client_set_verbose(client, verbose);

    int rc = tpq_client_say_hello(client, "tpq-test-client",
                                  server_address);
    assert(rc >= 0);
    if (verbose)
        zsys_debug("tpq-client-test: sent hello");

    rc = tpq_client_query(client, 4000, 100, 0xFFFFFFFFFFFFFFFF, 10000);
    assert (rc >= 0);

    if (verbose) {
        zsys_debug("tpq-client-test: seqno %d, status %d",
                   tpq_client_seqno(client),
                   tpq_client_status(client));
    }
    zmsg_t* msg = tpq_client_payload(client);
    assert(msg);
    if (verbose) {
        zsys_debug("tpq-client-test: %ld objects",
                   zmsg_size(msg));
    }

    tpq_client_destroy (&client);
    zactor_destroy(&server);
    zsock_destroy(&spigot);

    //  @end
    printf ("OK\n");
}
