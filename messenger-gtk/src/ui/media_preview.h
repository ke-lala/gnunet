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
 * @file ui/media_preview.h
 */

#ifndef UI_MEDIA_PREVIEW_H_
#define UI_MEDIA_PREVIEW_H_

#include <gnunet/gnunet_chat_lib.h>

#include "messenger.h"

typedef struct UI_MEDIA_PREVIEW_Handle
{
  const struct GNUNET_CHAT_File *file;

  GtkBuilder *builder;

  GtkWidget *media_box;
  
  GtkDrawingArea *preview_drawing_area;

  MESSENGER_Application *app;
} UI_MEDIA_PREVIEW_Handle;

/**
 * Allocates and creates a new media preview handle
 * to manage loading files for a given messenger
 * application.
 *
 * @param app Messenger application
 * @return New media preview handle
 */
UI_MEDIA_PREVIEW_Handle*
ui_media_preview_new(MESSENGER_Application *app);

/**
 * Updates a media preview handle with a selected
 * file to represent it visually.
 *
 * @param handle Media preview handle
 * @param file Chat file
 */
void
ui_media_preview_update(UI_MEDIA_PREVIEW_Handle *handle,
                        struct GNUNET_CHAT_File *file);

/**
 * Frees its resources and destroys a given media 
 * preview handle.
 *
 * @param handle Media preview handle
 */
void
ui_media_preview_delete(UI_MEDIA_PREVIEW_Handle *handle);

#endif /* UI_MEDIA_PREVIEW_H_ */
