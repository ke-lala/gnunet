/*
   This file is part of GNUnet.
   Copyright (C) 2022--2025 GNUnet e.V.

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
 * @file ui/contact_info.c
 */

#include "contact_info.h"

#include "chat_entry.h"

#include "../account.h"
#include "../application.h"
#include "../contact.h"
#include "../file.h"
#include "../ui.h"

#include <gnunet/gnunet_chat_lib.h>
#include <gnunet/gnunet_common.h>
#include <string.h>

static void
handle_contact_edit_button_click(UNUSED GtkButton *button,
                                 gpointer user_data)
{
  g_assert(user_data);

  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  gboolean editable = gtk_widget_is_sensitive(
    GTK_WIDGET(handle->contact_name_entry)
  );

  if (!editable)
    goto skip_change_name;

  const gboolean change_own_name = (
    (!(handle->contact)) ||
    (GNUNET_YES == GNUNET_CHAT_contact_is_owned(handle->contact))
  );

  const gchar *name = gtk_entry_get_text(handle->contact_name_entry);

  if ((name) && (0 == g_utf8_strlen(name, 1)))
    name = NULL;

  application_chat_lock(handle->app);

  if (change_own_name)
  {
    if (GNUNET_YES != GNUNET_CHAT_set_name(handle->app->chat.messenger.handle, name))
      gtk_entry_set_text(
	      handle->contact_name_entry,
	      GNUNET_CHAT_get_name(handle->app->chat.messenger.handle)
      );
  }
  else
    GNUNET_CHAT_contact_set_name(handle->contact, name);

  application_chat_unlock(handle->app);

skip_change_name:
  gtk_image_set_from_icon_name(
    handle->contact_edit_symbol,
    editable?
    "document-edit-symbolic" :
    "emblem-ok-symbolic",
    GTK_ICON_SIZE_BUTTON
  );

  gtk_widget_set_sensitive(
    GTK_WIDGET(handle->contact_name_entry),
    !editable
  );
}

static void
handle_contact_name_entry_activate(UNUSED GtkEntry *entry,
                                   gpointer user_data)
{
  g_assert(user_data);

  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  handle_contact_edit_button_click(handle->contact_edit_button, handle);
}

static void
handle_profile_chooser_update_preview(GtkFileChooser *file_chooser,
                                      gpointer user_data)
{
  g_assert((file_chooser) && (user_data));

  HdyAvatar *avatar = HDY_AVATAR(user_data);

  gboolean have_preview = false;
  gchar *filename = gtk_file_chooser_get_preview_filename(file_chooser);

  if ((!filename) || (!g_file_test(filename, G_FILE_TEST_IS_REGULAR)))
    goto skip_preview;

  GFile *file = g_file_new_for_path(filename);

  if (!file)
    goto skip_icon;

  GIcon *icon = g_file_icon_new(file);

  if (!icon)
    goto skip_avatar;

  hdy_avatar_set_loadable_icon(avatar, G_LOADABLE_ICON(icon));
  g_object_unref(icon);
  have_preview = true;

skip_avatar:
  g_object_unref(file);

skip_icon:
  g_free(filename);

skip_preview:
  gtk_file_chooser_set_preview_widget_active(file_chooser, have_preview);
}

static void
_cb_file_upload(void *cls,
                struct GNUNET_CHAT_File *file,
                uint64_t completed,
                uint64_t size)
{
  g_assert((cls) && (file));

  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  file_update_upload_info(file, completed, size);

  if (completed < size)
    return;

  struct GNUNET_CHAT_Uri *uri = GNUNET_CHAT_file_get_uri(file);

  if (!uri)
    return;

  char *uri_string = GNUNET_CHAT_uri_to_string(uri);

  if (uri_string)
  {
    GNUNET_CHAT_set_attribute(
      app->chat.messenger.handle,
      GNUNET_CHAT_ATTRIBUTE_AVATAR,
      uri_string
    );

    GNUNET_free(uri_string);
  }

  GNUNET_CHAT_uri_destroy(uri);
}

static void
handle_profile_chooser_file_set(GtkFileChooserButton *button,
                                gpointer user_data)
{
  g_assert(user_data);

  GtkFileChooser *file_chooser = GTK_FILE_CHOOSER(button);
  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  gchar *filename = gtk_file_chooser_get_preview_filename(file_chooser);

  if (!filename)
    return;

  application_chat_lock(handle->app);

  GNUNET_CHAT_upload_file(
    handle->app->chat.messenger.handle,
    filename,
    _cb_file_upload,
    handle->app
  );

  application_chat_unlock(handle->app);

  g_free(filename);
}

static void
_contact_info_switch_stack_to(UI_CONTACT_INFO_Handle *handle,
                              GtkWidget *page_widget)
{
  g_assert((handle) && (page_widget));

  gtk_widget_set_visible(GTK_WIDGET(handle->back_button), TRUE);

  gtk_stack_set_visible_child(
    handle->contact_info_stack,
    page_widget
  );
}

