/*
     This file is part of GNUnet.
     Copyright (C) 2009-2013, 2018 GNUnet e.V.

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
 * @file gnsrecord/gnsrecord_crypto.c
 * @brief API for GNS record-related crypto
 * @author Martin Schanzenbach
 * @author Matthias Wachs
 * @author Christian Grothoff
 */
#include "platform.h"
#include "gnsrecord_crypto.h"

#define LOG(kind, ...) GNUNET_log_from (kind, "gnsrecord", __VA_ARGS__)

/**
 * We disable deprecation warnings because we implement
 * RFC9408/LSD0001 record types here.
 * We may eventually stop doing that, however!
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
void
GNR_derive_block_aes_key (unsigned char *ctr,
                          unsigned char *key,
                          const char *label,
                          uint64_t exp,
                          const struct GNUNET_CRYPTO_EcdsaPublicKey *pub)
{
  static const char ctx_key[] = "gns-aes-ctx-key";
  static const char ctx_iv[] = "gns-aes-ctx-iv";

  GNUNET_CRYPTO_hkdf_gnunet (key, GNUNET_CRYPTO_AES_KEY_LENGTH,
                             ctx_key, strlen (ctx_key),
                             pub, sizeof(struct
                                         GNUNET_CRYPTO_EcdsaPublicKey),
                             GNUNET_CRYPTO_kdf_arg_string (label));
  memset (ctr, 0, GNUNET_CRYPTO_AES_KEY_LENGTH / 2);
  /** 4 byte nonce **/
  GNUNET_CRYPTO_hkdf_gnunet (ctr, 4,
                             ctx_iv, strlen (ctx_iv),
                             pub, sizeof(struct
                                         GNUNET_CRYPTO_EcdsaPublicKey),
                             GNUNET_CRYPTO_kdf_arg_string (label));
  /** Expiration time 64 bit. **/
  memcpy (ctr + 4, &exp, sizeof (exp));
  /** Set counter part to 1 **/
  ctr[15] |= 0x01;
}


void
GNR_derive_block_xsalsa_key (struct GNUNET_CRYPTO_XSalsa20Nonce *nonce,
                             struct GNUNET_CRYPTO_XSalsa20SecretKey *key,
                             const char *label,
                             uint64_t exp,
                             const struct GNUNET_CRYPTO_EddsaPublicKey *pub)
{
  static const char ctx_key[] = "gns-xsalsa-ctx-key";
  static const char ctx_iv[] = "gns-xsalsa-ctx-iv";

  GNUNET_CRYPTO_hkdf_gnunet (
    key, crypto_secretbox_KEYBYTES,
    ctx_key, strlen (ctx_key),
    pub, sizeof(struct GNUNET_CRYPTO_EddsaPublicKey),
    GNUNET_CRYPTO_kdf_arg_string (label));
  memset (nonce, 0, crypto_secretbox_NONCEBYTES);
  /** 16 byte nonce **/
  GNUNET_CRYPTO_hkdf_gnunet (
    nonce, (crypto_secretbox_NONCEBYTES - sizeof (exp)),
    ctx_iv, strlen (ctx_iv),
    pub, sizeof(struct GNUNET_CRYPTO_EddsaPublicKey),
    GNUNET_CRYPTO_kdf_arg_string (label));
  /** Expiration time 64 bit. **/
  memcpy (&nonce->nonce[crypto_secretbox_NONCEBYTES - sizeof (exp)],
          &exp,
          sizeof (exp));
}


static enum GNUNET_GenericReturnValue
block_sign_ecdsa (const struct
                  GNUNET_CRYPTO_EcdsaPrivateKey *key,
                  const struct
                  GNUNET_CRYPTO_EcdsaPublicKey *pkey,
                  const char *label,
                  struct GNUNET_GNSRECORD_Block *block)
{
  struct GNRBlockPS *gnr_block;
  struct GNUNET_GNSRECORD_EcdsaBlock *ecblock;
  size_t size = ntohl (block->size) - sizeof (*block) + sizeof (*gnr_block);

