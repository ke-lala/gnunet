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
 * @file src/mhd2/h2/hpack/mhd_hpack_codec.h
 * @brief  The declarations for HPACK header compression codec functions
 * @author Karlson2k (Evgeny Grin)
 *
 * The sizes of all strings are intentionally limited to 32 bits (4GiB).
 */

#ifndef MHD_HPACK_CODEC_H
#define MHD_HPACK_CODEC_H 1

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "mhd_buffer.h"

#ifndef mhd_HPACK_DTBL_BITS
#  if ((SIZEOF_SIZE_T + 0) >= 4) && ((SIZEOF_VOIDP + 0) >= 4)
#    define mhd_HPACK_DTBL_BITS        32
#  else
#    define mhd_HPACK_DTBL_BITS        16
#  endif
#else  /* mhd_HPACK_DTBL_BITS */
#  if (mhd_HPACK_DTBL_BITS != 16) && (mhd_HPACK_DTBL_BITS != 32)
#error Unsupported mhd_HPACK_DTBL_BITS value
#  endif
#endif /* mhd_HPACK_DTBL_BITS */

/**
 * @def mhd_DTBL_MAX_SIZE
 * The maximum possible size of the dynamic table
 */
#if mhd_HPACK_DTBL_BITS == 32
#  define mhd_DTBL_MAX_SIZE     (((size_t) 1u) * 1024u * 1024u)
#elif mhd_HPACK_DTBL_BITS == 16
#  define mhd_DTBL_MAX_SIZE     (((size_t) 60u) * 1024u)
#endif

struct mhd_HpackDecContext; /* forward declaration */

/**
 * Initialise HPACK decoder context and create the dynamic table.
 * @param hk_dec the decoder context to initialise
 * @return 'true' on success,
 *         'false' on allocation error
 */
MHD_INTERNAL bool
mhd_hpack_dec_init (struct mhd_HpackDecContext *hk_dec)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_OUT_ (1);

/**
 * Deinitialise HPACK decoder context and free the dynamic table if present.
 * @param hk_dec the pointer to the decoder context
 */
MHD_INTERNAL void
mhd_hpack_dec_deinit (struct mhd_HpackDecContext *hk_dec)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_INOUT_ (1);

/**
 * Set the maximum allowed dynamic table size for the decoder.
 * Should be called when the remote peer has ACKed settings with an updated
 * dynamic table size.
 * @param hk_dec the decoder context
 * @param new_allowed_dyn_size new limit in bytes,
 *        must be <= #mhd_DTBL_MAX_SIZE
 */
MHD_INTERNAL void
mhd_hpack_dec_set_allowed_dyn_size (struct mhd_HpackDecContext *hk_dec,
                                    size_t new_allowed_dyn_size)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_INOUT_ (1);

/**
 * Result codes of #mhd_hpack_dec_data()
 */
enum MHD_FIXED_ENUM_ mhd_HpackDecResult
{
  /**
   * Success, no new field decoded.
   * Could be decoding of a Dynamic Table Size Update message.
   */
  mhd_HPACK_DEC_RES_NO_NEW_FIELD = -1
  ,
  /**
   * Success, new field decoded.
   */
  mhd_HPACK_DEC_RES_NEW_FIELD = 0
  ,
  /**
   * The encoded data is incomplete. More data needed.
   */
  mhd_HPACK_DEC_RES_INCOMPLETE = 1
  ,
  /**
  * Memory allocation error when resizing the dynamic table.
   */
  mhd_HPACK_DEC_RES_ALLOC_ERR
  ,
  /**
   * The output buffer is too small for the decoded field.
   */
  mhd_HPACK_DEC_RES_BUFFER_TOO_SMALL
  ,
  /**
   * The length of the field strings is too long for this code (> 4GiB).
   */
  mhd_HPACK_DEC_RES_STRING_TOO_LONG
  ,
  /**
   * The length of the number in the encoded form is too long.
   * The encoded number used more bytes than needed to encode 64-bit nu
   */
  mhd_HPACK_DEC_RES_NUMBER_TOO_LONG
  ,
  /**
   * Received a Dynamic Table Size Update message with a size larger than
   * allowed.
   */
  mhd_HPACK_DEC_RES_DYN_SIZE_UPD_TOO_LARGE
  ,
  /**
   * The remote peer did not send the expected Dynamic Table Size
   * Update.
   */
  mhd_HPACK_DEC_RES_DYN_SIZE_UPD_MISSING
  ,
  /**
   * Huffman decoding error
   */
  mhd_HPACK_DEC_RES_HUFFMAN_ERR
  ,
  /**
   * Field (header) index specified in HPACK data does not exist.
   */
  mhd_HPACK_DEC_RES_HPACK_BAD_IDX
  ,
  /**
   * Other HPACK decoding errors
   */
  mhd_HPACK_DEC_RES_HPACK_ERR
  ,
  /**
   * Internal error.
   * Should never happen.
   */
  mhd_HPACK_DEC_RES_INTERNAL_ERR
};

