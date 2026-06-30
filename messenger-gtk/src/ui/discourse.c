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
 * @file ui/discourse.c
 */

#include "discourse.h"

#include <glib-2.0/glib.h>
#include <gnunet/gnunet_chat_lib.h>

#include "account_entry.h"
#include "discourse_panel.h"

#include "../application.h"
#include "../discourse.h"
#include "../request.h"
#include "../ui.h"
#include "../util.h"

#include <string.h>

static void
handle_back_button_click(UNUSED GtkButton *button,
                         gpointer user_data)
{
  g_assert(user_data);

  GtkWindow *window = GTK_WINDOW(user_data);
  gtk_window_close(window);
}

static void
handle_details_button_click(UNUSED GtkButton *button,
                            gpointer user_data)
{
  g_assert(user_data);

  HdyFlap *flap = HDY_FLAP(user_data);

  hdy_flap_set_reveal_flap(flap, !hdy_flap_get_reveal_flap(flap));
}

static void
handle_details_folded(GObject* object,
                      GParamSpec* pspec,
                      gpointer user_data)
{
  g_assert((object) && (pspec) && (user_data));

  HdyFlap* flap = HDY_FLAP(object);
  UI_DISCOURSE_Handle *handle = (UI_DISCOURSE_Handle*) user_data;

  const gboolean revealed = hdy_flap_get_reveal_flap(flap);

  gtk_widget_set_sensitive(
    GTK_WIDGET(handle->back_button),
    !revealed
  );
}

static void
_update_microphone_icon(UI_DISCOURSE_Handle *handle)
{
  g_assert(handle);

  if (handle->muted)
    gtk_stack_set_visible_child(handle->microphone_stack, handle->microphone_off_icon);
  else
    gtk_stack_set_visible_child(handle->microphone_stack, handle->microphone_on_icon);
}

static void
handle_microphone_button_click(UNUSED GtkButton *button,
                               gpointer user_data)
{
  g_assert(user_data);

  UI_DISCOURSE_Handle *handle = (UI_DISCOURSE_Handle*) user_data;

  handle->muted = !(handle->muted);
  if (handle->voice_discourse)
    discourse_set_mute(handle->voice_discourse, handle->muted);

  _update_microphone_icon(handle);
}

static void
_discourse_update_members(UI_DISCOURSE_Handle *handle);

static void
_update_streaming_state(UI_DISCOURSE_Handle *handle,
                        gboolean streaming)
{
  handle->streaming = streaming;

  if (handle->video_discourse)
    discourse_set_mute(handle->video_discourse, !(handle->streaming));

  if ((handle->app) && (!(handle->streaming)))
    application_set_active_session(handle->app, NULL);

  _discourse_update_members(handle);
}

static void
iterate_cameras(void *cls,
                const char *name,
                const char *description,
                const char *media_class,
                const char *media_role)
{
  g_assert(cls);

  UI_DISCOURSE_Handle *handle = (UI_DISCOURSE_Handle*) cls;

  if ((!name) || (!description) || (!media_class) || (!media_role))
    return;

  if (0 != g_strcmp0(media_class, "Video/Source"))
    return;
  if (0 != g_strcmp0(media_role, "Camera"))
    return;

  if (handle->video_discourse)
    discourse_set_target(handle->video_discourse, name);
}

static void
_request_camera_callback(MESSENGER_Application *app,
                         gboolean success,
                         gboolean error,
                         gpointer user_data)
{
  g_assert((app) && (user_data));

  UI_DISCOURSE_Handle *handle = (UI_DISCOURSE_Handle*) user_data;

  if ((!success) || (error))
    return;

  media_init_camera_capturing(&(app->media.camera), app);
  media_pw_main_loop_run(&(app->media.camera));

  media_pw_iterate_nodes(&(app->media.camera), iterate_cameras, handle);
  handle->streaming = true;

  _update_streaming_state(handle, handle->streaming);
}

