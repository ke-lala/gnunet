/*
   This file is part of GNUnet.
   Copyright (C) 2021--2024, 2026 GNUnet e.V.

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
 * @file gnunet_chat_util.c
 */

#include "gnunet_chat_util.h"

#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_messenger_service.h>
#include <gnunet/gnunet_util_lib.h>

static const char label_prefix_of_contact [] = "contact";
static const char label_prefix_of_group [] = "group";

static const char identity_prefix_of_lobby [] = "_gnunet_chat_lobby";

void
util_shorthash_from_member (const struct GNUNET_MESSENGER_Contact *member,
			                      struct GNUNET_ShortHashCode *shorthash)
{
  GNUNET_assert(shorthash);

  const size_t id = GNUNET_MESSENGER_contact_get_id(member);

  memset(shorthash, 0, sizeof(*shorthash));
  GNUNET_memcpy(
    shorthash,
    &id,
    sizeof(id) < sizeof(*shorthash) ? sizeof(id) : sizeof(*shorthash)
  );
}

void
util_shorthash_from_discourse_id (const struct GNUNET_CHAT_DiscourseId *id,
                                  struct GNUNET_ShortHashCode *shorthash)
{
  GNUNET_assert(shorthash);

  memset(shorthash, 0, sizeof(*shorthash));
  GNUNET_memcpy(
    shorthash,
    id,
    sizeof(*id) < sizeof(*shorthash) ? sizeof(*id) : sizeof(*shorthash)
  );
}

void
util_discourse_id_from_shorthash (const struct GNUNET_ShortHashCode *shorthash,
                                  struct GNUNET_CHAT_DiscourseId *id)
{
  GNUNET_assert(id);

  memset(id, 0, sizeof(*id));
  GNUNET_memcpy(
    id,
    shorthash,
    sizeof(*id) < sizeof(*shorthash) ? sizeof(*id) : sizeof(*shorthash)
  );
}

void
util_set_name_field (const char *name,
                     char **field)
{
  GNUNET_assert(field);

  if (*field)
    GNUNET_free(*field);

  if (name)
    *field = GNUNET_strdup(name);
  else
    *field = NULL;
}

enum GNUNET_GenericReturnValue
util_hash_file (const char *filename,
                struct GNUNET_HashCode *hash)
{
  GNUNET_assert((filename) && (hash));

  uint64_t size;

  if (GNUNET_OK != GNUNET_DISK_file_size(filename, &size, GNUNET_NO, GNUNET_YES))
    return GNUNET_SYSERR;

  struct GNUNET_DISK_FileHandle *file = GNUNET_DISK_file_open(
    filename, GNUNET_DISK_OPEN_READ, GNUNET_DISK_PERM_USER_READ
  );

  if (!file)
    return GNUNET_SYSERR;

  struct GNUNET_DISK_MapHandle *mapping;
  const void* data;

  if (size > 0)
  {
    data = GNUNET_DISK_file_map(
	    file, &mapping, GNUNET_DISK_MAP_TYPE_READ, size
    );

    if ((!data) || (!mapping))
    {
      GNUNET_DISK_file_close(file);
      return GNUNET_SYSERR;
    }
  }
  else
  {
    mapping = NULL;
    data = NULL;
  }

  GNUNET_CRYPTO_hash(data, size, hash);

  if (mapping)
    GNUNET_DISK_file_unmap(mapping);

  GNUNET_DISK_file_close(file);
  return GNUNET_OK;
}

int
util_get_dirname (const char *directory,
                  const char *subdir,
                  char **filename)
{
  GNUNET_assert(
    (filename) &&
    (directory) &&
    (subdir)
  );

  return GNUNET_asprintf (
    filename,
    "%s/%s",
    directory,
    subdir
  );
}

int
util_get_filename (const char *directory,
                   const char *subdir,
                   const struct GNUNET_HashCode *hash,
                   char **filename)
{
  GNUNET_assert(
    (filename) &&
		(directory) &&
		(subdir) &&
		(hash)
  );

  char* dirname;
  util_get_dirname(directory, subdir, &dirname);

  int result = GNUNET_asprintf (
    filename,
    "%s/%s",
    dirname,
    GNUNET_h2s_full(hash)
  );

  GNUNET_free(dirname);
  return result;
}