static void
handle_reveal_identity_button_click(UNUSED GtkButton *button,
                                    gpointer user_data)
{
  g_assert(user_data);

  struct UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  _contact_info_switch_stack_to(handle, handle->identity_box);
}

static void
handle_list_attributes_button_click(UNUSED GtkButton *button,
                                    gpointer user_data)
{
  g_assert(user_data);

  struct UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  _contact_info_switch_stack_to(handle, handle->attributes_box);
}

static void
handle_share_attributes_button_click(UNUSED GtkButton *button,
                                     gpointer user_data)
{
  g_assert(user_data);

  struct UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  _contact_info_switch_stack_to(handle, handle->sharing_box);
}

static void
handle_list_tags_button_click(UNUSED GtkButton *button,
                              gpointer user_data)
{
  g_assert(user_data);

  struct UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  _contact_info_switch_stack_to(handle, handle->tags_box);
}

static void
handle_block_button_click(UNUSED GtkButton *button,
                          gpointer user_data)
{
  g_assert(user_data);

  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  if (!(handle->contact))
    return;

  application_chat_lock(handle->app);
  GNUNET_CHAT_contact_set_blocked(handle->contact, GNUNET_YES);
  application_chat_unlock(handle->app);

  gtk_stack_set_visible_child(
    handle->block_stack,
    GTK_WIDGET(handle->unblock_button)
  );
}

static void
handle_unblock_button_click(UNUSED GtkButton *button,
                            gpointer user_data)
{
  g_assert(user_data);

  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  if (!(handle->contact))
    return;

  application_chat_lock(handle->app);
  GNUNET_CHAT_contact_set_blocked(handle->contact, GNUNET_NO);
  application_chat_unlock(handle->app);

  gtk_stack_set_visible_child(
    handle->block_stack, 
    GTK_WIDGET(handle->block_button)
  );
}

static void*
_open_direct_chat(MESSENGER_Application *app,
                  struct GNUNET_CHAT_Contact *contact)
{
  g_assert((app) && (contact));

  application_chat_lock(app);

  struct GNUNET_CHAT_Context *context = GNUNET_CHAT_contact_get_context(
    contact
  );

  if (!context)
    return GNUNET_NO;

  void *user_pointer = NULL;;
  enum GNUNET_GenericReturnValue status = GNUNET_CHAT_context_get_status(
    context
  );

  if (GNUNET_SYSERR != status)
    user_pointer = GNUNET_CHAT_context_get_user_pointer(
      context
    );
  else
    GNUNET_CHAT_context_request(context);

  application_chat_unlock(app);

  return user_pointer;
}

static void
handle_open_chat_button_click(UNUSED GtkButton *button,
                              gpointer user_data)
{
  g_assert(user_data);

  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  if (!(handle->contact))
    return;

  UI_CHAT_ENTRY_Handle *entry = _open_direct_chat(
    handle->app, handle->contact
  );

  if ((!entry) || (!(entry->entry_box)))
    goto close_dialog;

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
    gtk_widget_get_parent(entry->entry_box)
  );

  if (!row)
    goto close_dialog;

  gtk_list_box_select_row(handle->app->ui.messenger.chats_listbox, row);
  gtk_list_box_invalidate_filter(handle->app->ui.messenger.chats_listbox);

  gtk_widget_activate(GTK_WIDGET(row));

close_dialog:
  gtk_window_close(GTK_WINDOW(handle->dialog));
}

static void
handle_back_button_click(UNUSED GtkButton *button,
                         gpointer user_data)
{
  g_assert(user_data);

  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  gtk_widget_set_visible(GTK_WIDGET(handle->back_button), FALSE);

  gtk_stack_set_visible_child(
    handle->contact_info_stack,
    handle->details_box
  );
}

static void
handle_close_button_click(UNUSED GtkButton *button,
                          gpointer user_data)
{
  g_assert(user_data);

  GtkDialog *dialog = GTK_DIALOG(user_data);
  gtk_window_close(GTK_WINDOW(dialog));
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
                      gpointer user_data)
{
  g_assert(user_data);

  ui_contact_info_dialog_cleanup((UI_CONTACT_INFO_Handle*) user_data);
}