static void
handle_camera_button_click(UNUSED GtkButton *button,
                           gpointer user_data)
{
  g_assert(user_data);

  UI_DISCOURSE_Handle *handle = (UI_DISCOURSE_Handle*) user_data;

  if (handle->streaming)
    _update_streaming_state(handle, false);
  else
    request_new_camera(
      handle->app,
      XDP_CAMERA_FLAG_NONE,
      _request_camera_callback,
      handle
    );
}

static void
iterate_streams(void *cls,
                const char *name,
                const char *description,
                const char *media_class,
                const char *media_role)
{
  g_assert(cls);

  UI_DISCOURSE_Handle *handle = (UI_DISCOURSE_Handle*) cls;

  if ((!name) || (!media_class))
    return;

  if (0 != g_strcmp0(media_class, "Stream/Output/Video"))
    return;

  if (handle->video_discourse)
  {
    discourse_set_target(handle->video_discourse, name);
    handle->streaming = true;
  }
}

static void
_request_screen_callback(MESSENGER_Application *app,
                         gboolean success,
                         gboolean error,
                         gpointer user_data)
{
  g_assert((app) && (user_data));

  UI_DISCOURSE_Handle *handle = (UI_DISCOURSE_Handle*) user_data;

  if ((!success) || (error))
    return;

  media_init_screen_sharing(&(app->media.screen), app);
  media_pw_main_loop_run(&(app->media.screen));

  handle->streaming = false;
  media_pw_iterate_nodes(&(app->media.screen), iterate_streams, handle);

  _update_streaming_state(handle, handle->streaming);
}

static void
handle_screen_button_click(UNUSED GtkButton *button,
                           gpointer user_data)
{
  g_assert(user_data);

  UI_DISCOURSE_Handle *handle = (UI_DISCOURSE_Handle*) user_data;

  if (handle->streaming)
    _update_streaming_state(handle, false);
  else
    request_new_screencast(
      handle->app,
      XDP_OUTPUT_MONITOR | XDP_OUTPUT_WINDOW,
      XDP_SCREENCAST_FLAG_NONE,
      XDP_CURSOR_MODE_EMBEDDED,
      XDP_PERSIST_MODE_TRANSIENT,
      _request_screen_callback,
      handle
    );
}

static void
handle_speakers_button_value_changed(UNUSED GtkScaleButton *button,
                                     gdouble value,
                                     gpointer user_data)
{
  g_assert(user_data);

  UI_DISCOURSE_Handle *handle = (UI_DISCOURSE_Handle*) user_data;

  if (handle->voice_discourse)
    discourse_set_volume(handle->voice_discourse, value);
}

static void
_update_call_button(UI_DISCOURSE_Handle *handle)
{
  g_assert(handle);

  if (((handle->voice_discourse) && 
      (GNUNET_YES == GNUNET_CHAT_discourse_is_open(handle->voice_discourse))) ||
      ((handle->video_discourse) &&
      (GNUNET_YES == GNUNET_CHAT_discourse_is_open(handle->video_discourse))))
    gtk_stack_set_visible_child(handle->call_stack, handle->call_stop_button);
  else
    gtk_stack_set_visible_child(handle->call_stack, handle->call_start_button);
}

static void
handle_call_start_button_click(UNUSED GtkButton *button,
                               gpointer user_data)
{
  g_assert(user_data);

  UI_DISCOURSE_Handle *handle = (UI_DISCOURSE_Handle*) user_data;

  if (!(handle->context))
    return;

  application_chat_lock(handle->app);

  handle->voice_discourse = GNUNET_CHAT_context_open_discourse(
    handle->context, get_voice_discourse_id()
  );

  handle->video_discourse = GNUNET_CHAT_context_open_discourse(
    handle->context, get_video_discourse_id()
  );

  _update_call_button(handle);
  application_chat_unlock(handle->app);
}

static void
handle_call_stop_button_click(UNUSED GtkButton *button,
                              gpointer user_data)
{
  g_assert(user_data);

  UI_DISCOURSE_Handle *handle = (UI_DISCOURSE_Handle*) user_data;

  if ((!(handle->context)) || (!(handle->voice_discourse)))
    return;

  application_chat_lock(handle->app);
  
  if (handle->voice_discourse)
  {
    GNUNET_CHAT_discourse_close(handle->voice_discourse);
    handle->voice_discourse = NULL;
  }

  if (handle->video_discourse)
  {
    GNUNET_CHAT_discourse_close(handle->video_discourse);
    handle->video_discourse = NULL;
  }

  handle->muted = TRUE;
  handle->streaming = FALSE;

  _update_call_button(handle);
  application_chat_unlock(handle->app);
}

