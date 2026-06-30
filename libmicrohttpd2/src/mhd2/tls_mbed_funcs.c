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
 * @file src/mhd2/tls_mbed_funcs.c
 * @brief  The implementation of MbedTLS wrapper functions
 * @author Karlson2k (Evgeny Grin)
 */

/* A few macros can be defined at MHD build-time to adjust code interfacing
   with MbedTLS library:
   - MHD_TLS_MBED_USE_PSA_FREE
   - MHD_TLS_MBED_PREF_RNG_PSA
   - MHD_TLS_MBED_PREF_RNG_HMAC
   - MHD_TLS_MBED_PREF_RNG_CTR
   - MHD_TLS_MBED_SKIP_PLATFORM_SETUP
   - MHD_TLS_MBED_USE_PLATFORM_TEARDOWN
   - MHD_TLS_MBED_SKIP_CERT_KEY_MATCH_CHECK
   - MHD_TLS_MBED_DBG_PRINT_LEVEL
   See macros use in this file and in tls_mbed_tls_lib.h.
   Macros can be defined, for example, by CPPFLAGS before run of the configure.
 */

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "compat_calloc.h"
#include "sys_malloc.h"
#include <string.h>

#ifdef mhd_USE_TLS_DEBUG_MESSAGES
#  include <stdio.h> /* For TLS debug printing */
#endif

#include "mhd_assert.h"
#include "mhd_unreachable.h"
#include "mhd_assume.h"

#include "mhd_constexpr.h"
#include "mhd_arr_num_elems.h"

#include "mhd_conn_socket.h"

#include "mhd_tls_internal.h"

#include "tls_mbed_tls_lib.h"

#include "mhd_public_api.h"

#include "mhd_tls_ver_stct.h"

#include "daemon_logger.h"

#include "daemon_options.h"

#include "sckt_recv.h"
#include "sckt_send.h"

#include "tls_mbed_daemon_data.h"
#include "tls_mbed_conn_data.h"
#include "tls_mbed_funcs.h"

#if defined(mhd_USE_TLS_DEBUG_MESSAGES) && defined(MBEDTLS_DEBUG_C)
#  define mhd_TLS_MBED_HAS_DEBUG_PRINT  1

/* MHD_TLS_MBED_DBG_PRINT_LEVEL can be defined to number in range 0..5,
   where 5 is the most detailed log */
#ifdef MHD_TLS_MBED_DBG_PRINT_LEVEL
#  define mhd_DBG_PRINT_LEVEL (MHD_TLS_MBED_DBG_PRINT_LEVEL + 0)
#else
#  define mhd_DBG_PRINT_LEVEL (2)
#endif

static void
mhd_tls_mbed_debug_print (void *ctx,
                          int level,
                          const char *filename,
                          int line_num,
                          const char *msg)
{
  (void) ctx; /* Unused */
  /* The level should be pre-filtred by MbedTLS, but it is filtered again
     here in case if something else changed it. */
  if (mhd_DBG_PRINT_LEVEL < level)
    return;
  (void) fprintf (stderr, "## MbedTLS %02i [%s:%d]: %s",
                  level,
                  filename,
                  line_num,
                  msg);
  (void) fflush (stderr);
}


#endif /* mhd_USE_TLS_DEBUG_MESSAGES && MBEDTLS_DEBUG_C */


/* ** Global initialisation / de-initialisation ** */

#ifdef MHD_TLS_MBED_PREF_RNG_HMAC
static const mbedtls_md_info_t *
mbed_get_md_for_drbg (void)
{
  mhd_constexpr mbedtls_md_type_t mds[] = {
#ifdef mhd_TLS_MBED_HAS_SHA3_IDS
    MBEDTLS_MD_SHA3_256
    ,
#endif /* mhd_TLS_MBED_HAS_SHA3_IDS */
    MBEDTLS_MD_SHA256
#ifdef mhd_TLS_MBED_HAS_SHA3_IDS
    ,
    MBEDTLS_MD_SHA3_512
#endif /* mhd_TLS_MBED_HAS_SHA3_IDS */
    ,
    MBEDTLS_MD_SHA512
#ifdef mhd_TLS_MBED_HAS_SHA3_IDS
    ,
    MBEDTLS_MD_SHA3_384
#endif /* mhd_TLS_MBED_HAS_SHA3_IDS */
    ,
    MBEDTLS_MD_SHA384
#ifdef mhd_TLS_MBED_HAS_SHA3_IDS
    ,
    MBEDTLS_MD_SHA3_224
#endif /* mhd_TLS_MBED_HAS_SHA3_IDS */
    ,
    MBEDTLS_MD_SHA224
  };
  size_t i;

  for (i = 0; i < mhd_ARR_NUM_ELEMS (mds); ++i)
  {
    const mbedtls_md_info_t *const ret =
      mbedtls_md_info_from_type (mds[i]);
    if (NULL != ret)
      return ret;
  }

  return (const mbedtls_md_info_t *) NULL;
}


#endif /* MHD_TLS_MBED_PREF_RNG_HMAC */

static bool mbedtls_lib_inited_now = false;
/* Must be checked when MHD-internal random generator is used */
static bool mbedtls_rng_inited_now = false;
static bool mbedtls_lib_inited_once = false;

#ifdef mhd_TLS_MBED_HAS_PLATFORM_SETUP
static mbedtls_platform_context mhd_mbed_plat_ctx;
#endif /* mhd_TLS_MBED_HAS_PLATFORM_SETUP */

#if defined(mhd_TLS_MBED_USE_LIB_ENTROPY)
static mbedtls_entropy_context mhd_mbed_entr_ctx;
#endif /* mhd_TLS_MBED_USE_LIB_ENTROPY */

#if defined(MHD_TLS_MBED_PREF_RNG_CTR)
static mbedtls_ctr_drbg_context mhd_mbed_ctr_drbg_ctx;
#elif defined(MHD_TLS_MBED_PREF_RNG_HMAC)
static mbedtls_hmac_drbg_context mhd_mbed_hmac_drbg_ctx;
#endif /* MHD_TLS_MBED_PREF_RNG_HMAC */