/**
 * Check whether the @a dec_res is a kind of error code.
 * @param dec_res result code returned by #mhd_hpack_dec_data()
 * @return boolean 'true' if @a dec_res denotes an error;
 *         boolean 'false' otherwise
 */
#define mhd_HPACK_DEC_RES_IS_ERR(dec_res) \
        (mhd_HPACK_DEC_RES_NEW_FIELD < (dec_res))


/**
 * Decode a single HPACK representation (indexed, literal, or size
 * update).
 * For header fields, writes "name\0value\0" to @a out_buff.
 * @param hk_dec the decoder context
 * @param enc_data_size the size of @a enc_data, must not be zero
 * @param enc_data the encoded data
 * @param out_buff_size the size of @a out_buff, must be at least two bytes
 * @param[out] out_buff the output buffer for the decoded strings
 * @param[out] name_len to be set to the length of the name, not counting
 *                      zero-terminating
 * @param[out] val_len to be set to the length of the value, not counting
 *                     zero-terminating
 * @param[out] bytes_decoded to be set to the number of decoded bytes
 * @return #mhd_HPACK_DEC_RES_NEW_FIELD if new field successfully
 *                                      decoded and placed into
 *                                      @a out_buff,
 *         #mhd_HPACK_DEC_RES_NO_NEW_FIELD if chunk of data
 *                                         successfully decoded, but
 *                                         no new field added,
 *         error code otherwise
 */
MHD_INTERNAL enum mhd_HpackDecResult
mhd_hpack_dec_data (struct mhd_HpackDecContext *restrict hk_dec,
                    const size_t enc_data_size,
                    const uint8_t *restrict enc_data,
                    size_t out_buff_size,
                    char *restrict out_buff,
                    size_t *restrict name_len,
                    size_t *restrict val_len,
                    size_t *restrict bytes_decoded)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_INOUT_(1)
MHD_FN_PAR_IN_SIZE_(3, 2)
MHD_FN_PAR_OUT_SIZE_(5, 4)
MHD_FN_PAR_OUT_(6) MHD_FN_PAR_OUT_(7) MHD_FN_PAR_OUT_ (8);


struct mhd_HpackEncContext; /* forward declaration */

/**
 * Initialise HPACK encoder context and create the dynamic table.
 * @param hk_enc the encoder context to initialise
 * @return 'true' on success,
 *         'false' on allocation error
 */
MHD_INTERNAL bool
mhd_hpack_enc_init (struct mhd_HpackEncContext *hk_enc)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_OUT_ (1);

/**
 * Deinitialise HPACK encoder context and free the dynamic table if
 * present.
 * @param hk_enc the encoder context
 */
MHD_INTERNAL void
mhd_hpack_enc_deinit (struct mhd_HpackEncContext *hk_enc)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_INOUT_ (1);

/**
 * Set the current maximum dynamic table size for the encoder.
 *
 * To avoid repetitive memory allocations, the real table resize is performed
 * later.
 *
 * @param hk_enc the encoder context
 * @param new_dyn_size the new limit in bytes,
 *                     must be <= #mhd_DTBL_MAX_SIZE and must be within
 *                     the limits expected by the remote peer
 */
MHD_INTERNAL void
mhd_hpack_enc_set_dyn_size (struct mhd_HpackEncContext *hk_enc,
                            size_t new_dyn_size)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_INOUT_ (1);

/**
 * Perform dynamic table resize if it is pending.
 *
 * If table is resized before encoding new fields, then encoding functions
 * never return #mhd_HPACK_ENC_RES_ALLOC_ERR.
 *
 * @param hk_enc the encoder context
 * @return 'true' on success,
 *         'false' on allocation error
 */