  gnr_block = GNUNET_malloc (size);
  ecblock = &(block)->ecdsa_block;
  gnr_block->purpose.size = htonl (size);
  gnr_block->purpose.purpose =
    htonl (GNUNET_SIGNATURE_PURPOSE_GNS_RECORD_SIGN);
  gnr_block->expiration_time = ecblock->expiration_time;
  /* encrypt and sign */
  GNUNET_memcpy (&gnr_block[1], &ecblock[1],
                 size - sizeof (*gnr_block));
  GNUNET_CRYPTO_ecdsa_public_key_derive (pkey,
                                         label,
                                         "gns",
                                         &ecblock->derived_key);
  if (GNUNET_OK !=
      GNUNET_CRYPTO_ecdsa_sign_derived (key,
                                        label,
                                        "gns",
                                        &gnr_block->purpose,
                                        &ecblock->signature))
  {
    GNUNET_break (0);
    GNUNET_free (gnr_block);
    return GNUNET_SYSERR;
  }
  GNUNET_free (gnr_block);
  return GNUNET_OK;
}


static enum GNUNET_GenericReturnValue
block_sign_eddsa (const struct
                  GNUNET_CRYPTO_EddsaPrivateKey *key,
                  const struct
                  GNUNET_CRYPTO_EddsaPublicKey *pkey,
                  const char *label,
                  struct GNUNET_GNSRECORD_Block *block)
{
  struct GNRBlockPS *gnr_block;
  struct GNUNET_GNSRECORD_EddsaBlock *edblock;
  size_t size = ntohl (block->size) - sizeof (*block) + sizeof (*gnr_block);
  gnr_block = GNUNET_malloc (size);
  edblock = &(block)->eddsa_block;
  gnr_block->purpose.size = htonl (size);
  gnr_block->purpose.purpose =
    htonl (GNUNET_SIGNATURE_PURPOSE_GNS_RECORD_SIGN);
  gnr_block->expiration_time = edblock->expiration_time;
  GNUNET_memcpy (&gnr_block[1], &edblock[1],
                 size - sizeof (*gnr_block));
  /* encrypt and sign */
  GNUNET_CRYPTO_eddsa_public_key_derive (pkey,
                                         label,
                                         "gns",
                                         &edblock->derived_key);
  GNUNET_CRYPTO_eddsa_sign_derived (key,
                                    label,
                                    "gns",
                                    &gnr_block->purpose,
                                    &edblock->signature);
  GNUNET_free (gnr_block);
  return GNUNET_OK;
}


enum GNUNET_GenericReturnValue
GNUNET_GNSRECORD_block_sign (const struct
                             GNUNET_CRYPTO_BlindablePrivateKey *key,
                             const char *label,
                             struct GNUNET_GNSRECORD_Block *block)
{
  struct GNUNET_CRYPTO_BlindablePublicKey pkey;
  enum GNUNET_GenericReturnValue res = GNUNET_SYSERR;
  char *norm_label;

  GNUNET_CRYPTO_blindable_key_get_public (key,
                                          &pkey);
  norm_label = GNUNET_GNSRECORD_string_normalize (label);

  switch (ntohl (key->type))
  {
  case GNUNET_GNSRECORD_TYPE_PKEY:
    res = block_sign_ecdsa (&key->ecdsa_key,
                            &pkey.ecdsa_key,
                            norm_label,
                            block);
    break;
  case GNUNET_GNSRECORD_TYPE_EDKEY:
    res = block_sign_eddsa (&key->eddsa_key,
                            &pkey.eddsa_key,
                            norm_label,
                            block);
    break;
  default:
    GNUNET_assert (0);
  }
  GNUNET_free (norm_label);
  return res;
}


/**
 * Sign name and records
 *
 * @param key the private key
 * @param pkey associated public key
 * @param expire block expiration
 * @param label the name for the records
 * @param rd record data
 * @param rd_count number of records
 * @param block the block result. Must be allocated sufficiently.
 * @param sign sign the block GNUNET_NO if block will be signed later.
 * @return GNUNET_SYSERR on error (otherwise GNUNET_OK)
 */
