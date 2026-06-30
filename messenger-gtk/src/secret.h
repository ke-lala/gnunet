/*
   This file is part of GNUnet.
   Copyright (C) 2026 GNUnet e.V.

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
 * @file secret.h
 */

#ifndef SECRET_H_
#define SECRET_H_

#include <gnunet/gnunet_chat_lib.h>
#include <stdbool.h>
#include <stdint.h>

#include "application.h"

typedef void (*MESSENGER_SecretCallback)(
  MESSENGER_Application *application,
  const char *secret,
  uint32_t secret_len,
  gboolean success,
  gboolean error,
  gpointer user_data
);

typedef struct MESSENGER_SecretOperation {
  MESSENGER_Application *application;

  MESSENGER_SecretCallback callback;
  GCancellable *cancellable;
  gpointer user_data;
  gboolean ownership;

  char *secret;
  uint32_t secret_len;
} MESSENGER_SecretOperation;

/**
 * Lookup a secret from identity with
 * a given name.
 *
 * @param[in] name Identity name
 * @param[out] secret_len Length of secret
 * @return Secret or NULL
 */
MESSENGER_SecretOperation*
secret_operation_lookup(MESSENGER_Application *application,
                        const char *name,
                        MESSENGER_SecretCallback callback,
                        gpointer user_data);

/**
 * Stores a secret for identity with
 * a given name.
 *
 * @param[in] name Identity name
 * @param[in] secret Secret
 * @param[in] secret_len Length of secret
 * @return Whether the storage was successful
 */
MESSENGER_SecretOperation*
secret_operation_store(MESSENGER_Application *application,
                       const char *name,
                       const char *secret,
                       uint32_t secret_len,
                       MESSENGER_SecretCallback callback,
                       gpointer user_data);

MESSENGER_SecretOperation*
secret_operation_generate(MESSENGER_Application *application,
                          const char *name,
                          MESSENGER_SecretCallback callback,
                          gpointer user_data);

/**
 * Delete a secret from identity with
 * a given name.
 *
 * @param[in] name Identity name
 * @return Whether the deletion was successful
 */
MESSENGER_SecretOperation*
secret_operation_delete(MESSENGER_Application *application,
                        const char *name,
                        MESSENGER_SecretCallback callback,
                        gpointer user_data);

void
secret_operation_own_user_data(MESSENGER_SecretOperation *op);

void
secret_operation_cancel(MESSENGER_SecretOperation *op);

void
secret_operation_cleanup(MESSENGER_SecretOperation *op);

void
secret_operation_drop(MESSENGER_SecretOperation *op);

void
secret_operation_destroy(MESSENGER_SecretOperation *op);

#endif /* SECRET_H_ */
