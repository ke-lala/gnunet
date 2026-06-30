/*
   This file is part of GNUnet.
   Copyright (C) 2022--2024 GNUnet e.V.

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
 * @file ui.h
 */

#ifndef UI_H_
#define UI_H_

#include <gtk-3.0/gtk/gtk.h>
#include <libhandy-1/handy.h>
#include <stdint.h>

/**
 * Returns a new builder instance using the UI
 * definitions from a given resource path.
 *
 * @param resource_path Resource path
 * @return New builder
 */
GtkBuilder*
ui_builder_from_resource(const char *resource_path);

/**
 * Sets the text of a GtkLabel applying automatic utf8
 * conversion.
 *
 * @param label Label
 * @param text Non-utf8 text
 */
void
ui_label_set_text(GtkLabel *label,
                  const char *text);

/**
 * Sets the text of a GtkLabel applying automatic utf8
 * conversion and replaces supported syntax with proper
 * markup.
 *
 * @param label Label
 * @param text Non-utf8 text
 */
void
ui_label_set_markup_text(GtkLabel *label,
                         const char *text);

/**
 * Sets the text of a GtkLabel applying conversion from
 * file size to string representation.
 *
 * @param label Label
 * @param size File size
 */
void
ui_label_set_size(GtkLabel *label,
                  uint64_t size);

/**
 * Sets the text of a GtkEntry applying automatic utf8
 * conversion.
 *
 * @param entry Entry
 * @param text Non-utf8 text
 */
void
ui_entry_set_text(GtkEntry *entry,
                  const char *text);

/**
 * Returns the text from a GtkEntry applying automatic 
 * inverse utf8 conversion.
 *
 * @param entry Entry
 */
char*
ui_entry_get_text(GtkEntry *entry);

/**
 * Sets the text of a HdyAvatar applying automatic utf8
 * conversion.
 *
 * @param avatar Avatar
 * @param text Non-utf8 text
 */
void
ui_avatar_set_text(HdyAvatar *avatar,
                   const char *text);

/**
 * Sets the icon of a HdyAvatar.
 *
 * @param avatar Avatar
 * @param icon Loadable icon
 */
void
ui_avatar_set_icon(HdyAvatar *avatar,
                   GIcon *icon);

/**
 * Searches for a specific data set as qdata inside a 
 * container.
 *
 * @param container Container
 * @param quark Quark
 * @param data Data
 */
gboolean
ui_find_qdata_in_container(GtkContainer *container,
                           GQuark quark,
                           const gpointer data);

/**
 * Removes children from container which qdata is missing
 * inside a list of data.
 *
 * @param container Container
 * @param quark Quark
 * @param list List of data
 */
void
ui_clear_container_of_missing_qdata(GtkContainer *container,
                                    GQuark quark,
                                    const GList *list);

#endif /* UI_H_ */