static enum GNUNET_GenericReturnValue
block_create_ecdsa (const struct GNUNET_CRYPTO_EcdsaPrivateKey *key,
                    const struct GNUNET_CRYPTO_EcdsaPublicKey *pkey,
                    struct GNUNET_TIME_Absolute expire,
                    const char *label,
                    const unsigned char *rdata,
                    size_t rdata_len,
                    struct GNUNET_GNSRECORD_Block **block,
                    int sign)
{
  struct GNUNET_GNSRECORD_EcdsaBlock *ecblock;
  unsigned char ctr[GNUNET_CRYPTO_AES_KEY_LENGTH / 2];
  unsigned char skey[GNUNET_CRYPTO_AES_KEY_LENGTH];

  if (rdata_len > GNUNET_GNSRECORD_MAX_BLOCK_SIZE)
  {
    GNUNET_break (0);
    return GNUNET_SYSERR;
  }
  /* serialize */
  *block = GNUNET_malloc (sizeof (struct GNUNET_GNSRECORD_Block) + rdata_len);
  (*block)->size = htonl (sizeof (struct GNUNET_GNSRECORD_Block) + rdata_len);
  {
    ecblock = &(*block)->ecdsa_block;
    (*block)->type = htonl (GNUNET_GNSRECORD_TYPE_PKEY);
    ecblock->expiration_time = GNUNET_TIME_absolute_hton (expire);
    GNR_derive_block_aes_key (ctr,
                              skey,
                              label,
                              ecblock->expiration_time.abs_value_us__,
                              pkey);
    GNUNET_CRYPTO_aes_ctr (rdata,
                           rdata_len,
                           skey,
                           ctr,
                           &ecblock[1]);
  }
  if (GNUNET_YES != sign)
    return GNUNET_OK;
  if (GNUNET_OK !=
      block_sign_ecdsa (key, pkey, label, *block))
  {
    GNUNET_break (0);
    GNUNET_free (*block);
    return GNUNET_SYSERR;
  }
  return GNUNET_OK;
}


/**
 * Sign name and records (EDDSA version)
 *
 * @param key the private key
 * @param pkey associated public key
 * @param expire block expiration
 * @param label the name for the records
 * @param rd record data
 * @param rd_count number of records
 * @param block where to store the block. Must be allocated sufficiently.
 * @param sign GNUNET_YES if block shall be signed as well
 * @return GNUNET_SYSERR on error (otherwise GNUNET_OK)
 */
static enum GNUNET_GenericReturnValue
block_create_eddsa (const struct GNUNET_CRYPTO_EddsaPrivateKey *key,
                    const struct GNUNET_CRYPTO_EddsaPublicKey *pkey,
                    struct GNUNET_TIME_Absolute expire,
                    const char *label,
                    const unsigned char *rdata,
                    size_t rdata_len,
                    struct GNUNET_GNSRECORD_Block **block,
                    int sign)
{
  struct GNUNET_GNSRECORD_EddsaBlock *edblock;
  struct GNUNET_CRYPTO_XSalsa20SecretKey skey;
  struct GNUNET_CRYPTO_XSalsa20Nonce nonce;

  if (rdata_len > GNUNET_GNSRECORD_MAX_BLOCK_SIZE)
  {
    GNUNET_break (0);
    return GNUNET_SYSERR;
  }
  /* serialize */
  *block = GNUNET_malloc (sizeof (struct GNUNET_GNSRECORD_Block)
                          + rdata_len + crypto_secretbox_MACBYTES);
  (*block)->size = htonl (sizeof (struct GNUNET_GNSRECORD_Block)
                          + rdata_len + crypto_secretbox_MACBYTES);
  {
    edblock = &(*block)->eddsa_block;
    (*block)->type = htonl (GNUNET_GNSRECORD_TYPE_EDKEY);
    edblock->expiration_time = GNUNET_TIME_absolute_hton (expire);
    GNR_derive_block_xsalsa_key (&nonce,
                                 &skey,
                                 label,
                                 edblock->expiration_time.abs_value_us__,
                                 pkey);
    GNUNET_assert (GNUNET_OK ==
                   GNUNET_CRYPTO_xsalsa20poly1305_encrypt (
                     rdata_len,
                     (unsigned char*) rdata,
                     &skey,
                     &nonce,
                     &edblock[1]));
    if (GNUNET_YES != sign)
      return GNUNET_OK;
    block_sign_eddsa (key, pkey, label, *block);
  }
  return GNUNET_OK;
}


/**
 * Line in cache mapping private keys to public keys.
 */
struct KeyCacheLine
{
  /**
   * A private key.
   */
  struct GNUNET_CRYPTO_EcdsaPrivateKey key;

  /**
   * Associated public key.
   */
  struct GNUNET_CRYPTO_EcdsaPublicKey pkey;
};

