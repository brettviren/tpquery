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

//  ---------------------------------------------------------------------------
//  Forward declarations for the two main classes we use here

typedef struct _server_t server_t;
typedef struct _client_t client_t;

//  This structure defines the context for each running server. Store
//  whatever properties and structures you need for the server.

struct _server_t {
    //  These properties must always be present in the server_t
    //  and are set by the generated engine; do not modify them!
    zsock_t *pipe;              //  Actor pipe back to caller
    zconfig_t *config;          //  Current loaded configuration

    //  TODO: Add any properties you need here
    zsock_t* ingest;            // PTMP message input
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

//  Process server API method, return reply message if any

static zmsg_t *
server_method (server_t *self, const char *method, zmsg_t *msg)
{
    ZPROTO_UNUSED(self);
    ZPROTO_UNUSED(method);
    ZPROTO_UNUSED(msg);
    return NULL;
}

//  Apply new configuration.

static void
server_configuration (server_t *self, zconfig_t *config)
{
    ZPROTO_UNUSED(self);
    ZPROTO_UNUSED(config);
    //  Apply new configuration
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
//  set_result
//

static void
set_result (client_t *self)
{

}


//  ---------------------------------------------------------------------------
//  push_query
//

static void
push_query (client_t *self)
{

}


//  ---------------------------------------------------------------------------
//  fill_queue
//

static void
fill_queue (client_t *self)
{

}


//  ---------------------------------------------------------------------------
//  check_delayed
//

static void
check_delayed (client_t *self)
{

}


//  ---------------------------------------------------------------------------
//  purge_queue
//

static void
purge_queue (client_t *self)
{

}


//  ---------------------------------------------------------------------------
//  signal_command_invalid
//

static void
signal_command_invalid (client_t *self)
{

}