static void
handle_window_destroy(UNUSED GtkWidget *window,
                      gpointer user_data)
{
  g_assert(user_data);

  ui_discourse_window_cleanup((UI_DISCOURSE_Handle*) user_data);
}

void
ui_discourse_window_init(MESSENGER_Application *app,
                         UI_DISCOURSE_Handle *handle)
{
  g_assert((app) && (handle));

  handle->app = app;
  handle->context = NULL;

  handle->voice_discourse = NULL;
  handle->video_discourse = NULL;

  handle->muted = TRUE;
  handle->streaming = FALSE;

  handle->parent = GTK_WINDOW(app->ui.messenger.main_window);

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/discourse.ui")
  );

  handle->window = HDY_WINDOW(
    gtk_builder_get_object(handle->builder, "discourse_window")
  );

  gtk_window_set_position(
    GTK_WINDOW(handle->window),
    GTK_WIN_POS_CENTER_ON_PARENT
  );

  gtk_window_set_transient_for(
    GTK_WINDOW(handle->window),
    handle->parent
  );

  handle->title_bar = HDY_HEADER_BAR(
    gtk_builder_get_object(handle->builder, "title_bar")
  );

  handle->back_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "back_button")
  );

  g_signal_connect(
    handle->back_button,
    "clicked",
    G_CALLBACK(handle_back_button_click),
    handle->window
  );

  handle->details_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "details_button")
  );

  handle->details_flap = HDY_FLAP(
    gtk_builder_get_object(handle->builder, "details_flap")
  );

  g_signal_connect(
    handle->details_button,
    "clicked",
    G_CALLBACK(handle_details_button_click),
    handle->details_flap
  );

  g_signal_connect(
    handle->details_flap,
    "notify::reveal-flap",
    G_CALLBACK(handle_details_folded),
    handle
  );

  handle->discourse_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "discourse_stack")
  );

  handle->offline_page = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "offline_page")
  );

  handle->members_page = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "members_page")
  );

  handle->members_flowbox = GTK_FLOW_BOX(
    gtk_builder_get_object(handle->builder, "members_flowbox")
  );

  handle->microphone_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "microphone_button")
  );

  g_signal_connect(
    handle->microphone_button,
    "clicked",
    G_CALLBACK(handle_microphone_button_click),
    handle
  );

  handle->camera_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "camera_button")
  );

  handle->screen_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "screen_button")
  );

  handle->speakers_button = GTK_VOLUME_BUTTON(
    gtk_builder_get_object(handle->builder, "speakers_button")
  );

  g_signal_connect(
    handle->camera_button,
    "clicked",
    G_CALLBACK(handle_camera_button_click),
    handle
  );

  g_signal_connect(
    handle->screen_button,
    "clicked",
    G_CALLBACK(handle_screen_button_click),
    handle
  );

  g_signal_connect(
    handle->speakers_button,
    "value-changed",
    G_CALLBACK(handle_speakers_button_value_changed),
    handle
  );

  handle->microphone_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "microphone_stack")
  );

  handle->microphone_on_icon = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "microphone_on_icon")
  );

  handle->microphone_off_icon = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "microphone_off_icon")
  );

  handle->call_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "call_stack")
  );

  handle->call_start_button = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "call_start_button")
  );

  g_signal_connect(
    handle->call_start_button,
    "clicked",
    G_CALLBACK(handle_call_start_button_click),
    handle
  );

  handle->call_stop_button = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "call_stop_button")
  );

  g_signal_connect(
    handle->call_stop_button,
    "clicked",
    G_CALLBACK(handle_call_stop_button_click),
    handle
  );

  handle->close_details_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "close_details_button")
  );

  g_signal_connect(
    handle->close_details_button,
    "clicked",
    G_CALLBACK(handle_details_button_click),
    handle->details_flap
  );

  handle->contacts_listbox = GTK_LIST_BOX(
    gtk_builder_get_object(handle->builder, "contacts_listbox")
  );

  g_signal_connect(
    handle->window,
    "destroy",
    G_CALLBACK(handle_window_destroy),
    handle
  );

  gtk_widget_show_all(GTK_WIDGET(handle->window));
}