static enum GNUNET_GenericReturnValue
block_create2 (const struct GNUNET_CRYPTO_BlindablePrivateKey *pkey,
               struct GNUNET_TIME_Absolute expire,
               const char *label,
               const unsigned char *rdata,
               size_t rdata_len,
               struct GNUNET_GNSRECORD_Block **result,
               int sign)
{
  const struct GNUNET_CRYPTO_EcdsaPrivateKey *key;
  struct GNUNET_CRYPTO_EddsaPublicKey edpubkey;
  enum GNUNET_GenericReturnValue res = GNUNET_SYSERR;
  char *norm_label;
#define CSIZE 64
  static struct KeyCacheLine cache[CSIZE];
  struct KeyCacheLine *line;

  norm_label = GNUNET_GNSRECORD_string_normalize (label);

  if (GNUNET_PUBLIC_KEY_TYPE_ECDSA == ntohl (pkey->type))
  {
    key = &pkey->ecdsa_key;

    line = &cache[(*(unsigned int *) key) % CSIZE];
    if (0 != memcmp (&line->key,
                     key,
                     sizeof(*key)))
    {
      /* cache miss, recompute */
      line->key = *key;
      GNUNET_CRYPTO_ecdsa_key_get_public (key,
                                          &line->pkey);
    }
    res = block_create_ecdsa (key,
                              &line->pkey,
                              expire,
                              norm_label,
                              rdata,
                              rdata_len,
                              result,
                              sign);
  }
  else if (GNUNET_PUBLIC_KEY_TYPE_EDDSA == ntohl (pkey->type))
  {
    GNUNET_CRYPTO_eddsa_key_get_public (&pkey->eddsa_key,
                                        &edpubkey);
    res = block_create_eddsa (&pkey->eddsa_key,
                              &edpubkey,
                              expire,
                              norm_label,
                              rdata,
                              rdata_len,
                              result,
                              sign);
  }
#undef CSIZE
  GNUNET_free (norm_label);
  return res;
}


/**
 * Check if a signature is valid.  This API is used by the GNS Block
 * to validate signatures received from the network.
 *
 * @param block block to verify
 * @return #GNUNET_OK if the signature is valid
 */
enum GNUNET_GenericReturnValue
GNUNET_GNSRECORD_block_verify (const struct GNUNET_GNSRECORD_Block *block)
{
  struct GNRBlockPS *purp;
  size_t payload_len = ntohl (block->size)
                       - sizeof (struct GNUNET_GNSRECORD_Block);
  enum GNUNET_GenericReturnValue res = GNUNET_NO;

  if (ntohl (block->size) <
      sizeof (struct GNUNET_GNSRECORD_Block))
  {
    GNUNET_break_op (0);
    return GNUNET_SYSERR;
  }
  GNUNET_assert (payload_len <= UINT16_MAX);
  purp = GNUNET_malloc (sizeof (struct GNRBlockPS) + payload_len);
  purp->purpose.size = htonl (sizeof (struct GNRBlockPS) + payload_len);
  purp->purpose.purpose = htonl (GNUNET_SIGNATURE_PURPOSE_GNS_RECORD_SIGN);
  GNUNET_memcpy (&purp[1],
                 &block[1],
                 payload_len);
  switch (ntohl (block->type))
  {
  case GNUNET_GNSRECORD_TYPE_PKEY:
    purp->expiration_time = block->ecdsa_block.expiration_time;
    res = GNUNET_CRYPTO_ecdsa_verify_ (
      GNUNET_SIGNATURE_PURPOSE_GNS_RECORD_SIGN,
      &purp->purpose,
      &block->ecdsa_block.signature,
      &block->ecdsa_block.derived_key);
    break;
  case GNUNET_GNSRECORD_TYPE_EDKEY:
    purp->expiration_time = block->eddsa_block.expiration_time;
    res = GNUNET_CRYPTO_eddsa_verify_ (
      GNUNET_SIGNATURE_PURPOSE_GNS_RECORD_SIGN,
      &purp->purpose,
      &block->eddsa_block.signature,
      &block->eddsa_block.derived_key);
    break;
  default:
    res = GNUNET_NO;
  }
  GNUNET_free (purp);
  return res;
}


