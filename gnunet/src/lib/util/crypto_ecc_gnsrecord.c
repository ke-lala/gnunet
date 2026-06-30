/*
     This file is part of GNUnet.
     Copyright (C) 2012, 2013, 2015 GNUnet e.V.

     GNUnet is free software: you can redistribute it and/or modify it
     under the terms of the GNU Affero General Public License as published
     by the Free Software Foundation, either version 3 of the License,
     or (at your option) any later version.

     GNUnet is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Affero General Public License for more details.

     You should have received a copy of the GNU Affero General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.

     SPDX-License-Identifier: AGPL3.0-or-later
 */

/**
 * @file util/crypto_ecc_gnsrecord.c
 * @brief public key cryptography (ECC) for GNS records (LSD0001)
 * @author Christian Grothoff
 * @author Florian Dold
 * @author Martin Schanzenbach
 */

#include "platform.h"
#include <sodium.h>
#include "gnunet_util_lib.h"

#define CURVE "Ed25519"

/**
 * Derive the 'h' value for key derivation, where
 * 'h = H(l,P)'.
 *
 * @param pub public key for deriviation
 * @param pubsize the size of the public key
 * @param label label for deriviation
 * @param context additional context to use for HKDF of 'h';
 *        typically the name of the subsystem/application
 * @param hc where to write the result
 */
static void
derive_h (const void *pub,
          size_t pubsize,
          const char *label,
          const char *context,
          struct GNUNET_HashCode *hc)
{
  /** NOTE: While (H)KDF calls this value a salt
   *  it is not necessary for it to be a random value.
   *  It is more common to use a NULL value here
   *  (https://www.rfc-editor.org/rfc/rfc8446#section-7.1)
   *  But it is safe either way (See RFC 5869)
   */
  static const char *const salt = "key-derivation";

  GNUNET_CRYPTO_hkdf_gnunet (
    hc,
    sizeof(*hc),
    salt,
    strlen (salt),
    pub,
    pubsize,
    GNUNET_CRYPTO_kdf_arg_string (label),
    GNUNET_CRYPTO_kdf_arg_string (context));
}


enum GNUNET_GenericReturnValue
GNUNET_CRYPTO_eddsa_sign_derived (
  const struct GNUNET_CRYPTO_EddsaPrivateKey *pkey,
  const char *label,
  const char *context,
  const struct GNUNET_CRYPTO_SignaturePurpose *purpose,
  struct GNUNET_CRYPTO_EddsaSignature *sig)
{
  struct GNUNET_CRYPTO_EddsaPrivateScalar priv;
  crypto_hash_sha512_state hs;
  unsigned char sk[64];
  unsigned char r[64];
  unsigned char hram[64];
  unsigned char R[32];
  unsigned char zk[32];
  unsigned char tmp[32];
  unsigned char r_mod[64];
  unsigned char hram_mod[64];

  /**
   * Derive the private key
   */
  GNUNET_CRYPTO_eddsa_private_key_derive (pkey,
                                          label,
                                          context,
                                          &priv);

  crypto_hash_sha512_init (&hs);

  /**
   * Instead of expanding the private here, we already
   * have the secret scalar as input. Use it.
   * Note that sk is not plain SHA512 (d).
   * sk[0..31] contains the derived private scalar
   * sk[0..31] = h * SHA512 (d)[0..31]
   * sk[32..63] = SHA512 (d)[32..63]
   */
  memcpy (sk, priv.s, 64);

  /**
   * Calculate the derived zone key zk' from the
   * derived private scalar.
   */
  crypto_scalarmult_ed25519_base_noclamp (zk,
                                          sk);

  /**
   * Calculate r:
   * r = SHA512 (sk[32..63] | M)
   * where M is our message (purpose).
   * Note that sk[32..63] is the other half of the
   * expansion from the original, non-derived private key
   * "d".
   */
  crypto_hash_sha512_update (&hs, sk + 32, 32);
  crypto_hash_sha512_update (&hs, (uint8_t*) purpose, ntohl (purpose->size));
  crypto_hash_sha512_final (&hs, r);

  /**
   * Temporarily put zk into S
   */
  memcpy (sig->s, zk, 32);

  /**
   * Reduce the scalar value r
   */
  crypto_core_ed25519_scalar_reduce (r_mod, r);