static enum GNUNET_GenericReturnValue
append_discourse_members_to_list(void *cls,
                                 UNUSED struct GNUNET_CHAT_Discourse *discourse,
                                 struct GNUNET_CHAT_Contact *contact)
{
  g_assert((cls) && (contact));

  GList **list = (GList**) cls;
  *list = g_list_append(*list, contact);
  return GNUNET_YES;
}

static enum GNUNET_GenericReturnValue
append_discourses_members(void *cls,
                          UNUSED struct GNUNET_CHAT_Context *context,
                          struct GNUNET_CHAT_Discourse *discourse)
{
  g_assert((cls) && (discourse));

  GNUNET_CHAT_discourse_iterate_contacts(
    discourse,
    append_discourse_members_to_list,
    cls
  );

  return GNUNET_YES;
}

static enum GNUNET_GenericReturnValue
append_group_contacts(void *cls,
                      UNUSED struct GNUNET_CHAT_Group *group,
                      struct GNUNET_CHAT_Contact *contact)
{
  g_assert((cls) && (contact));

  GList **list = (GList**) cls;
  *list = g_list_append(*list, contact);
  return GNUNET_YES;
}

struct IterateDiscourseClosure {
  MESSENGER_Application *app;
  UI_DISCOURSE_Handle *handle;
  GtkContainer *container;
};

static enum GNUNET_GenericReturnValue
iterate_ui_discourse_update_discourse_members(void *cls,
                                              struct GNUNET_CHAT_Discourse *discourse,
                                              struct GNUNET_CHAT_Contact *contact)
{
  struct IterateDiscourseClosure *closure = (
    (struct IterateDiscourseClosure*) cls
  );

  if (ui_find_qdata_in_container(closure->container, closure->app->quarks.data, contact))
    return GNUNET_YES;

  GtkFlowBox *flowbox = GTK_FLOW_BOX(closure->container);

  UI_DISCOURSE_PANEL_Handle* panel = ui_discourse_panel_new(closure->app);
  ui_discourse_panel_set_contact(panel, contact);

  GtkWidget *parent = gtk_widget_get_parent(panel->panel_box);
  if (parent)
    gtk_container_remove(GTK_CONTAINER(parent), panel->panel_box);

  gtk_flow_box_insert(flowbox, panel->panel_box, -1);

  GtkFlowBoxChild *child = GTK_FLOW_BOX_CHILD(
    gtk_widget_get_parent(panel->panel_box)
  );

  g_object_set_qdata(G_OBJECT(child), closure->app->quarks.data, contact);
  g_object_set_qdata_full(
    G_OBJECT(child),
    closure->app->quarks.ui,
    panel,
    (GDestroyNotify) ui_discourse_panel_delete
  );

  return GNUNET_YES;
}

static enum GNUNET_GenericReturnValue
iterate_ui_discourse_update_context_discourses(void *cls,
                                               struct GNUNET_CHAT_Context *context,
                                               struct GNUNET_CHAT_Discourse *discourse)
{
  g_assert((cls) && (context) && (discourse));

  GNUNET_CHAT_discourse_iterate_contacts(
    discourse,
    iterate_ui_discourse_update_discourse_members,
    cls
  );

  return GNUNET_YES;
}

struct IterateDiscourseVideoClosure {
  MESSENGER_Application *app;
  UI_DISCOURSE_Handle *handle;
  GList *children;
};

static enum GNUNET_GenericReturnValue
iterate_ui_discourse_update_discourse_video(void *cls,
                                            struct GNUNET_CHAT_Discourse *discourse,
                                            struct GNUNET_CHAT_Contact *contact)
{
  g_assert((cls) && (discourse) && (contact));