MHD_INTERNAL bool
mhd_hpack_enc_dyn_resize (struct mhd_HpackEncContext *hk_enc)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_INOUT_ (1);

/**
 * Preference for HPACK encoding
 *
 * The items are sorted, keep them in this order.
 */
enum MHD_FIXED_ENUM_ mhd_HpackEncPolicy
{
  /**
   * Force the field to be encoded literally and added to the dynamic
   * table. If the field does not fit the dynamic table, the dynamic
   * table is completely evicted without adding the new field.
   * The name of the field can be encoded as a reference to the name
   * in the static or dynamic table.
   */
  mhd_HPACK_ENC_POL_FORCED_NEW_IDX
  ,
  /**
   * Encode the field literally; if the field fits the dynamic table,
   * add it to the dynamic table as a new index even if the same field
   * is already in the tables. If the field does not fit the dynamic
   * table, it is encoded literally without adding it to the table.
   * The name of the field can be encoded as a reference to the name
   * in the static or dynamic table.
   */
  mhd_HPACK_ENC_POL_ALWAYS_NEW_IDX_IF_FIT
  ,
  /**
   * Allow field encoding as a reference to an indexed field in the
   * static or dynamic table; if the field is not in the tables and
   * the field does not fit the dynamic table, the dynamic table is
   * completely evicted without adding the new field.
   * When the field is not in the tables, the name of the field can be
   * encoded as a reference to the name in the static or dynamic
   * table.
   */
  mhd_HPACK_ENC_POL_FORCED
  ,
  /**
   * Allow field encoding as a reference to an indexed field in the
   * static or dynamic table; if the field is not in the tables,
   * always add the field to the dynamic table if it fits the table.
   * When the field is not in the tables, the name of the field can be
   * encoded as a reference to the name in the static or dynamic
   * table.
   */
  mhd_HPACK_ENC_POL_ALWAYS_IF_FIT
  ,
  /**
   * Allow field encoding as a reference to an indexed field in the
   * static or dynamic table; if the field is not in the tables,
   * prefer adding the field to the dynamic table if possible.
   * When the field is not in the tables, the name of the field can be
   * encoded as a reference to the name in the static or dynamic
   * table.
   */
  mhd_HPACK_ENC_POL_DESIRABLE
  ,
  /**
   * Allow field encoding as a reference to an indexed field in the
   * static or dynamic table; if the field is not in the tables, use
   * a neutral preference regarding adding the field to the dynamic
   * table.
   * When the field is not in the tables, the name of the field can be
   * encoded as a reference to the name in the static or dynamic
   * table.
   */
  mhd_HPACK_ENC_POL_NEUTRAL
  ,
  /**
   * Allow field encoding as a reference to an indexed field in the
   * static or dynamic table; if the field is not in the tables,
   * consider adding the field to the dynamic table only with low
   * priority (the encoder may choose not to add it).
   * When the field is not in the tables, the name of the field can be
   * encoded as a reference to the name in the static or dynamic
   * table.
   */
  mhd_HPACK_ENC_POL_LOW_PRIO
  ,
  /**
   * Allow field encoding as a reference to an indexed field in the
   * static or dynamic table; if the field is not in the tables,
   * consider adding the field to the dynamic table only with the
   * lowest priority (the encoder is expected to avoid adding it in
   * most cases).
   * When the field is not in the tables, the name of the field can be
   * encoded as a reference to the name in the static or dynamic
   * table.
   */
  mhd_HPACK_ENC_POL_LOWEST_PRIO
  ,
  /**
   * Allow field encoding as a reference to an indexed field in the
   * static or dynamic table; if the field is not in the tables already,
   * avoid adding the field to the dynamic table.
   * When the field is not in the tables, the name of the field can be
   * encoded as a reference to the name in the static or dynamic
   * table.
   */
  mhd_HPACK_ENC_POL_AVOID_NEW_IDX
  ,
  /**
   * Use field literal encoding without indexing.
   * The name of the field can still be encoded as a reference to the
   * name in the static or dynamic table.
   */
  mhd_HPACK_ENC_POL_NOT_INDEXED
  ,
  /**
   * Use field literal encoding without indexing, with an additional
   * "never indexed" mark to signal intermediaries to avoid using it
   * as an indexed field when re-encoding the message.
   * The name of the field can still be encoded as a reference to the
   * name in the static or dynamic table.
   */
  mhd_HPACK_ENC_POL_NEVER_W_NAME_IDX
  ,
  /**
   * Use field literal encoding without indexing, with an additional
   * "never indexed" mark to signal intermediaries to avoid using it
   * as an indexed field when re-encoding the message.
   * The name of the field can be encoded as a reference to the name
   * in the static table.
   */
  mhd_HPACK_ENC_POL_NEVER_W_NAME_IDX_STATIC
  ,
  /**
   * Use field literal encoding without indexing, with an additional
   * "never indexed" mark to signal intermediaries to avoid using it
   * as an indexed field when re-encoding the message.
   * The name of the field is always encoded literally.
   */
  mhd_HPACK_ENC_POL_NEVER_W_NAME_LIT_FORCED
  ,
  /**
   * Use field literal encoding without indexing, with an additional
   * "never indexed" mark to signal intermediaries to avoid using it
   * as an indexed field when re-encoding the message.
   * The name of the field is always encoded literally, and Huffman
   * encoding is not used for the name or the value of the field.
   */
  mhd_HPACK_ENC_POL_NEVER_W_NAME_LIT_NO_HUFFMAN
};


