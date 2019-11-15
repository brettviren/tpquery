/*  =========================================================================
    tpq_codec - Trigger Primitive Queue Codec

    Codec header for tpq_codec.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: tpq_codec.xml, or
     * The code generation script that built this file: zproto_codec_c
    ************************************************************************
    Copyright 2019.  May use and distribute according to LGPL v3 or later.
    =========================================================================
*/

#ifndef TPQ_CODEC_H_INCLUDED
#define TPQ_CODEC_H_INCLUDED

/*  These are the tpq_codec messages:

    HELLO - Greet a server.
        nickname            string      Client nickname

    COVERAGE - Declare TP input coverage supported by a server.
        detmask             number 8    Detector ID mask

    QUERY - Request TPSets
        seqno               number 4    A message sequence number.
        tstart              number 8    A tstart value.
        tspan               number 8    A tspan value.
        detmask             number 8    Detector ID mask
        timeout             number 4    Timeout in msec

    RESULT -
        seqno               number 4    A message sequence number.
        status              number 2    The 3-digit status code related to the result.
        payload             msg         A msg holding one frame per TPSet.

    PING -

    PONG -

    GOODBYE -

    GOODBYE_OK -

    ERROR - Command failed for some specific reason
        status              number 2    3-digit status code
        reason              string      Printable explanation
*/

#define TPQ_CODEC_SUCCESS                   200
#define TPQ_CODEC_PARTIAL_CONTENT           206
#define TPQ_CODEC_NOT_FOUND                 404
#define TPQ_CODEC_COMMAND_INVALID           500
#define TPQ_CODEC_NOT_IMPLEMENTED           501
#define TPQ_CODEC_INTERNAL_ERROR            502

#define TPQ_CODEC_HELLO                     1
#define TPQ_CODEC_COVERAGE                  2
#define TPQ_CODEC_QUERY                     3
#define TPQ_CODEC_RESULT                    4
#define TPQ_CODEC_PING                      5
#define TPQ_CODEC_PONG                      6
#define TPQ_CODEC_GOODBYE                   7
#define TPQ_CODEC_GOODBYE_OK                8
#define TPQ_CODEC_ERROR                     9

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef TPQ_CODEC_T_DEFINED
typedef struct _tpq_codec_t tpq_codec_t;
#define TPQ_CODEC_T_DEFINED
#endif

//  @interface
//  Create a new empty tpq_codec
tpq_codec_t *
    tpq_codec_new (void);

//  Create a new tpq_codec from zpl/zconfig_t *
tpq_codec_t *
    tpq_codec_new_zpl (zconfig_t *config);

//  Destroy a tpq_codec instance
void
    tpq_codec_destroy (tpq_codec_t **self_p);

//  Create a deep copy of a tpq_codec instance
tpq_codec_t *
    tpq_codec_dup (tpq_codec_t *other);

//  Receive a tpq_codec from the socket. Returns 0 if OK, -1 if
//  the read was interrupted, or -2 if the message is malformed.
//  Blocks if there is no message waiting.
int
    tpq_codec_recv (tpq_codec_t *self, zsock_t *input);

//  Send the tpq_codec to the output socket, does not destroy it
int
    tpq_codec_send (tpq_codec_t *self, zsock_t *output);



//  Print contents of message to stdout
void
    tpq_codec_print (tpq_codec_t *self);

//  Export class as zconfig_t*. Caller is responsibe for destroying the instance
zconfig_t *
    tpq_codec_zpl (tpq_codec_t *self, zconfig_t* parent);

//  Get/set the message routing id
zframe_t *
    tpq_codec_routing_id (tpq_codec_t *self);
void
    tpq_codec_set_routing_id (tpq_codec_t *self, zframe_t *routing_id);

//  Get the tpq_codec id and printable command
int
    tpq_codec_id (tpq_codec_t *self);
void
    tpq_codec_set_id (tpq_codec_t *self, int id);
const char *
    tpq_codec_command (tpq_codec_t *self);

//  Get/set the nickname field
const char *
    tpq_codec_nickname (tpq_codec_t *self);
void
    tpq_codec_set_nickname (tpq_codec_t *self, const char *value);

//  Get/set the detmask field
uint64_t
    tpq_codec_detmask (tpq_codec_t *self);
void
    tpq_codec_set_detmask (tpq_codec_t *self, uint64_t detmask);

//  Get/set the seqno field
uint32_t
    tpq_codec_seqno (tpq_codec_t *self);
void
    tpq_codec_set_seqno (tpq_codec_t *self, uint32_t seqno);

//  Get/set the tstart field
uint64_t
    tpq_codec_tstart (tpq_codec_t *self);
void
    tpq_codec_set_tstart (tpq_codec_t *self, uint64_t tstart);

//  Get/set the tspan field
uint64_t
    tpq_codec_tspan (tpq_codec_t *self);
void
    tpq_codec_set_tspan (tpq_codec_t *self, uint64_t tspan);

//  Get/set the timeout field
uint32_t
    tpq_codec_timeout (tpq_codec_t *self);
void
    tpq_codec_set_timeout (tpq_codec_t *self, uint32_t timeout);

//  Get/set the status field
uint16_t
    tpq_codec_status (tpq_codec_t *self);
void
    tpq_codec_set_status (tpq_codec_t *self, uint16_t status);

//  Get a copy of the payload field
zmsg_t *
    tpq_codec_payload (tpq_codec_t *self);
//  Get the payload field and transfer ownership to caller
zmsg_t *
    tpq_codec_get_payload (tpq_codec_t *self);
//  Set the payload field, transferring ownership from caller
void
    tpq_codec_set_payload (tpq_codec_t *self, zmsg_t **msg_p);

//  Get/set the reason field
const char *
    tpq_codec_reason (tpq_codec_t *self);
void
    tpq_codec_set_reason (tpq_codec_t *self, const char *value);

//  Self test of this class
void
    tpq_codec_test (bool verbose);
//  @end

//  For backwards compatibility with old codecs
#define tpq_codec_dump      tpq_codec_print

#ifdef __cplusplus
}
#endif

#endif
