/*  =========================================================================
    tpq_codec - Trigger Primitive Queue Codec

    Codec class for tpq_codec.

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

/*
@header
    tpq_codec - Trigger Primitive Queue Codec
@discuss
@end
*/

#ifdef NDEBUG
#undef NDEBUG
#endif

#include "../include/tpq_codec.hpp"

//  Structure of our class

struct _tpq_codec_t {
    zframe_t *routing_id;               //  Routing_id from ROUTER, if any
    int id;                             //  tpq_codec message ID
    byte *needle;                       //  Read/write pointer for serialization
    byte *ceiling;                      //  Valid upper limit for read pointer
    char nickname [256];                //  Client nickname
    uint64_t detmask;                   //  Detector ID mask
    uint32_t seqno;                     //  A message sequence number.
    uint64_t tstart;                    //  A tstart value.
    uint64_t tspan;                     //  A tspan value.
    uint32_t timeout;                   //  Timeout in msec
    uint16_t status;                    //  The 3-digit status code related to the result.
    zmsg_t *payload;                    //  A msg holding one frame per TPSet.
    char reason [256];                  //  Printable explanation
};

//  --------------------------------------------------------------------------
//  Network data encoding macros

//  Put a block of octets to the frame
#define PUT_OCTETS(host,size) { \
    memcpy (self->needle, (host), size); \
    self->needle += size; \
}

//  Get a block of octets from the frame
#define GET_OCTETS(host,size) { \
    if (self->needle + size > self->ceiling) { \
        zsys_warning ("tpq_codec: GET_OCTETS failed"); \
        goto malformed; \
    } \
    memcpy ((host), self->needle, size); \
    self->needle += size; \
}

//  Put a 1-byte number to the frame
#define PUT_NUMBER1(host) { \
    *(byte *) self->needle = (byte) (host); \
    self->needle++; \
}

//  Put a 2-byte number to the frame
#define PUT_NUMBER2(host) { \
    self->needle [0] = (byte) (((host) >> 8)  & 255); \
    self->needle [1] = (byte) (((host))       & 255); \
    self->needle += 2; \
}

//  Put a 4-byte number to the frame
#define PUT_NUMBER4(host) { \
    self->needle [0] = (byte) (((host) >> 24) & 255); \
    self->needle [1] = (byte) (((host) >> 16) & 255); \
    self->needle [2] = (byte) (((host) >> 8)  & 255); \
    self->needle [3] = (byte) (((host))       & 255); \
    self->needle += 4; \
}

//  Put a 8-byte number to the frame
#define PUT_NUMBER8(host) { \
    self->needle [0] = (byte) (((host) >> 56) & 255); \
    self->needle [1] = (byte) (((host) >> 48) & 255); \
    self->needle [2] = (byte) (((host) >> 40) & 255); \
    self->needle [3] = (byte) (((host) >> 32) & 255); \
    self->needle [4] = (byte) (((host) >> 24) & 255); \
    self->needle [5] = (byte) (((host) >> 16) & 255); \
    self->needle [6] = (byte) (((host) >> 8)  & 255); \
    self->needle [7] = (byte) (((host))       & 255); \
    self->needle += 8; \
}

//  Get a 1-byte number from the frame
#define GET_NUMBER1(host) { \
    if (self->needle + 1 > self->ceiling) { \
        zsys_warning ("tpq_codec: GET_NUMBER1 failed"); \
        goto malformed; \
    } \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

//  Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) { \
        zsys_warning ("tpq_codec: GET_NUMBER2 failed"); \
        goto malformed; \
    } \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
           +  (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

//  Get a 4-byte number from the frame
#define GET_NUMBER4(host) { \
    if (self->needle + 4 > self->ceiling) { \
        zsys_warning ("tpq_codec: GET_NUMBER4 failed"); \
        goto malformed; \
    } \
    (host) = ((uint32_t) (self->needle [0]) << 24) \
           + ((uint32_t) (self->needle [1]) << 16) \
           + ((uint32_t) (self->needle [2]) << 8) \
           +  (uint32_t) (self->needle [3]); \
    self->needle += 4; \
}

//  Get a 8-byte number from the frame
#define GET_NUMBER8(host) { \
    if (self->needle + 8 > self->ceiling) { \
        zsys_warning ("tpq_codec: GET_NUMBER8 failed"); \
        goto malformed; \
    } \
    (host) = ((uint64_t) (self->needle [0]) << 56) \
           + ((uint64_t) (self->needle [1]) << 48) \
           + ((uint64_t) (self->needle [2]) << 40) \
           + ((uint64_t) (self->needle [3]) << 32) \
           + ((uint64_t) (self->needle [4]) << 24) \
           + ((uint64_t) (self->needle [5]) << 16) \
           + ((uint64_t) (self->needle [6]) << 8) \
           +  (uint64_t) (self->needle [7]); \
    self->needle += 8; \
}

