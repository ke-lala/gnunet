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
 * @file file.c
 */

#include "file.h"
#include <gnunet/gnunet_chat_lib.h>

void
file_create_info(struct GNUNET_CHAT_File *file)
{
  if ((!file) || (GNUNET_CHAT_file_get_user_pointer(file)))
    return;

  MESSENGER_FileInfo* info = g_malloc(sizeof(MESSENGER_FileInfo));

  info->app = NULL;

  info->update_task = 0;
  info->file_messages = NULL;

  info->preview_image = NULL;
  info->preview_animation = NULL;
  info->preview_animation_iter = NULL;

  info->redraw_animation_task = 0;
  info->preview_widgets = NULL;

  GNUNET_CHAT_file_set_user_pointer(file, info);
}

void
file_destroy_info(struct GNUNET_CHAT_File *file)
{
  g_assert(file);

  MESSENGER_FileInfo* info = GNUNET_CHAT_file_get_user_pointer(file);

  if (!info)
    return;

  if (info->preview_widgets)
    g_list_free(info->preview_widgets);

  file_unload_preview_image(file);

  if (info->update_task)
    util_source_remove(info->update_task);

  if (info->file_messages)
    g_list_free(info->file_messages);

  g_free(info);

  GNUNET_CHAT_file_set_user_pointer(file, NULL);
}

void
file_add_ui_message_to_info(const struct GNUNET_CHAT_File *file,
                            UI_MESSAGE_Handle *message)
{
  g_assert(message);

  MESSENGER_FileInfo* info = GNUNET_CHAT_file_get_user_pointer(file);

  if (!info)
   return;

  info->file_messages = g_list_append(info->file_messages, message);
}

void
file_add_widget_to_preview(const struct GNUNET_CHAT_File *file,
                           GtkWidget *widget)
{
  g_assert(widget);

  MESSENGER_FileInfo* info = GNUNET_CHAT_file_get_user_pointer(file);

  if (!info)
    return;

  info->preview_widgets = g_list_append(info->preview_widgets, widget);

  if ((info->preview_image) ||
      (info->preview_animation) ||
      (info->preview_animation_iter))
    gtk_widget_queue_draw(widget);
}

void
file_remove_widget_from_preview(const struct GNUNET_CHAT_File *file,
                                GtkWidget *widget)
{
  g_assert(widget);

  MESSENGER_FileInfo* info = GNUNET_CHAT_file_get_user_pointer(file);

  if (!info)
    return;

  if (info->preview_widgets)
    info->preview_widgets = g_list_remove(info->preview_widgets, widget);

  if (!(info->preview_widgets))
    file_unload_preview_image(file);
}

void
file_update_upload_info(const struct GNUNET_CHAT_File *file,
                        uint64_t completed,
                        uint64_t size)
{
  MESSENGER_FileInfo* info = GNUNET_CHAT_file_get_user_pointer(file);

  if (!info)
    return;

  GList *list = info->file_messages;

  while (list)
  {
    UI_MESSAGE_Handle *message = (UI_MESSAGE_Handle*) list->data;

    gtk_progress_bar_set_fraction(
      message->file_progress_bar,
      1.0 * completed / size
    );

    list = list->next;
  }
}

static gboolean
file_update_messages(gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_FileInfo* info = (MESSENGER_FileInfo*) user_data;

  info->update_task = 0;

  GList *list = info->file_messages;

  while (list)
  {
    UI_MESSAGE_Handle *message = (UI_MESSAGE_Handle*) list->data;

    ui_message_update(message, info->app, message->msg);

    list = list->next;
  }

  return FALSE;
}

void
file_update_download_info(const struct GNUNET_CHAT_File *file,
                          MESSENGER_Application *app,
                          uint64_t completed,
                          uint64_t size)
{
  MESSENGER_FileInfo* info = GNUNET_CHAT_file_get_user_pointer(file);

  if (!info)
    return;

  GList *list = info->file_messages;

  while (list)
  {
    UI_MESSAGE_Handle *message = (UI_MESSAGE_Handle*) list->data;

    gtk_progress_bar_set_fraction(
      message->file_progress_bar,
      1.0 * completed / size
    );

    list = list->next;
  }

  if ((completed < size) || (info->update_task))
    return;

  info->app = app;
  info->update_task = util_idle_add(file_update_messages, info);
}

static void
file_draw_preview(MESSENGER_FileInfo* info)
{
  g_assert(info);

  GList *list = info->preview_widgets;

  while (list)
  {
    if (!GTK_IS_WIDGET(list->data))
      goto skip_data;

    GtkWidget *widget = GTK_WIDGET(list->data);
    gtk_widget_queue_draw(widget);

  skip_data:
    list = list->next;
  }
}

void
file_load_preview_image(struct GNUNET_CHAT_File *file)
{
  MESSENGER_FileInfo* info = GNUNET_CHAT_file_get_user_pointer(file);

  if (!info)
    return;

  const char *preview = GNUNET_CHAT_file_open_preview(file);

  if (!preview)
    return;

  file_unload_preview_image(file);

  info->preview_animation = gdk_pixbuf_animation_new_from_file(
    preview, NULL
  );

  if (!(info->preview_animation))
    info->preview_image = gdk_pixbuf_new_from_file(preview, NULL);

  GNUNET_CHAT_file_close_preview(file);

  if (info->preview_widgets)
    file_draw_preview(info);
}

void
file_unload_preview_image(const struct GNUNET_CHAT_File *file)
{
  MESSENGER_FileInfo* info = GNUNET_CHAT_file_get_user_pointer(file);

  if (!info)
    return;

  if (info->preview_image)
  {
    g_object_unref(info->preview_image);
    info->preview_image = NULL;
  }

  if (info->redraw_animation_task)
  {
    util_source_remove(info->redraw_animation_task);
    info->redraw_animation_task = 0;
  }

  if (info->preview_animation_iter)
  {
    g_object_unref(info->preview_animation_iter);
    info->preview_animation_iter = NULL;
  }

  if (info->preview_animation)
  {
    g_object_unref(info->preview_animation);
    info->preview_animation = NULL;
  }
}

static gboolean
file_redraw_animation(gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_FileInfo* info = (MESSENGER_FileInfo*) user_data;

  info->redraw_animation_task = 0;

  file_draw_preview(info);

  return FALSE;
}

GdkPixbuf*
file_get_current_preview_image(const struct GNUNET_CHAT_File *file)
{
  MESSENGER_FileInfo* info = GNUNET_CHAT_file_get_user_pointer(file);

  if (!info)
    return NULL;

  GdkPixbuf *image = info->preview_image;

  if (!(info->preview_animation))
    return image;

  if (info->preview_animation_iter)
    gdk_pixbuf_animation_iter_advance(info->preview_animation_iter, NULL);
  else
    info->preview_animation_iter = gdk_pixbuf_animation_get_iter(
	    info->preview_animation, NULL
    );

  image = gdk_pixbuf_animation_iter_get_pixbuf(info->preview_animation_iter);

  if (!(info->redraw_animation_task))
  {
    const int delay = gdk_pixbuf_animation_iter_get_delay_time(
      info->preview_animation_iter
    );

    info->redraw_animation_task = util_timeout_add(
      delay, file_redraw_animation, info
    );
  }

  return image;
}
