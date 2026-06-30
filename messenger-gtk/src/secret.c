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
 * @file secret.c
 */

#include "secret.h"

#include <glib-2.0/glib.h>
#include <gnunet/gnunet_chat_lib.h>
#include <gnunet/gnunet_common.h>
#include <string.h>
#include <libsecret/secret.h>
#include <gnunet/gnunet_util_lib.h>

#ifndef MESSENGER_APPLICATION_ID
#define SECRET_APP_ID "org.gnunet.Messenger"
#else
#define SECRET_APP_ID MESSENGER_APPLICATION_ID
#endif

const SecretSchema *
_secret_schema(void)
{
  static const SecretSchema schema = {
    "org.gnunet.chat.AccountSecret", SECRET_SCHEMA_NONE,
    {
      { "name", SECRET_SCHEMA_ATTRIBUTE_STRING },
      { "app_id", SECRET_SCHEMA_ATTRIBUTE_STRING },
      { "NULL", 0 },
    }
  };
  return &schema;
}

char*
_secret_description(const char *name)
{
  char *desc;

  GNUNET_asprintf(
    &desc,
    "GNUnet Messenger account secret for identity %s",
    name
  );

  return desc;
}

MESSENGER_SecretOperation*
_secret_operation_new(MESSENGER_Application *application,
                      MESSENGER_SecretCallback callback,
                      gpointer user_data)
{
  g_assert(application);

  GCancellable *cancellable = g_cancellable_new();

  if (!cancellable)
    return NULL;

  MESSENGER_SecretOperation* op = g_malloc(sizeof(MESSENGER_SecretOperation));

  op->application = application;

  op->callback = callback;
  op->cancellable = cancellable;
  op->user_data = user_data;
  op->ownership = FALSE;

  op->secret = NULL;
  op->secret_len = 0;

  application->secrets = g_list_append(
    application->secrets, 
    op
  );

  return op;
}

void
_secret_operation_callback(MESSENGER_SecretOperation *op,
                           gboolean success,
                           gboolean error)
{
  g_assert(op);

  if (op->callback)
  {
    op->callback(
      op->application,
      op->secret,
      op->secret_len,
      success,
      error,
      op->user_data
    );
  }

  secret_operation_drop(op);
}

void
_secret_lookup_callback(GNUNET_UNUSED GObject *source_object,
                        GAsyncResult *result,
                        gpointer data)
{
  GError *error = NULL;
  gchar *password;

  MESSENGER_SecretOperation *op = data;

  password = secret_password_lookup_finish(result, &error);

  if (error)
    _secret_operation_callback(op, FALSE, TRUE);
  else if (password)
  {
    op->secret = GNUNET_strdup(password);
    op->secret_len = g_utf8_strlen(password, -1);

    _secret_operation_callback(op, TRUE, FALSE);

    secret_password_free(password);
  }
  else
    _secret_operation_callback(op, FALSE, FALSE);
}

MESSENGER_SecretOperation*
secret_operation_lookup(MESSENGER_Application *application,
                        const char *name,
                        MESSENGER_SecretCallback callback,
                        gpointer user_data)
{
  g_assert((application) && (name));

  MESSENGER_SecretOperation *op = _secret_operation_new(
    application,
    callback,
    user_data
  );

  if (!op)
    return NULL;
  
  secret_password_lookup(
    _secret_schema(),
    op->cancellable,
    &_secret_lookup_callback,
    op,
    "name", name,
    "app_id", SECRET_APP_ID,
    NULL
  );

  return op;
}

void
_secret_store_callback(GNUNET_UNUSED GObject *source_object,
                       GAsyncResult *result,
                       gpointer data)
{
  GError *error = NULL;
  gboolean success;

  MESSENGER_SecretOperation *op = data;

  success = secret_password_store_finish(result, &error);

  if (error)
    _secret_operation_callback(op, FALSE, TRUE);
  else
    _secret_operation_callback(op, success, FALSE);
}

