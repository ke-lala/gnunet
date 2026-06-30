/*
   This file is part of GNUnet.
   Copyright (C) 2022--2025 GNUnet e.V.

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
/*
 * @author Tobias Frisch
 * @file gnunet_chat_uri.h
 */

#ifndef GNUNET_CHAT_URI_H_
#define GNUNET_CHAT_URI_H_

#include <gnunet/gnunet_fs_service.h>
#include <gnunet/gnunet_util_lib.h>

#include "gnunet_chat_util.h"

struct GNUNET_CHAT_UriChat
{
  struct GNUNET_CRYPTO_BlindablePublicKey zone;
  char *label;
};

struct GNUNET_CHAT_UriFile
{
  struct GNUNET_FS_Uri *uri;
};

struct GNUNET_CHAT_Uri
{
  enum GNUNET_CHAT_UriType type;

  union {
    struct GNUNET_CHAT_UriChat chat;
    struct GNUNET_CHAT_UriFile fs;
  };
};

/**
 * Creates a chat uri with a selected key as <i>zone</i>
 * and a <i>label</i> of type #GNUNET_CHAT_URI_TYPE_CHAT.
 *
 * @param[in] zone URI zone
 * @param[in] label URI label
 * @return New chat uri
 */
struct GNUNET_CHAT_Uri*
uri_create_chat (const struct GNUNET_CRYPTO_BlindablePublicKey *zone,
	               const char *label);

/**
 * Creates a chat uri from a selected FS <i>uri</i>
 * setting the type to #GNUNET_CHAT_URI_TYPE_FS.
 *
 * @param[in] uri FS URI
 * @return New chat uri
 */
struct GNUNET_CHAT_Uri*
uri_create_file (const struct GNUNET_FS_Uri *uri);

/**
 * Destroys a chat <i>uri</i> and frees its memory.
 *
 * @param[in,out] uri Chat uri
 */
void
uri_destroy (struct GNUNET_CHAT_Uri *uri);

/**
 * Parses an UTF-8 string to a chat URI which will 
 * be newly allocated.
 *
 * @param[in] string UTF-8 string to parse
 * @param[out] emsg Where to store the parser error message (if any)
 * @return URI on success, NULL on error
 */
struct GNUNET_CHAT_Uri*
uri_parse_from_string (const char *string,
                       char **emsg);

/**
 * Returns an allocated UTF-8 string representing
 * a given chat <i>uri</i>.
 *
 * @param[in] uri Chat uri
 * @return The UTF-8 string representing the URI
 */
char*
uri_to_string (const struct GNUNET_CHAT_Uri *uri);

#endif /* GNUNET_CHAT_URI_H_ */