//  Put a string to the frame
#define PUT_STRING(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER1 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a string from the frame
#define GET_STRING(host) { \
    size_t string_size; \
    GET_NUMBER1 (string_size); \
    if (self->needle + string_size > (self->ceiling)) { \
        zsys_warning ("tpq_codec: GET_STRING failed"); \
        goto malformed; \
    } \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}

//  Put a long string to the frame
#define PUT_LONGSTR(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER4 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a long string from the frame
#define GET_LONGSTR(host) { \
    size_t string_size; \
    GET_NUMBER4 (string_size); \
    if (self->needle + string_size > (self->ceiling)) { \
        zsys_warning ("tpq_codec: GET_LONGSTR failed"); \
        goto malformed; \
    } \
    free ((host)); \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}

//  --------------------------------------------------------------------------
//  bytes cstring conversion macros

// create new array of unsigned char from properly encoded string
// len of resulting array is strlen (str) / 2
// caller is responsibe for freeing up the memory
#define BYTES_FROM_STR(dst, _str) { \
    char *str = (char*) (_str); \
    if (!str || (strlen (str) % 2) != 0) \
        return NULL; \
\
    size_t strlen_2 = strlen (str) / 2; \
    byte *mem = (byte*) zmalloc (strlen_2); \
    size_t i; \
\
    for (i = 0; i != strlen_2; i++) \
    { \
        char buff[3] = {0x0, 0x0, 0x0}; \
        strncpy (buff, str, 2); \
        unsigned int uint; \
        sscanf (buff, "%x", &uint); \
        assert (uint <= 0xff); \
        mem [i] = (byte) (0xff & uint); \
        str += 2; \
    } \
    dst = mem; \
}

// convert len bytes to hex string
// caller is responsibe for freeing up the memory
#define STR_FROM_BYTES(dst, _mem, _len) { \
    byte *mem = (byte*) (_mem); \
    size_t len = (size_t) (_len); \
    char* ret = (char*) zmalloc (2*len + 1); \
    char* aux = ret; \
    size_t i; \
    for (i = 0; i != len; i++) \
    { \
        sprintf (aux, "%02x", mem [i]); \
        aux+=2; \
    } \
    dst = ret; \
}

//  --------------------------------------------------------------------------
//  Create a new tpq_codec

tpq_codec_t *
tpq_codec_new (void)
{
    tpq_codec_t *self = (tpq_codec_t *) zmalloc (sizeof (tpq_codec_t));
    return self;
}

//  --------------------------------------------------------------------------
//  Create a new tpq_codec from zpl/zconfig_t *