/**
 * Result codes of field encoding
 */
enum MHD_FIXED_ENUM_ mhd_HpackEncResult
{
  /**
   * The field has been encoded successfully
   */
  mhd_HPACK_ENC_RES_OK = 0
  ,
  /**
   * The output buffer is too small to fit the data
   */
  mhd_HPACK_ENC_BUFFER_TOO_SMALL
  ,
  /**
   * Error allocating memory when resizing the dynamic table
   */
  mhd_HPACK_ENC_RES_ALLOC_ERR
};

/**
 * Encode a single HPACK field.
 *
 * May emit a Dynamic Table Size Update representation first if the encoder
 * has a pending size change. Encodes the field using indexed or literal
 * representation according to @a enc_pol and the state of the static/dynamic
 * tables. On success, the number of bytes written is stored in
 * @a bytes_encoded.
 *
 * @param hk_enc the encoder context
 * @param name the field name, must be a "real" header,
 *             must not start with the ':' character
 * @param value the field value
 * @param enc_pol the encoding policy to apply
 * @param out_buff_size the size of @a out_buff in bytes
 * @param[out] out_buff the output buffer to receive the encoded data
 * @param[out] bytes_encoded set to the number of bytes written to @a out_buff
 * @return #mhd_HPACK_ENC_RES_OK on success;
 *         #mhd_HPACK_ENC_BUFFER_TOO_SMALL if the output buffer is too small;
 *         #mhd_HPACK_ENC_RES_ALLOC_ERR on dynamic table allocation error,
 *         never returned if #mhd_hpack_enc_set_dyn_size() was not earlier or
 *         if #mhd_hpack_enc_dyn_resize() after #mhd_hpack_enc_set_dyn_size()
 */
MHD_INTERNAL enum mhd_HpackEncResult
mhd_hpack_enc_field (struct mhd_HpackEncContext *restrict hk_enc,
                     const struct mhd_BufferConst *restrict name,
                     const struct mhd_BufferConst *restrict value,
                     enum mhd_HpackEncPolicy enc_pol,
                     const size_t out_buff_size,
                     uint8_t *restrict out_buff,
                     size_t *restrict bytes_encoded)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_INOUT_(1)
MHD_FN_PAR_IN_(2) MHD_FN_PAR_IN_(3)
MHD_FN_PAR_OUT_SIZE_(6,5) MHD_FN_PAR_OUT_ (7);


/**
 * Preference for HPACK encoding of pseudo-header ":status"
 *
 * The items are sorted, keep them in this order.
 */
