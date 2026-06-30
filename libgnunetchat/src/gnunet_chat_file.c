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
 * @file gnunet_chat_file.c
 */

#include "gnunet_chat_file.h"

#include "gnunet_chat_context.h"
#include "gnunet_chat_handle.h"
#include "gnunet_chat_util.h"

#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_fs_service.h>
#include <gnunet/gnunet_util_lib.h>
#include <string.h>

static void
file_initialize (struct GNUNET_CHAT_File *file)
{
  GNUNET_assert(file);

  file->download = NULL;
  file->publish = NULL;
  file->unindex = NULL;

  file->upload_head = NULL;
  file->upload_tail = NULL;

  file->download_head = NULL;
  file->download_tail = NULL;

  file->unindex_head = NULL;
  file->unindex_tail = NULL;

  file->status = 0;
  file->preview = NULL;

  file->user_pointer = NULL;
}

struct GNUNET_CHAT_File*
file_create_from_message (struct GNUNET_CHAT_Handle *handle,
			                    const struct GNUNET_MESSENGER_MessageFile *message)
{
  GNUNET_assert((handle) && (message) && (message->uri));

  struct GNUNET_CHAT_File* file = GNUNET_new(struct GNUNET_CHAT_File);

  if (!file)
    return NULL;

  file->handle = handle;
  file->name = GNUNET_strndup(message->name, NAME_MAX);

  memcpy(&(file->hash), &(message->hash), sizeof(file->hash));

  file->meta = GNUNET_FS_meta_data_create();
  file->uri = GNUNET_FS_uri_parse(message->uri, NULL);

  file_initialize(file);

  return file;
}

struct GNUNET_CHAT_File*
file_create_from_chk_uri (struct GNUNET_CHAT_Handle *handle,
                          const struct GNUNET_FS_Uri *uri)
{
  GNUNET_assert((handle) && (uri));

  const struct GNUNET_HashCode *hash = GNUNET_FS_uri_chk_get_file_hash(uri);

  if (!hash)
    return NULL;

  struct GNUNET_CHAT_File* file = GNUNET_new(struct GNUNET_CHAT_File);

  if (!file)
    return NULL;

  file->handle = handle;
  file->name = NULL;

  memcpy(&(file->hash), hash, sizeof(file->hash));

  file->meta = GNUNET_FS_meta_data_create();
  file->uri = GNUNET_FS_uri_dup(uri);

  file_initialize(file);

  return file;
}

struct GNUNET_CHAT_File*
file_create_from_disk (struct GNUNET_CHAT_Handle *handle,
                       const char *name,
                       const struct GNUNET_HashCode *hash)
{
  GNUNET_assert((handle) && (name) && (hash));

  struct GNUNET_CHAT_File* file = GNUNET_new(struct GNUNET_CHAT_File);

  if (!file)
    return NULL;

  file->handle = handle;
  file->name = GNUNET_strndup(name, NAME_MAX);

  GNUNET_memcpy(&(file->hash), hash, sizeof(file->hash));

  file->meta = GNUNET_FS_meta_data_create();
  file->uri = NULL;

  file_initialize(file);

  return file;
}

void
file_destroy (struct GNUNET_CHAT_File *file)
{
  GNUNET_assert(file);

  struct GNUNET_CHAT_FileUpload *upload;
  struct GNUNET_CHAT_FileDownload *download;
  struct GNUNET_CHAT_FileUnindex *unindex;

  if (!(file->preview))
    goto skip_preview;

  char *filename = handle_create_file_path(
    file->handle, &(file->hash)
  );

  if (!filename)
    goto skip_filename;

  if (0 != strcmp(filename, file->preview))
    remove(file->preview);

  GNUNET_free(filename);

skip_filename:
  GNUNET_free(file->preview);

skip_preview:
  if (file->publish)
    GNUNET_FS_publish_stop(file->publish);

  if (file->download)
    GNUNET_FS_download_stop(file->download, GNUNET_NO);

  if (file->unindex)
    GNUNET_FS_unindex_stop(file->unindex);

  while (file->upload_head)
  {
    upload = file->upload_head;

    GNUNET_CONTAINER_DLL_remove(
      file->upload_head,
      file->upload_tail,
      upload
    );

    GNUNET_free(upload);
  }

  while (file->download_head)
  {
    download = file->download_head;

    GNUNET_CONTAINER_DLL_remove(
      file->download_head,
      file->download_tail,
      download
    );

    GNUNET_free(download);
  }

  while (file->unindex_head)
  {
    unindex = file->unindex_head;

    GNUNET_CONTAINER_DLL_remove(
      file->unindex_head,
      file->unindex_tail,
      unindex
    );

    GNUNET_free(unindex);
  }

  if (file->uri)
    GNUNET_FS_uri_destroy(file->uri);

  if (file->meta)
    GNUNET_FS_meta_data_destroy(file->meta);

  if (file->name)
    GNUNET_free(file->name);

  GNUNET_free(file);
}