static enum GNUNET_GenericReturnValue
block_decrypt_ecdsa (
  const struct GNUNET_GNSRECORD_Block *block,
  const struct GNUNET_CRYPTO_EcdsaPublicKey *zone_key,
  const char *label,
  GNUNET_GNSRECORD_RecordCallback proc,
  void *proc_cls)
{
  size_t payload_len = ntohl (block->size)
                       - sizeof (struct GNUNET_GNSRECORD_Block);
  unsigned char ctr[GNUNET_CRYPTO_AES_KEY_LENGTH / 2];
  unsigned char key[GNUNET_CRYPTO_AES_KEY_LENGTH];

  if (ntohl (block->size) <
      sizeof (struct GNUNET_GNSRECORD_Block))
  {
    GNUNET_break_op (0);
    return GNUNET_SYSERR;
  }
  if (payload_len > UINT16_MAX)
  {
    GNUNET_break_op (0);
    return GNUNET_SYSERR;
  }
  GNR_derive_block_aes_key (ctr,
                            key,
                            label,
                            block->ecdsa_block.expiration_time.abs_value_us__,
                            zone_key);
  {
    char payload[payload_len];
    unsigned int rd_count;

    GNUNET_CRYPTO_aes_ctr (&block[1],
                           payload_len,
                           key,
                           ctr,
                           payload);
    rd_count = GNUNET_GNSRECORD_records_deserialize_get_size (payload_len,
                                                              payload);
    if (rd_count > 2048)
    {
      /* limit to sane value */
      GNUNET_break_op (0);
      return GNUNET_SYSERR;
    }
    {
      struct GNUNET_GNSRECORD_Data rd[GNUNET_NZL (rd_count)];
      unsigned int j;
      struct GNUNET_TIME_Absolute now;

      if (GNUNET_OK !=
          GNUNET_GNSRECORD_records_deserialize (payload_len,
                                                payload,
                                                rd_count,
                                                rd))
      {
        GNUNET_break_op (0);
        return GNUNET_SYSERR;
      }
      /* hide expired records */
      now = GNUNET_TIME_absolute_get ();
      j = 0;
      for (unsigned int i = 0; i < rd_count; i++)
      {
        if (0 != (rd[i].flags & GNUNET_GNSRECORD_RF_RELATIVE_EXPIRATION))
        {
          /* encrypted blocks must never have relative expiration times, skip! */
          GNUNET_break_op (0);
          continue;
        }

        if (0 != (rd[i].flags & GNUNET_GNSRECORD_RF_SHADOW))
        {
          int include_record = GNUNET_YES;
          /* Shadow record, figure out if we have a not expired active record */
          for (unsigned int k = 0; k < rd_count; k++)
          {
            if (k == i)
              continue;
            if (rd[i].expiration_time < now.abs_value_us)
              include_record = GNUNET_NO;       /* Shadow record is expired */
            if ((rd[k].record_type == rd[i].record_type) &&
                (rd[k].expiration_time >= now.abs_value_us) &&
                (0 == (rd[k].flags & GNUNET_GNSRECORD_RF_SHADOW)))
            {
              include_record = GNUNET_NO;         /* We have a non-expired, non-shadow record of the same type */
              GNUNET_log (GNUNET_ERROR_TYPE_INFO,
                          "Ignoring shadow record\n");
              break;
            }
          }
          if (GNUNET_YES == include_record)
          {
            rd[i].flags ^= GNUNET_GNSRECORD_RF_SHADOW;       /* Remove Flag */
            if (j != i)
              rd[j] = rd[i];
            j++;
          }
        }
        else if (rd[i].expiration_time >= now.abs_value_us)
        {
          /* Include this record */
          if (j != i)
            rd[j] = rd[i];
          j++;
        }
        else
        {
          struct GNUNET_TIME_Absolute at;

          at.abs_value_us = rd[i].expiration_time;
          GNUNET_log (GNUNET_ERROR_TYPE_INFO,
                      "Excluding record that expired %s (%llu ago)\n",
                      GNUNET_STRINGS_absolute_time_to_string (at),
                      (unsigned long long) rd[i].expiration_time
                      - now.abs_value_us);
        }
      }
      rd_count = j;
      if (NULL != proc)
        proc (proc_cls,
              rd_count,
              (0 != rd_count) ? rd : NULL);
    }
  }
  return GNUNET_OK;
}