static bool
mbed_rng_init (void)
{
#ifdef mhd_TLS_MBED_RNG_PREF_NEEDS_ENTROPY
  int (*entropy_cb)(void *ctx, unsigned char *out, size_t out_size);
  void *entropy_cb_ctx;

#  ifdef mhd_TLS_MBED_USE_LIB_ENTROPY
  mbedtls_entropy_init (&mhd_mbed_entr_ctx);
  entropy_cb = &mbedtls_entropy_func;
  entropy_cb_ctx = &mhd_mbed_entr_ctx;
#  else  /* ! mhd_TLS_MBED_USE_LIB_ENTROPY */
  /* Seeding with system's entropy sources could be implemented here */
#error MbedTLS random generator needs entropy sources
#  endif /* ! mhd_TLS_MBED_USE_LIB_ENTROPY */
#endif /* mhd_TLS_MBED_RNG_PREF_NEEDS_ENTROPY */

  mhd_assert (! mbedtls_rng_inited_now);

  if (1) /* For local scope only */
  {
#ifdef mhd_TLS_MBED_RNG_PREF_NEEDS_ENTROPY
    static const char id_str[] = "libmicrohttpd2";
    static uint_fast64_t init_cntr = 0u;
    static void *uniq_ptr1 = &init_cntr; /* Any address should be unique in the same address space */
    void *uniq_ptr2 = &uniq_ptr2; /* Any address should be unique in the same address space */
    unsigned char pers[sizeof(id_str)
                       + sizeof(uniq_ptr1)
                       + sizeof(uniq_ptr2)
                       + sizeof(init_cntr)];

    memcpy (pers,
            id_str,
            sizeof(id_str));
    memcpy (pers + sizeof(id_str),
            &uniq_ptr1,
            sizeof(uniq_ptr1));
    memcpy (pers + sizeof(id_str) + sizeof(uniq_ptr1),
            &uniq_ptr2,
            sizeof(uniq_ptr2));
    memcpy (pers + sizeof(id_str) + sizeof(uniq_ptr1) + sizeof(uniq_ptr2),
            &init_cntr,
            sizeof(init_cntr));
    ++init_cntr;
#endif /* mhd_TLS_MBED_RNG_PREF_NEEDS_ENTROPY */

#if defined(MHD_TLS_MBED_PREF_RNG_CTR)
    mbedtls_ctr_drbg_init (&mhd_mbed_ctr_drbg_ctx);
    mbedtls_rng_inited_now =
      (0 == mbedtls_ctr_drbg_seed (&mhd_mbed_ctr_drbg_ctx,
                                   entropy_cb,
                                   entropy_cb_ctx,
                                   pers,
                                   sizeof(pers)));
    if (! mbedtls_rng_inited_now)
      mbedtls_ctr_drbg_free (&mhd_mbed_ctr_drbg_ctx);
#elif defined(MHD_TLS_MBED_PREF_RNG_HMAC)
    mbedtls_hmac_drbg_init (&mhd_mbed_hmac_drbg_ctx);
    mbedtls_rng_inited_now =
      (0 == mbedtls_hmac_drbg_seed (&mhd_mbed_hmac_drbg_ctx,
                                    mbed_get_md_for_drbg (), /* NULL is handled by mbedtls_hmac_drbg_seed() */
                                    entropy_cb,
                                    entropy_cb_ctx,
                                    pers,
                                    sizeof(pers)));
    if (! mbedtls_rng_inited_now)
      mbedtls_hmac_drbg_free (&mhd_mbed_hmac_drbg_ctx);
#elif defined(MHD_TLS_MBED_PREF_RNG_PSA)
    mbedtls_rng_inited_now = true; /* No additional initialisation needed */
#elif ! defined(mhd_TLS_MBED_INIT_TLS_REQ_RNG)
    mbedtls_rng_inited_now = false;
#else /* mhd_TLS_MBED_INIT_TLS_REQ_RNG */
#error MbedTLS backend requires random generator
    /* Support for external strong random generator could be added */
    mbedtls_rng_inited_now = false;
#endif
    if (mbedtls_rng_inited_now)
      return true; /* Success exit point */
  }

#ifdef mhd_TLS_MBED_USE_LIB_ENTROPY
  mbedtls_entropy_free (&mhd_mbed_entr_ctx);
#endif /* mhd_TLS_MBED_USE_LIB_ENTROPY */

  return false;  /* Failure exit point */
}


static void
mbed_rng_deinit (void)
{
  if (! mbedtls_rng_inited_now)
    return;

#if defined(MHD_TLS_MBED_PREF_RNG_CTR)
  mbedtls_ctr_drbg_free (&mhd_mbed_ctr_drbg_ctx);
#elif defined(MHD_TLS_MBED_PREF_RNG_HMAC)
  mbedtls_hmac_drbg_free (&mhd_mbed_hmac_drbg_ctx);
#endif

#ifdef mhd_TLS_MBED_USE_LIB_ENTROPY
  mbedtls_entropy_free (&mhd_mbed_entr_ctx);
#endif /* mhd_TLS_MBED_USE_LIB_ENTROPY */

  mbedtls_rng_inited_now = false;
}


MHD_INTERNAL void
mhd_tls_mbed_global_init (void)
{
#ifdef MBEDTLS_VERSION_C
  if (1)
  {
    const unsigned int ver = mbedtls_version_get_number ();
    if (MBEDTLS_VERSION_NUMBER > ver)
      return; /* Run-time version is lower than build-time version */
    if (((MBEDTLS_VERSION_NUMBER) >> 24u) != (ver >> 24u))
      return; /* Run-time major version does not match build-time major version */
  }
#endif /* MBEDTLS_VERSION_C */

#ifdef mhd_TLS_MBED_HAS_PLATFORM_SETUP
#  ifdef mhd_TLS_MBED_USE_PLATFORM_TEARDOWN
  /* 'setup' platform repeatedly only only if 'teardown' is called */
  if (mbedtls_lib_inited_once)
    (void) 0; /* Do not repeat 'setup' */
  else /* combined with tne next 'if()' */
#  endif /* mhd_TLS_MBED_USE_PLATFORM_TEARDOWN */
  if (0 != mbedtls_platform_setup (&mhd_mbed_plat_ctx))
    return; /* Error platform initialising */
#endif /* mhd_TLS_MBED_HAS_PLATFORM_SETUP */

#ifdef mhd_TLS_MBED_USE_PSA
  /* It is safe to call psa_crypto_init() several times */
  if (PSA_SUCCESS != psa_crypto_init ())
  {
#ifdef mhd_TLS_MBED_USE_PLATFORM_TEARDOWN
    mbedtls_platform_teardown (&mhd_mbed_plat_ctx);
#endif /* mhd_TLS_MBED_USE_PLATFORM_TEARDOWN */
    return;
  }
#endif /* mhd_TLS_MBED_USE_PSA */
  mbedtls_lib_inited_once = true;

#if mhd_TLS_MBED_INIT_TLS_REQ_RNG
  mbedtls_lib_inited_now = mbed_rng_init ();
#else  /* ! mhd_TLS_MBED_INIT_TLS_REQ_RNG */
  (void) mbed_rng_init ();
  mbedtls_lib_inited_now = true; /* MbedTLS could be used even without random generator */
#endif /* ! mhd_TLS_MBED_INIT_TLS_REQ_RNG */

  if (! mbedtls_lib_inited_now)
  {
#ifdef mhd_TLS_MBED_USE_PLATFORM_TEARDOWN
    mbedtls_platform_teardown (&mhd_mbed_plat_ctx);
#endif /* mhd_TLS_MBED_USE_PLATFORM_TEARDOWN */

#ifdef mhd_TLS_MBED_USE_PSA_FREE
    mbedtls_psa_crypto_free ();
#endif /* mhd_TLS_MBED_USE_PLATFORM_TEARDOWN */
    (void) 0;
  }
}


MHD_INTERNAL void
mhd_tls_mbed_global_deinit (void)
{
  if (! mbedtls_lib_inited_now)
    return;

  mbed_rng_deinit ();

#ifdef mhd_TLS_MBED_USE_PSA_FREE
  /* Not used by default as it will break all calls to PSA performed
     directly by the application after closing all active MHD daemons */
  mbedtls_psa_crypto_free ();
#endif /* mhd_TLS_MBED_USE_PLATFORM_TEARDOWN */

#ifdef mhd_TLS_MBED_USE_PLATFORM_TEARDOWN
  /* Not used by default as it will break all calls to MbedTLS performed
     directly by the application after closing all active MHD daemons */
  mbedtls_platform_teardown (&mhd_mbed_plat_ctx);
#endif /* mhd_TLS_MBED_USE_PLATFORM_TEARDOWN */

  mbedtls_lib_inited_now = false;
}