tpq_codec_t *
    tpq_codec_new_zpl (zconfig_t *config)
{
    assert (config);
    tpq_codec_t *self = NULL;
    char *message = zconfig_get (config, "message", NULL);
    if (!message) {
        zsys_error ("Can't find 'message' section");
        return NULL;
    }

    if (streq ("TPQ_CODEC_HELLO", message)) {
        self = tpq_codec_new ();
        tpq_codec_set_id (self, TPQ_CODEC_HELLO);
    }
    else
    if (streq ("TPQ_CODEC_COVERAGE", message)) {
        self = tpq_codec_new ();
        tpq_codec_set_id (self, TPQ_CODEC_COVERAGE);
    }
    else
    if (streq ("TPQ_CODEC_QUERY", message)) {
        self = tpq_codec_new ();
        tpq_codec_set_id (self, TPQ_CODEC_QUERY);
    }
    else
    if (streq ("TPQ_CODEC_RESULT", message)) {
        self = tpq_codec_new ();
        tpq_codec_set_id (self, TPQ_CODEC_RESULT);
    }
    else
    if (streq ("TPQ_CODEC_PING", message)) {
        self = tpq_codec_new ();
        tpq_codec_set_id (self, TPQ_CODEC_PING);
    }
    else
    if (streq ("TPQ_CODEC_PONG", message)) {
        self = tpq_codec_new ();
        tpq_codec_set_id (self, TPQ_CODEC_PONG);
    }
    else
    if (streq ("TPQ_CODEC_GOODBYE", message)) {
        self = tpq_codec_new ();
        tpq_codec_set_id (self, TPQ_CODEC_GOODBYE);
    }
    else
    if (streq ("TPQ_CODEC_GOODBYE_OK", message)) {
        self = tpq_codec_new ();
        tpq_codec_set_id (self, TPQ_CODEC_GOODBYE_OK);
    }
    else
    if (streq ("TPQ_CODEC_ERROR", message)) {
        self = tpq_codec_new ();
        tpq_codec_set_id (self, TPQ_CODEC_ERROR);
    }
    else
       {
        zsys_error ("message=%s is not known", message);
        return NULL;
       }

    char *s = zconfig_get (config, "routing_id", NULL);
    if (s) {
        byte *bvalue;
        BYTES_FROM_STR (bvalue, s);
        if (!bvalue) {
            tpq_codec_destroy (&self);
            return NULL;
        }
        zframe_t *frame = zframe_new (bvalue, strlen (s) / 2);
        free (bvalue);
        self->routing_id = frame;
    }


    zconfig_t *content = NULL;
    switch (self->id) {
        case TPQ_CODEC_HELLO:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                tpq_codec_destroy (&self);
                return NULL;
            }
            {
            char *s = zconfig_get (content, "nickname", NULL);
            if (!s) {
                tpq_codec_destroy (&self);
                return NULL;
            }
            strncpy (self->nickname, s, 255);
            }
            break;
        case TPQ_CODEC_COVERAGE:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                tpq_codec_destroy (&self);
                return NULL;
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "detmask", NULL);
            if (!s) {
                zsys_error ("content/detmask not found");
                tpq_codec_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/detmask: %s is not a number", s);
                tpq_codec_destroy (&self);
                return NULL;
            }
            self->detmask = uvalue;
            }
            break;
        case TPQ_CODEC_QUERY:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                tpq_codec_destroy (&self);
                return NULL;
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "seqno", NULL);
            if (!s) {
                zsys_error ("content/seqno not found");
                tpq_codec_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/seqno: %s is not a number", s);
                tpq_codec_destroy (&self);
                return NULL;
            }
            self->seqno = uvalue;
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "tstart", NULL);
            if (!s) {
                zsys_error ("content/tstart not found");
                tpq_codec_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/tstart: %s is not a number", s);
                tpq_codec_destroy (&self);
                return NULL;
            }
            self->tstart = uvalue;
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "tspan", NULL);
            if (!s) {
                zsys_error ("content/tspan not found");
                tpq_codec_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/tspan: %s is not a number", s);
                tpq_codec_destroy (&self);
                return NULL;
            }
            self->tspan = uvalue;
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "detmask", NULL);
            if (!s) {
                zsys_error ("content/detmask not found");
                tpq_codec_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/detmask: %s is not a number", s);
                tpq_codec_destroy (&self);
                return NULL;
            }
            self->detmask = uvalue;
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "timeout", NULL);
            if (!s) {
                zsys_error ("content/timeout not found");
                tpq_codec_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/timeout: %s is not a number", s);
                tpq_codec_destroy (&self);
                return NULL;
            }
            self->timeout = uvalue;
            }
            break;
        case TPQ_CODEC_RESULT:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                tpq_codec_destroy (&self);
                return NULL;
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "seqno", NULL);
            if (!s) {
                zsys_error ("content/seqno not found");
                tpq_codec_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/seqno: %s is not a number", s);
                tpq_codec_destroy (&self);
                return NULL;
            }
            self->seqno = uvalue;
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "status", NULL);
            if (!s) {
                zsys_error ("content/status not found");
                tpq_codec_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/status: %s is not a number", s);
                tpq_codec_destroy (&self);
                return NULL;
            }
            self->status = uvalue;
            }
            {
            char *s = zconfig_get (content, "payload", NULL);
            if (!s) {
                tpq_codec_destroy (&self);
                return NULL;
            }
            byte *bvalue;
            BYTES_FROM_STR (bvalue, s);
            if (!bvalue) {
                tpq_codec_destroy (&self);
                return NULL;
            }
#if CZMQ_VERSION_MAJOR == 4
            zframe_t *frame = zframe_new (bvalue, strlen (s) / 2);
            zmsg_t *msg = zmsg_decode (frame);
            zframe_destroy (&frame);
#else
            zmsg_t *msg = zmsg_decode (bvalue, strlen (s) / 2);
#endif
            free (bvalue);
            self->payload = msg;
            }
            break;
        case TPQ_CODEC_PING:
            break;
        case TPQ_CODEC_PONG:
            break;
        case TPQ_CODEC_GOODBYE:
            break;
        case TPQ_CODEC_GOODBYE_OK:
            break;
        case TPQ_CODEC_ERROR:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                tpq_codec_destroy (&self);
                return NULL;
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "status", NULL);
            if (!s) {
                zsys_error ("content/status not found");
                tpq_codec_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/status: %s is not a number", s);
                tpq_codec_destroy (&self);
                return NULL;
            }
            self->status = uvalue;
            }
            {
            char *s = zconfig_get (content, "reason", NULL);
            if (!s) {
                tpq_codec_destroy (&self);
                return NULL;
            }
            strncpy (self->reason, s, 255);
            }
            break;
    }
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the tpq_codec

void
tpq_codec_destroy (tpq_codec_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        tpq_codec_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->routing_id);
        zmsg_destroy (&self->payload);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Create a deep copy of a tpq_codec instance