  /**
   * Calculate R := r * G of the signature
   */
  crypto_scalarmult_ed25519_base_noclamp (R, r_mod);
  memcpy (sig->r, R, sizeof (R));

  /**
   * Calculate
   * hram := SHA512 (R | zk' | M)
   */
  crypto_hash_sha512_init (&hs);
  crypto_hash_sha512_update (&hs, (uint8_t*) sig, 64);
  crypto_hash_sha512_update (&hs, (uint8_t*) purpose,
                             ntohl (purpose->size));
  crypto_hash_sha512_final (&hs, hram);

  /**
   * Reduce the resulting scalar value
   */
  crypto_core_ed25519_scalar_reduce (hram_mod, hram);

  /**
   * Calculate
   * S := r + hram * s mod L
   */
  crypto_core_ed25519_scalar_mul (tmp, hram_mod, sk);
  crypto_core_ed25519_scalar_add (sig->s, tmp, r_mod);

  sodium_memzero (sk, sizeof (sk));
  sodium_memzero (r, sizeof (r));
  sodium_memzero (r_mod, sizeof (r_mod));
  return GNUNET_OK;
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
enum GNUNET_GenericReturnValue
GNUNET_CRYPTO_ecdsa_sign_derived (
  const struct GNUNET_CRYPTO_EcdsaPrivateKey *priv,
  const char *label,
  const char *context,
  const struct GNUNET_CRYPTO_SignaturePurpose *purpose,
  struct GNUNET_CRYPTO_EcdsaSignature *sig)
{
  struct GNUNET_CRYPTO_EcdsaPrivateKey *key;
  enum GNUNET_GenericReturnValue res;
  key = GNUNET_CRYPTO_ecdsa_private_key_derive (priv,
                                                label,
                                                context);
  res = GNUNET_CRYPTO_ecdsa_sign_ (key,
                                   purpose,
                                   sig);
  GNUNET_free (key);
  return res;
}


struct GNUNET_CRYPTO_EcdsaPrivateKey *
GNUNET_CRYPTO_ecdsa_private_key_derive (
  const struct GNUNET_CRYPTO_EcdsaPrivateKey *priv,
  const char *label,
  const char *context)
{
  struct GNUNET_CRYPTO_EcdsaPublicKey pub;
  struct GNUNET_CRYPTO_EcdsaPrivateKey *ret;
  struct GNUNET_HashCode h;
  unsigned char h_mod_L[crypto_core_ed25519_SCALARBYTES];
  unsigned char h_le[64];


  ret = GNUNET_new (struct GNUNET_CRYPTO_EcdsaPrivateKey);
  GNUNET_CRYPTO_ecdsa_key_get_public (priv, &pub);

  derive_h (&pub, sizeof (pub), label, context, &h);

  /**
   * "The result of the HKDF must be clamped and interpreted in network byte order. " -- RFC 9498
   * We need to convert for libsodium
   */
  for (size_t i = 0; i < 64; i++)
    h_le[i] = ((unsigned char*) &h)[63 - i];

  /**
   * hc_le now contains the scalar h.
   * The private scalar a is sk[0:31]
   * We calculate:
   * d' := h * a mod L
   */
  crypto_core_ed25519_scalar_reduce (h_mod_L,
                                     (unsigned char*) &h_le);
  crypto_core_ed25519_scalar_mul (ret->d, h_mod_L, priv->d);
  return ret;
}


void
GNUNET_CRYPTO_ecdsa_public_key_derive (
  const struct GNUNET_CRYPTO_EcdsaPublicKey *pub,
  const char *label,
  const char *context,
  struct GNUNET_CRYPTO_EcdsaPublicKey *result)
{
  struct GNUNET_HashCode hc;
  unsigned char h_mod_L[crypto_core_ed25519_SCALARBYTES];
  unsigned char h_le[64];

  derive_h (pub, sizeof (*pub), label, context, &hc);
  /**
   * "The result of the HKDF must be clamped and interpreted in network byte order. " -- RFC 9498
   * We need to convert for libsodium
   */
  for (size_t i = 0; i < 64; i++)
    h_le[i] = ((unsigned char*) &hc)[63 - i];