MHD_INTERNAL MHD_FN_PURE_ bool
mhd_tls_mbed_is_inited_fine (void)
{
  mhd_assert (! mbedtls_lib_inited_now || mbedtls_lib_inited_once);
  return mbedtls_lib_inited_now;
}


/* ** Daemon initialisation / de-initialisation ** */

/**
 * Check application-provided daemon TLS settings
 * @param d the daemon handle
 * @param s the application-provided settings
 * @return #MHD_SC_OK on success,
 *         error code otherwise
 */
static MHD_FN_PAR_NONNULL_ALL_ MHD_FN_MUST_CHECK_RESULT_ enum MHD_StatusCode
check_app_tls_settings (struct MHD_Daemon *restrict d,
                        struct DaemonOptions *restrict s)
{
  mhd_assert (MHD_TLS_BACKEND_NONE != s->tls);
  mhd_assert ((MHD_TLS_BACKEND_MBEDTLS == s->tls) || \
              (MHD_TLS_BACKEND_ANY == s->tls));

  if (NULL == s->tls_cert_key.v_mem_cert)
  {
    mhd_LOG_MSG (d, MHD_SC_TLS_CONF_BAD_CERT, \
                 "No valid TLS certificate is provided");
    return MHD_SC_TLS_CONF_BAD_CERT;
  }
  mhd_assert (NULL != s->tls_cert_key.v_mem_key);

  if ((MHD_WM_THREAD_PER_CONNECTION == s->work_mode.mode) ||
      ((MHD_WM_WORKER_THREADS == s->work_mode.mode)
       && (1u < s->work_mode.params.num_worker_threads)))
  {
    bool threads_supported;
#if ! defined(MBEDTLS_THREADING_C)
    threads_supported = false;
#else  /* MBEDTLS_THREADING_C */
#  if defined(MBEDTLS_VERSION_FEATURES)
    threads_supported =
      (0 == mbedtls_version_check_feature ("MBEDTLS_THREADING_C"));
#  else  /* ! MBEDTLS_VERSION_FEATURES */
    threads_supported = true;
#  endif /* ! MBEDTLS_VERSION_FEATURES */
#endif /* MBEDTLS_THREADING_C */
    if (! threads_supported)
    {
      mhd_LOG_MSG (d, MHD_SC_TLS_BACKEND_DAEMON_INCOMPATIBLE_SETTINGS, \
                   "MbedTLS built without threading support and cannot "
                   "be used in multi-threaded modes");
      return MHD_SC_TLS_BACKEND_DAEMON_INCOMPATIBLE_SETTINGS;
    }
  }

  return MHD_SC_OK;
}


#if defined(mhd_TLS_MBED_INIT_TLS_REQ_RNG)
/**
 * Set daemon TLS credentials.
 * This function puts error messages to the log if needed.
 * @param d the daemon handle
 * @param d_tls the daemon TLS settings
 * @param s the application-provided settings
 * @param rng_func the random generator function
 * @param rng_ctx the random generator function context
 * @return #MHD_SC_OK on success,
 *         error code otherwise
 */
#else  /* ! mhd_TLS_MBED_INIT_TLS_REQ_RNG */
/**
 * Set daemon TLS credentials.
 * This function puts error messages to the log if needed.
 * @param d the daemon handle
 * @param d_tls the daemon TLS settings
 * @param s the application-provided settings
 * @return #MHD_SC_OK on success,
 *         error code otherwise
 */
#endif /* ! mhd_TLS_MBED_INIT_TLS_REQ_RNG */
static MHD_FN_PAR_NONNULL_ (1) MHD_FN_PAR_NONNULL_ (2)
MHD_FN_PAR_NONNULL_ (3) MHD_FN_MUST_CHECK_RESULT_ enum MHD_StatusCode
daemon_init_credentials (
  struct MHD_Daemon *restrict d,
  struct mhd_TlsMbedDaemonData *restrict d_tls,
  struct DaemonOptions *restrict s
#if defined(mhd_TLS_MBED_INIT_TLS_REQ_RNG)
  ,
  int (*rng_func)(void *ctx, unsigned char *out, size_t out_size),
  void *rng_ctx
#endif /* mhd_TLS_MBED_INIT_TLS_REQ_RNG */
  )
{
  enum MHD_StatusCode ret;
  size_t cert_len;
  size_t key_len;
  size_t pwd_len;
  int res;

  ret = MHD_SC_OK;

  // TODO: Support multiple certificates
  cert_len = strlen (s->tls_cert_key.v_mem_cert); // TODO: Reuse calculated length
  key_len = strlen (s->tls_cert_key.v_mem_key);   // TODO: Reuse calculated length
  pwd_len = (NULL == s->tls_cert_key.v_mem_pass) ?
            0u : strlen (s->tls_cert_key.v_mem_pass); // TODO: Reuse calculated length

  mhd_assert (0 != cert_len);
  mhd_assert (0 != key_len);

  mbedtls_x509_crt_init (&(d_tls->cert_chain));

  res = mbedtls_x509_crt_parse (&(d_tls->cert_chain),
                                (const unsigned char *)
                                s->tls_cert_key.v_mem_cert,
                                cert_len + 1u /* Include terminating zero */);
  if (0 == res)
  {
    mbedtls_pk_init (&(d_tls->prv_key));
#if defined(mhd_TLS_MBED_INIT_TLS_REQ_RNG)
    if (0 != mbedtls_pk_parse_key (&(d_tls->prv_key),
                                   (const unsigned char *)
                                   s->tls_cert_key.v_mem_key,
                                   key_len + 1u,
                                   (const unsigned char *)
                                   s->tls_cert_key.v_mem_pass,
                                   pwd_len,
                                   rng_func,
                                   rng_ctx))
      ret = MHD_SC_TLS_CONF_BAD_CERT;
#else  /* ! mhd_TLS_MBED_INIT_TLS_REQ_RNG */
    if (0 != mbedtls_pk_parse_key (&(d_tls->prv_key),
                                   (const unsigned char *)
                                   s->tls_cert_key.v_mem_key,
                                   key_len + 1u,
                                   (const unsigned char *)
                                   s->tls_cert_key.v_mem_pass,
                                   pwd_len))
      ret = MHD_SC_TLS_CONF_BAD_CERT;
#endif  /* ! mhd_TLS_MBED_INIT_TLS_REQ_RNG */

    if (MHD_SC_OK == ret)
    {
      /* The next macro can be defined at MHD build-time to skip potentially
         expensive check */
#ifdef MHD_TLS_MBED_SKIP_CERT_KEY_MATCH_CHECK
      return MHD_SC_OK; /* Success exit point */
#else  /* ! MHD_TLS_MBED_SKIP_CERT_KEY_MATCH_CHECK */
#  if defined(mhd_TLS_MBED_INIT_TLS_REQ_RNG)
      res =  mbedtls_pk_check_pair (&(d_tls->cert_chain.pk),
                                    &(d_tls->prv_key),
                                    rng_func,
                                    rng_ctx);
#  else  /* ! mhd_TLS_MBED_INIT_TLS_REQ_RNG */
      res =  mbedtls_pk_check_pair (&(d_tls->cert_chain.pk),
                                    &(d_tls->prv_key));
#  endif  /* ! mhd_TLS_MBED_INIT_TLS_REQ_RNG */
      if ((0 == res) ||
          (MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE == res))
        return MHD_SC_OK; /* Success exit point */

      mhd_LOG_MSG (d, MHD_SC_TLS_CONF_BAD_CERT, \
                   "The private key data does not match the certificate");
      ret = MHD_SC_TLS_CONF_BAD_CERT;
#endif /* ! MHD_TLS_MBED_SKIP_CERT_KEY_MATCH_CHECK */
    }
    else
    {
      mhd_LOG_MSG (d, MHD_SC_TLS_CONF_BAD_CERT, \
                   "The private key data cannot be decoded");
      ret = MHD_SC_TLS_CONF_BAD_CERT;
    }

    mbedtls_pk_free (&(d_tls->prv_key));
  }
  else
  {
    mhd_LOG_PRINT (d,
                   MHD_SC_TLS_CONF_BAD_CERT,
                   mhd_LOG_FMT ("Failed to parse certificates chain. "
                                "Number of failed certificates: %i"),
                   res);
    ret = MHD_SC_TLS_CONF_BAD_CERT;
  }
  mbedtls_x509_crt_free (&(d_tls->cert_chain));

  mhd_assert (MHD_SC_OK != ret);
  return ret; /* Failure exit point */
}