tpq_codec_t *
tpq_codec_dup (tpq_codec_t *other)
{
    assert (other);
    tpq_codec_t *copy = tpq_codec_new ();

    // Copy the routing and message id
    tpq_codec_set_routing_id (copy, tpq_codec_routing_id (other));
    tpq_codec_set_id (copy, tpq_codec_id (other));


    // Copy the rest of the fields
    tpq_codec_set_nickname (copy, tpq_codec_nickname (other));
    tpq_codec_set_detmask (copy, tpq_codec_detmask (other));
    tpq_codec_set_seqno (copy, tpq_codec_seqno (other));
    tpq_codec_set_tstart (copy, tpq_codec_tstart (other));
    tpq_codec_set_tspan (copy, tpq_codec_tspan (other));
    tpq_codec_set_timeout (copy, tpq_codec_timeout (other));
    tpq_codec_set_status (copy, tpq_codec_status (other));
    {
        zmsg_t *dup_msg = zmsg_dup (tpq_codec_payload (other));
        tpq_codec_set_payload (copy, &dup_msg);
    }
    tpq_codec_set_reason (copy, tpq_codec_reason (other));

    return copy;
}

//  --------------------------------------------------------------------------
//  Receive a tpq_codec from the socket. Returns 0 if OK, -1 if
//  the recv was interrupted, or -2 if the message is malformed.
//  Blocks if there is no message waiting.

int
tpq_codec_recv (tpq_codec_t *self, zsock_t *input)
{
    assert (input);
    int rc = 0;
    zmq_msg_t frame;
    zmq_msg_init (&frame);

    if (zsock_type (input) == ZMQ_ROUTER) {
        zframe_destroy (&self->routing_id);
        self->routing_id = zframe_recv (input);
        if (!self->routing_id || !zsock_rcvmore (input)) {
            zsys_warning ("tpq_codec: no routing ID");
            rc = -1;            //  Interrupted
            goto malformed;
        }
    }
    int size;
    size = zmq_msg_recv (&frame, zsock_resolve (input), 0);
    if (size == -1) {
        zsys_warning ("tpq_codec: interrupted");
        rc = -1;                //  Interrupted
        goto malformed;
    }
    self->needle = (byte *) zmq_msg_data (&frame);
    self->ceiling = self->needle + zmq_msg_size (&frame);


    //  Get and check protocol signature
    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 0)) {
        zsys_warning ("tpq_codec: invalid signature");
        rc = -2;                //  Malformed
        goto malformed;
    }
    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case TPQ_CODEC_HELLO:
            GET_STRING (self->nickname);
            break;

        case TPQ_CODEC_COVERAGE:
            GET_NUMBER8 (self->detmask);
            break;

        case TPQ_CODEC_QUERY:
            GET_NUMBER4 (self->seqno);
            GET_NUMBER8 (self->tstart);
            GET_NUMBER8 (self->tspan);
            GET_NUMBER8 (self->detmask);
            GET_NUMBER4 (self->timeout);
            break;

        case TPQ_CODEC_RESULT:
            GET_NUMBER4 (self->seqno);
            GET_NUMBER2 (self->status);
            //  Get zero or more remaining frames
            zmsg_destroy (&self->payload);
            if (zsock_rcvmore (input))
                self->payload = zmsg_recv (input);
            else
                self->payload = zmsg_new ();
            break;

        case TPQ_CODEC_PING:
            break;

        case TPQ_CODEC_PONG:
            break;

        case TPQ_CODEC_GOODBYE:
            break;

        case TPQ_CODEC_GOODBYE_OK:
            break;

        case TPQ_CODEC_ERROR:
            GET_NUMBER2 (self->status);
            GET_STRING (self->reason);
            break;

        default:
            zsys_warning ("tpq_codec: bad message ID");
            rc = -2;            //  Malformed
            goto malformed;
    }
    //  Successful return
    zmq_msg_close (&frame);
    return rc;

    //  Error returns
    malformed:
        zmq_msg_close (&frame);
        return rc;              //  Invalid message
}


//  --------------------------------------------------------------------------
//  Send the tpq_codec to the socket. Does not destroy it. Returns 0 if
//  OK, else -1.