MESSENGER_SecretOperation*
secret_operation_store(MESSENGER_Application *application,
                       const char *name,
                       const char *secret,
                       uint32_t secret_len,
                       MESSENGER_SecretCallback callback,
                       gpointer user_data)
{
  g_assert((application) && (name) && (secret));

  if (strlen(secret) != secret_len)
    return NULL;

  MESSENGER_SecretOperation *op = _secret_operation_new(
    application,
    callback,
    user_data
  );

  if (!op)
    return NULL;

  op->secret = GNUNET_strndup(secret, secret_len + 1);
  op->secret_len = secret_len;

  secret_password_store(
    _secret_schema(),
    SECRET_COLLECTION_DEFAULT,
    _secret_description(name),
    secret,
    op->cancellable,
    &_secret_store_callback,
    op,
    "name", name,
    "app_id", SECRET_APP_ID,
    NULL
  );

  return op;
}

MESSENGER_SecretOperation*
secret_operation_generate(MESSENGER_Application *application,
                          const char *name,
                          MESSENGER_SecretCallback callback,
                          gpointer user_data)
{
  char new_secret [65];
  uint32_t secret_len;

  g_assert((application) && (name));

  secret_len = 64;

  if (GNUNET_OK != GNUNET_CHAT_generate_secret(new_secret, secret_len))
    return NULL;

  new_secret[secret_len] = '\0';

  MESSENGER_SecretOperation *op = secret_operation_store(
    application,
    name,
    new_secret,
    secret_len,
    callback,
    user_data
  );

  secret_password_wipe(new_secret);
  return op;
}

void
_secret_delete_callback(GNUNET_UNUSED GObject *source_object,
                        GAsyncResult *result,
                        gpointer data)
{
  GError *error = NULL;
  gboolean success;

  MESSENGER_SecretOperation *op = data;

  success = secret_password_clear_finish(result, &error);

  if (error)
    _secret_operation_callback(op, FALSE, TRUE);
  else
    _secret_operation_callback(op, success, FALSE);
}

MESSENGER_SecretOperation*
secret_operation_delete(MESSENGER_Application *application,
                        const char *name,
                        MESSENGER_SecretCallback callback,
                        gpointer user_data)
{
  g_assert((application) && (name));

  MESSENGER_SecretOperation *op = _secret_operation_new(
    application,
    callback,
    user_data
  );

  if (!op)
    return NULL;

  secret_password_clear(
    _secret_schema(),
    op->cancellable,
    &_secret_delete_callback,
    op,
    "name", name,
    "app_id", SECRET_APP_ID,
    NULL
  );

  return op;
}

void
secret_operation_own_user_data(MESSENGER_SecretOperation *op)
{
  g_assert(op);

  op->ownership = TRUE;
}

void
secret_operation_cancel(MESSENGER_SecretOperation *op)
{
  g_assert(op);

  if (!op->cancellable)
    return;
  
  if (!g_cancellable_is_cancelled(op->cancellable))
    g_cancellable_cancel(op->cancellable);
}

void
secret_operation_cleanup(MESSENGER_SecretOperation *op)
{
  g_assert(op);

  if (op->secret)
  {
    secret_password_wipe(op->secret);
    GNUNET_free(op->secret);
  }

  if ((op->ownership) && (op->user_data))
  {
    g_free(op->user_data);
    op->user_data = NULL;
  }

  if (!op->cancellable)
    return;
  
  g_object_unref(op->cancellable);
  op->cancellable = NULL;
}

void
secret_operation_drop(MESSENGER_SecretOperation *op)
{
  g_assert(op);

  if (op->application->secrets)
    op->application->secrets = g_list_remove(
      op->application->secrets,
      op
    );
  
  secret_operation_destroy(op);
}

void
secret_operation_destroy(MESSENGER_SecretOperation *op)
{
  g_assert(op);

  secret_operation_cleanup(op);
  g_free(op);
}