/**
 * De-initialise daemon TLS credentials.
 * @param d_tls the daemon TLS settings
 */
static MHD_FN_PAR_NONNULL_ALL_ void
daemon_deinit_credentials (struct mhd_TlsMbedDaemonData *restrict d_tls)
{
  mbedtls_pk_free (&(d_tls->prv_key));

  mbedtls_x509_crt_free (&(d_tls->cert_chain));
}


#ifdef MBEDTLS_SSL_ALPN
/**
 * Initialise daemon ALPN data
 * This function puts error messages to the log if needed.
 * @param d the daemon handle
 * @param d_tls the daemon TLS settings
 * @param s the application-provided settings
 * @return #MHD_SC_OK on success,
 *         error code otherwise
 */
static MHD_FN_PAR_NONNULL_ALL_ MHD_FN_MUST_CHECK_RESULT_ enum MHD_StatusCode
daemon_set_alpn (struct MHD_Daemon *restrict d,
                 struct mhd_TlsMbedDaemonData *restrict d_tls,
                 struct DaemonOptions *restrict s)
{
  static const char alpn_str_http1_0[] = mhd_ALPN_H1_0;
  static const char alpn_str_http1_1[] = mhd_ALPN_H1_1;
#  ifdef MHD_SUPPORT_HTTP2
  static const char alpn_str_http2[] = mhd_ALPN_H2;
#  endif
  size_t i;

  (void) s; /* Unused currently. Implement reading allowed HTTP versions */

  i = 0u;
  // TODO: implement reading protocol versions from settings */
#ifdef MHD_SUPPORT_HTTP2
  if (1 /* enabled HTTP/2 ? */)
    d_tls->alpn_prots[i++] = alpn_str_http2;
#endif /* MHD_SUPPORT_HTTP2 */

  if (1 /* enabled HTTP/1.x ? */)
  {
    d_tls->alpn_prots[i++] = alpn_str_http1_1;
    d_tls->alpn_prots[i++] = alpn_str_http1_0;
  }

  d_tls->alpn_prots[i] = NULL; /* NULL termination */
  mhd_assert (mhd_ARR_NUM_ELEMS (d_tls->alpn_prots) > i);
  mhd_assert (0u != i);

  if (0 == mbedtls_ssl_conf_alpn_protocols (&(d_tls->tls_conf),
                                            d_tls->alpn_prots))
    return MHD_SC_OK; /* Success exit point */

  mhd_LOG_MSG (d, MHD_SC_TLS_DAEMON_INIT_FAILED, \
               "Failed to set ALPN data");
  return MHD_SC_TLS_DAEMON_INIT_FAILED;
}


#else  /* ! MBEDTLS_SSL_ALPN */
#  define daemon_set_alpn(d,d_tls,s) (MHD_SC_OK)
#endif /* ! MBEDTLS_SSL_ALPN */

/**
 * Set daemon TLS configuration.
 * This function puts error messages to the log if needed.
 * @param d the daemon handle
 * @param d_tls the daemon TLS settings
 * @param s the application-provided settings
 * @return #MHD_SC_OK on success,
 *         error code otherwise
 */
static MHD_FN_PAR_NONNULL_ALL_ MHD_FN_MUST_CHECK_RESULT_ enum MHD_StatusCode
daemon_init_config (struct MHD_Daemon *restrict d,
                    struct mhd_TlsMbedDaemonData *restrict d_tls,
                    struct DaemonOptions *restrict s)
{
  enum MHD_StatusCode ret;
#if defined(mhd_TLS_MBED_INIT_TLS_REQ_RNG)
  int (*rng_func)(void *ctx, unsigned char *out, size_t out_size);
  void *rng_ctx;

#  if defined(MHD_TLS_MBED_PREF_RNG_CTR)
  rng_func = &mbedtls_ctr_drbg_random;
  rng_ctx = &mhd_mbed_ctr_drbg_ctx;
#  elif defined(MHD_TLS_MBED_PREF_RNG_HMAC)
  rng_func = &mbedtls_hmac_drbg_random;
  rng_ctx = &mhd_mbed_hmac_drbg_ctx;
#  elif defined(MHD_TLS_MBED_PREF_RNG_PSA)
  rng_func = &mbedtls_psa_get_random;
  rng_ctx = MBEDTLS_PSA_RANDOM_STATE;
#  else  /* MHD_TLS_MBED_PREF_RNG_PSA */
  /* Support for external strong random generator could be added here */
#error No random generator is enabled in MbedTLS
  return MHD_SC_INTERNAL_ERROR;
#  endif /* MHD_TLS_MBED_PREF_RNG_PSA */

  ret = daemon_init_credentials (d,
                                 d_tls,
                                 s,
                                 rng_func,
                                 rng_ctx);
#else  /* mhd_TLS_MBED_INIT_TLS_REQ_RNG */
  ret = daemon_init_credentials (d,
                                 d_tls,
                                 s);
#endif /* mhd_TLS_MBED_INIT_TLS_REQ_RNG */

  if (MHD_SC_OK != ret)
    return ret;

  mbedtls_ssl_config_init (&(d_tls->tls_conf));

#ifdef mhd_TLS_MBED_HAS_DEBUG_PRINT
  mbedtls_ssl_conf_dbg (&(d_tls->tls_conf),
                        mhd_tls_mbed_debug_print,
                        NULL);
  mbedtls_debug_set_threshold (mhd_DBG_PRINT_LEVEL);
#endif /* mhd_TLS_MBED_HAS_DEBUG_PRINT */

  if (0 ==
      mbedtls_ssl_config_defaults (&(d_tls->tls_conf),
                                   MBEDTLS_SSL_IS_SERVER,
                                   MBEDTLS_SSL_TRANSPORT_STREAM,
                                   MBEDTLS_SSL_PRESET_DEFAULT))
  {
#if defined(mhd_TLS_MBED_INIT_TLS_REQ_RNG)
    mbedtls_ssl_conf_rng (&(d_tls->tls_conf),
                          rng_func,
                          rng_ctx);
#endif /* mhd_TLS_MBED_INIT_TLS_REQ_RNG */

    /* Client certificates are not implemented yet */
    mbedtls_ssl_conf_authmode (&(d_tls->tls_conf),
                               MBEDTLS_SSL_VERIFY_NONE);

    if (0 ==
        mbedtls_ssl_conf_own_cert (&(d_tls->tls_conf),
                                   &(d_tls->cert_chain),
                                   &(d_tls->prv_key)))
    {
      ret = daemon_set_alpn (d,
                             d_tls,
                             s);
      if (MHD_SC_OK == ret)
        return MHD_SC_OK; /* Success exit point */

      /* Below is a cleanup path */
    }
    else
      ret = MHD_SC_DAEMON_MEM_ALLOC_FAILURE; /* Do not waste binary space on the additional message */
  }
  else
    ret = MHD_SC_DAEMON_MEM_ALLOC_FAILURE; /* Do not waste binary space on the additional message */

