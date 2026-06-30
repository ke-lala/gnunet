/*
   This file is part of GNUnet.
   Copyright (C) 2022--2026 GNUnet e.V.

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
 * @file gnunet_chat_uri.c
 */

#include "gnunet_chat_uri.h"
#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_fs_service.h>

#define _(String) ((const char*) String)

struct GNUNET_CHAT_Uri*
uri_create_chat (const struct GNUNET_CRYPTO_BlindablePublicKey *zone,
                 const char *label)
{
  GNUNET_assert((zone) && (label));

  struct GNUNET_CHAT_Uri *uri = GNUNET_new(struct GNUNET_CHAT_Uri);

  uri->type = GNUNET_CHAT_URI_TYPE_CHAT;

  GNUNET_memcpy(&(uri->chat.zone), zone, sizeof(uri->chat.zone));
  uri->chat.label = GNUNET_strdup(label);

  return uri;
}

struct GNUNET_CHAT_Uri*
uri_create_file (const struct GNUNET_FS_Uri *uri)
{
  GNUNET_assert(uri);

  struct GNUNET_CHAT_Uri *chat_uri = GNUNET_new(struct GNUNET_CHAT_Uri);

  chat_uri->type = GNUNET_CHAT_URI_TYPE_FS;
  chat_uri->fs.uri = GNUNET_FS_uri_dup(uri);

  return chat_uri;
}

void
uri_destroy (struct GNUNET_CHAT_Uri *uri)
{
  GNUNET_assert(uri);

  switch (uri->type)
  {
    case GNUNET_CHAT_URI_TYPE_CHAT:
      if (uri->chat.label)
        GNUNET_free(uri->chat.label);
      break;
    case GNUNET_CHAT_URI_TYPE_FS:
      if (uri->fs.uri)
        GNUNET_FS_uri_destroy(uri->fs.uri);
      break;
    default:
      break;
  }

  GNUNET_free(uri);
}

static enum GNUNET_GenericReturnValue
string_starts_with (const char *string,
                    const char *prefix,
                    size_t *prefix_len)
{
  GNUNET_assert((string) && (prefix) && (prefix_len));

  *prefix_len = strlen(prefix);

  return (0 == strncasecmp(
    prefix, string, *prefix_len
  )? GNUNET_YES : GNUNET_NO);
}

struct GNUNET_CHAT_Uri*
uri_parse_from_string (const char *string,
                       char **emsg)
{
  GNUNET_assert(string);

  size_t prefix_len;

  if (GNUNET_YES == string_starts_with(string, GNUNET_CHAT_URI_PREFIX, &prefix_len))
  {
    struct GNUNET_CRYPTO_BlindablePublicKey zone;

    const char *data = string + prefix_len;
    const char *end = strchr(data, '.');

    if (!end)
    {
      if (emsg)
        *emsg = GNUNET_strdup (_ ("CHAT URI malformed (zone key missing)"));

      return NULL;
    }

    char *zone_data = GNUNET_strndup(data, (size_t) (end - data));

    if (GNUNET_OK != GNUNET_CRYPTO_blindable_public_key_from_string(zone_data, &zone))
    {
      GNUNET_free(zone_data);

      if (emsg)
        *emsg = GNUNET_strdup (_ ("CHAT URI malformed (zone key invalid)"));

      return NULL;
    }

    GNUNET_free(zone_data);

    return uri_create_chat(&zone, end + 1);
  }
  else if (GNUNET_YES == string_starts_with(string, GNUNET_FS_URI_PREFIX, &prefix_len))
  {
    struct GNUNET_FS_Uri *fs_uri = GNUNET_FS_uri_parse(string, emsg);

    if (!fs_uri)
      return NULL;

    struct GNUNET_CHAT_Uri *uri = uri_create_file(fs_uri);
    GNUNET_FS_uri_destroy(fs_uri);
    return uri;
  }
  else
  {
    if (emsg)
      *emsg = GNUNET_strdup (_ ("CHAT URI malformed (invalid prefix)"));

    return NULL;
  }
}

char*
uri_to_string (const struct GNUNET_CHAT_Uri *uri)
{
  GNUNET_assert(uri);

  switch (uri->type)
  {
    case GNUNET_CHAT_URI_TYPE_CHAT:
    {
      char *zone = GNUNET_CRYPTO_blindable_public_key_to_string(&(uri->chat.zone));
      char *result;

      GNUNET_asprintf (
        &result,
        "%s%s.%s",
        GNUNET_CHAT_URI_PREFIX,
        zone,
        uri->chat.label
      );
      
      GNUNET_free(zone);
      return result;
    }
    case GNUNET_CHAT_URI_TYPE_FS:
      return GNUNET_FS_uri_to_string(uri->fs.uri);
    default:
      return NULL;
  }
}
