/*  =========================================================================
    tpq_client - TP Query Client

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: tpq_client.xml, or
     * The code generation script that built this file: zproto_client_c
    ************************************************************************
    Copyright 2019.  May use and distribute according to LGPL v3 or later.
    =========================================================================
*/

#ifndef TPQ_CLIENT_H_INCLUDED
#define TPQ_CLIENT_H_INCLUDED

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef TPQ_CLIENT_T_DEFINED
typedef struct _tpq_client_t tpq_client_t;
#define TPQ_CLIENT_T_DEFINED
#endif

//  @interface
//  Create a new tpq_client, return the reference if successful, or NULL
//  if construction failed due to lack of available memory.
tpq_client_t *
    tpq_client_new (void);

//  Destroy the tpq_client and free all memory used by the object.
void
    tpq_client_destroy (tpq_client_t **self_p);

//  Return actor, when caller wants to work with multiple actors and/or
//  input sockets asynchronously.
zactor_t *
    tpq_client_actor (tpq_client_t *self);

//  Return message pipe for asynchronous message I/O. In the high-volume case,
//  we send methods and get replies to the actor, in a synchronous manner, and
//  we send/recv high volume message data to a second pipe, the msgpipe. In
//  the low-volume case we can do everything over the actor pipe, if traffic
//  is never ambiguous.
zsock_t *
    tpq_client_msgpipe (tpq_client_t *self);

//  Return true if client is currently connected, else false. Note that the
//  client will automatically re-connect if the server dies and restarts after
//  a successful first connection.
bool
    tpq_client_connected (tpq_client_t *self);

//  Connect to and say hello to server at endpoint, providing our nickname.
//  Returns >= 0 if successful, -1 if interrupted.
uint16_t
    tpq_client_say_hello (tpq_client_t *self, const char *nickname, const char *endpoint);

//  Request TPs.
//  Returns >= 0 if successful, -1 if interrupted.
uint16_t
    tpq_client_query (tpq_client_t *self, uint64_t tstart, uint64_t tspan, uint64_t detmask, uint32_t timeout);

//  Return last received seqno
uint32_t
    tpq_client_seqno (tpq_client_t *self);

//  Return last received status
uint16_t
    tpq_client_status (tpq_client_t *self);

//  Return last received payload
zmsg_t *
    tpq_client_payload (tpq_client_t *self);

//  Return last received payload and transfer ownership to caller
zmsg_t *
    tpq_client_get_payload (tpq_client_t *self);

//  Return last received reason
const char *
    tpq_client_reason (tpq_client_t *self);

//  Return last received reason and transfer ownership to caller
const char *
    tpq_client_get_reason (tpq_client_t *self);

//  Enable verbose tracing (animation) of state machine activity.
void
    tpq_client_set_verbose (tpq_client_t *self, bool verbose);

//  Self test of this class
void
    tpq_client_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
