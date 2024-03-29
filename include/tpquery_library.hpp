/*  =========================================================================
    tpquery - generated layer of public API

    Copyright 2019.  May use and distribute according to LGPL v3 or later.

################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  Read the zproject/README.md for information about making permanent changes. #
################################################################################
    =========================================================================
*/

#ifndef TPQUERY_LIBRARY_H_INCLUDED
#define TPQUERY_LIBRARY_H_INCLUDED

//  Set up environment for the application

//  External dependencies
#include <czmq.h>
#include <ptmp/api.h>

//  TPQUERY version macros for compile-time API detection
#define TPQUERY_VERSION_MAJOR 0
#define TPQUERY_VERSION_MINOR 0
#define TPQUERY_VERSION_PATCH 0

#define TPQUERY_MAKE_VERSION(major, minor, patch) \
    ((major) * 10000 + (minor) * 100 + (patch))
#define TPQUERY_VERSION \
    TPQUERY_MAKE_VERSION(TPQUERY_VERSION_MAJOR, TPQUERY_VERSION_MINOR, TPQUERY_VERSION_PATCH)

#if defined (__WINDOWS__)
#   if defined TPQUERY_STATIC
#       define TPQUERY_EXPORT
#   elif defined TPQUERY_INTERNAL_BUILD
#       if defined DLL_EXPORT
#           define TPQUERY_EXPORT __declspec(dllexport)
#       else
#           define TPQUERY_EXPORT
#       endif
#   elif defined TPQUERY_EXPORTS
#       define TPQUERY_EXPORT __declspec(dllexport)
#   else
#       define TPQUERY_EXPORT __declspec(dllimport)
#   endif
#   define TPQUERY_PRIVATE
#elif defined (__CYGWIN__)
#   define TPQUERY_EXPORT
#   define TPQUERY_PRIVATE
#else
#   if (defined __GNUC__ && __GNUC__ >= 4) || defined __INTEL_COMPILER
#       define TPQUERY_PRIVATE __attribute__ ((visibility ("hidden")))
#       define TPQUERY_EXPORT __attribute__ ((visibility ("default")))
#   else
#       define TPQUERY_PRIVATE
#       define TPQUERY_EXPORT
#   endif
#endif

//  Project has no stable classes, so we build the draft API
#undef  TPQUERY_BUILD_DRAFT_API
#define TPQUERY_BUILD_DRAFT_API

//  Opaque class structures to allow forward references
//  These classes are stable or legacy and built in all releases
//  Draft classes are by default not built in stable releases
#ifdef TPQUERY_BUILD_DRAFT_API
typedef struct _tpq_codec_t tpq_codec_t;
#define TPQ_CODEC_T_DEFINED
typedef struct _tpq_server_t tpq_server_t;
#define TPQ_SERVER_T_DEFINED
typedef struct _tpq_client_t tpq_client_t;
#define TPQ_CLIENT_T_DEFINED
#endif // TPQUERY_BUILD_DRAFT_API


//  Public classes, each with its own header file
#ifdef TPQUERY_BUILD_DRAFT_API
#include "tpq_codec.hpp"
#include "tpq_server.hpp"
#include "tpq_client.hpp"
#endif // TPQUERY_BUILD_DRAFT_API

#ifdef TPQUERY_BUILD_DRAFT_API

#ifdef __cplusplus
extern "C" {
#endif

//  Self test for private classes
TPQUERY_EXPORT void
    tpquery_private_selftest (bool verbose, const char *subtest);

#ifdef __cplusplus
}
#endif
#endif // TPQUERY_BUILD_DRAFT_API

#endif
/*
################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  Read the zproject/README.md for information about making permanent changes. #
################################################################################
*/