  mbedtls_ssl_config_free (&(d_tls->tls_conf));

  daemon_deinit_credentials (d_tls);

  mhd_assert (MHD_SC_OK != ret);
  return ret; /* Failure exit point */
}


MHD_INTERNAL MHD_FN_MUST_CHECK_RESULT_ MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_ (3) mhd_StatusCodeInt
mhd_tls_mbed_daemon_init3 (struct MHD_Daemon *restrict d,
                           struct DaemonOptions *restrict s,
                           struct mhd_TlsMbedDaemonData **restrict p_d_tls)
{
  mhd_StatusCodeInt res;
  struct mhd_TlsMbedDaemonData *restrict d_tls;

  /* Successful initialisation must be checked earlier */
  mhd_assert (mbedtls_lib_inited_once);
  mhd_assert (mbedtls_lib_inited_now);

  res = check_app_tls_settings (d,
                                s);
  if (MHD_SC_OK != res)
    return res;

  d_tls = (struct mhd_TlsMbedDaemonData *)
          mhd_calloc (1, sizeof (struct mhd_TlsMbedDaemonData));
  *p_d_tls = d_tls;
  if (NULL == d_tls)
    return MHD_SC_DAEMON_MEM_ALLOC_FAILURE;

  res = daemon_init_config (d,
                            d_tls,
                            s);
  if (MHD_SC_OK == res)
  {
    return MHD_SC_OK; /* Success exit point */
  }
  /* Below is a clean-up code path */
  free (d_tls);
  *p_d_tls = NULL;
  mhd_assert (MHD_SC_OK != res);
  return res; /* Failure exit point */
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1) void
mhd_tls_mbed_daemon_deinit (struct mhd_TlsMbedDaemonData *restrict d_tls)
{
  mhd_assert (NULL != d_tls);

  mbedtls_ssl_config_free (&(d_tls->tls_conf));

  daemon_deinit_credentials (d_tls);

  free (d_tls);
}


/* ** Connection initialisation / de-initialisation ** */

MHD_INTERNAL size_t
mhd_tls_mbed_conn_get_tls_size_v (void)
{
  return sizeof (struct mhd_TlsMbedConnData);
}


/* Forward declarations of custom transport callbacks */
static int
mhd_mbed_cb_recv (void *ctx,
                  unsigned char *buf,
                  size_t size);

static int
mhd_mbed_cb_send (void *ctx,
                  const unsigned char *buf,
                  size_t size);

MHD_INTERNAL MHD_FN_MUST_CHECK_RESULT_ MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_ (3) bool
mhd_tls_mbed_conn_init (const struct mhd_TlsMbedDaemonData *restrict d_tls,
                        struct mhd_ConnSocket *sk,
                        struct mhd_TlsMbedConnData *restrict c_tls)
{
  c_tls->tr.sk = sk;

  mbedtls_ssl_init (&(c_tls->sess));

  if (0 == mbedtls_ssl_setup (&(c_tls->sess),
                              &(d_tls->tls_conf)))
  {
    mbedtls_ssl_set_bio (&(c_tls->sess),
                         c_tls,
                         &mhd_mbed_cb_send,
                         &mhd_mbed_cb_recv,
                         NULL /* no recv_timeout callback */);

#ifndef NDEBUG
    c_tls->dbg.is_inited = true;
#endif
    return true; /* Success exit point */
  }