int
tpq_codec_send (tpq_codec_t *self, zsock_t *output)
{
    assert (self);
    assert (output);

    if (zsock_type (output) == ZMQ_ROUTER)
        zframe_send (&self->routing_id, output, ZFRAME_MORE + ZFRAME_REUSE);

    size_t frame_size = 2 + 1;          //  Signature and message ID

    switch (self->id) {
        case TPQ_CODEC_HELLO:
            frame_size += 1 + strlen (self->nickname);
            break;
        case TPQ_CODEC_COVERAGE:
            frame_size += 8;            //  detmask
            break;
        case TPQ_CODEC_QUERY:
            frame_size += 4;            //  seqno
            frame_size += 8;            //  tstart
            frame_size += 8;            //  tspan
            frame_size += 8;            //  detmask
            frame_size += 4;            //  timeout
            break;
        case TPQ_CODEC_RESULT:
            frame_size += 4;            //  seqno
            frame_size += 2;            //  status
            break;
        case TPQ_CODEC_ERROR:
            frame_size += 2;            //  status
            frame_size += 1 + strlen (self->reason);
            break;
    }
    //  Now serialize message into the frame
    zmq_msg_t frame;
    zmq_msg_init_size (&frame, frame_size);
    self->needle = (byte *) zmq_msg_data (&frame);
    PUT_NUMBER2 (0xAAA0 | 0);
    PUT_NUMBER1 (self->id);
    bool have_payload = false;
    size_t nbr_frames = 1;              //  Total number of frames to send

    switch (self->id) {
        case TPQ_CODEC_HELLO:
            PUT_STRING (self->nickname);
            break;

        case TPQ_CODEC_COVERAGE:
            PUT_NUMBER8 (self->detmask);
            break;

        case TPQ_CODEC_QUERY:
            PUT_NUMBER4 (self->seqno);
            PUT_NUMBER8 (self->tstart);
            PUT_NUMBER8 (self->tspan);
            PUT_NUMBER8 (self->detmask);
            PUT_NUMBER4 (self->timeout);
            break;

        case TPQ_CODEC_RESULT:
            PUT_NUMBER4 (self->seqno);
            PUT_NUMBER2 (self->status);
            nbr_frames += self->payload? zmsg_size (self->payload): 1;
            have_payload = true;
            break;

        case TPQ_CODEC_ERROR:
            PUT_NUMBER2 (self->status);
            PUT_STRING (self->reason);
            break;

    }
    //  Now send the data frame
    zmq_msg_send (&frame, zsock_resolve (output), --nbr_frames? ZMQ_SNDMORE: 0);

    //  Now send the payload if necessary
    if (have_payload) {
        if (self->payload) {
            zframe_t *frame = zmsg_first (self->payload);
            while (frame) {
                zframe_send (&frame, output, ZFRAME_REUSE + (--nbr_frames? ZFRAME_MORE: 0));
                frame = zmsg_next (self->payload);
            }
        }
        else
            zmq_send (zsock_resolve (output), NULL, 0, 0);
    }
    return 0;
}


//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
tpq_codec_print (tpq_codec_t *self)
{
    assert (self);
    switch (self->id) {
        case TPQ_CODEC_HELLO:
            zsys_debug ("TPQ_CODEC_HELLO:");
            zsys_debug ("    nickname='%s'", self->nickname);
            break;

        case TPQ_CODEC_COVERAGE:
            zsys_debug ("TPQ_CODEC_COVERAGE:");
            zsys_debug ("    detmask=%ld", (long) self->detmask);
            break;

        case TPQ_CODEC_QUERY:
            zsys_debug ("TPQ_CODEC_QUERY:");
            zsys_debug ("    seqno=%ld", (long) self->seqno);
            zsys_debug ("    tstart=%ld", (long) self->tstart);
            zsys_debug ("    tspan=%ld", (long) self->tspan);
            zsys_debug ("    detmask=%ld", (long) self->detmask);
            zsys_debug ("    timeout=%ld", (long) self->timeout);
            break;

        case TPQ_CODEC_RESULT:
            zsys_debug ("TPQ_CODEC_RESULT:");
            zsys_debug ("    seqno=%ld", (long) self->seqno);
            zsys_debug ("    status=%ld", (long) self->status);
            zsys_debug ("    payload=");
            if (self->payload)
                zmsg_print (self->payload);
            else
                zsys_debug ("(NULL)");
            break;

        case TPQ_CODEC_PING:
            zsys_debug ("TPQ_CODEC_PING:");
            break;

        case TPQ_CODEC_PONG:
            zsys_debug ("TPQ_CODEC_PONG:");
            break;

        case TPQ_CODEC_GOODBYE:
            zsys_debug ("TPQ_CODEC_GOODBYE:");
            break;

        case TPQ_CODEC_GOODBYE_OK:
            zsys_debug ("TPQ_CODEC_GOODBYE_OK:");
            break;

        case TPQ_CODEC_ERROR:
            zsys_debug ("TPQ_CODEC_ERROR:");
            zsys_debug ("    status=%ld", (long) self->status);
            zsys_debug ("    reason='%s'", self->reason);
            break;

    }
}

//  --------------------------------------------------------------------------
//  Export class as zconfig_t*. Caller is responsibe for destroying the instance