static gboolean
handle_id_drawing_area_draw(GtkWidget* drawing_area,
                            cairo_t* cairo,
                            gpointer user_data)
{
  g_assert((drawing_area) && (cairo) && (user_data));

  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  GtkStyleContext* context = gtk_widget_get_style_context(drawing_area);

  if (!context)
    return FALSE;

  const guint width = gtk_widget_get_allocated_width(drawing_area);
  const guint height = gtk_widget_get_allocated_height(drawing_area);

  gtk_render_background(context, cairo, 0, 0, width, height);

  if ((!(handle->qr)) || (handle->qr->width <= 0))
    return FALSE;

  const guint m = 3;
  const guint w = handle->qr->width;
  const guint w2 = w + m * 2;

  guchar *pixels = (guchar*) g_malloc(sizeof(guchar) * w2 * w2 * 3);

  guint x, y, z;
  for (y = 0; y < w2; y++)
    for (x = 0; x < w2; x++)
    {
      guchar value;

      if ((x >= m) && (y >= m) && (x - m < w) && (y - m < w))
	      value  = ((handle->qr->data[(y - m) * w + x - m]) & 1);
      else
	      value = 0;

      for (z = 0; z < 3; z++)
	      pixels[(y * w2 + x) * 3 + z] = value? 0x00 : 0xff;
    }

  GdkPixbuf *image = gdk_pixbuf_new_from_data(
    pixels,
    GDK_COLORSPACE_RGB,
    FALSE,
    8,
    w2,
    w2,
    w2 * 3,
    NULL,
    NULL
  );

  if (!image)
    return FALSE;

  int dwidth = gdk_pixbuf_get_width(image);
  int dheight = gdk_pixbuf_get_height(image);

  double ratio_width = 1.0 * width / dwidth;
  double ratio_height = 1.0 * height / dheight;

  const double ratio = ratio_width < ratio_height? ratio_width : ratio_height;

  dwidth = (int) (dwidth * ratio);
  dheight = (int) (dheight * ratio);

  double dx = (width - dwidth) * 0.5;
  double dy = (height - dheight) * 0.5;

  const int interp_type = (ratio >= 1.0?
    GDK_INTERP_NEAREST :
    GDK_INTERP_BILINEAR
  );

  GdkPixbuf* scaled = gdk_pixbuf_scale_simple(
    image,
    dwidth,
    dheight,
    interp_type
  );

  gtk_render_icon(context, cairo, scaled, dx, dy);

  cairo_fill(cairo);

  g_object_unref(scaled);
  g_object_unref(image);

  g_free(pixels);

  return FALSE;
}

static enum GNUNET_GenericReturnValue
cb_contact_info_attributes(void *cls,
                           struct GNUNET_CHAT_Handle *chat,
                           const char *name,
                           const char *value)
{
  g_assert((cls) && (chat) && (name) && (value));

  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) cls;

  gtk_list_store_insert_with_values(
    handle->attributes_list,
    NULL,
    -1,
    0,
    name,
    1,
    value,
    -1
  );

  return GNUNET_YES;
}

static enum GNUNET_GenericReturnValue
cb_contact_info_contact_attributes(void *cls,
                                   struct GNUNET_CHAT_Contact *contact,
                                   const char *name,
                                   const char *value)
{
  g_assert((cls) && (contact) && (name) && (value));

  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) cls;

  GtkTreeModel *model = GTK_TREE_MODEL(handle->attributes_list);
  GtkTreeIter iter;

  GValue val_name = G_VALUE_INIT;
  GValue val_value = G_VALUE_INIT;

  gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
  gboolean match = FALSE;

  while (valid)
  {
    gtk_tree_model_get_value(model, &iter, 0, &val_name);
    gtk_tree_model_get_value(model, &iter, 1, &val_value);
    
    if (0 == strcmp(g_value_get_string(&val_name), name))
    {
      gtk_list_store_set(
        handle->attributes_list,
        &iter,
        1,
        g_value_get_string(&val_value),
        -1
      );
      
      match = TRUE;
    }

    g_value_unset(&val_name);
    g_value_unset(&val_value);

    if (match)
      break;

    valid = gtk_tree_model_iter_next(model, &iter);
  }

  if (!match)
    gtk_list_store_insert_with_values(
      handle->attributes_list,
      NULL,
      -1,
      0,
      name,
      1,
      value,
      -1
    );

  return GNUNET_YES;
}

static enum GNUNET_GenericReturnValue
cb_contact_info_unshared_attributes(void *cls,
                                    struct GNUNET_CHAT_Handle *chat,
                                    const char *name,
                                    const char *value)
{
  g_assert((cls) && (chat) && (name) && (value));

  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) cls;
  GtkTreeModel *model = GTK_TREE_MODEL(handle->sharing_list);
  GtkTreeIter iter;

  GValue val_name = G_VALUE_INIT;
  GValue val_value = G_VALUE_INIT;

  gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
  gboolean match = FALSE;

  while (valid)
  {
    gtk_tree_model_get_value(model, &iter, 0, &val_name);
    gtk_tree_model_get_value(model, &iter, 1, &val_value);
    
    if ((0 == strcmp(g_value_get_string(&val_name), name)) &&
        (0 == strcmp(g_value_get_string(&val_value), value)))
      match = TRUE;

    g_value_unset(&val_name);
    g_value_unset(&val_value);

    if (match)
      break;

    valid = gtk_tree_model_iter_next(model, &iter);
  }

  if (!match)
    gtk_list_store_insert_with_values(
      handle->sharing_list,
      NULL,
      -1,
      0,
      name,
      1,
      value,
      2,
      FALSE,
      -1
    );

  return GNUNET_YES;
}

