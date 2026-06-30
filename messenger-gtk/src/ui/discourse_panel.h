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
 * @file ui/discourse_panel.h
 */

#ifndef UI_DISCOURSE_PANEL_H_
#define UI_DISCOURSE_PANEL_H_

#include "messenger.h"

#include <gnunet/gnunet_chat_lib.h>

typedef struct UI_DISCOURSE_PANEL_Handle
{
  const struct GNUNET_CHAT_Contact *contact;

  GtkBuilder *builder;

  GtkWidget *panel_box;

  GtkStack *panel_stack;
  GtkWidget *avatar_box;

  HdyAvatar *panel_avatar;
  GtkLabel *panel_label;

  GtkWidget *video_box;
} UI_DISCOURSE_PANEL_Handle;

/**
 * Allocates and creates a new discourse panel
 * handle to visualize a contact in a discourse
 * a given messenger application.
 *
 * @param app Messenger application
 * @return New discourse panel handle
 */
UI_DISCOURSE_PANEL_Handle*
ui_discourse_panel_new(MESSENGER_Application *app);

/**
 * Sets the content of the given discourse panel
 * handle respectively to visually represent a
 * selected chat contact.
 *
 * @param handle Discourse panel handle
 * @param contact Chat contact
 */
void
ui_discourse_panel_set_contact(UI_DISCOURSE_PANEL_Handle* handle,
                               const struct GNUNET_CHAT_Contact *contact);

/**
 * Frees its resources and destroys a given
 * discourse panel handle.
 *
 * @param handle Discourse panel handle
 */
void
ui_discourse_panel_delete(UI_DISCOURSE_PANEL_Handle *handle);

#endif /* UI_DISCOURSE_PANEL_H_ */