static enum GNUNET_GenericReturnValue
block_decrypt_eddsa (const struct GNUNET_GNSRECORD_Block *block,
                     const struct
                     GNUNET_CRYPTO_EddsaPublicKey *zone_key,
                     const char *label,
                     GNUNET_GNSRECORD_RecordCallback proc,
                     void *proc_cls)
{
  size_t payload_len = ntohl (block->size)
                       - sizeof (struct GNUNET_GNSRECORD_Block);
  struct GNUNET_CRYPTO_XSalsa20SecretKey skey;
  struct GNUNET_CRYPTO_XSalsa20Nonce nonce;


  if (ntohl (block->size) <
      sizeof(struct GNUNET_GNSRECORD_Block))
  {
    GNUNET_break_op (0);
    return GNUNET_SYSERR;
  }
  if (payload_len > UINT16_MAX)
  {
    GNUNET_break_op (0);
    return GNUNET_SYSERR;
  }
  GNR_derive_block_xsalsa_key (&nonce,
                               &skey,
                               label,
                               block->eddsa_block.expiration_time.abs_value_us__
                               ,
                               zone_key);
  {
    char payload[payload_len];
    unsigned int rd_count;

    if (GNUNET_OK !=
        GNUNET_CRYPTO_xsalsa20poly1305_decrypt (
          payload_len,
          (unsigned char*) &block[1],
          &skey,
          &nonce,
          payload))
    {
      GNUNET_break_op (0);
      return GNUNET_SYSERR;
    }
    payload_len -= crypto_secretbox_MACBYTES;
    rd_count = GNUNET_GNSRECORD_records_deserialize_get_size (payload_len,
                                                              payload);
    if (rd_count > 2048)
    {
      /* limit to sane value */
      GNUNET_break_op (0);
      return GNUNET_SYSERR;
    }
    {
      struct GNUNET_GNSRECORD_Data rd[GNUNET_NZL (rd_count)];
      unsigned int j;
      struct GNUNET_TIME_Absolute now;

      if (GNUNET_OK !=
          GNUNET_GNSRECORD_records_deserialize (payload_len,
                                                payload,
                                                rd_count,
                                                rd))
      {
        GNUNET_break_op (0);
        return GNUNET_SYSERR;
      }
      /* hide expired records */
      now = GNUNET_TIME_absolute_get ();
      j = 0;
      for (unsigned int i = 0; i < rd_count; i++)
      {
        if (0 != (rd[i].flags & GNUNET_GNSRECORD_RF_RELATIVE_EXPIRATION))
        {
          /* encrypted blocks must never have relative expiration times, skip! */
          GNUNET_break_op (0);
          continue;
        }

        if (0 != (rd[i].flags & GNUNET_GNSRECORD_RF_SHADOW))
        {
          int include_record = GNUNET_YES;
          /* Shadow record, figure out if we have a not expired active record */
          for (unsigned int k = 0; k < rd_count; k++)
          {
            if (k == i)
              continue;
            if (rd[i].expiration_time < now.abs_value_us)
              include_record = GNUNET_NO;       /* Shadow record is expired */
            if ((rd[k].record_type == rd[i].record_type) &&
                (rd[k].expiration_time >= now.abs_value_us) &&
                (0 == (rd[k].flags & GNUNET_GNSRECORD_RF_SHADOW)))
            {
              include_record = GNUNET_NO;         /* We have a non-expired, non-shadow record of the same type */
              GNUNET_log (GNUNET_ERROR_TYPE_INFO,
                          "Ignoring shadow record\n");
              break;
            }
          }
          if (GNUNET_YES == include_record)
          {
            rd[i].flags ^= GNUNET_GNSRECORD_RF_SHADOW;       /* Remove Flag */
            if (j != i)
              rd[j] = rd[i];
            j++;
          }
        }
        else if (rd[i].expiration_time >= now.abs_value_us)
        {
          /* Include this record */
          if (j != i)
            rd[j] = rd[i];
          j++;
        }
        else
        {
          struct GNUNET_TIME_Absolute at;

          at.abs_value_us = rd[i].expiration_time;
          GNUNET_log (GNUNET_ERROR_TYPE_INFO,
                      "Excluding record that expired %s (%llu ago)\n",
                      GNUNET_STRINGS_absolute_time_to_string (at),
                      (unsigned long long) rd[i].expiration_time
                      - now.abs_value_us);
        }
      }
      rd_count = j;
      if (NULL != proc)
        proc (proc_cls,
              rd_count,
              (0 != rd_count) ? rd : NULL);
    }
  }
  return GNUNET_OK;
}