static enum GNUNET_GenericReturnValue
cb_contact_info_shared_attributes(void *cls,
                                  struct GNUNET_CHAT_Contact *contact,
                                  const char *name,
                                  const char *value)
{
  g_assert((cls) && (contact) && (name) && (value));

  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) cls;
  GtkTreeModel *model = GTK_TREE_MODEL(handle->sharing_list);
  GtkTreeIter iter;

  GValue val_name = G_VALUE_INIT;
  GValue val_value = G_VALUE_INIT;

  gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
  gboolean match = FALSE;

  while (valid)
  {
    gtk_tree_model_get_value(model, &iter, 0, &val_name);
    gtk_tree_model_get_value(model, &iter, 1, &val_value);
    
    if ((0 == strcmp(g_value_get_string(&val_name), name)) &&
        (0 == strcmp(g_value_get_string(&val_value), value)))
      match = TRUE;

    g_value_unset(&val_name);
    g_value_unset(&val_value);

    if (match)
      break;

    valid = gtk_tree_model_iter_next(model, &iter);
  }

  if (match)
    gtk_list_store_set(handle->sharing_list, &iter, 2, TRUE, -1);
  else
    gtk_list_store_insert_with_values(
      handle->sharing_list,
      NULL,
      -1,
      0,
      name,
      1,
      value,
      2,
      TRUE,
      -1
    );

  return GNUNET_YES;
}

static enum GNUNET_GenericReturnValue
cb_contact_info_contact_tags(void *cls,
                             struct GNUNET_CHAT_Contact *contact,
                             const char *tag)
{
  g_assert((cls) && (contact) && (tag));

  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) cls;
  GtkTreeModel *model = GTK_TREE_MODEL(handle->tags_list);
  GtkTreeIter iter;

  GValue val_tag = G_VALUE_INIT;

  gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
  gboolean match = FALSE;

  while (valid)
  {
    gtk_tree_model_get_value(model, &iter, 0, &val_tag);
    
    if (0 == strcmp(g_value_get_string(&val_tag), tag))
      match = TRUE;

    g_value_unset(&val_tag);

    if (match)
      break;

    valid = gtk_tree_model_iter_next(model, &iter);
  }

  if (!match)
    gtk_list_store_insert_with_values(
      handle->tags_list,
      NULL,
      -1,
      0,
      tag,
      -1
    );

  return GNUNET_YES;
}

static void
handle_value_renderer_edit(GtkCellRendererText *renderer,
                           char *path,
                           char *new_text,
                           gpointer user_data)
{
  g_assert((renderer) && (path) && (new_text) && (user_data));

  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;
  GtkTreeIter iter;

  if (!gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(handle->attributes_list), &iter, path))
    return;

  struct GNUNET_CHAT_Handle *chat = handle->app->chat.messenger.handle;

  if (!chat)
    return;

  application_chat_lock(handle->app);
  const gboolean owned = (GNUNET_YES == GNUNET_CHAT_contact_is_owned(handle->contact));
  application_chat_unlock(handle->app);

  if ((handle->contact) && (!owned))
    return;

  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value(GTK_TREE_MODEL(handle->attributes_list), &iter, 0, &value);

  const gchar *name = g_value_get_string(&value);

  if ((new_text) && (strlen(new_text)))
  {
    application_chat_lock(handle->app);
    GNUNET_CHAT_set_attribute(chat, name, new_text);
    application_chat_unlock(handle->app);

    gtk_list_store_set(handle->attributes_list, &iter, 1, new_text, -1);
  }
  else
  {
    application_chat_lock(handle->app);
    GNUNET_CHAT_delete_attribute(chat, name);
    application_chat_unlock(handle->app);

    gtk_list_store_remove(handle->attributes_list, &iter);
  }

  g_value_unset(&value);
}

static void
handle_attribute_entry_changed(GtkEditable *editable,
                               gpointer user_data)
{
  g_assert((editable) && (user_data));

  GtkEntry *entry = GTK_ENTRY(editable);
  GtkWidget *target = GTK_WIDGET(user_data);

  const gchar *text = gtk_entry_get_text(entry);

  gtk_widget_set_sensitive(target, (text) && (strlen(text)));
}

static void
handle_add_attribute_button_click(UNUSED GtkButton *button,
                                  gpointer user_data)
{
  g_assert(user_data);

  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  struct GNUNET_CHAT_Handle *chat = handle->app->chat.messenger.handle;

  if (!chat)
    return;

  const gchar *name = gtk_entry_get_text(handle->attribute_name_entry);
  const gchar *value = gtk_entry_get_text(handle->attribute_value_entry);

  if ((name) && (value))
  {
    application_chat_lock(handle->app);
    GNUNET_CHAT_set_attribute(chat, name, value);
    application_chat_unlock(handle->app);

    gtk_list_store_insert_with_values(
      handle->attributes_list,
      NULL,
      -1,
      0,
      name,
      1,
      value,
      -1
    );
  }

  gtk_entry_set_text(handle->attribute_name_entry, "");
  gtk_entry_set_text(handle->attribute_value_entry, "");
}