zconfig_t *
tpq_codec_zpl (tpq_codec_t *self, zconfig_t *parent)
{
    assert (self);

    zconfig_t *root = zconfig_new ("tpq_codec", parent);

    switch (self->id) {
        case TPQ_CODEC_HELLO:
        {
            zconfig_put (root, "message", "TPQ_CODEC_HELLO");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }


            zconfig_t *config = zconfig_new ("content", root);
            zconfig_putf (config, "nickname", "%s", self->nickname);
            break;
            }
        case TPQ_CODEC_COVERAGE:
        {
            zconfig_put (root, "message", "TPQ_CODEC_COVERAGE");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }


            zconfig_t *config = zconfig_new ("content", root);
            zconfig_putf (config, "detmask", "%ld", (long) self->detmask);
            break;
            }
        case TPQ_CODEC_QUERY:
        {
            zconfig_put (root, "message", "TPQ_CODEC_QUERY");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }


            zconfig_t *config = zconfig_new ("content", root);
            zconfig_putf (config, "seqno", "%ld", (long) self->seqno);
            zconfig_putf (config, "tstart", "%ld", (long) self->tstart);
            zconfig_putf (config, "tspan", "%ld", (long) self->tspan);
            zconfig_putf (config, "detmask", "%ld", (long) self->detmask);
            zconfig_putf (config, "timeout", "%ld", (long) self->timeout);
            break;
            }
        case TPQ_CODEC_RESULT:
        {
            zconfig_put (root, "message", "TPQ_CODEC_RESULT");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }


            zconfig_t *config = zconfig_new ("content", root);
            zconfig_putf (config, "seqno", "%ld", (long) self->seqno);
            zconfig_putf (config, "status", "%ld", (long) self->status);
            {
            char *hex = NULL;
#if CZMQ_VERSION_MAJOR == 4
            zframe_t *frame = zmsg_encode (self->payload);
            STR_FROM_BYTES (hex, zframe_data (frame), zframe_size (frame));
            zconfig_putf (config, "payload", "%s", hex);
            zstr_free (&hex);
            zframe_destroy (&frame);
#else
            byte *buffer;
            size_t size = zmsg_encode (self->payload, &buffer);
            STR_FROM_BYTES (hex, buffer, size);
            zconfig_putf (config, "payload", "%s", hex);
            zstr_free (&hex);
            free (buffer); buffer= NULL;
#endif
            }
            break;
            }
        case TPQ_CODEC_PING:
        {
            zconfig_put (root, "message", "TPQ_CODEC_PING");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }


            break;
            }
        case TPQ_CODEC_PONG:
        {
            zconfig_put (root, "message", "TPQ_CODEC_PONG");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }


            break;
            }
        case TPQ_CODEC_GOODBYE:
        {
            zconfig_put (root, "message", "TPQ_CODEC_GOODBYE");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }


            break;
            }
        case TPQ_CODEC_GOODBYE_OK:
        {
            zconfig_put (root, "message", "TPQ_CODEC_GOODBYE_OK");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }


            break;
            }
        case TPQ_CODEC_ERROR:
        {
            zconfig_put (root, "message", "TPQ_CODEC_ERROR");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }


            zconfig_t *config = zconfig_new ("content", root);
            zconfig_putf (config, "status", "%ld", (long) self->status);
            zconfig_putf (config, "reason", "%s", self->reason);
            break;
            }
    }
    return root;
}

//  --------------------------------------------------------------------------
//  Get/set the message routing_id

zframe_t *
tpq_codec_routing_id (tpq_codec_t *self)
{
    assert (self);
    return self->routing_id;
}

void
tpq_codec_set_routing_id (tpq_codec_t *self, zframe_t *routing_id)
{
    if (self->routing_id)
        zframe_destroy (&self->routing_id);
    self->routing_id = zframe_dup (routing_id);
}


//  --------------------------------------------------------------------------
//  Get/set the tpq_codec id

int
tpq_codec_id (tpq_codec_t *self)
{
    assert (self);
    return self->id;
}