#pragma GCC diagnostic pop

/**
 * Calculate the DHT query for a given @a label in a given @a zone.
 *
 * @param zone private key of the zone
 * @param label label of the record
 * @param query hash to use for the query
 */
void
GNUNET_GNSRECORD_query_from_private_key (const struct
                                         GNUNET_CRYPTO_BlindablePrivateKey *zone
                                         ,
                                         const char *label,
                                         struct GNUNET_HashCode *query)
{
  char *norm_label;
  struct GNUNET_CRYPTO_BlindablePublicKey pub;

  norm_label = GNUNET_GNSRECORD_string_normalize (label);
  switch (ntohl (zone->type))
  {
  case GNUNET_GNSRECORD_TYPE_PKEY:
  case GNUNET_GNSRECORD_TYPE_EDKEY:

    GNUNET_CRYPTO_blindable_key_get_public (zone,
                                            &pub);
    GNUNET_GNSRECORD_query_from_public_key (&pub,
                                            norm_label,
                                            query);
    break;
  default:
    GNUNET_assert (0);
  }
  GNUNET_free (norm_label);
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
void
GNUNET_GNSRECORD_query_from_public_key (const struct
                                        GNUNET_CRYPTO_BlindablePublicKey *pub,
                                        const char *label,
                                        struct GNUNET_HashCode *query)
{
  char *norm_label;
  struct GNUNET_CRYPTO_BlindablePublicKey pd;

  norm_label = GNUNET_GNSRECORD_string_normalize (label);

  switch (ntohl (pub->type))
  {
  case GNUNET_GNSRECORD_TYPE_PKEY:
    pd.type = pub->type;
    GNUNET_CRYPTO_ecdsa_public_key_derive (&pub->ecdsa_key,
                                           norm_label,
                                           "gns",
                                           &pd.ecdsa_key);
    GNUNET_CRYPTO_hash (&pd.ecdsa_key,
                        sizeof (pd.ecdsa_key),
                        query);
    break;
  case GNUNET_GNSRECORD_TYPE_EDKEY:
    pd.type = pub->type;
    GNUNET_CRYPTO_eddsa_public_key_derive (&pub->eddsa_key,
                                           norm_label,
                                           "gns",
                                           &(pd.eddsa_key));
    GNUNET_CRYPTO_hash (&pd.eddsa_key,
                        sizeof (pd.eddsa_key),
                        query);
    break;
  default:
    GNUNET_assert (0);
  }
  GNUNET_free (norm_label);
}


#pragma GCC diagnostic pop

struct EncryptionContextData
{
  struct GNUNET_CRYPTO_BlindablePrivateKey *sk;

  struct GNUNET_CRYPTO_BlindablePublicKey zkey;
};

static enum GNUNET_GenericReturnValue
block_open_ecdsa (void *cls,
                  const char *label,
                  const struct GNUNET_GNSRECORD_Block *block,
                  GNUNET_GNSRECORD_RecordCallback proc,
                  void *proc_cls)
{
  struct EncryptionContextData *ecd = cls;
  enum GNUNET_GenericReturnValue res = GNUNET_SYSERR;
  char *norm_label;

  norm_label = GNUNET_GNSRECORD_string_normalize (label);
  return block_decrypt_ecdsa (block,
                              &ecd->zkey.ecdsa_key,
                              norm_label, proc,
                              proc_cls);
  GNUNET_free (norm_label);
  return res;

}


static enum GNUNET_GenericReturnValue
block_open_eddsa (void *cls,
                  const char *label,
                  const struct GNUNET_GNSRECORD_Block *block,
                  GNUNET_GNSRECORD_RecordCallback proc,
                  void *proc_cls)
{
  struct EncryptionContextData *ecd = cls;
  enum GNUNET_GenericReturnValue res;
  char *norm_label;

  norm_label = GNUNET_GNSRECORD_string_normalize (label);
  res = block_decrypt_eddsa (block,
                             &ecd->zkey.eddsa_key,
                             norm_label, proc,
                             proc_cls);
  GNUNET_free (norm_label);
  return res;
}


static enum GNUNET_GenericReturnValue
block_seal_not_implemented (void *cls,
                            const char *label,
                            struct GNUNET_TIME_Absolute expire,
                            unsigned char *rdata,
                            size_t rdata_len,
                            struct GNUNET_GNSRECORD_Block **result)
{
  GNUNET_break (0);
  return GNUNET_SYSERR;
}


static enum GNUNET_GenericReturnValue
block_seal (void *cls,
            const char *label,
            struct GNUNET_TIME_Absolute expire,
            unsigned char *rdata,
            size_t rdata_len,
            struct GNUNET_GNSRECORD_Block **result)
{
  struct EncryptionContextData *ecd = cls;