static void
handle_attribute_entry_activate(UNUSED GtkEntry *entry,
                                gpointer user_data)
{
  g_assert(user_data);

  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  handle_add_attribute_button_click(handle->add_attribute_button, handle);
}

static void
handle_share_renderer_toggle(GtkCellRendererToggle *renderer,
                             char *path,
                             gpointer user_data)
{
  g_assert((renderer) && (path) && (user_data));

  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;
  GtkTreeIter iter;

  if (!gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(handle->sharing_list), &iter, path))
    return;

  struct GNUNET_CHAT_Handle *chat = handle->app->chat.messenger.handle;

  if (!chat)
    return;

  application_chat_lock(handle->app);
  const gboolean owned = (GNUNET_YES == GNUNET_CHAT_contact_is_owned(handle->contact));
  application_chat_unlock(handle->app);

  if ((!(handle->contact)) || (owned))
    return;

  GValue value_name = G_VALUE_INIT;
  GValue value_shared = G_VALUE_INIT;

  gtk_tree_model_get_value(GTK_TREE_MODEL(handle->sharing_list), &iter, 0, &value_name);
  gtk_tree_model_get_value(GTK_TREE_MODEL(handle->sharing_list), &iter, 2, &value_shared);

  const gchar *name = g_value_get_string(&value_name);
  const gboolean shared = g_value_get_boolean(&value_shared);

  application_chat_lock(handle->app);

  if (shared)
    GNUNET_CHAT_unshare_attribute_from(chat, handle->contact, name);
  else
    GNUNET_CHAT_share_attribute_with(chat, handle->contact, name);

  application_chat_unlock(handle->app);

  gtk_list_store_set(handle->sharing_list, &iter, 2, !shared, -1);

  g_value_unset(&value_name);
  g_value_unset(&value_shared);
}

static void
handle_tag_tree_selection_changed(GtkTreeSelection *selection,
                                  gpointer user_data)
{
  g_assert((selection) && (user_data));

  GtkWidget *widget = GTK_WIDGET(user_data);

  const gboolean selected = gtk_tree_selection_get_selected(
    selection, NULL, NULL);

  gtk_widget_set_sensitive(widget, selected);
}

static void
handle_tag_entry_changed(GtkEditable *editable,
                         gpointer user_data)
{
  g_assert((editable) && (user_data));

  GtkEntry *entry = GTK_ENTRY(editable);
  GtkWidget *target = GTK_WIDGET(user_data);

  const gchar *text = gtk_entry_get_text(entry);

  gtk_widget_set_sensitive(target, (text) && (strlen(text)));
}

static void
handle_add_tag_button_click(UNUSED GtkButton *button,
                            gpointer user_data)
{
  g_assert(user_data);

  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  struct GNUNET_CHAT_Handle *chat = handle->app->chat.messenger.handle;

  if (!chat)
    return;

  const gchar *tag = gtk_entry_get_text(handle->tag_name_entry);

  if (tag)
  {
    application_chat_lock(handle->app);
    GNUNET_CHAT_contact_tag(handle->contact, tag);
    application_chat_unlock(handle->app);

    gtk_list_store_insert_with_values(
      handle->tags_list,
      NULL,
      -1,
      0,
      tag,
      -1
    );
  }

  gtk_entry_set_text(handle->tag_name_entry, "");
}

static void
handle_remove_tag_button_click(UNUSED GtkButton *button,
                               gpointer user_data)
{
  g_assert(user_data);

  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;
  GtkTreeIter iter;

  if (!gtk_tree_selection_get_selected(handle->tags_tree_selection, NULL, &iter))
    return;

  struct GNUNET_CHAT_Handle *chat = handle->app->chat.messenger.handle;

  if (!chat)
    return;

  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value(GTK_TREE_MODEL(handle->tags_list), &iter, 0, &value);

  const gchar *tag = g_value_get_string(&value);

  if (tag)
  {
    application_chat_lock(handle->app);
    GNUNET_CHAT_contact_untag(handle->contact, tag);
    application_chat_unlock(handle->app);

    gtk_list_store_remove(
      handle->tags_list,
      &iter
    );
  }
}

static void
handle_tag_entry_activate(UNUSED GtkEntry *entry,
                          gpointer user_data)
{
  g_assert(user_data);

  UI_CONTACT_INFO_Handle *handle = (UI_CONTACT_INFO_Handle*) user_data;

  handle_add_tag_button_click(handle->add_tag_button, handle);
}