  /**
   * We calculate:
   * Q := h * P mod L
   */
  crypto_core_ed25519_scalar_reduce (h_mod_L,
                                     (unsigned char*) &h_le);
  GNUNET_assert (0 == crypto_scalarmult_ed25519_noclamp (result->q_y,
                                                         h_mod_L,
                                                         pub->q_y));
}


#pragma GCC diagnostic pop


void
GNUNET_CRYPTO_eddsa_private_key_derive (
  const struct GNUNET_CRYPTO_EddsaPrivateKey *priv,
  const char *label,
  const char *context,
  struct GNUNET_CRYPTO_EddsaPrivateScalar *result)
{
  struct GNUNET_CRYPTO_EddsaPublicKey pub;
  struct GNUNET_HashCode h;
  unsigned char h_le[64];
  unsigned char sk[64];
  unsigned char *d;
  unsigned char *nonce;
  unsigned char h_mod_L[crypto_core_ed25519_SCALARBYTES];

  d = result->s;
  nonce = result->s + 32;

  /**
   * This is the standard private key expansion in Ed25519.
   * The first 32 octets are used as a little-endian private
   * scalar.
   * We derive this scalar using our "h".
   */
  crypto_hash_sha512 (sk, priv->d, 32);
  sk[0] &= 248;
  sk[31] &= 127;
  sk[31] |= 64;

  /**
   * Get h mod L
   */
  GNUNET_CRYPTO_eddsa_key_get_public (priv, &pub);
  derive_h (&pub, sizeof (pub), label, context, &h);

  /**
   * "The result of the HKDF must be clamped and interpreted in network byte order. " -- RFC 9498
   * We need to convert for libsodium
   */
  for (size_t i = 0; i < 64; i++)
    h_le[i] = ((unsigned char*) &h)[63 - i];


  /**
   * h_le now contains the scalar h.
   * The private scalar a is sk[0:31]
   * We calculate:
   * d' := h * a mod L
   */
  crypto_core_ed25519_scalar_reduce (h_mod_L,
                                     (unsigned char*) &h_le);
  crypto_core_ed25519_scalar_mul (d, h_mod_L, sk);

  {
    /**
     * We hash the derived "h" parameter with the
     * other half of the expanded private key. This ensures
     * that for signature generation, the "R" is derived from
     * the same derivation path as "h" and is not reused.
     */
    crypto_hash_sha256_state hs;
    crypto_hash_sha256_init (&hs);
    crypto_hash_sha256_update (&hs, sk + 32, 32);
    crypto_hash_sha256_update (&hs, (unsigned char*) &h, sizeof (h));
    crypto_hash_sha256_final (&hs, nonce);
  }

}


void
GNUNET_CRYPTO_eddsa_public_key_derive (
  const struct GNUNET_CRYPTO_EddsaPublicKey *pub,
  const char *label,
  const char *context,
  struct GNUNET_CRYPTO_EddsaPublicKey *result)
{
  struct GNUNET_HashCode h;
  unsigned char h_le[64];
  unsigned char h_mod_L[crypto_core_ed25519_SCALARBYTES];

  /* calculate h_mod_n = h % n */
  derive_h (pub, sizeof (*pub), label, context, &h);

  /**
   * "The result of the HKDF must be clamped and interpreted in network byte order. " -- RFC 9498
   * We need to convert for libsodium
   */
  for (size_t i = 0; i < 64; i++)
    h_le[i] = ((unsigned char*) &h)[63 - i];

  /**
   * h_le now contains the scalar h.
   * We calculate:
   * Q := h * P mod L
   */
  crypto_core_ed25519_scalar_reduce (h_mod_L,
                                     (unsigned char*) &h_le);

  GNUNET_assert (0 == crypto_scalarmult_ed25519_noclamp (result->q_y,
                                                         h_mod_L,
                                                         pub->q_y));
}


void
GNUNET_CRYPTO_eddsa_key_get_public_from_scalar (
  const struct GNUNET_CRYPTO_EddsaPrivateScalar *priv,
  struct GNUNET_CRYPTO_EddsaPublicKey *pkey)
{
  unsigned char sk[32];

  memcpy (sk, priv->s, 32);

  /**
   * Calculate the derived zone key zk' from the
   * derived private scalar.
   */
  crypto_scalarmult_ed25519_base_noclamp (pkey->q_y,
                                          sk);
}