  return block_create2 (ecd->sk,
                        expire,
                        label,
                        rdata,
                        rdata_len,
                        result,
                        GNUNET_YES);
}


struct GNUNET_GNSRECORD_EncryptionContext*
GNUNET_GNSRECORD_encryption_context_setup_owner (
  const struct GNUNET_CRYPTO_BlindablePrivateKey *sk)
{
  struct GNUNET_GNSRECORD_EncryptionContext *ec;
  struct EncryptionContextData *ecd;
  size_t sk_len;

  ec = GNUNET_malloc (sizeof(*ec) + sizeof(struct EncryptionContextData));
  ec->cls = &ec[1];
  ecd = ec->cls;
  sk_len = GNUNET_CRYPTO_blindable_sk_get_length (sk);
  ecd->sk = GNUNET_malloc (sk_len);
  GNUNET_memcpy (ecd->sk,
                 sk,
                 sk_len);
  GNUNET_CRYPTO_blindable_key_get_public (ecd->sk, &ecd->zkey);
  switch (ntohl (sk->type))
  {
  case GNUNET_PUBLIC_KEY_TYPE_ECDSA:
    ec->open = block_open_ecdsa;
    ec->seal = block_seal;
    break;
  case GNUNET_PUBLIC_KEY_TYPE_EDDSA:
    ec->open = block_open_eddsa;
    ec->seal = block_seal;
    break;
  default:
    GNUNET_assert (0);
  }
  return ec;
}


struct GNUNET_GNSRECORD_EncryptionContext*
GNUNET_GNSRECORD_encryption_context_setup_resolver (
  const struct GNUNET_CRYPTO_BlindablePublicKey *zkey)
{
  struct GNUNET_GNSRECORD_EncryptionContext *ec;
  struct EncryptionContextData *ecd;
  size_t pk_len;

  ec = GNUNET_malloc (sizeof (*ec) + sizeof (*ecd));
  ec->cls = &ec[1];
  ecd = ec->cls;
  pk_len = GNUNET_CRYPTO_blindable_pk_get_length (zkey);
  GNUNET_memcpy (&ecd->zkey,
                 zkey,
                 pk_len);
  switch (ntohl (zkey->type))
  {
  case GNUNET_PUBLIC_KEY_TYPE_ECDSA:
    ec->open = block_open_ecdsa;
    ec->seal = block_seal_not_implemented;
    break;
  case GNUNET_PUBLIC_KEY_TYPE_EDDSA:
    ec->open = block_open_eddsa;
    ec->seal = block_seal_not_implemented;
    break;
  default:
    GNUNET_assert (0);
  }
  return ec;
}


void
GNUNET_GNSRECORD_encryption_context_destroy (struct
                                             GNUNET_GNSRECORD_EncryptionContext
                                             *ec)
{
  struct EncryptionContextData *ecd = ec->cls;

  GNUNET_free (ecd->sk);
  GNUNET_free (ec);

}


enum GNUNET_GenericReturnValue
GNUNET_GNSRECORD_block_decrypt (const struct GNUNET_GNSRECORD_Block *block,
                                const struct
                                GNUNET_CRYPTO_BlindablePublicKey *zone_key,
                                const char *label,
                                GNUNET_GNSRECORD_RecordCallback proc,
                                void *proc_cls)
{
  struct GNUNET_GNSRECORD_EncryptionContext *ec;
  enum GNUNET_GenericReturnValue ret;

  ec = GNUNET_GNSRECORD_encryption_context_setup_resolver (zone_key);
  ret = ec->open (ec->cls,
                  label,
                  block,
                  proc,
                  proc_cls);
  GNUNET_GNSRECORD_encryption_context_destroy (ec);
  return ret;
}


/* end of gnsrecord_crypto.c */
