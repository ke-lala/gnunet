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
 * @file contact.h
 */

#ifndef CONTACT_H_
#define CONTACT_H_

#include "application.h"

typedef struct MESSENGER_ContactInfo
{
  void *last_message;
  GFile *icon_file;
  GIcon *icon;
  guint task;

  GList *name_labels;
  GList *name_avatars;
  GList *visible_widgets;
} MESSENGER_ContactInfo;

/**
 * Creates a contact information struct to potentially
 * update all GUI appearances of a specific contact at
 * once.
 *
 * @param contact Chat contact
 * @return #GNUNET_YES on info creation, otherwise #GNUNET_NO
 */
enum GNUNET_GenericReturnValue
contact_create_info(struct GNUNET_CHAT_Contact *contact);

/**
 * Destroys and frees resources allocated for a given
 * contact information struct.
 *
 * @param contact Chat contact
 */
void
contact_destroy_info(struct GNUNET_CHAT_Contact *contact);

/**
 * Sets the latest join/leave UI message handle so that
 * the application can check whether a contact is available.
 *
 * @param contact Chat contact
 * @param message Pointer to UI message handle
 */
void
contact_set_last_message_to_info(const struct GNUNET_CHAT_Contact *contact,
				                         void *message);

/**
 * Returns the latest join/leave UI message handle of
 * a specific contact.
 *
 * @param contact Chat contact
 */
void*
contact_get_last_message_from_info(const struct GNUNET_CHAT_Contact *contact);

/**
 * Adds a GtkLabel to the list of labels
 * which get updated by state changes.
 *
 * @param contact Chat contact
 * @param label Label
 */
void
contact_add_name_label_to_info(const struct GNUNET_CHAT_Contact *contact,
			                         GtkLabel *label);

/**
 * Removes a GtkLabel from the list of labels
 * which get updated by state changes.
 *
 * @param contact Chat contact
 * @param label Label
 */
void
contact_remove_name_label_from_info(const struct GNUNET_CHAT_Contact *contact,
			                              GtkLabel *label);

/**
 * Adds a HdyAvatar to the list of avatars
 * which get updated by state changes.
 *
 * @param contact Chat contact
 * @param avatar Avatar
 */
void
contact_add_name_avatar_to_info(const struct GNUNET_CHAT_Contact *contact,
			                          HdyAvatar *avatar);

/**
 * Removes a HdyAvatar from the list of avatars
 * which get updated by state changes.
 *
 * @param contact Chat contact
 * @param avatar Avatar
 */
void
contact_remove_name_avatar_from_info(const struct GNUNET_CHAT_Contact *contact,
			                               HdyAvatar *avatar);

/**
 * Adds a GtkWidget to the list of widgets
 * which get visibility updated by state changes.
 *
 * @param contact Chat contact
 * @param widget Widget
 */
void
contact_add_visible_widget_to_info(const struct GNUNET_CHAT_Contact *contact,
                                   GtkWidget *widget);

/**
 * Removes a GtkWidget from the list of widgets
 * which get visibility updated by state changes.
 *
 * @param contact Chat contact
 * @param widget Widget
 */
void
contact_remove_visible_widget_to_info(const struct GNUNET_CHAT_Contact *contact,
                                      GtkWidget *widget);

/**
 * Updates the connected UI elements for a given
 * contact depending on the current state.
 *
 * @param contact Chat contact
 * @param attributes Flag to check for attributes changes
 */
void
contact_update_info(const struct GNUNET_CHAT_Contact *contact);

/**
 * Updates the connected UI elements for a given
 * contact based on current attributes changes.
 *
 * @param contact Chat contact
 * @param app Messenger application
 */
void
contact_update_attributes(struct GNUNET_CHAT_Contact *contact,
                          MESSENGER_Application *app);

#endif /* CONTACT_H_ */
