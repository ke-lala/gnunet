/*
   This file is part of GNUnet.
   Copyright (C) 2024 GNUnet e.V.

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
 * @file gnunet_chat_attribute_process.c
 */

#include "gnunet_chat_attribute_process.h"

#include "../gnunet_chat_handle.h"

#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_reclaim_service.h>
#include <string.h>

struct GNUNET_CHAT_AttributeProcess*
internal_attributes_create(struct GNUNET_CHAT_Handle *handle,
                           const char *name)
{
  GNUNET_assert(handle);

  struct GNUNET_CHAT_AttributeProcess *attributes = GNUNET_new(
    struct GNUNET_CHAT_AttributeProcess
  );

  if (!attributes)
    return NULL;

  memset(attributes, 0, sizeof(*attributes));

  attributes->handle = handle;
  
  if (!name)
    goto skip_name;

  attributes->name = GNUNET_strdup(name);

  if (!(attributes->name))
  {
    GNUNET_free(attributes);
    return NULL;
  }

skip_name:
  GNUNET_CONTAINER_DLL_insert_tail(
    attributes->handle->attributes_head,
    attributes->handle->attributes_tail,
    attributes
  );

  return attributes;
}

struct GNUNET_CHAT_AttributeProcess*
internal_attributes_create_store(struct GNUNET_CHAT_Handle *handle,
                                 const char *name,
                                 struct GNUNET_TIME_Relative expires)
{
  GNUNET_assert((handle) && (name));

  struct GNUNET_CHAT_AttributeProcess *attributes;
  attributes = internal_attributes_create(handle, name);

  if (!attributes)
    return NULL;

  attributes->attribute = GNUNET_RECLAIM_attribute_new(
    name,
    NULL,
    GNUNET_RECLAIM_ATTRIBUTE_TYPE_NONE,
    NULL,
    0
  );

  if (!(attributes->attribute))
  {
    internal_attributes_destroy(attributes);
    return NULL;
  }

  attributes->expires = expires;

  return attributes;
}

struct GNUNET_CHAT_AttributeProcess*
internal_attributes_create_share(struct GNUNET_CHAT_Handle *handle,
                                 struct GNUNET_CHAT_Contact *contact,
                                 const char *name)
{
  GNUNET_assert((handle) && (contact) && (name));

  struct GNUNET_CHAT_AttributeProcess *attributes;
  attributes = internal_attributes_create(handle, name);

  if (!attributes)
    return NULL;

  attributes->contact = contact;

  return attributes;
}

struct GNUNET_CHAT_AttributeProcess*
internal_attributes_create_request(struct GNUNET_CHAT_Handle *handle,
                                   struct GNUNET_CHAT_Account *account)
{
  GNUNET_assert((handle) && (account));

  struct GNUNET_CHAT_AttributeProcess *attributes;
  attributes = internal_attributes_create(handle, NULL);

  if (!attributes)
    return NULL;

  attributes->account = account;

  return attributes;
}

void
internal_attributes_destroy(struct GNUNET_CHAT_AttributeProcess *attributes)
{
  GNUNET_assert((attributes) && (attributes->handle));

  GNUNET_CONTAINER_DLL_remove(
    attributes->handle->attributes_head,
    attributes->handle->attributes_tail,
    attributes
  );

  if (attributes->attribute)
    GNUNET_free(attributes->attribute);
  if (attributes->name)
    GNUNET_free(attributes->name);
  if (attributes->data)
    GNUNET_free(attributes->data);

  if (attributes->iter)
    GNUNET_RECLAIM_get_attributes_stop(attributes->iter);
  if (attributes->op)
    GNUNET_RECLAIM_cancel(attributes->op);

  GNUNET_free(attributes);
}

void
internal_attributes_next_iter(struct GNUNET_CHAT_AttributeProcess *attributes)
{
  GNUNET_assert((attributes) && (attributes->iter));

  GNUNET_RECLAIM_get_attributes_next(attributes->iter);
}

void
internal_attributes_stop_iter(struct GNUNET_CHAT_AttributeProcess *attributes)
{
  GNUNET_assert((attributes) && (attributes->iter));

  GNUNET_RECLAIM_get_attributes_stop(attributes->iter);
  attributes->iter = NULL;
}