void
tpq_codec_set_id (tpq_codec_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

const char *
tpq_codec_command (tpq_codec_t *self)
{
    assert (self);
    switch (self->id) {
        case TPQ_CODEC_HELLO:
            return ("HELLO");
            break;
        case TPQ_CODEC_COVERAGE:
            return ("COVERAGE");
            break;
        case TPQ_CODEC_QUERY:
            return ("QUERY");
            break;
        case TPQ_CODEC_RESULT:
            return ("RESULT");
            break;
        case TPQ_CODEC_PING:
            return ("PING");
            break;
        case TPQ_CODEC_PONG:
            return ("PONG");
            break;
        case TPQ_CODEC_GOODBYE:
            return ("GOODBYE");
            break;
        case TPQ_CODEC_GOODBYE_OK:
            return ("GOODBYE_OK");
            break;
        case TPQ_CODEC_ERROR:
            return ("ERROR");
            break;
    }
    return "?";
}


//  --------------------------------------------------------------------------
//  Get/set the nickname field

const char *
tpq_codec_nickname (tpq_codec_t *self)
{
    assert (self);
    return self->nickname;
}

void
tpq_codec_set_nickname (tpq_codec_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->nickname)
        return;
    strncpy (self->nickname, value, 255);
    self->nickname [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the detmask field

uint64_t
tpq_codec_detmask (tpq_codec_t *self)
{
    assert (self);
    return self->detmask;
}

void
tpq_codec_set_detmask (tpq_codec_t *self, uint64_t detmask)
{
    assert (self);
    self->detmask = detmask;
}


//  --------------------------------------------------------------------------
//  Get/set the seqno field

uint32_t
tpq_codec_seqno (tpq_codec_t *self)
{
    assert (self);
    return self->seqno;
}

void
tpq_codec_set_seqno (tpq_codec_t *self, uint32_t seqno)
{
    assert (self);
    self->seqno = seqno;
}


//  --------------------------------------------------------------------------
//  Get/set the tstart field

uint64_t
tpq_codec_tstart (tpq_codec_t *self)
{
    assert (self);
    return self->tstart;
}

void
tpq_codec_set_tstart (tpq_codec_t *self, uint64_t tstart)
{
    assert (self);
    self->tstart = tstart;
}


//  --------------------------------------------------------------------------
//  Get/set the tspan field

uint64_t
tpq_codec_tspan (tpq_codec_t *self)
{
    assert (self);
    return self->tspan;
}

void
tpq_codec_set_tspan (tpq_codec_t *self, uint64_t tspan)
{
    assert (self);
    self->tspan = tspan;
}


//  --------------------------------------------------------------------------
//  Get/set the timeout field

uint32_t
tpq_codec_timeout (tpq_codec_t *self)
{
    assert (self);
    return self->timeout;
}

void
tpq_codec_set_timeout (tpq_codec_t *self, uint32_t timeout)
{
    assert (self);
    self->timeout = timeout;
}


//  --------------------------------------------------------------------------
//  Get/set the status field

uint16_t
tpq_codec_status (tpq_codec_t *self)
{
    assert (self);
    return self->status;
}

void
tpq_codec_set_status (tpq_codec_t *self, uint16_t status)
{
    assert (self);
    self->status = status;
}


//  --------------------------------------------------------------------------
//  Get the payload field without transferring ownership

zmsg_t *
tpq_codec_payload (tpq_codec_t *self)
{
    assert (self);
    return self->payload;
}

//  Get the payload field and transfer ownership to caller

zmsg_t *
tpq_codec_get_payload (tpq_codec_t *self)
{
    zmsg_t *payload = self->payload;
    self->payload = NULL;
    return payload;
}

//  Set the payload field, transferring ownership from caller

void
tpq_codec_set_payload (tpq_codec_t *self, zmsg_t **msg_p)
{
    assert (self);
    assert (msg_p);
    zmsg_destroy (&self->payload);
    self->payload = *msg_p;
    *msg_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get/set the reason field

const char *
tpq_codec_reason (tpq_codec_t *self)
{
    assert (self);
    return self->reason;
}

void
tpq_codec_set_reason (tpq_codec_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->reason)
        return;
    strncpy (self->reason, value, 255);
    self->reason [255] = 0;
}



//  --------------------------------------------------------------------------
//  Selftest

void
tpq_codec_test (bool verbose)
{
    printf (" * tpq_codec: ");

    if (verbose)
        printf ("\n");

    //  @selftest
    //  Simple create/destroy test
    zconfig_t *config;
    tpq_codec_t *self = tpq_codec_new ();
    assert (self);
    tpq_codec_destroy (&self);
    //  Create pair of sockets we can send through
    //  We must bind before connect if we wish to remain compatible with ZeroMQ < v4
    zsock_t *output = zsock_new (ZMQ_DEALER);
    assert (output);
    int rc = zsock_bind (output, "inproc://selftest-tpq_codec");
    assert (rc == 0);

    zsock_t *input = zsock_new (ZMQ_ROUTER);
    assert (input);
    rc = zsock_connect (input, "inproc://selftest-tpq_codec");
    assert (rc == 0);


    //  Encode/send/decode and verify each message type
    int instance;
    self = tpq_codec_new ();
    tpq_codec_set_id (self, TPQ_CODEC_HELLO);
    tpq_codec_set_nickname (self, "Life is short but Now lasts for ever");
    // convert to zpl
    config = tpq_codec_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    tpq_codec_send (self, output);
    tpq_codec_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        tpq_codec_t *self_temp = self;
        if (instance < 2)
            tpq_codec_recv (self, input);
        else {
            self = tpq_codec_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (tpq_codec_routing_id (self));
        assert (streq (tpq_codec_nickname (self), "Life is short but Now lasts for ever"));
        if (instance == 2) {
            tpq_codec_destroy (&self);
            self = self_temp;
        }
    }
    tpq_codec_set_id (self, TPQ_CODEC_COVERAGE);
    tpq_codec_set_detmask (self, 123);
    // convert to zpl
    config = tpq_codec_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    tpq_codec_send (self, output);
    tpq_codec_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        tpq_codec_t *self_temp = self;
        if (instance < 2)
            tpq_codec_recv (self, input);
        else {
            self = tpq_codec_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (tpq_codec_routing_id (self));
        assert (tpq_codec_detmask (self) == 123);
        if (instance == 2) {
            tpq_codec_destroy (&self);
            self = self_temp;
        }
    }
    tpq_codec_set_id (self, TPQ_CODEC_QUERY);
    tpq_codec_set_seqno (self, 123);
    tpq_codec_set_tstart (self, 123);
    tpq_codec_set_tspan (self, 123);
    tpq_codec_set_detmask (self, 123);
    tpq_codec_set_timeout (self, 123);
    // convert to zpl
    config = tpq_codec_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    tpq_codec_send (self, output);
    tpq_codec_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        tpq_codec_t *self_temp = self;
        if (instance < 2)
            tpq_codec_recv (self, input);
        else {
            self = tpq_codec_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (tpq_codec_routing_id (self));
        assert (tpq_codec_seqno (self) == 123);
        assert (tpq_codec_tstart (self) == 123);
        assert (tpq_codec_tspan (self) == 123);
        assert (tpq_codec_detmask (self) == 123);
        assert (tpq_codec_timeout (self) == 123);
        if (instance == 2) {
            tpq_codec_destroy (&self);
            self = self_temp;
        }
    }
    tpq_codec_set_id (self, TPQ_CODEC_RESULT);
    tpq_codec_set_seqno (self, 123);
    tpq_codec_set_status (self, 123);
    zmsg_t *result_payload = zmsg_new ();
    tpq_codec_set_payload (self, &result_payload);
    zmsg_addstr (tpq_codec_payload (self), "Captcha Diem");
    // convert to zpl
    config = tpq_codec_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    tpq_codec_send (self, output);
    tpq_codec_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        tpq_codec_t *self_temp = self;
        if (instance < 2)
            tpq_codec_recv (self, input);
        else {
            self = tpq_codec_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (tpq_codec_routing_id (self));
        assert (tpq_codec_seqno (self) == 123);
        assert (tpq_codec_status (self) == 123);
        assert (zmsg_size (tpq_codec_payload (self)) == 1);
        char *content = zmsg_popstr (tpq_codec_payload (self));
        assert (streq (content, "Captcha Diem"));
        zstr_free (&content);
        if (instance == 2)
            zmsg_destroy (&result_payload);
        if (instance == 2) {
            tpq_codec_destroy (&self);
            self = self_temp;
        }
    }
    tpq_codec_set_id (self, TPQ_CODEC_PING);
    // convert to zpl
    config = tpq_codec_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    tpq_codec_send (self, output);
    tpq_codec_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        tpq_codec_t *self_temp = self;
        if (instance < 2)
            tpq_codec_recv (self, input);
        else {
            self = tpq_codec_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (tpq_codec_routing_id (self));
        if (instance == 2) {
            tpq_codec_destroy (&self);
            self = self_temp;
        }
    }
    tpq_codec_set_id (self, TPQ_CODEC_PONG);
    // convert to zpl
    config = tpq_codec_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    tpq_codec_send (self, output);
    tpq_codec_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        tpq_codec_t *self_temp = self;
        if (instance < 2)
            tpq_codec_recv (self, input);
        else {
            self = tpq_codec_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (tpq_codec_routing_id (self));
        if (instance == 2) {
            tpq_codec_destroy (&self);
            self = self_temp;
        }
    }
    tpq_codec_set_id (self, TPQ_CODEC_GOODBYE);
    // convert to zpl
    config = tpq_codec_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    tpq_codec_send (self, output);
    tpq_codec_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        tpq_codec_t *self_temp = self;
        if (instance < 2)
            tpq_codec_recv (self, input);
        else {
            self = tpq_codec_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (tpq_codec_routing_id (self));
        if (instance == 2) {
            tpq_codec_destroy (&self);
            self = self_temp;
        }
    }
    tpq_codec_set_id (self, TPQ_CODEC_GOODBYE_OK);
    // convert to zpl
    config = tpq_codec_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    tpq_codec_send (self, output);
    tpq_codec_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        tpq_codec_t *self_temp = self;
        if (instance < 2)
            tpq_codec_recv (self, input);
        else {
            self = tpq_codec_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (tpq_codec_routing_id (self));
        if (instance == 2) {
            tpq_codec_destroy (&self);
            self = self_temp;
        }
    }
    tpq_codec_set_id (self, TPQ_CODEC_ERROR);
    tpq_codec_set_status (self, 123);
    tpq_codec_set_reason (self, "Life is short but Now lasts for ever");
    // convert to zpl
    config = tpq_codec_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    tpq_codec_send (self, output);
    tpq_codec_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        tpq_codec_t *self_temp = self;
        if (instance < 2)
            tpq_codec_recv (self, input);
        else {
            self = tpq_codec_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (tpq_codec_routing_id (self));
        assert (tpq_codec_status (self) == 123);
        assert (streq (tpq_codec_reason (self), "Life is short but Now lasts for ever"));
        if (instance == 2) {
            tpq_codec_destroy (&self);
            self = self_temp;
        }
    }

    tpq_codec_destroy (&self);
    zsock_destroy (&input);
    zsock_destroy (&output);
#if defined (__WINDOWS__)
    zsys_shutdown();
#endif
    //  @end

    printf ("OK\n");
}
