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
    LICENSE FOR THIS PROJECT IS NOT DEFINED!

    Copyright (C) 2019- by tpquery Developers <bv@bnl.gov>

    Please edit license.xml and populate the 'license' tag with proper
    copyright and legalese contents, and regenerate the zproject.

    LICENSE FOR THIS PROJECT IS NOT DEFINED!
    =========================================================================
*/

#ifndef TPQ_CODEC_H_INCLUDED
#define TPQ_CODEC_H_INCLUDED

/*  These are the tpq_codec messages:

    HELLO - Greet a server.
        nickname            string      Client nickname

    HELLO_OK - Server return greeting.
        nickname            string      Client nickname

    ONE - Request based on one detid and one tstart.
        detid               number 4    A detid mask, matched via logical AND.
        tstart              number 8    A tstart value matched via binning.

    MANY - Request based on multiple detids and tstarts.
        detids              chunk       An array of 4 byte integers giving detids to match via logical AND.
        tstarts             chunk       An array of 8 byte integers giving tstarts to match via binned.

    BLOCK - Request a block of TPSets based on one detid, one tstart and a tspan.
        detid               number 4    A detid mask, matched via logical AND.
        tstart              number 8    A tstart value matched via binning.
        tspan               number 8    A tspan value matched via binning.

    PING -

    PONG -

    GOODBYE -

    GOODBYE_OK -

    ERROR - Command failed for some specific reason
        status              number 2    3-digit status code
        reason              string      Printable explanation
*/

#define TPQ_CODEC_SUCCESS                   200
#define TPQ_CODEC_STORED                    201
#define TPQ_CODEC_DELIVERED                 202
#define TPQ_CODEC_NOT_DELIVERED             300
#define TPQ_CODEC_CONTENT_TOO_LARGE         301
#define TPQ_CODEC_TIMEOUT_EXPIRED           302
#define TPQ_CODEC_CONNECTION_REFUSED        303
#define TPQ_CODEC_RESOURCE_LOCKED           400
#define TPQ_CODEC_ACCESS_REFUSED            401
#define TPQ_CODEC_NOT_FOUND                 404
#define TPQ_CODEC_COMMAND_INVALID           500
#define TPQ_CODEC_NOT_IMPLEMENTED           501
#define TPQ_CODEC_INTERNAL_ERROR            502

#define TPQ_CODEC_HELLO                     1
#define TPQ_CODEC_HELLO_OK                  2
#define TPQ_CODEC_ONE                       3
#define TPQ_CODEC_MANY                      4
#define TPQ_CODEC_BLOCK                     5
#define TPQ_CODEC_PING                      6
#define TPQ_CODEC_PONG                      7
#define TPQ_CODEC_GOODBYE                   8
#define TPQ_CODEC_GOODBYE_OK                9
#define TPQ_CODEC_ERROR                     10

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

//  Get/set the detid field
uint32_t
    tpq_codec_detid (tpq_codec_t *self);
void
    tpq_codec_set_detid (tpq_codec_t *self, uint32_t detid);

//  Get/set the tstart field
uint64_t
    tpq_codec_tstart (tpq_codec_t *self);
void
    tpq_codec_set_tstart (tpq_codec_t *self, uint64_t tstart);

//  Get a copy of the detids field
zchunk_t *
    tpq_codec_detids (tpq_codec_t *self);
//  Get the detids field and transfer ownership to caller
zchunk_t *
    tpq_codec_get_detids (tpq_codec_t *self);
//  Set the detids field, transferring ownership from caller
void
    tpq_codec_set_detids (tpq_codec_t *self, zchunk_t **chunk_p);

//  Get a copy of the tstarts field
zchunk_t *
    tpq_codec_tstarts (tpq_codec_t *self);
//  Get the tstarts field and transfer ownership to caller
zchunk_t *
    tpq_codec_get_tstarts (tpq_codec_t *self);
//  Set the tstarts field, transferring ownership from caller
void
    tpq_codec_set_tstarts (tpq_codec_t *self, zchunk_t **chunk_p);

//  Get/set the tspan field
uint64_t
    tpq_codec_tspan (tpq_codec_t *self);
void
    tpq_codec_set_tspan (tpq_codec_t *self, uint64_t tspan);

//  Get/set the status field
uint16_t
    tpq_codec_status (tpq_codec_t *self);
void
    tpq_codec_set_status (tpq_codec_t *self, uint16_t status);

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
