/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2025 Evgeny Grin (Karlson2k)

  GNU libmicrohttpd is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  GNU libmicrohttpd is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  Alternatively, you can redistribute GNU libmicrohttpd and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License, or (at your option) any later version, together
  with the eCos exception, as follows:

    As a special exception, if other files instantiate templates or
    use macros or inline functions from this file, or you compile this
    file and link it with other works to produce a work based on this
    file, this file does not by itself cause the resulting work to be
    covered by the GNU General Public License. However the source code
    for this file must still be made available in accordance with
    section (3) of the GNU General Public License v2.

    This exception does not invalidate any other reasons why a work
    based on this file might be covered by the GNU General Public
    License.

  You should have received copies of the GNU Lesser General Public
  License and the GNU General Public License along with this library;
  if not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file src/mhd2/tls_mbed_tls_lib.h
 * @brief  The wrapper for MbedTLS headers
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_TLS_MBED_TLS_LIB_H
#define MHD_TLS_MBED_TLS_LIB_H 1

#include "mhd_sys_options.h"

#ifndef MHD_SUPPORT_MBEDTLS
#error This header can be used only if MbedTLS is enabled
#endif

#ifndef __cplusplus
#  define mhd_MBETLS_C_HDR_START        /* Empty */
#  define mhd_MBETLS_C_HDR_FINISH       /* Empty */
#else  /* __cplusplus */
/* *INDENT-OFF* */
#  define mhd_MBETLS_C_HDR_START        extern "C" {
#  define mhd_MBETLS_C_HDR_FINISH       }
/* *INDENT-ON* */
#endif /* __cplusplus */

#include "microhttpd2_portability.h"

#if defined(MHD_WARN_IGNORE_STYLE_GCC)
MHD_WARN_PUSH_
#  ifdef HAVE_WZERO_AS_NULL_POINTER_CONSTANT
MHD_WARN_IGNORE_ ("-Wzero-as-null-pointer-constant")
#  endif /* HAVE_WZERO_AS_NULL_POINTER_CONSTANT */
#  ifdef HAVE_WREDUNDANT_DECLS
MHD_WARN_IGNORE_ ("-Wredundant-decls")
#  endif /* HAVE_WREDUNDANT_DECLS */
#  ifdef HAVE_WSWITCH_ENUM
MHD_WARN_IGNORE_ ("-Wswitch-enum")
#  endif /* HAVE_WSWITCH_ENUM */
#  ifdef HAVE_WDOCUMENTATION_DEPRECATED_SYNC
MHD_WARN_IGNORE_ ("-Wdocumentation-deprecated-sync")
#  endif /* HAVE_WDOCUMENTATION_DEPRECATED_SYNC */
#  ifdef HAVE_WDOCUMENTATION_UNKNOWN_COMMAND
MHD_WARN_IGNORE_ ("-Wdocumentation-unknown-command")
#  endif /* HAVE_WDOCUMENTATION_UNKNOWN_COMMAND */
#  ifdef HAVE_WDOCUMENTATION_PEDANTIC
MHD_WARN_IGNORE_ ("-Wdocumentation-pedantic")
#  endif /* HAVE_WDOCUMENTATION_PEDANTIC */
#  define mhd_RESTORE_MBEDTLS_MUTED_WARNS       MHD_WARN_POP_
#else  /* ! MHD_WARN_IGNORE_STYLE_GCC */
#  define mhd_RESTORE_MBEDTLS_MUTED_WARNS       /* empty */
#endif /* ! MHD_WARN_IGNORE_STYLE_GCC */

#include <mbedtls/build_info.h>
#include <mbedtls/platform.h>
#ifdef MBEDTLS_VERSION_C
#  include <mbedtls/version.h>
#endif /* MBEDTLS_VERSION_C */

#if ((MBEDTLS_VERSION_MAJOR + 0) < 3)
#error MbedTLS version 3.0 or later is required
#endif
#if ((MBEDTLS_VERSION_NUMBER + 0) < 0x03000000)
#error MbedTLS version 3.0 or later is required
#endif

/* #mhd_TLS_MBED_USE_PSA_FREE is MHD build-time user-definable macro */
#if defined(MBEDTLS_PSA_CRYPTO_CLIENT)
#  define mhd_TLS_MBED_USE_PSA  1
#  ifdef MHD_TLS_MBED_USE_PSA_FREE
/* The application must not use MbedTLS directly */
#    define mhd_TLS_MBED_USE_PSA_FREE   1
#  endif
#endif

#ifdef mhd_TLS_MBED_USE_PSA
#  include <psa/crypto.h>
#endif /* mhd_TLS_MBED_USE_PSA */

#ifdef MBEDTLS_MD_C
/* Actually MD must be available if TLS is enabled */
#  include <mbedtls/md.h>
#endif

#if ((MBEDTLS_VERSION_NUMBER + 0) >= 0x03050000)
#  define mhd_TLS_MBED_HAS_SHA3_IDS     1
#endif

#ifdef MBEDTLS_ENTROPY_C
#  include <mbedtls/entropy.h>
#endif /* MBEDTLS_ENTROPY_C */