char*
util_get_lower(const char *name)
{
  GNUNET_assert(name);

  char *lower = GNUNET_STRINGS_utf8_tolower(name);
  if (lower == NULL)
    return GNUNET_strdup(name);

  return lower;
}

int
util_get_context_label (enum GNUNET_CHAT_ContextType type,
                        const struct GNUNET_HashCode *hash,
                        char **label)
{
  GNUNET_assert((hash) && (label));

  const char *type_string = "chat";

  switch (type)
  {
    case GNUNET_CHAT_CONTEXT_TYPE_CONTACT:
      type_string = "contact";
      break;
    case GNUNET_CHAT_CONTEXT_TYPE_GROUP:
      type_string = "group";
      break;
    default:
      break;
  }

  char *low = util_get_lower(GNUNET_h2s(hash));

  int result = GNUNET_asprintf (
    label,
    "%s_%s",
    type_string,
    low
  );

  GNUNET_free(low);
  return result;
}

enum GNUNET_CHAT_ContextType
util_get_context_label_type (const char *label,
			                       const struct GNUNET_HashCode *hash)
{
  GNUNET_assert((hash) && (label));

  enum GNUNET_CHAT_ContextType type = GNUNET_CHAT_CONTEXT_TYPE_UNKNOWN;

  char *low = util_get_lower(GNUNET_h2s(hash));

  const char *sub = strstr(label, low);
  if ((!sub) || (sub == label) || (sub[-1] != '_'))
    goto cleanup;

  const size_t len = (size_t) (sub - label - 1);

  if (0 == strncmp(label, label_prefix_of_group, len))
    type = GNUNET_CHAT_CONTEXT_TYPE_GROUP;
  else if (0 == strncmp(label, label_prefix_of_contact, len))
    type = GNUNET_CHAT_CONTEXT_TYPE_CONTACT;

cleanup:
  GNUNET_free(low);
  return type;
}

int
util_lobby_name (const struct GNUNET_HashCode *hash,
		             char **name)
{
  GNUNET_assert((hash) && (name));

  char *low = util_get_lower(GNUNET_h2s(hash));

  int result = GNUNET_asprintf (
    name,
    "%s_%s",
    identity_prefix_of_lobby,
    low
  );

  GNUNET_free(low);
  return result;
}

enum GNUNET_GenericReturnValue
util_is_lobby_name(const char *name)
{
  GNUNET_assert(name);

  const char *sub = strstr(name, identity_prefix_of_lobby);
  if ((!sub) || (sub != name))
    return GNUNET_NO;

  const size_t len = strlen(identity_prefix_of_lobby);

  if (name[len] != '_')
    return GNUNET_NO;
  else
    return GNUNET_YES;
}

enum GNUNET_CHAT_MessageKind
util_message_kind_from_kind (enum GNUNET_MESSENGER_MessageKind kind)
{
  switch (kind)
  {
    case GNUNET_MESSENGER_KIND_JOIN:
      return GNUNET_CHAT_KIND_JOIN;
    case GNUNET_MESSENGER_KIND_LEAVE:
      return GNUNET_CHAT_KIND_LEAVE;
    case GNUNET_MESSENGER_KIND_NAME:
    case GNUNET_MESSENGER_KIND_KEY:
    case GNUNET_MESSENGER_KIND_ID:
      return GNUNET_CHAT_KIND_CONTACT;
    case GNUNET_MESSENGER_KIND_INVITE:
      return GNUNET_CHAT_KIND_INVITATION;
    case GNUNET_MESSENGER_KIND_TEXT:
      return GNUNET_CHAT_KIND_TEXT;
    case GNUNET_MESSENGER_KIND_FILE:
      return GNUNET_CHAT_KIND_FILE;
    case GNUNET_MESSENGER_KIND_DELETION:
      return GNUNET_CHAT_KIND_DELETION;
    case GNUNET_MESSENGER_KIND_TICKET:
      return GNUNET_CHAT_KIND_SHARED_ATTRIBUTES;
    case GNUNET_MESSENGER_KIND_TAG:
      return GNUNET_CHAT_KIND_TAG;
    case GNUNET_MESSENGER_KIND_SUBSCRIBTION:
      return GNUNET_CHAT_KIND_DISCOURSE;
    case GNUNET_MESSENGER_KIND_TALK:
      return GNUNET_CHAT_KIND_DATA;
    default:
      return GNUNET_CHAT_KIND_UNKNOWN;
  }
}
