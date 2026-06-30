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
 * @file gnunet_chat_attribute_process.h
 */

#ifndef GNUNET_CHAT_INTERNAL_ATTRIBUTE_PROCESS_H_
#define GNUNET_CHAT_INTERNAL_ATTRIBUTE_PROCESS_H_

#include <gnunet/gnunet_reclaim_service.h>

#include "gnunet_chat_lib.h"

struct GNUNET_CHAT_Handle;
struct GNUNET_CHAT_Account;
struct GNUNET_CHAT_Contact;

struct GNUNET_CHAT_AttributeProcess
{
  struct GNUNET_CHAT_Handle *handle;

  struct GNUNET_CHAT_Account *account;
  struct GNUNET_CHAT_Contact *contact;

  struct GNUNET_RECLAIM_Attribute *attribute;
  struct GNUNET_TIME_Relative expires;
  char *name;
  void *data;

  GNUNET_CHAT_AttributeCallback callback;
  GNUNET_CHAT_AccountAttributeCallback account_callback;

  void *closure;

  struct GNUNET_RECLAIM_AttributeIterator *iter;
  struct GNUNET_RECLAIM_Operation *op;

  struct GNUNET_CHAT_AttributeProcess *next;
  struct GNUNET_CHAT_AttributeProcess *prev;
};

/**
 * Creates a new attribute process resource to
 * handle general attribute management using
 * a given chat <i>handle</i>.
 *
 * The attribute process gets appended to the
 * list of processes from the handle implicitly.
 *
 * @param[in,out] handle Chat handle
 * @param[in] name Attribute name or NULL
 * @return New attribute process
 */
struct GNUNET_CHAT_AttributeProcess*
internal_attributes_create(struct GNUNET_CHAT_Handle *handle,
                           const char *name);

/**
 * Creates a new attribute process resource to
 * handle attribute storage of an attribute with
 * a specific <i>name</i> using a given chat
 * <i>handle</i> and a relative datetime it
 * <i>expires</i>.
 *
 * @see internal_attributes_create()
 *
 * @param[in,out] handle Chat handle
 * @param[in] Attribute name
 * @param[in] expires Relative time for expiration
 * @return New attribute storage process
 */
struct GNUNET_CHAT_AttributeProcess*
internal_attributes_create_store(struct GNUNET_CHAT_Handle *handle,
                                 const char *name,
                                 struct GNUNET_TIME_Relative expires);

/**
 * Creates a new attribute process resource to
 * handle attribute sharing of an attribute with
 * a specific <i>name</i> for a certain chat 
 * <i>contact</i> using a given chat <i>handle</i>.
 *
 * @see internal_attributes_create()
 *
 * @param[in,out] handle Chat handle
 * @param[in,out] contact Chat contact
 * @param[in] Attribute name
 * @return New attribute sharing process
 */
struct GNUNET_CHAT_AttributeProcess*
internal_attributes_create_share(struct GNUNET_CHAT_Handle *handle,
                                 struct GNUNET_CHAT_Contact *contact,
                                 const char *name);

/**
 * Creates a new attribute process resource to
 * request attributes from a specific chat 
 * <i>account</i> using a given chat <i>handle</i>.
 *
 * @see internal_attributes_create()
 *
 * @param[in,out] handle Chat handle
 * @param[in,out] account Chat account
 * @return New attribute request process
 */
struct GNUNET_CHAT_AttributeProcess*
internal_attributes_create_request(struct GNUNET_CHAT_Handle *handle,
                                   struct GNUNET_CHAT_Account *account);

/**
 * Destroys and frees a given <i>attributes</i> 
 * process resource. This will implicitly remove
 * it from its chat handles list of processes.
 *
 * @param[out] attributes Attribute process
 */
void
internal_attributes_destroy(struct GNUNET_CHAT_AttributeProcess *attributes);

/**
 * Continues the iteration of a given <i>attributes</i> 
 * process resource to its next step.
 *
 * @param[in,out] attributes Attribute process
 */
void
internal_attributes_next_iter(struct GNUNET_CHAT_AttributeProcess *attributes);

/**
 * Stops the iteration of a given <i>attributes</i> 
 * process resource.
 *
 * @param[in,out] attributes Attribute process
 */
void
internal_attributes_stop_iter(struct GNUNET_CHAT_AttributeProcess *attributes);

#endif /* GNUNET_CHAT_INTERNAL_ATTRIBUTE_PROCESS_H_ */
