/*  =========================================================================
    tpq_server - TP Query Server

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: tpq_server.xml, or
     * The code generation script that built this file: zproto_server_c
    ************************************************************************
    Copyright 2019.  May use and distribute according to LGPL v3 or later.
    =========================================================================
*/

#ifndef TPQ_SERVER_H_INCLUDED
#define TPQ_SERVER_H_INCLUDED

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  To work with tpq_server, use the CZMQ zactor API:
//
//  Create new tpq_server instance, passing logging prefix:
//
//      zactor_t *tpq_server = zactor_new (tpq_server, "myname");
//
//  Destroy tpq_server instance
//
//      zactor_destroy (&tpq_server);
//
//  Enable verbose logging of commands and activity:
//
//      zstr_send (tpq_server, "VERBOSE");
//
//  Bind tpq_server to specified endpoint. TCP endpoints may specify
//  the port number as "*" to acquire an ephemeral port:
//
//      zstr_sendx (tpq_server, "BIND", endpoint, NULL);
//
//  Return assigned port number, specifically when BIND was done using an
//  an ephemeral port:
//
//      zstr_sendx (tpq_server, "PORT", NULL);
//      char *command, *port_str;
//      zstr_recvx (tpq_server, &command, &port_str, NULL);
//      assert (streq (command, "PORT"));
//
//  Specify configuration file to load, overwriting any previous loaded
//  configuration file or options:
//
//      zstr_sendx (tpq_server, "LOAD", filename, NULL);
//
//  Set configuration path value:
//
//      zstr_sendx (tpq_server, "SET", path, value, NULL);
//
//  Save configuration data to config file on disk:
//
//      zstr_sendx (tpq_server, "SAVE", filename, NULL);
//
//  Send zmsg_t instance to tpq_server:
//
//      zactor_send (tpq_server, &msg);
//
//  Receive zmsg_t instance from tpq_server:
//
//      zmsg_t *msg = zactor_recv (tpq_server);
//
//  This is the tpq_server constructor as a zactor_fn:
//
void
    tpq_server (zsock_t *pipe, void *args);

//  Self test of this class
void
    tpq_server_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