  struct IterateDiscourseVideoClosure *closure = (
    (struct IterateDiscourseVideoClosure*) cls
  );

  GList *list = closure->children;
  while (list)
  {
    GtkFlowBoxChild *child = GTK_FLOW_BOX_CHILD(list->data);

    UI_DISCOURSE_PANEL_Handle* panel = (UI_DISCOURSE_PANEL_Handle*) (
      g_object_get_qdata(
        G_OBJECT(child),
        closure->app->quarks.ui
      )
    );

    if (contact != panel->contact)
      goto skip_child;

    GtkContainer *parent = NULL;
    if (closure->handle->context)
      parent = GTK_CONTAINER(panel->video_box);

    const gboolean linked = discourse_link_widget(
      discourse,
      contact,
      parent
    );

    if ((linked) && (discourse_is_active(discourse, contact)))
      gtk_stack_set_visible_child(panel->panel_stack, panel->video_box);
    else
      gtk_stack_set_visible_child(panel->panel_stack, panel->avatar_box);

  skip_child:
    list = g_list_next(list);
  }

  return GNUNET_YES;
}

static void
_discourse_update_members(UI_DISCOURSE_Handle *handle)
{
  g_assert(handle);

  GList *list = NULL;
  GNUNET_CHAT_context_iterate_discourses(
    handle->context,
    append_discourses_members,
    &list
  );
  
  ui_clear_container_of_missing_qdata(
    GTK_CONTAINER(handle->members_flowbox),
    handle->app->quarks.data,
    list
  );

  if (list)
  {
    gtk_stack_set_visible_child(handle->discourse_stack, handle->members_page);
    g_list_free(list);
  }
  else
    gtk_stack_set_visible_child(handle->discourse_stack, handle->offline_page);

  if (!(handle->context))
    return;

  struct IterateDiscourseClosure closure;
  closure.app = handle->app;
  closure.handle = handle;
  closure.container = GTK_CONTAINER(handle->members_flowbox);

  GNUNET_CHAT_context_iterate_discourses(
    handle->context,
    iterate_ui_discourse_update_context_discourses,
    &closure
  );

  list = gtk_container_get_children(GTK_CONTAINER(handle->members_flowbox));
  if (list)
  {
    struct IterateDiscourseVideoClosure closure_video;
    closure_video.app = handle->app;
    closure_video.handle = handle;
    closure_video.children = list;

    GNUNET_CHAT_discourse_iterate_contacts(
      handle->video_discourse,
      iterate_ui_discourse_update_discourse_video,
      &closure_video
    );

    g_list_free(list);
  }
}

static enum GNUNET_GenericReturnValue
iterate_ui_discourse_update_group_contacts(void *cls,
                                           UNUSED struct GNUNET_CHAT_Group *group,
                                           struct GNUNET_CHAT_Contact *contact)
{
  struct IterateDiscourseClosure *closure = (
    (struct IterateDiscourseClosure*) cls
  );

  if (ui_find_qdata_in_container(closure->container, closure->app->quarks.data, contact))
    return GNUNET_YES;

  GtkListBox *listbox = GTK_LIST_BOX(closure->container);
  UI_ACCOUNT_ENTRY_Handle* entry = ui_account_entry_new(closure->app);

  ui_account_entry_set_contact(entry, contact);

  gtk_list_box_prepend(listbox, entry->entry_box);

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
    gtk_widget_get_parent(entry->entry_box)
  );

  g_object_set_qdata(G_OBJECT(row), closure->app->quarks.data, contact);
  g_object_set_qdata_full(
    G_OBJECT(row),
    closure->app->quarks.ui,
    entry,
    (GDestroyNotify) ui_account_entry_delete
  );

  return GNUNET_YES;
}

static void
_discourse_update_contacts(UI_DISCOURSE_Handle *handle,
                           struct GNUNET_CHAT_Group* group)
{
  g_assert((handle) && (handle->app));

  GList *list = NULL;
  if (group)
    GNUNET_CHAT_group_iterate_contacts(
	    group,
      append_group_contacts,
      &list
    );
  