void
ui_contact_info_dialog_init(MESSENGER_Application *app,
                            UI_CONTACT_INFO_Handle *handle)
{
  g_assert((app) && (handle));

  handle->app = app;

  handle->account = NULL;
  handle->contact = NULL;

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/contact_info.ui")
  );

  handle->dialog = GTK_DIALOG(
    gtk_builder_get_object(handle->builder, "contact_info_dialog")
  );

  gtk_window_set_transient_for(
    GTK_WINDOW(handle->dialog),
    GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->contact_info_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "contact_info_stack")
  );

  handle->details_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "details_box")
  );

  handle->contact_avatar = HDY_AVATAR(
    gtk_builder_get_object(handle->builder, "contact_avatar")
  );

  handle->contact_name_entry = GTK_ENTRY(
    gtk_builder_get_object(handle->builder, "contact_name")
  );

  handle->contact_edit_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "contact_edit_button")
  );

  handle->contact_edit_symbol = GTK_IMAGE(
    gtk_builder_get_object(handle->builder, "contact_edit_symbol")
  );

  g_signal_connect(
    handle->contact_name_entry,
    "activate",
    G_CALLBACK(handle_contact_name_entry_activate),
    handle
  );

  g_signal_connect(
    handle->contact_edit_button,
    "clicked",
    G_CALLBACK(handle_contact_edit_button_click),
    handle
  );

  handle->profile_chooser_button = GTK_FILE_CHOOSER_BUTTON(
    gtk_builder_get_object(handle->builder, "profile_chooser_button")
  );

  g_signal_connect(
    handle->profile_chooser_button,
    "update-preview",
    G_CALLBACK(handle_profile_chooser_update_preview),
    handle->contact_avatar
  );

  g_signal_connect(
    handle->profile_chooser_button,
    "file-set",
    G_CALLBACK(handle_profile_chooser_file_set),
    handle
  );

  handle->reveal_identity_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "reveal_identity_button")
  );

  g_signal_connect(
    handle->reveal_identity_button,
    "clicked",
    G_CALLBACK(handle_reveal_identity_button_click),
    handle
  );

  handle->list_attributes_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "list_attributes_button")
  );

  handle->share_attributes_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "share_attributes_button")
  );

  g_signal_connect(
    handle->list_attributes_button,
    "clicked",
    G_CALLBACK(handle_list_attributes_button_click),
    handle
  );

  g_signal_connect(
    handle->share_attributes_button,
    "clicked",
    G_CALLBACK(handle_share_attributes_button_click),
    handle
  );

  handle->list_tags_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "list_tags_button")
  );

  g_signal_connect(
    handle->list_tags_button,
    "clicked",
    G_CALLBACK(handle_list_tags_button_click),
    handle
  );

  handle->block_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "block_stack")
  );

  handle->block_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "block_button")
  );

  g_signal_connect(
    handle->block_button,
    "clicked",
    G_CALLBACK(handle_block_button_click),
    handle
  );

  handle->unblock_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "unblock_button")
  );

  g_signal_connect(
    handle->unblock_button,
    "clicked",
    G_CALLBACK(handle_unblock_button_click),
    handle
  );

  handle->open_chat_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "open_chat_button")
  );

  g_signal_connect(
    handle->open_chat_button,
    "clicked",
    G_CALLBACK(handle_open_chat_button_click),
    handle
  );

  handle->identity_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "identity_box")
  );

  handle->name_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "name_label")
  );

  handle->id_drawing_area = GTK_DRAWING_AREA(
    gtk_builder_get_object(handle->builder, "id_drawing_area")
  );

  handle->id_draw_signal = g_signal_connect(
    handle->id_drawing_area,
    "draw",
    G_CALLBACK(handle_id_drawing_area_draw),
    handle
  );

  handle->id_entry = GTK_ENTRY(
    gtk_builder_get_object(handle->builder, "id_entry")
  );

  handle->attributes_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "attributes_box")
  );

  handle->attributes_tree = GTK_TREE_VIEW(
    gtk_builder_get_object(handle->builder, "attributes_tree")
  );

  handle->attributes_list = GTK_LIST_STORE(
    gtk_builder_get_object(handle->builder, "attributes_list")
  );

  handle->value_renderer = GTK_CELL_RENDERER_TEXT(
    gtk_builder_get_object(handle->builder, "value_renderer")
  );

  g_signal_connect(
    handle->value_renderer,
    "edited",
    G_CALLBACK(handle_value_renderer_edit),
    handle
  );

  handle->new_attribute_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "new_attribute_box")
  );

  handle->attribute_name_entry = GTK_ENTRY(
    gtk_builder_get_object(handle->builder, "attribute_name_entry")
  );

  handle->attribute_value_entry = GTK_ENTRY(
    gtk_builder_get_object(handle->builder, "attribute_value_entry")
  );

  handle->add_attribute_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "add_attribute_button")
  );

  g_signal_connect(
    handle->attribute_name_entry,
    "changed",
    G_CALLBACK(handle_attribute_entry_changed),
    handle->attribute_value_entry
  );

  g_signal_connect(
    handle->attribute_value_entry,
    "changed",
    G_CALLBACK(handle_attribute_entry_changed),
    handle->add_attribute_button
  );

  g_signal_connect(
    handle->attribute_value_entry,
    "activate",
    G_CALLBACK(handle_attribute_entry_activate),
    handle
  );

  g_signal_connect(
    handle->add_attribute_button,
    "clicked",
    G_CALLBACK(handle_add_attribute_button_click),
    handle
  );

  handle->sharing_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "sharing_box")
  );

  handle->sharing_tree = GTK_TREE_VIEW(
    gtk_builder_get_object(handle->builder, "sharing_tree")
  );

  handle->sharing_list = GTK_LIST_STORE(
    gtk_builder_get_object(handle->builder, "sharing_list")
  );

  handle->share_renderer = GTK_CELL_RENDERER_TOGGLE(
    gtk_builder_get_object(handle->builder, "share_renderer")
  );

  g_signal_connect(
    handle->share_renderer,
    "toggled",
    G_CALLBACK(handle_share_renderer_toggle),
    handle
  );

  handle->tags_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "tags_box")
  );

  handle->tags_tree = GTK_TREE_VIEW(
    gtk_builder_get_object(handle->builder, "tags_tree")
  );

  handle->tags_tree_selection = GTK_TREE_SELECTION(
    gtk_builder_get_object(handle->builder, "tags_tree_selection")
  );

  handle->tags_list = GTK_LIST_STORE(
    gtk_builder_get_object(handle->builder, "tags_list")
  );

  handle->new_tag_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "new_tag_box")
  );

  handle->tag_name_entry = GTK_ENTRY(
    gtk_builder_get_object(handle->builder, "tag_name_entry")
  );

  handle->add_tag_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "add_tag_button")
  );

  handle->remove_tag_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "remove_tag_button")
  );

  g_signal_connect(
    handle->tags_tree_selection,
    "changed",
    G_CALLBACK(handle_tag_tree_selection_changed),
    handle->remove_tag_button
  );

  g_signal_connect(
    handle->tag_name_entry,
    "changed",
    G_CALLBACK(handle_tag_entry_changed),
    handle->add_tag_button
  );

  g_signal_connect(
    handle->tag_name_entry,
    "activate",
    G_CALLBACK(handle_tag_entry_activate),
    handle
  );

  g_signal_connect(
    handle->add_tag_button,
    "clicked",
    G_CALLBACK(handle_add_tag_button_click),
    handle
  );

  g_signal_connect(
    handle->remove_tag_button,
    "clicked",
    G_CALLBACK(handle_remove_tag_button_click),
    handle
  );

  handle->open_chat_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "open_chat_stack")
  );

  handle->back_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "back_button")
  );

  g_signal_connect(
    handle->back_button,
    "clicked",
    G_CALLBACK(handle_back_button_click),
    handle
  );

  handle->close_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "close_button")
  );

  g_signal_connect(
    handle->close_button,
    "clicked",
    G_CALLBACK(handle_close_button_click),
    handle->dialog
  );

  g_signal_connect(
    handle->dialog,
    "destroy",
    G_CALLBACK(handle_dialog_destroy),
    handle
  );
}