  mbedtls_ssl_free (&(c_tls->sess));
  return false; /* Failure exit point */
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_tls_mbed_conn_deinit (struct mhd_TlsMbedConnData *restrict c_tls)
{
  mhd_assert (c_tls->dbg.is_inited);
  mbedtls_ssl_free (&(c_tls->sess));
#ifndef NDEBUG
  c_tls->dbg.is_inited = false;
#endif
}


/* ** Custom transport functions ** */

/**
 * Prepare for network operation
 * @param c_tls the connection TLS data
 */
mhd_static_inline MHD_FN_PAR_NONNULL_ALL_ void
mhd_tls_mbed_sckt_comm_prep (struct mhd_TlsMbedConnData *restrict c_tls)
{
  memset (&(c_tls->tr.state),
          0,
          sizeof(c_tls->tr.state));
}


/**
 * Prepare for send() network operation
 * @param c_tls the connection TLS data
 * @param unencr_size the size of the data to send before encryption
 * @param push_data set to 'false' if it is know that the data to be sent
 *                  is incomplete (message or chunk),
 *                  set to 'true' if the data is complete or the final part
 */
mhd_static_inline MHD_FN_PAR_NONNULL_ALL_ void
mhd_tls_mbed_sckt_comm_prep_send (struct mhd_TlsMbedConnData *restrict c_tls,
                                  size_t unencr_size,
                                  bool push_data)
{
  mhd_tls_mbed_sckt_comm_prep (c_tls);

  if (push_data)
    c_tls->tr.state.send_unenc_size = unencr_size;
  else
    c_tls->tr.state.send_unenc_size = (size_t) (~((size_t) 0));
}


/**
 * The callback which called by MbedTLS to receive the data
 * @param ctx the context for the send callback
 * @param buf the buffer to put received data
 * @param size the size of the @a buf
 * @return the positive number of bytes received on success,
 *         0 if EOF received (peer closed write/send),
 *         #MBEDTLS_ERR_SSL_WANT_READ if receiving would block OR
 *                                    receiving was interrupted,
 *         #MBEDTLS_ERR_NET_CONN_RESET if connection was broken
 *         or #MBEDTLS_ERR_NET_RECV_FAILED in case of other errors
 */
static int
mhd_mbed_cb_recv (void *ctx,
                  unsigned char *buf,
                  size_t size)
{
  struct mhd_TlsMbedConnData *const c_tls = (struct mhd_TlsMbedConnData *) ctx;
  struct mhd_TlsMbedConnCstmTrtState *const state = &(c_tls->tr.state);
  size_t received;

  /* MbedTLS may call recv() several times.
     This may result in unwanted extra syscalls, unfair connections
     processing or even blocking if socket is blocking.
     MHD limits to single send() syscall per operation to evenly distribute
     workload to all connections. */
  if (state->recv_called)
    return MBEDTLS_ERR_SSL_WANT_READ;

  /* MbedTLS may call blindly recv() after calling send() first.
     If send() was the first socket operation then the socket has been
     checked by MHD for 'send-ready' as receiving operation was expected.
     Do not use recv() if 'recv-ready' is not known and the socket is blocking.
   */
  if (state->send_called && ! c_tls->tr.sk->props.is_nonblck)
    return MBEDTLS_ERR_SSL_WANT_READ;

  if (1)
  {
    const int size_i = (int) size;

    if ((0 > size_i) ||
        (size != (size_t) size_i))
    {
      /* Return value limitation */
      size = (size_t) (((unsigned int) ~((unsigned int) 0)) >> 1u);
    }
  }

  state->recv_res = mhd_sckt_recv (c_tls->tr.sk,
                                   size,
                                   (char *) buf,
                                   &received);
  state->recv_called = true;

  if (mhd_SOCKET_ERR_NO_ERROR == state->recv_res)
  {
    mhd_ASSUME (size >= received);
    mhd_assert (0 <= (int) received);
    return (int) received;
  }

  if (mhd_SOCKET_ERR_INTR >= state->recv_res)
  {
    mhd_assert ((mhd_SOCKET_ERR_INTR == state->recv_res) ||
                (mhd_SOCKET_ERR_AGAIN == state->recv_res));
    return MBEDTLS_ERR_SSL_WANT_READ;
  }

  if (mhd_SOCKET_ERR_IS_HARD (state->recv_res))
  {
    c_tls->tr.sk->state.discnt_err = state->recv_res;
    return MBEDTLS_ERR_NET_CONN_RESET;
  }

  return MBEDTLS_ERR_NET_RECV_FAILED;
}


/**
 * The callback called by MbedTLS to send the data
 * @param ctx the context for the send callback
 * @param buf the buffer with the data to send
 * @param size the size of the data in the @a buf
 * @return the positive number of bytes sent on success,
 *         #MBEDTLS_ERR_SSL_WANT_WRITE if sending would block OR
 *                                     sending was interrupted,
 *         #MBEDTLS_ERR_NET_CONN_RESET if connection was broken
 *         or #MBEDTLS_ERR_NET_SEND_FAILED in case of other errors
 */
static int
mhd_mbed_cb_send (void *ctx,
                  const unsigned char *buf,
                  size_t size)
{
  struct mhd_TlsMbedConnData *const c_tls = (struct mhd_TlsMbedConnData *) ctx;
  struct mhd_TlsMbedConnCstmTrtState *const state = &(c_tls->tr.state);
  /* Check whether the complete data is sending.
     The compression is not used so the data after the encryption must not
     be smaller than before the encryption.
     The check may result in false-positive (unlikely in practice), but
     this should not hurt the performance. */
  bool push_data = (size >= state->send_unenc_size);
  size_t sent;

  /* MbedTLS may call send() several times in a loop, until all data is sent.
     This may result in unwanted extra syscalls, unfair connections processing
     or even blocking if socket is blocking.
     MHD limits to single send() syscall per operation to evenly distribute
     workload to all connections. */
  if (state->send_called)
    return MBEDTLS_ERR_SSL_WANT_WRITE;

  /* MbedTLS may call blindly send() after calling recv() first.
     If recv() was the first socket operation then the socket has been
     checked by MHD for 'recv-ready' as receiving operation was expected.
     Do not use send() if 'send-ready' is not known and the socket is blocking.
   */
  if (state->recv_called && ! c_tls->tr.sk->props.is_nonblck)
    return MBEDTLS_ERR_SSL_WANT_WRITE;

  if (1)
  {
    const int size_i = (int) size;

    if ((0 > size_i) ||
        (size != (size_t) size_i))
    {
      /* Return value limitation */
      size = (size_t) (((unsigned int) ~((unsigned int) 0)) >> 1u);
      push_data = false;
    }
  }

  state->send_res = mhd_sckt_send (c_tls->tr.sk,
                                   size,
                                   (const char *) buf,
                                   push_data,
                                   &sent);
  state->send_called = true;

  if (mhd_SOCKET_ERR_NO_ERROR == state->send_res)
  {
    mhd_ASSUME (size >= sent);
    mhd_assert (0 < (int) sent);
    return (int) sent;
  }

  if (mhd_SOCKET_ERR_INTR >= state->send_res)
  {
    mhd_assert ((mhd_SOCKET_ERR_INTR == state->send_res) ||
                (mhd_SOCKET_ERR_AGAIN == state->send_res));
    return MBEDTLS_ERR_SSL_WANT_WRITE;
  }

  if (mhd_SOCKET_ERR_IS_HARD (state->send_res))
  {
    c_tls->tr.sk->state.discnt_err = state->send_res;
    return MBEDTLS_ERR_NET_CONN_RESET;
  }

  return MBEDTLS_ERR_NET_SEND_FAILED;
}


/* ** TLS connection establishing ** */

MHD_INTERNAL MHD_FN_MUST_CHECK_RESULT_ MHD_FN_PAR_NONNULL_ALL_
enum mhd_TlsProcedureResult
mhd_tls_mbed_conn_handshake (struct mhd_TlsMbedConnData *c_tls)
{
  int res;

  mhd_assert (c_tls->dbg.is_inited);
  mhd_assert (! c_tls->dbg.is_tls_handshake_completed);
  mhd_assert (! c_tls->shut_tls_wr_sent);
  mhd_assert (! c_tls->shut_tls_wr_received);
  mhd_assert (! c_tls->dbg.is_failed);

  mhd_tls_mbed_sckt_comm_prep (c_tls);

  res = mbedtls_ssl_handshake (&(c_tls->sess));

  mhd_assert ((c_tls->tr.state.recv_called) ||
              (mhd_SOCKET_ERR_NO_ERROR == c_tls->tr.state.recv_res));
  mhd_assert ((c_tls->tr.state.send_called) ||
              (mhd_SOCKET_ERR_NO_ERROR == c_tls->tr.state.send_res));

  switch (res)
  {
  case 0:
#ifndef NDEBUG
    c_tls->dbg.is_tls_handshake_completed = true;
#endif /* ! NDEBUG */
    return mhd_TLS_PROCED_SUCCESS; /* Success exit point */

  case MBEDTLS_ERR_SSL_WANT_READ:
    mhd_assert (! mhd_SOCKET_ERR_IS_HARD (c_tls->tr.state.recv_res));
    mhd_assert (! mhd_SOCKET_ERR_IS_HARD (c_tls->tr.state.send_res));

    if (! c_tls->tr.state.recv_called)
      return mhd_TLS_PROCED_RECV_INTERRUPTED; /* Do not clear 'recv-ready' flag */

    if (mhd_SOCKET_ERR_AGAIN == c_tls->tr.state.recv_res)
      return mhd_TLS_PROCED_RECV_MORE_NEEDED; /* Clear 'recv-ready' flag */

    return mhd_TLS_PROCED_RECV_INTERRUPTED; /* Do not clear 'recv-ready' flag */

  case MBEDTLS_ERR_SSL_WANT_WRITE:
    mhd_assert (! mhd_SOCKET_ERR_IS_HARD (c_tls->tr.state.recv_res));
    mhd_assert (! mhd_SOCKET_ERR_IS_HARD (c_tls->tr.state.send_res));

    if (! c_tls->tr.state.send_called)
      return mhd_TLS_PROCED_SEND_INTERRUPTED; /* Do not clear 'send-ready' flag */

    if (mhd_SOCKET_ERR_AGAIN == c_tls->tr.state.send_res)
      return mhd_TLS_PROCED_SEND_MORE_NEEDED; /* Clear 'send-ready' flag */

    return mhd_TLS_PROCED_SEND_INTERRUPTED; /* Do not clear 'send-ready' flag */

  case MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS:
    mhd_assert (0 && "MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS must not be returned");
    break;

  case MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS:
    /* The result means that mbedtls_ssl_handshake() must be called again
       later.
       As this result does not map directly to any of available flags,
       so map it to "waiting for send-ready" as the socket should be already
       'send-ready'. */

    return (c_tls->tr.state.send_called &&
            (mhd_SOCKET_ERR_AGAIN == c_tls->tr.state.send_res)) ?
           mhd_TLS_PROCED_SEND_MORE_NEEDED : mhd_TLS_PROCED_SEND_INTERRUPTED;

  case MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA:
#ifdef MBEDTLS_SSL_EARLY_DATA
    /* Could be replaced with early data support is implemented */
#endif /* MBEDTLS_SSL_EARLY_DATA */
    mhd_assert (0 &&
                "MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA must not be returned");
    break;

  default:
    break; /* Handle other values below */
  }

  /* All other result codes must be interpreted as a hard error */
#ifndef NDEBUG
  c_tls->dbg.is_failed = true;
#endif /* ! NDEBUG */

  return mhd_TLS_PROCED_FAILED;
}


MHD_INTERNAL MHD_FN_MUST_CHECK_RESULT_ MHD_FN_PAR_NONNULL_ALL_
enum mhd_TlsProcedureResult
mhd_tls_mbed_conn_shutdown (struct mhd_TlsMbedConnData *c_tls)
{
  int res;

  mhd_assert (c_tls->dbg.is_inited);
  mhd_assert (c_tls->dbg.is_tls_handshake_completed);
  mhd_assert (! c_tls->dbg.is_failed);

  mhd_tls_mbed_sckt_comm_prep (c_tls);

  res = mbedtls_ssl_close_notify (&(c_tls->sess));

  switch (res)
  {
  case 0:
    c_tls->shut_tls_wr_sent = true;
    c_tls->shut_tls_wr_received = true;
    return mhd_TLS_PROCED_SUCCESS; /* Success exit point */

  case MBEDTLS_ERR_SSL_WANT_READ:
    mhd_assert (! mhd_SOCKET_ERR_IS_HARD (c_tls->tr.state.recv_res));
    mhd_assert (! mhd_SOCKET_ERR_IS_HARD (c_tls->tr.state.send_res));

    if (! c_tls->tr.state.recv_called)
      return mhd_TLS_PROCED_RECV_INTERRUPTED; /* Do not clear 'recv-ready' flag */

    if (mhd_SOCKET_ERR_AGAIN == c_tls->tr.state.recv_res)
      return mhd_TLS_PROCED_RECV_MORE_NEEDED; /* Clear 'recv-ready' flag */

    return mhd_TLS_PROCED_RECV_INTERRUPTED; /* Do not clear 'recv-ready' flag */

  case MBEDTLS_ERR_SSL_WANT_WRITE:
    mhd_assert (! mhd_SOCKET_ERR_IS_HARD (c_tls->tr.state.recv_res));
    mhd_assert (! mhd_SOCKET_ERR_IS_HARD (c_tls->tr.state.send_res));

    if (! c_tls->tr.state.send_called)
      return mhd_TLS_PROCED_SEND_INTERRUPTED; /* Do not clear 'send-ready' flag */

    if (mhd_SOCKET_ERR_AGAIN == c_tls->tr.state.send_res)
      return mhd_TLS_PROCED_SEND_MORE_NEEDED; /* Clear 'send-ready' flag */

    return mhd_TLS_PROCED_SEND_INTERRUPTED; /* Do not clear 'send-ready' flag */

  default:
    break; /* Handle other values below */
  }

  /* All other result codes must be interpreted as a hard error */
#ifndef NDEBUG
  c_tls->dbg.is_failed = true;
#endif /* ! NDEBUG */

  return mhd_TLS_PROCED_FAILED;
}


/* ** Data receiving and sending ** */

MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_SIZE_ (3,2)
MHD_FN_PAR_OUT_ (4) enum mhd_SocketError
mhd_tls_mbed_conn_recv (struct mhd_TlsMbedConnData *c_tls,
                        size_t buf_size,
                        char buf[MHD_FN_PAR_DYN_ARR_SIZE_ (buf_size)],
                        size_t *restrict received)
{
  int res;

  mhd_assert (0 != buf_size);

  mhd_assert (c_tls->dbg.is_inited);
  mhd_assert (c_tls->dbg.is_tls_handshake_completed);
  mhd_assert (! c_tls->shut_tls_wr_sent);
  mhd_assert (! c_tls->dbg.is_failed);

  if (1)
  {
    const int buf_size_i = (int) buf_size;
    if ((0 > buf_size_i) ||
        (buf_size != (size_t) buf_size_i))
    {
      /* Called function return value limitation */
      buf_size = (size_t) (((unsigned int) ~((unsigned int) 0)) >> 1u);
    }
  }

  c_tls->recv_data_in_buff = false;
  mhd_tls_mbed_sckt_comm_prep (c_tls);

  res = mbedtls_ssl_read (&(c_tls->sess),
                          (unsigned char *) buf,
                          buf_size);

  if (0 <= res)
  {
    mhd_ASSUME (buf_size >= (size_t) res);
    *received = (size_t) res;

    return mhd_SOCKET_ERR_NO_ERROR; /* Success exit point */
  }

  switch (res)
  {
  case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
    c_tls->shut_tls_wr_received = true;
    *received = 0u;

    return mhd_SOCKET_ERR_NO_ERROR; /* Success exit point */

  case MBEDTLS_ERR_SSL_WANT_READ:
    mhd_assert (! mhd_SOCKET_ERR_IS_HARD (c_tls->tr.state.recv_res));
    mhd_assert (! mhd_SOCKET_ERR_IS_HARD (c_tls->tr.state.send_res));

    if (! c_tls->tr.state.recv_called)
      return mhd_SOCKET_ERR_INTR; /* Do not clear 'recv-ready' flag */

    if (mhd_SOCKET_ERR_NO_ERROR == c_tls->tr.state.recv_res)
    {
      /* recv() succeed for the first time and then called again */
      return c_tls->tr.sk->props.is_nonblck ?
             mhd_SOCKET_ERR_INTR : mhd_SOCKET_ERR_AGAIN;
    }

    return c_tls->tr.state.recv_res;

  case MBEDTLS_ERR_SSL_WANT_WRITE:
    mhd_assert (0 &&
                "The handshake must be fully completed earlier");
    break;

  case MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS:
    mhd_assert (0 && "MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS must not be returned");
    break;

  case MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS:
    /* MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS means that recv() should be called
       again later. Pretend that data is already pending to not block on
       waiting for the new incoming data. */
    c_tls->recv_data_in_buff = true;
    return mhd_SOCKET_ERR_INTR; /* Do not clear 'recv-ready' flag */

  case MBEDTLS_ERR_SSL_CLIENT_RECONNECT:
    mhd_assert (0 &&
                "MBEDTLS_ERR_SSL_CLIENT_RECONNECT must not be "
                "returned for non-DTLS");
    break;

  case MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET:
    mhd_assert (0 &&
                "MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET must not be "
                "returned on the server side");
    break;

  case MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA:
#ifdef MBEDTLS_SSL_EARLY_DATA
    /* Could be replaced with early data support is implemented */
#endif /* MBEDTLS_SSL_EARLY_DATA */
    mhd_assert (0 &&
                "MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA must not be returned");
    break;

  default:
    break; /* Handle other values below */
  }

  /* Treat all other kinds of errors as hard errors */
#ifndef NDEBUG
  c_tls->dbg.is_failed = true;
#endif /* ! NDEBUG */
  return mhd_SOCKET_ERR_TLS;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_tls_mbed_conn_has_data_in (struct mhd_TlsMbedConnData *restrict c_tls)
{
  return c_tls->recv_data_in_buff ||
         (0 != mbedtls_ssl_check_pending (&(c_tls->sess)));
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (3,2)
MHD_FN_PAR_OUT_ (5) enum mhd_SocketError
mhd_tls_mbed_conn_send (struct mhd_TlsMbedConnData *c_tls,
                        size_t buf_size,
                        const char buf[MHD_FN_PAR_DYN_ARR_SIZE_ (buf_size)],
                        bool push_data,
                        size_t *restrict sent)
{
  int res;

  mhd_assert (0 != buf_size);

  mhd_assert (c_tls->dbg.is_inited);
  mhd_assert (c_tls->dbg.is_tls_handshake_completed);
  mhd_assert (! c_tls->shut_tls_wr_sent);
  mhd_assert (! c_tls->dbg.is_failed);

  if (1)
  {
    const int buf_size_i = (int) buf_size;
    if ((0 > buf_size_i) ||
        (buf_size != (size_t) buf_size_i))
    {
      /* Called function return value limitation */
      buf_size = (size_t) (((unsigned int) ~((unsigned int) 0)) >> 1u);
      push_data = false;
    }
  }

  mhd_tls_mbed_sckt_comm_prep_send (c_tls,
                                    buf_size,
                                    push_data);

  res = mbedtls_ssl_write (&(c_tls->sess),
                           (const unsigned char *) buf,
                           buf_size);

  if (0 < res)
  {
    mhd_ASSUME (buf_size >= (size_t) res);
    *sent = (size_t) res;

    return mhd_SOCKET_ERR_NO_ERROR; /* Success exit point */
  }

  switch (res)
  {
  case 0:
    mhd_assert (0 &&
                "Zero must not be returned when sending non-zero size");
    break;

  case MBEDTLS_ERR_SSL_WANT_WRITE:
    mhd_assert (! mhd_SOCKET_ERR_IS_HARD (c_tls->tr.state.send_res));
    mhd_assert (! mhd_SOCKET_ERR_IS_HARD (c_tls->tr.state.recv_res));

    if (! c_tls->tr.state.send_called)
      return mhd_SOCKET_ERR_INTR; /* Do not clear 'recv-ready' flag */

    if (mhd_SOCKET_ERR_NO_ERROR == c_tls->tr.state.send_res)
    {
      /* send() succeed for the first time and then called again */
      return c_tls->tr.sk->props.is_nonblck ?
             mhd_SOCKET_ERR_INTR : mhd_SOCKET_ERR_AGAIN;
    }

    return c_tls->tr.state.send_res;

  case MBEDTLS_ERR_SSL_WANT_READ:
    mhd_assert (0 &&
                "The handshake must be fully completed earlier");
    break;

  case MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS:
    mhd_assert (0 && "MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS must not be returned");
    break;

  case MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS:
    /* MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS means that send() should be called
       again later. Wait for 'send-ready' which should be already set or
       will be set later, when OS pushed the data to the network. */
    return mhd_SOCKET_ERR_INTR; /* Do not clear 'recv-ready' flag */

  case MBEDTLS_ERR_SSL_CLIENT_RECONNECT:
    mhd_assert (0 &&
                "MBEDTLS_ERR_SSL_CLIENT_RECONNECT must not be "
                "returned for non-DTLS");
    break;

  case MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET:
    mhd_assert (0 &&
                "MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET must not be "
                "returned on the server side");
    break;

  case MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA:
#ifdef MBEDTLS_SSL_EARLY_DATA
    /* Could be replaced with early data support is implemented */
#endif /* MBEDTLS_SSL_EARLY_DATA */
    mhd_assert (0 &&
                "MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA must not be returned");
    break;

  default:
    break; /* Handle other values below */
  }

  /* Treat all other kinds of errors as hard errors */
#ifndef NDEBUG
  c_tls->dbg.is_failed = true;
#endif /* ! NDEBUG */
  return mhd_SOCKET_ERR_TLS;
}


/* ** TLS connection information ** */

MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_ (2) void
mhd_tls_mbed_conn_get_tls_sess (
  struct mhd_TlsMbedConnData *restrict c_tls,
  union MHD_ConnInfoDynamicTlsSess *restrict tls_sess_out)
{
  tls_sess_out->v_mbedtls_session = &(c_tls->sess);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_ (2) bool
mhd_tls_mbed_conn_get_tls_ver (struct mhd_TlsMbedConnData *restrict c_tls,
                               struct mhd_StctTlsVersion *restrict tls_ver_out)
{
  mhd_assert (c_tls->dbg.is_tls_handshake_completed);

#ifndef MBEDTLS_VERSION_NUMBER
  if (1)
    return false; /* Need MbedTLS version number to implement */
#else  /* MBEDTLS_VERSION_NUMBER */
  if (1)
  {
    uint_fast16_t tls_ver_num;
#if ((MBEDTLS_VERSION_NUMBER + 0) >= 0x03020000)
    mbedtls_ssl_protocol_version mbedtls_tls_ver;

    mbedtls_tls_ver = mbedtls_ssl_get_version_number (&(c_tls->sess));

    tls_ver_num = (uint_fast16_t) mbedtls_tls_ver;
#else  /* MBEDTLS_VERSION_NUMBER < 0x03020000 */
    tls_ver_num = (uint_fast16_t) c_tls->sess.MBEDTLS_PRIVATE (major_ver);
    tls_ver_num <<= 8u;
    tls_ver_num |= (uint_fast16_t) c_tls->sess.MBEDTLS_PRIVATE (minor_ver);
#endif /* MBEDTLS_VERSION_NUMBER < 0x03020000 */
    /* Avoid MbedTLS helper macros and enum values in switch() as they are
       unstable in MbedTLS. */
    switch (tls_ver_num)
    {
    case 0u:
      return false;

    case 0x0301u: /* Not really supported by MbedTLS >=3.0 */
      tls_ver_out->tls_ver = MHD_TLS_VERSION_1_0;
      break;

    case 0x0302u: /* Not really supported by MbedTLS >=3.0 */
      tls_ver_out->tls_ver = MHD_TLS_VERSION_1_1;
      break;

    case 0x0303u:
      tls_ver_out->tls_ver = MHD_TLS_VERSION_1_2;
      break;

    case 0x0304u:
      tls_ver_out->tls_ver = MHD_TLS_VERSION_1_3;
      break;

    default:
      tls_ver_out->tls_ver = MHD_TLS_VERSION_UNKNOWN;
      break;
    }
  }
#endif /* MBEDTLS_VERSION_NUMBER */

  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ enum mhd_TlsAlpnProt
mhd_tls_mbed_conn_get_alpn_prot (struct mhd_TlsMbedConnData *restrict c_tls)
{
#ifdef MBEDTLS_SSL_ALPN
  const char *alpn_str;

  alpn_str = mbedtls_ssl_get_alpn_protocol (&(c_tls->sess));
  if (NULL == alpn_str)
    return mhd_TLS_ALPN_PROT_NOT_SELECTED;

  return mhd_tls_alpn_decode_n (strlen (alpn_str),
                                (const unsigned char *) alpn_str);
#else  /* ! MBEDTLS_SSL_ALPN */
  return mhd_TLS_ALPN_PROT_NOT_SELECTED;
#endif /* ! MBEDTLS_SSL_ALPN */
}