  ui_clear_container_of_missing_qdata(
    GTK_CONTAINER(handle->contacts_listbox),
    handle->app->quarks.data,
    list
  );

  if (list)
    g_list_free(list);

  if (group)
  {
    struct IterateDiscourseClosure closure;
    closure.app = handle->app;
    closure.container = GTK_CONTAINER(handle->contacts_listbox);

    GNUNET_CHAT_group_iterate_contacts(
	    group,
      iterate_ui_discourse_update_group_contacts,
      &closure
    );
  }

  gtk_widget_set_visible(
    GTK_WIDGET(handle->details_button),
    group? TRUE : FALSE
  );
}

static enum GNUNET_GenericReturnValue
iterate_ui_discourse_search_context_discourses(void *cls,
                                               struct GNUNET_CHAT_Context *context,
                                               struct GNUNET_CHAT_Discourse *discourse)
{
  g_assert((cls) && (context) && (discourse));

  struct GNUNET_CHAT_Discourse **discourses = (struct GNUNET_CHAT_Discourse**) cls;

  const struct GNUNET_CHAT_DiscourseId *id = GNUNET_CHAT_discourse_get_id(discourse);

  if (0 == GNUNET_memcmp(id, get_voice_discourse_id()))
    discourses[0] = discourse;
  else if (0 == GNUNET_memcmp(id, get_video_discourse_id()))
    discourses[1] = discourse;

  return GNUNET_YES;
}

static void
_update_discourse_via_context(UI_DISCOURSE_Handle *handle)
{
  g_assert(handle);

  handle->voice_discourse = NULL;
  handle->video_discourse = NULL;

  if (!(handle->context))
    return;

  struct GNUNET_CHAT_Discourse *discourses [2];
  memset(discourses, 0, sizeof(struct GNUNET_CHAT_Discourse*) * 2);

  GNUNET_CHAT_context_iterate_discourses(
    handle->context,
    iterate_ui_discourse_search_context_discourses,
    discourses
  );

  gtk_widget_set_sensitive(GTK_WIDGET(handle->microphone_button), discourse_has_controls(
    discourses[0], MESSENGER_DISCOURSE_CTRL_MICROPHONE
  ));

  gtk_widget_set_sensitive(GTK_WIDGET(handle->camera_button), discourse_has_controls(
    discourses[1], MESSENGER_DISCOURSE_CTRL_WEBCAM
  )
#ifndef MESSENGER_APPLICATION_NO_PORTAL
  && ((handle->app->portal) && (xdp_portal_is_camera_present(handle->app->portal)))
#endif
  );

  gtk_widget_set_sensitive(GTK_WIDGET(handle->screen_button), discourse_has_controls(
    discourses[1], MESSENGER_DISCOURSE_CTRL_SCREEN_CAPTURE
  ));

  gtk_widget_set_sensitive(GTK_WIDGET(handle->speakers_button), discourse_has_controls(
    discourses[0], MESSENGER_DISCOURSE_CTRL_SPEAKERS
  ));

  if (discourses[0])
  {
    handle->muted = discourse_is_mute(discourses[0]);

    gtk_scale_button_set_value(
      GTK_SCALE_BUTTON(handle->speakers_button),
      discourse_get_volume(discourses[0])
    );
  }

  handle->voice_discourse = discourses[0];
  handle->video_discourse = discourses[1];
}

void
ui_discourse_window_update(UI_DISCOURSE_Handle *handle,
                           struct GNUNET_CHAT_Context *context)
{
  g_assert(handle);

  handle->context = context;

  _update_discourse_via_context(handle);
  _update_call_button(handle);
  _update_microphone_icon(handle);
  _discourse_update_members(handle);

  struct GNUNET_CHAT_Group* group = GNUNET_CHAT_context_get_group(
    handle->context
  );

  _discourse_update_contacts(handle, group);
}

void
ui_discourse_window_cleanup(UI_DISCOURSE_Handle *handle)
{
  g_assert(handle);

  ui_discourse_window_update(handle, NULL);

  g_object_unref(handle->builder);

  memset(handle, 0, sizeof(*handle));
}
