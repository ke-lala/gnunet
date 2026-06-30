/*
   This file is part of GNUnet.
   Copyright (C) 2021--2024 GNUnet e.V.

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
 * @file gnunet_chat_file.h
 */

#ifndef GNUNET_CHAT_FILE_H_
#define GNUNET_CHAT_FILE_H_

#include <gnunet/gnunet_fs_service.h>
#include <gnunet/gnunet_messenger_service.h>
#include <gnunet/gnunet_util_lib.h>

#include "gnunet_chat_lib.h"

struct GNUNET_CHAT_FileUpload
{
  struct GNUNET_CHAT_FileUpload *prev;
  struct GNUNET_CHAT_FileUpload *next;

  struct GNUNET_CHAT_Context *context;
  GNUNET_CHAT_FileUploadCallback callback;

  void *cls;
};

struct GNUNET_CHAT_FileDownload
{
  struct GNUNET_CHAT_FileDownload *prev;
  struct GNUNET_CHAT_FileDownload *next;

  GNUNET_CHAT_FileDownloadCallback callback;

  void *cls;
};

struct GNUNET_CHAT_FileUnindex
{
  struct GNUNET_CHAT_FileUnindex *prev;
  struct GNUNET_CHAT_FileUnindex *next;

  GNUNET_CHAT_FileUnindexCallback callback;

  void *cls;
};

struct GNUNET_CHAT_Handle;

#define GNUNET_CHAT_FILE_STATUS_DOWNLOAD 0x1
#define GNUNET_CHAT_FILE_STATUS_PUBLISH  0x2
#define GNUNET_CHAT_FILE_STATUS_UNINDEX  0x4
#define GNUNET_CHAT_FILE_STATUS_MASK     0x7

struct GNUNET_CHAT_File
{
  struct GNUNET_CHAT_Handle *handle;

  char *name;

  struct GNUNET_HashCode hash;
  struct GNUNET_CRYPTO_SymmetricSessionKey *key;
  struct GNUNET_FS_MetaData *meta;
  struct GNUNET_FS_Uri *uri;

  struct GNUNET_FS_DownloadContext *download;
  struct GNUNET_FS_PublishContext *publish;
  struct GNUNET_FS_UnindexContext *unindex;

  struct GNUNET_CHAT_FileUpload *upload_head;
  struct GNUNET_CHAT_FileUpload *upload_tail;

  struct GNUNET_CHAT_FileDownload *download_head;
  struct GNUNET_CHAT_FileDownload *download_tail;

  struct GNUNET_CHAT_FileUnindex *unindex_head;
  struct GNUNET_CHAT_FileUnindex *unindex_tail;

  int status;
  char *preview;

  void *user_pointer;
};

/**
 * Creates a chat file handle from a file body in a
 * <i>message</i> with a selected chat <i>handle</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in] message File message body
 * @return New chat file handle
 */
struct GNUNET_CHAT_File*
file_create_from_message (struct GNUNET_CHAT_Handle *handle,
			                    const struct GNUNET_MESSENGER_MessageFile *message);

/**
 * Creates a chat file handle from a FS CHK URI and
 * with a selected chat <i>handle</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in] uri FS CHK URI
 * @return New chat file handle
 */
struct GNUNET_CHAT_File*
file_create_from_chk_uri (struct GNUNET_CHAT_Handle *handle,
                          const struct GNUNET_FS_Uri *uri);

/**
 * Creates a chat file handle from a local file on disk
 * under a given <i>name</i> using a <i>hash</i> and a
 * selected symmetric <i>key</i> with a selected chat
 * <i>handle</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in] name File name
 * @param[in] hash File hash
 * @param[in] key Symmetric key
 * @return New chat file handle
 */
struct GNUNET_CHAT_File*
file_create_from_disk (struct GNUNET_CHAT_Handle *handle,
                       const char *name,
                       const struct GNUNET_HashCode *hash,
                       const struct GNUNET_CRYPTO_SymmetricSessionKey *key);

/**
 * Destroys a chat <i>file</i> handle and frees its memory.
 *
 * @param[in,out] file Chat file handle
 */
void
file_destroy (struct GNUNET_CHAT_File *file);

/**
 * Binds a chat <i>context</i>, a callback and a closure
 * to a given chat <i>file</i> handle to be called on any
 * progress uploading the regarding file.
 *
 * @param[in,out] file Chat file handle
 * @param[in,out] context Chat context
 * @param[in] cb Callback for upload progress
 * @param[in,out] cls Closure
 */
void
file_bind_upload (struct GNUNET_CHAT_File *file,
                  struct GNUNET_CHAT_Context *context,
                  GNUNET_CHAT_FileUploadCallback cb,
                  void *cls);

/**
 * Binds a callback and a closure to a given chat <i>file</i>
 * handle to be called on any progress downloading the
 * regarding file.
 *
 * @param[in,out] file Chat file handle
 * @param[in] cb Callback for download progress
 * @param[in,out] cls Closure
 */
void
file_bind_downlaod (struct GNUNET_CHAT_File *file,
                    GNUNET_CHAT_FileDownloadCallback cb,
                    void *cls);

/**
 * Binds a callback and a closure to a given chat <i>file</i>
 * handle to be called on any progress unindexing the
 * regarding file.
 *
 * @param[in,out] file Chat file handle
 * @param[in] cb Callback for unindex progress
 * @param[in,out] cls Closure
 */
void
file_bind_unindex (struct GNUNET_CHAT_File *file,
                   GNUNET_CHAT_FileUnindexCallback cb,
                   void *cls);

/**
 * Calls the regarding events and bound callback of a given
 * chat <i>file</i> handle to a file upload progress using
 * the provided state of <i>completed</i> bytes and its file
 * <i>size</i>.
 *
 * @param[in,out] file Chat file handle
 * @param[in] completed Amount of uploaded bytes
 * @param[in] size Full file size
 */
void
file_update_upload (struct GNUNET_CHAT_File *file,
                    uint64_t completed,
                    uint64_t size);

/**
 * Calls the regarding events and bound callback of a given
 * chat <i>file</i> handle to a file download progress using
 * the provided state of <i>completed</i> bytes and its file
 * <i>size</i>.
 *
 * @param[in,out] file Chat file handle
 * @param[in] completed Amount of downloaded bytes
 * @param[in] size Full file size
 */
void
file_update_download (struct GNUNET_CHAT_File *file,
                      uint64_t completed,
                      uint64_t size);

/**
 * Calls the regarding events and bound callback of a given
 * chat <i>file</i> handle to a file unindex progress using
 * the provided state of <i>completed</i> bytes and its file
 * <i>size</i>.
 *
 * @param[in,out] file Chat file handle
 * @param[in] completed Amount of unindexed bytes
 * @param[in] size Full file size
 */
void
file_update_unindex (struct GNUNET_CHAT_File *file,
                     uint64_t completed,
                     uint64_t size);

#endif /* GNUNET_CHAT_FILE_H_ */
