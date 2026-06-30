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
 * @file ui/send_file.c
 */

#include "send_file.h"

#include "chat_entry.h"
#include "chat_title.h"
#include "file_load_entry.h"

#include "../application.h"
#include "../file.h"
#include "../ui.h"

static void
handle_cancel_button_click(UNUSED GtkButton *button,
                           gpointer user_data)
{
  g_assert(user_data);

  GtkDialog *dialog = GTK_DIALOG(user_data);
  gtk_window_close(GTK_WINDOW(dialog));
}

static void
handle_sending_upload_file(UNUSED void *cls,
                           struct GNUNET_CHAT_File *file,
                           uint64_t completed,
                           uint64_t size)
{
  g_assert(file);

  UI_FILE_LOAD_ENTRY_Handle *file_load = cls;

  gtk_progress_bar_set_fraction(
      file_load->load_progress_bar,
      1.0 * completed / size
  );

  file_update_upload_info(file, completed, size);

  if ((completed >= size) && (file_load->chat_title))
    ui_chat_title_remove_file_load(file_load->chat_title, file_load);
}

static void
handle_send_button_click(GtkButton *button,
                         gpointer user_data)
{
  g_assert((button) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  GtkTextView *text_view = GTK_TEXT_VIEW(
    g_object_get_qdata(G_OBJECT(button), app->quarks.widget)
  );

  if (!text_view)
    return;

  gchar *filename = gtk_file_chooser_get_filename(
    GTK_FILE_CHOOSER(app->ui.send_file.file_chooser_button)
  );

  if (!filename)
    return;

  struct GNUNET_CHAT_Context *context = (struct GNUNET_CHAT_Context*) (
    g_object_get_qdata(G_OBJECT(text_view), app->quarks.data)
  );

  UI_CHAT_ENTRY_Handle *entry = GNUNET_CHAT_context_get_user_pointer(context);
  UI_CHAT_Handle *handle = entry? entry->chat : NULL;

  UI_FILE_LOAD_ENTRY_Handle *file_load = NULL;
  struct GNUNET_CHAT_File *file = NULL;

  if ((context) && (handle))
  {
    file_load = ui_file_load_entry_new(app);

    gtk_label_set_text(file_load->file_label, filename);
    gtk_progress_bar_set_fraction(file_load->load_progress_bar, 0.0);

    application_chat_lock(app);

    file = GNUNET_CHAT_context_send_file(
      context,
      filename,
      handle_sending_upload_file,
      file_load
    );

    application_chat_unlock(app);
  }

  g_free(filename);

  gtk_window_close(GTK_WINDOW(app->ui.send_file.dialog));

  if (!file)
  {
    if (file_load)
      ui_file_load_entry_delete(file_load);

    return;
  }

  file_create_info(file);

  ui_chat_title_add_file_load(handle->title, file_load);
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
                      gpointer user_data)
{
  g_assert(user_data);

  ui_send_file_dialog_cleanup((UI_SEND_FILE_Handle*) user_data);
}

static int
handle_file_redraw_animation(gpointer user_data)
{
  g_assert(user_data);

  UI_SEND_FILE_Handle *handle = (UI_SEND_FILE_Handle*) user_data;

  handle->redraw_animation = 0;

  if ((handle->file_drawing_area) &&
      ((handle->image) || (handle->animation) || (handle->animation_iter)))
    gtk_widget_queue_draw(GTK_WIDGET(handle->file_drawing_area));

  return FALSE;
}

static gboolean
handle_file_drawing_area_draw(GtkWidget* drawing_area,
                              cairo_t* cairo,
                              gpointer user_data)
{
  g_assert((drawing_area) && (cairo) && (user_data));

  UI_SEND_FILE_Handle *handle = (UI_SEND_FILE_Handle*) user_data;

  GtkStyleContext* context = gtk_widget_get_style_context(drawing_area);

  const guint width = gtk_widget_get_allocated_width(drawing_area);
  const guint height = gtk_widget_get_allocated_height(drawing_area);

  gtk_render_background(context, cairo, 0, 0, width, height);

  GdkPixbuf *image = handle->image;

  if (!(handle->animation))
    goto render_image;

  if (handle->animation_iter)
    gdk_pixbuf_animation_iter_advance(handle->animation_iter, NULL);
  else
    handle->animation_iter = gdk_pixbuf_animation_get_iter(
	    handle->animation, NULL
    );

  image = gdk_pixbuf_animation_iter_get_pixbuf(handle->animation_iter);

  const int delay = gdk_pixbuf_animation_iter_get_delay_time(
    handle->animation_iter
  );

  handle->redraw_animation = util_timeout_add(
    delay,
    handle_file_redraw_animation,
    handle
  );

render_image:
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
  return FALSE;
}

static void
_clear_file_preview_data(UI_SEND_FILE_Handle *handle)
{
  g_assert(handle);

  if (handle->image)
  {
    g_object_unref(handle->image);
    handle->image = NULL;
  }

  if (handle->redraw_animation)
  {
    util_source_remove(handle->redraw_animation);
    handle->redraw_animation = 0;
  }

  if (handle->animation_iter)
  {
    g_object_unref(handle->animation_iter);
    handle->animation_iter = NULL;
  }

  if (handle->animation)
  {
    g_object_unref(handle->animation);
    handle->animation = NULL;
  }
}

static void
handle_file_chooser_button_file_set(GtkFileChooserButton *file_chooser_button,
                                    gpointer user_data)
{
  g_assert((file_chooser_button) && (user_data));

  UI_SEND_FILE_Handle *handle = (UI_SEND_FILE_Handle*) user_data;

  _clear_file_preview_data(handle);

  char *filename = gtk_file_chooser_get_filename(
      GTK_FILE_CHOOSER(file_chooser_button)
  );

  if (filename)
  {
    handle->animation = gdk_pixbuf_animation_new_from_file(filename, NULL);

    if (!(handle->animation))
      handle->image = gdk_pixbuf_new_from_file(filename, NULL);

    g_free(filename);
  }

  if (handle->file_drawing_area)
    gtk_widget_queue_draw(GTK_WIDGET(handle->file_drawing_area));
}

void
ui_send_file_dialog_init(MESSENGER_Application *app,
                         UI_SEND_FILE_Handle *handle)
{
  g_assert((app) && (handle));

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/send_file.ui")
  );

  handle->dialog = GTK_DIALOG(
    gtk_builder_get_object(handle->builder, "send_file_dialog")
  );

  gtk_window_set_title(
    GTK_WINDOW(handle->dialog),
    _("Send File")
  );

  gtk_window_set_transient_for(
    GTK_WINDOW(handle->dialog),
    GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->file_drawing_area = GTK_DRAWING_AREA(
    gtk_builder_get_object(handle->builder, "file_drawing_area")
  );

  handle->file_chooser_button = GTK_FILE_CHOOSER_BUTTON(
    gtk_builder_get_object(handle->builder, "file_chooser_button")
  );

  handle->file_draw_signal = g_signal_connect(
    handle->file_drawing_area,
    "draw",
    G_CALLBACK(handle_file_drawing_area_draw),
    handle
  );

  g_signal_connect(
    handle->file_chooser_button,
    "file-set",
    G_CALLBACK(handle_file_chooser_button_file_set),
    handle
  );

  handle->cancel_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "cancel_button")
  );

  g_signal_connect(
    handle->cancel_button,
    "clicked",
    G_CALLBACK(handle_cancel_button_click),
    handle->dialog
  );

  handle->send_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "send_button")
  );

  g_signal_connect(
    handle->send_button,
    "clicked",
    G_CALLBACK(handle_send_button_click),
    app
  );

  g_signal_connect(
    handle->dialog,
    "destroy",
    G_CALLBACK(handle_dialog_destroy),
    handle
  );

  handle->image = NULL;
  handle->animation = NULL;
  handle->animation_iter = NULL;

  handle->redraw_animation = 0;
}

void
ui_send_file_dialog_update(UI_SEND_FILE_Handle *handle,
                           const gchar *filename)
{
  g_assert((handle) && (filename));

  if (!(handle->file_chooser_button))
    return;

  gtk_file_chooser_set_filename(
    GTK_FILE_CHOOSER(handle->file_chooser_button),
    filename
  );

  handle_file_chooser_button_file_set(
    handle->file_chooser_button,
    handle
  );
}

void
ui_send_file_dialog_cleanup(UI_SEND_FILE_Handle *handle)
{
  g_assert(handle);

  _clear_file_preview_data(handle);

  g_signal_handler_disconnect(
      handle->file_drawing_area,
      handle->file_draw_signal
  );

  g_object_unref(handle->builder);

  memset(handle, 0, sizeof(*handle));
}
