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
 * @file ui/picker.h
 */

#ifndef UI_PICKER_H_
#define UI_PICKER_H_

#include "chat.h"

typedef struct UI_PICKER_Handle
{
  GtkBuilder *builder;
  GtkWidget *picker_box;

  GtkStack *picker_stack;
  GtkStack *emoji_stack;

  GtkWidget *recent_emoji_page;

  HdyViewSwitcherBar *picker_switcher_bar;
  HdyViewSwitcherBar *emoji_switcher_bar;

  GtkFlowBox *recent_flow_box;
  GtkFlowBox *people_flow_box;
  GtkFlowBox *nature_flow_box;
  GtkFlowBox *food_flow_box;
  GtkFlowBox *activities_flow_box;
  GtkFlowBox *travel_flow_box;
  GtkFlowBox *objects_flow_box;
  GtkFlowBox *symbols_flow_box;
  GtkFlowBox *flags_flow_box;

  HdySearchBar *emoji_search_bar;
  GtkSearchEntry *emoji_search_entry;

  GtkButton *search_button;
  GtkButton *settings_button;
} UI_PICKER_Handle;

/**
 * Allocates and creates a new picker handle to
 * manage emoji selection in a chat for a given
 * messenger application.
 *
 * @param app Messenger application
 * @param chat Chat handle
 * @return New picker handle
 */
UI_PICKER_Handle*
ui_picker_new(MESSENGER_Application *app,
              UI_CHAT_Handle *chat);

/**
 * Frees its resources and destroys a given picker
 * handle.
 *
 * @param handle Picker handle
 */
void
ui_picker_delete(UI_PICKER_Handle *handle);

#endif /* UI_PICKER_H_ */