#ifdef mhd_TLS_MBED_USE_PSA
mhd_MBETLS_C_HDR_START
#  include <mbedtls/psa_util.h>
mhd_MBETLS_C_HDR_FINISH
#  define mhd_TLS_MBED_HAS_RNG_PSA      1
#elif defined(MHD_TLS_MBED_PREF_RNG_PSA)
#  undef MHD_TLS_MBED_PREF_RNG_PSA
#endif

#ifdef MBEDTLS_HMAC_DRBG_C
#  include <mbedtls/hmac_drbg.h>
#  define mhd_TLS_MBED_HAS_RNG_HMAC     1
#elif defined(MHD_TLS_MBED_PREF_RNG_HMAC)
#  undef MHD_TLS_MBED_PREF_RNG_HMAC
#endif /* MBEDTLS_HMAC_DRBG_C */

#ifdef MBEDTLS_CTR_DRBG_C
#  include <mbedtls/ctr_drbg.h>
#  define mhd_TLS_MBED_HAS_RNG_CTR      1
#elif defined(MHD_TLS_MBED_PREF_RNG_CTR)
#  undef MHD_TLS_MBED_PREF_RNG_CTR
#endif /* MBEDTLS_CTR_DRBG_C */

#if ! defined(MHD_TLS_MBED_PREF_RNG_PSA) && \
  ! defined(MHD_TLS_MBED_PREF_RNG_HMAC) && \
  ! defined(MHD_TLS_MBED_PREF_RNG_CTR)
#  if defined(mhd_TLS_MBED_HAS_RNG_PSA)
#    define MHD_TLS_MBED_PREF_RNG_PSA   1
#  elif defined(mhd_TLS_MBED_HAS_RNG_HMAC) && \
  defined(MBEDTLS_MD_C)
#    define MHD_TLS_MBED_PREF_RNG_HMAC  1
#    define mhd_TLS_MBED_RNG_PREF_NEEDS_ENTROPY         1
#  elif defined(mhd_TLS_MBED_HAS_RNG_CTR)
#    define MHD_TLS_MBED_PREF_RNG_CTR   1
#    define mhd_TLS_MBED_RNG_PREF_NEEDS_ENTROPY         1
#  endif
#endif

#if defined(mhd_TLS_MBED_RNG_PREF_NEEDS_ENTROPY) && \
  defined(MBEDTLS_ENTROPY_C)
#  define mhd_TLS_MBED_USE_LIB_ENTROPY          1
#endif

#if ((MBEDTLS_VERSION_NUMBER + 0) < 0x04000000)
/**
 * TLS initialisation requires random generator
 */
#  define mhd_TLS_MBED_INIT_TLS_REQ_RNG         1
#endif

#include <mbedtls/x509_crt.h>

#if ! defined(MBEDTLS_X509_CRT_PARSE_C)
#error X.509 certificate parsing functions are required
#endif /* ! MBEDTLS_X509_CRT_PARSE_C */

#include <mbedtls/pk.h>

#if ! defined(MBEDTLS_PK_PARSE_C)
#error Public key parser is required
#endif /* ! MBEDTLS_PK_PARSE_C */

#if ! defined(MBEDTLS_PEM_PARSE_C)
#error PEM parser is required
#endif /* ! MBEDTLS_PEM_PARSE_C */

/* Required header, checked in 'configure' */
#include <mbedtls/ssl.h>

/* #MHD_TLS_MBED_SKIP_PLATFORM_SETUP and #MHD_TLS_MBED_USE_PLATFORM_TEARDOWN
   are MHD build-time user-definable macros */
/* User may set #MHD_TLS_MBED_SKIP_PLATFORM_SETUP and/or
   #MHD_TLS_MBED_USE_PLATFORM_TEARDOWN when building MHD to control
   automatic platform setup / teardown */
#if defined(MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT) && \
  ! defined(MHD_TLS_MBED_SKIP_PLATFORM_SETUP)
#  define mhd_TLS_MBED_HAS_PLATFORM_SETUP       1
#  ifdef MHD_TLS_MBED_USE_PLATFORM_TEARDOWN
/* The application must not use MbedTLS directly */
#    define mhd_TLS_MBED_USE_PLATFORM_TEARDOWN  1
#  endif
#endif

#ifdef MBEDTLS_NET_C
/* Actually, the header should be available unconditionally, but could be
   accidently excluded if module is disabled. */
#  include <mbedtls/net_sockets.h>
#endif

#ifndef MBEDTLS_ERR_NET_RECV_FAILED
/* Unknown error when receiving the data */
#  define MBEDTLS_ERR_NET_RECV_FAILED   (-0x004C)
#endif
#ifndef MBEDTLS_ERR_NET_SEND_FAILED
/* Unknown error when sending the data */
#  define MBEDTLS_ERR_NET_SEND_FAILED   (-0x004E)
#endif
#ifndef MBEDTLS_ERR_NET_CONN_RESET
/* The network connection is broken */
#  define MBEDTLS_ERR_NET_CONN_RESET    (-0x0050)
#endif

#ifdef MBEDTLS_DEBUG_C
#  include <mbedtls/debug.h>
#endif

mhd_RESTORE_MBEDTLS_MUTED_WARNS

#endif /* ! MHD_TLS_MBED_TLS_LIB_H */