enum MHD_FIXED_ENUM_ mhd_HpackEncPFieldStatusPolicy
{
  /**
   * Encode the pseudo-header literally; if the pseudo-header fits the dynamic
   * table, add it to the dynamic table as a new index even if the same
   * pseudo-header is already in the tables. If the pseudo-header does not fit
   * the dynamic table, it is encoded literally without adding it to the
   * table.
   * The name of the pseudo-header is encoded as a reference to the name
   * in the static table.
   */
  mhd_HPACK_ENC_PFS_POL_ALWAYS_NEW_IDX_IF_FIT
  ,
  /**
   * Allow pseudo-header encoding as a reference to an indexed pseudo-header in
   * the static or dynamic table; if the pseudo-header is not in the tables,
   * add the pseudo-header to the dynamic table if possible.
   * When the pseudo-header is not in the tables, the name of the pseudo-header
   * is encoded as a reference to the name in the static table.
   */
  mhd_HPACK_ENC_PFS_POL_NORMAL
  ,
  /**
   * Allow pseudo-header encoding as a reference to an indexed pseudo-header in
   * the static or dynamic table; if the pseudo-header is not in the tables
   * already, avoid adding the pseudo-header to the dynamic table.
   * When the pseudo-header is not in the tables, the name of the pseudo-header
   * is encoded as a reference to the name in the static table.
   */
  mhd_HPACK_ENC_PFS_POL_AVOID_NEW_IDX
  ,
  /**
   * Allow pseudo-header encoding as a reference to an indexed pseudo-header in
   * the static table only; if the pseudo-header is not in the static table,
   * encode it literally, without adding to the dynamic table.
   * When the pseudo-header is not in the static table, the name of the
   * pseudo-header is encoded as a reference to the name in the static table.
   */
  mhd_HPACK_ENC_PFS_POL_STATIC_IDX
  ,
  /**
   * Use pseudo-header literal encoding without indexing.
   * The name of the pseudo-header is encoded as a reference to the name in
   * the static table.
   */
  mhd_HPACK_ENC_PFS_POL_NOT_INDEXED
  ,
  /**
   * Use pseudo-header literal encoding without indexing, with an additional
   * "never indexed" mark to signal intermediaries to avoid using it
   * as an indexed pseudo-header when re-encoding the message.
   * The name of the pseudo-header is encoded as a reference to the name in
   * the static table.
   */
  mhd_HPACK_ENC_PFS_POL_NEVER_W_NAME_IDX
  ,
  /**
   * Use pseudo-header literal encoding without indexing, with an additional
   * "never indexed" mark to signal intermediaries to avoid using it
   * as an indexed pseudo-header when re-encoding the message.
   * The name of the pseudo-header is always encoded literally.
   */
  mhd_HPACK_ENC_PFS_POL_NEVER_W_NAME_LIT_FORCED
  ,
  /**
   * Use pseudo-header literal encoding without indexing, with an additional
   * "never indexed" mark to signal intermediaries to avoid using it
   * as an indexed pseudo-header when re-encoding the message.
   * The name of the pseudo-header is always encoded literally, and Huffman
   * encoding is not used for the name or the value of the pseudo-header.
   */
  mhd_HPACK_ENC_PFS_POL_NEVER_W_NAME_LIT_NO_HUFFMAN
};

/**
 * Encode a single HPACK pseudo-header ":status".
 *
 * May emit a Dynamic Table Size Update representation first if the encoder
 * has a pending size change.
 * On success, the number of bytes written is stored in @a bytes_encoded.
 *
 * @param hk_enc the encoder context
 * @param code the status code, must be >= 100 and <= 699
 * @param enc_pol the encoding policy to apply
 * @param out_buff_size the size of @a out_buff in bytes
 * @param[out] out_buff the output buffer to receive the encoded data
 * @param[out] bytes_encoded set to the number of bytes written to @a out_buff
 * @return #mhd_HPACK_ENC_RES_OK on success,
 *         #mhd_HPACK_ENC_BUFFER_TOO_SMALL if the output buffer is too small,
 *         #mhd_HPACK_ENC_RES_ALLOC_ERR on dynamic table allocation error
 */
MHD_INTERNAL enum mhd_HpackEncResult
mhd_hpack_enc_ph_status (struct mhd_HpackEncContext *restrict hk_enc,
                         uint_fast16_t code,
                         enum mhd_HpackEncPFieldStatusPolicy enc_pol,
                         const size_t out_buff_size,
                         uint8_t *restrict out_buff,
                         size_t *restrict bytes_encoded)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_INOUT_(1)
MHD_FN_PAR_OUT_SIZE_(5,4) MHD_FN_PAR_OUT_ (6);

#endif /* ! MHD_HPACK_CODEC_H */
