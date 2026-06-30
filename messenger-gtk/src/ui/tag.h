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
 * @file ui/tag.h
 */

#ifndef UI_TAG_H_
#define UI_TAG_H_

#include "messenger.h"

typedef struct UI_TAG_Handle
{
  GtkBuilder *builder;

  GtkLabel *tag_label;
} UI_TAG_Handle;

/**
 * Allocates and creates a new tag handle 
 * to manage a tag in a list for a given 
 * messenger application.
 *
 * @param app Messenger application
 * @return New tag handle
 */
UI_TAG_Handle*
ui_tag_new(MESSENGER_Application *app);

/**
 * Sets the content of the given tag handle 
 * respectively to visually represent a
 * selected chat tag message.
 *
 * @param handle Tag handle
 * @param app Messenger application
 * @param message Chat tag message
 */
void
ui_tag_set_message(UI_TAG_Handle* handle,
                   MESSENGER_Application *app,
                   const struct GNUNET_CHAT_Message *message);

/**
 * Frees its resources and destroys a given
 * tag handle.
 *
 * @param handle Tag handle
 */
void
ui_tag_delete(UI_TAG_Handle *handle);

#endif /* UI_TAG_H_ */