static void
_contact_info_update(UI_CONTACT_INFO_Handle *handle,
                     struct GNUNET_CHAT_Contact *contact)
{
  g_assert(handle);

  if (handle->contact)
    contact_remove_name_avatar_from_info(handle->contact, handle->contact_avatar);
  if (contact)
    contact_add_name_avatar_to_info(contact, handle->contact_avatar);

  handle->contact = contact;
}

static void
_account_info_update(UI_CONTACT_INFO_Handle *handle,
                     struct GNUNET_CHAT_Account *account)
{
  g_assert(handle);

  if (handle->account)
    account_remove_name_avatar_from_info(handle->account, handle->contact_avatar);
  if (account)
    account_add_name_avatar_to_info(account, handle->contact_avatar);

  handle->account = account;
}

void
ui_contact_info_dialog_update(UI_CONTACT_INFO_Handle *handle,
                              struct GNUNET_CHAT_Contact *contact,
                              gboolean reveal)
{
  g_assert(handle);

  if (!contact)
    contact = GNUNET_CHAT_get_own_contact(handle->app->chat.messenger.handle);

  const char *name = NULL;
  const char *key = NULL;

  if (contact)
    name = GNUNET_CHAT_contact_get_name(contact);
  else
    name = GNUNET_CHAT_get_name(handle->app->chat.messenger.handle);

  if (contact)
    _contact_info_update(handle, contact);
  else
  {
    struct GNUNET_CHAT_Account *account = GNUNET_CHAT_get_connected(
      handle->app->chat.messenger.handle
    );

    _account_info_update(handle, account);
  }

  ui_entry_set_text(handle->contact_name_entry, name);

  const gboolean editable = (
    (!contact) ||
    (GNUNET_YES == GNUNET_CHAT_contact_is_owned(contact))
  );

  GValue value = G_VALUE_INIT;
  g_value_init(&value, G_TYPE_BOOLEAN);
  g_value_set_boolean(&value, editable);

  g_object_set_property(
    G_OBJECT(handle->value_renderer),
    "editable",
    &value
  );

  g_value_unset(&value);

  gtk_widget_set_visible(
    handle->new_attribute_box, 
    editable
  );

  if (contact)
    key = GNUNET_CHAT_contact_get_key(contact);
  else
    key = GNUNET_CHAT_get_key(handle->app->chat.messenger.handle);

  if (handle->qr)
    QRcode_free(handle->qr);

  if (key)
    handle->qr = QRcode_encodeString(
      key,
      0,
      QR_ECLEVEL_L,
      QR_MODE_8,
      0
    );
  else
    handle->qr = NULL;

  ui_label_set_text(handle->name_label, name);

  if (handle->id_drawing_area)
    gtk_widget_queue_draw(GTK_WIDGET(handle->id_drawing_area));

  ui_entry_set_text(handle->id_entry, key);

  gtk_widget_set_sensitive(
    GTK_WIDGET(handle->profile_chooser_button),
    editable
  );

  gtk_widget_set_sensitive(
    GTK_WIDGET(handle->reveal_identity_button),
    key? TRUE : FALSE
  );

  gtk_widget_set_sensitive(
    GTK_WIDGET(handle->share_attributes_button),
    !editable
  );

  gtk_widget_set_sensitive(
    GTK_WIDGET(handle->list_tags_button),
    !editable
  );

  gtk_widget_set_sensitive(
    GTK_WIDGET(handle->tag_name_entry),
    !editable
  );

  gtk_widget_set_sensitive(
    GTK_WIDGET(handle->block_button),
    !editable
  );

  gtk_widget_set_sensitive(
    GTK_WIDGET(handle->unblock_button),
    !editable
  );

  gtk_widget_set_visible(
    GTK_WIDGET(handle->profile_chooser_button),
    editable
  );

  gtk_widget_set_visible(
    GTK_WIDGET(handle->share_attributes_button),
    !editable
  );

  gtk_widget_set_visible(
    GTK_WIDGET(handle->list_tags_button),
    !editable
  );

  gtk_widget_set_visible(
    GTK_WIDGET(handle->new_tag_box),
    !editable
  );

  gtk_stack_set_visible_child(
    handle->block_stack,
    GNUNET_YES == GNUNET_CHAT_contact_is_blocked(contact)?
    GTK_WIDGET(handle->unblock_button) :
    GTK_WIDGET(handle->block_button)
  );

  gtk_widget_set_visible(
    GTK_WIDGET(handle->block_stack),
    !editable
  );

  gtk_list_store_clear(handle->attributes_list);
  gtk_list_store_clear(handle->sharing_list);
  gtk_list_store_clear(handle->tags_list);

  if (editable)
    GNUNET_CHAT_get_attributes(
      handle->app->chat.messenger.handle,
      cb_contact_info_attributes,
      handle
    );
  else
  {
    GNUNET_CHAT_contact_get_attributes(
      contact,
      cb_contact_info_contact_attributes,
      handle
    );

    GNUNET_CHAT_get_attributes(
      handle->app->chat.messenger.handle,
      cb_contact_info_unshared_attributes,
      handle
    );

    GNUNET_CHAT_get_shared_attributes(
      handle->app->chat.messenger.handle,
      contact,
      cb_contact_info_shared_attributes,
      handle
    );

    GNUNET_CHAT_contact_iterate_tags(
      contact,
      cb_contact_info_contact_tags,
      handle
    );
  }

  gtk_stack_set_visible_child_name(
    handle->open_chat_stack,
    editable? "open_notes_page" : "open_private_chat_page"
  );

  struct GNUNET_CHAT_Context *context = GNUNET_CHAT_contact_get_context(
    contact
  );

  gtk_widget_set_sensitive(
    GTK_WIDGET(handle->open_chat_button),
    context? TRUE : FALSE
  );

  gtk_widget_set_visible(
    GTK_WIDGET(handle->open_chat_button),
    context? TRUE : FALSE
  );

  if (reveal)
    _contact_info_switch_stack_to(handle, handle->identity_box);
}

void
ui_contact_info_dialog_cleanup(UI_CONTACT_INFO_Handle *handle)
{
  g_assert(handle);

  g_signal_handler_disconnect(
    handle->id_drawing_area,
    handle->id_draw_signal
  );

  _account_info_update(handle, NULL);
  _contact_info_update(handle, NULL);

  g_object_unref(handle->builder);

  if (handle->qr)
    QRcode_free(handle->qr);

  memset(handle, 0, sizeof(*handle));
}