void
file_bind_upload (struct GNUNET_CHAT_File *file,
                  struct GNUNET_CHAT_Context *context,
                  GNUNET_CHAT_FileUploadCallback cb,
                  void *cls)
{
  GNUNET_assert(file);

  struct GNUNET_CHAT_FileUpload *upload = GNUNET_new(
    struct GNUNET_CHAT_FileUpload
  );

  upload->context = context;
  upload->callback = cb;
  upload->cls = cls;

  GNUNET_CONTAINER_DLL_insert(
    file->upload_head,
    file->upload_tail,
    upload
  );
}

void
file_bind_downlaod (struct GNUNET_CHAT_File *file,
                    GNUNET_CHAT_FileDownloadCallback cb,
                    void *cls)
{
  GNUNET_assert(file);

  struct GNUNET_CHAT_FileDownload *download = GNUNET_new(
    struct GNUNET_CHAT_FileDownload
  );

  download->callback = cb;
  download->cls = cls;

  GNUNET_CONTAINER_DLL_insert(
    file->download_head,
    file->download_tail,
    download
  );
}

void
file_bind_unindex (struct GNUNET_CHAT_File *file,
                   GNUNET_CHAT_FileUnindexCallback cb,
                   void *cls)
{
  GNUNET_assert(file);

  struct GNUNET_CHAT_FileUnindex *unindex = GNUNET_new(
    struct GNUNET_CHAT_FileUnindex
  );

  unindex->callback = cb;
  unindex->cls = cls;

  GNUNET_CONTAINER_DLL_insert(
    file->unindex_head,
    file->unindex_tail,
    unindex
  );
}

void
file_update_upload (struct GNUNET_CHAT_File *file,
                    uint64_t completed,
                    uint64_t size)
{
  GNUNET_assert(file);

  file->status |= GNUNET_CHAT_FILE_STATUS_PUBLISH;

  struct GNUNET_CHAT_FileUpload *upload = file->upload_head;

  while (upload)
  {
    if (upload->callback)
      upload->callback(upload->cls, file, completed, size);

    upload = upload->next;
  }

  if (!(file->uri))
    return;

  struct GNUNET_MESSENGER_Message msg;
  memset(&msg, 0, sizeof(msg));

  msg.header.kind = GNUNET_MESSENGER_KIND_FILE;

  GNUNET_memcpy(&(msg.body.file.hash), &(file->hash), sizeof(file->hash));
  GNUNET_strlcpy(msg.body.file.name, file->name, NAME_MAX);
  msg.body.file.uri = GNUNET_FS_uri_to_string(file->uri);

  while (file->upload_head)
  {
    upload = file->upload_head;

    if (upload->context)
      GNUNET_MESSENGER_send_message(upload->context->room, &msg, NULL);

    GNUNET_CONTAINER_DLL_remove(
      file->upload_head,
      file->upload_tail,
      upload
    );

    GNUNET_free(upload);
  }

  GNUNET_free(msg.body.file.uri);

  file->status &= (
    GNUNET_CHAT_FILE_STATUS_MASK ^ GNUNET_CHAT_FILE_STATUS_PUBLISH
  );
}

void
file_update_download (struct GNUNET_CHAT_File *file,
                      uint64_t completed,
                      uint64_t size)
{
  GNUNET_assert(file);

  file->status |= GNUNET_CHAT_FILE_STATUS_DOWNLOAD;

  struct GNUNET_CHAT_FileDownload *download = file->download_head;

  while (download)
  {
    if (download->callback)
      download->callback(download->cls, file, completed, size);

    download = download->next;
  }

  if (completed < size)
    return;

  while (file->download_head)
  {
    download = file->download_head;

    GNUNET_CONTAINER_DLL_remove(
      file->download_head,
      file->download_tail,
      download
    );

    GNUNET_free(download);
  }

  file->status &= (
    GNUNET_CHAT_FILE_STATUS_MASK ^ GNUNET_CHAT_FILE_STATUS_DOWNLOAD
  );
}

void
file_update_unindex (struct GNUNET_CHAT_File *file,
                     uint64_t completed,
                     uint64_t size)
{
  GNUNET_assert(file);

  file->status |= GNUNET_CHAT_FILE_STATUS_UNINDEX;

  struct GNUNET_CHAT_FileUnindex *unindex = file->unindex_head;

  while (unindex)
  {
    if (unindex->callback)
      unindex->callback(unindex->cls, file, completed, size);

    unindex = unindex->next;
  }

  if (completed < size)
    return;

  while (file->unindex_head)
  {
    unindex = file->unindex_head;

    GNUNET_CONTAINER_DLL_remove(
      file->unindex_head,
      file->unindex_tail,
      unindex
    );

    GNUNET_free(unindex);
  }

  file->status &= (
    GNUNET_CHAT_FILE_STATUS_MASK ^ GNUNET_CHAT_FILE_STATUS_UNINDEX
  );
}
