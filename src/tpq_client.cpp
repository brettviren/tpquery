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
//  Selftest

void
tpq_client_test (bool verbose)
{
    printf (" * tpq_client: ");
    if (verbose)
        printf ("\n");

    //  @selftest
    // TODO: fill this out
    tpq_client_t *client = tpq_client_new ();
    tpq_client_set_verbose(client, verbose);
    tpq_client_destroy (&client);
    //  @end
    printf ("OK\n");
}


//  ---------------------------------------------------------------------------
//  set_nickname
//

static void
set_nickname (client_t *self)
{
}


//  ---------------------------------------------------------------------------
//  use_connect_timeout
//

static void
use_connect_timeout (client_t *self)
{
}


//  ---------------------------------------------------------------------------
//  connect_to_server
//

static void
connect_to_server (client_t *self)
{
}


//  ---------------------------------------------------------------------------
//  signal_connected
//

static void
signal_connected (client_t *self)
{
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
}


//  ---------------------------------------------------------------------------
//  signal_result
//

static void
signal_result (client_t *self)
{
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
}


//  ---------------------------------------------------------------------------
//  signal_internal_error
//

static void
signal_internal_error (client_t *self)
{
}


//  ---------------------------------------------------------------------------
//  signal_unhandled_error
//

static void
signal_unhandled_error (client_t *self)
{
}
