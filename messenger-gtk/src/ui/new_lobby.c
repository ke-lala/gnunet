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
 * @file ui/new_lobby.c
 */

#include "new_lobby.h"

#include "../application.h"
#include "../ui.h"

static void
handle_warning_info_bar_close(GtkInfoBar *info_bar,
                              UNUSED gpointer user_data)
{
  g_assert(info_bar);

  gtk_info_bar_set_revealed(info_bar, FALSE);
}

static void
handle_warning_info_bar_response(GtkInfoBar *info_bar,
                                 UNUSED int response_id,
                                 UNUSED gpointer user_data)
{
  g_assert(info_bar);

  gtk_info_bar_set_revealed(info_bar, FALSE);
}

static void
handle_cancel_button_click(UNUSED GtkButton *button,
                           gpointer user_data)
{
  g_assert(user_data);

  GtkDialog *dialog = GTK_DIALOG(user_data);
  gtk_window_close(GTK_WINDOW(dialog));
}

void
handle_lobby_opened_and_uri_generated(void *cls,
                                      const struct GNUNET_CHAT_Uri *uri)
{
  g_assert(cls);

  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  if (app->ui.new_lobby.qr)
    QRcode_free(app->ui.new_lobby.qr);

  if (!uri)
  {
    if (app->ui.new_lobby.preview_stack)
      gtk_stack_set_visible_child(
        app->ui.new_lobby.preview_stack,
        app->ui.new_lobby.fail_box
      );

    app->ui.new_lobby.qr = NULL;
    return;
  }

  gchar *uri_string = GNUNET_CHAT_uri_to_string(uri);

  app->ui.new_lobby.qr = QRcode_encodeString(
    uri_string,
    0,
    QR_ECLEVEL_L,
    QR_MODE_8,
    0
  );

  if (app->ui.new_lobby.id_drawing_area)
    gtk_widget_queue_draw(GTK_WIDGET(app->ui.new_lobby.id_drawing_area));

  if (app->ui.new_lobby.preview_stack)
    gtk_stack_set_visible_child(
      app->ui.new_lobby.preview_stack,
      GTK_WIDGET(app->ui.new_lobby.id_drawing_area)
    );

  if (app->ui.new_lobby.id_entry)
    gtk_entry_set_text(app->ui.new_lobby.id_entry, uri_string);

  GNUNET_free(uri_string);

  if (!(app->ui.new_lobby.id_entry))
    return;

  const gint id_length = gtk_entry_get_text_length(app->ui.new_lobby.id_entry);

  gtk_widget_set_sensitive(
    GTK_WIDGET(app->ui.new_lobby.copy_button),
    id_length > 0? TRUE : FALSE
  );
}

static void
handle_generate_button_click(UNUSED GtkButton *button,
                             gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  GtkTreeModel *model = gtk_combo_box_get_model(
    app->ui.new_lobby.expiration_combo_box
  );

  gulong delay = 0;

  GtkTreeIter iter;
  if (gtk_combo_box_get_active_iter(app->ui.new_lobby.expiration_combo_box, &iter))
    gtk_tree_model_get(model, &iter, 1, &delay, -1);

  gtk_stack_set_visible_child(
    app->ui.new_lobby.preview_stack,
    GTK_WIDGET(app->ui.new_lobby.loading_spinner)
  );

  gtk_stack_set_visible_child(
    app->ui.new_lobby.stack,
    app->ui.new_lobby.copy_box
  );

  gtk_widget_set_sensitive(GTK_WIDGET(app->ui.new_lobby.copy_button), FALSE);

  gtk_widget_set_visible(GTK_WIDGET(app->ui.new_lobby.generate_button), FALSE);
  gtk_widget_set_visible(GTK_WIDGET(app->ui.new_lobby.copy_button), TRUE);

  application_chat_lock(app);
  GNUNET_CHAT_lobby_open(
    app->chat.messenger.handle,
    delay,
    handle_lobby_opened_and_uri_generated,
    app
  );
  application_chat_unlock(app);
}

static void
handle_copy_button_click(UNUSED GtkButton *button,
                         gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  const gint id_length = gtk_entry_get_text_length(app->ui.new_lobby.id_entry);
  const gchar *id_text = gtk_entry_get_text(app->ui.new_lobby.id_entry);

  GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

  if ((clipboard) && (id_length > 0))
    gtk_clipboard_set_text(clipboard, id_text, id_length);

  gtk_window_close(GTK_WINDOW(app->ui.new_lobby.dialog));
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
                      gpointer user_data)
{
  g_assert(user_data);

  ui_new_lobby_dialog_cleanup((UI_NEW_LOBBY_Handle*) user_data);
}

static gboolean
handle_id_drawing_area_draw(GtkWidget* drawing_area,
                            cairo_t* cairo,
                            gpointer user_data)
{
  g_assert((drawing_area) && (cairo) && (user_data));

  UI_NEW_LOBBY_Handle *handle = (UI_NEW_LOBBY_Handle*) user_data;

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

void
ui_new_lobby_dialog_init(MESSENGER_Application *app,
                         UI_NEW_LOBBY_Handle *handle)
{
  g_assert((app) && (handle));

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/new_lobby.ui")
  );

  handle->dialog = GTK_DIALOG(
    gtk_builder_get_object(handle->builder, "new_lobby_dialog")
  );

  gtk_window_set_transient_for(
    GTK_WINDOW(handle->dialog),
    GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->warning_info_bar = GTK_INFO_BAR(
    gtk_builder_get_object(handle->builder, "warning_info_bar")
  );

  g_signal_connect(
    handle->warning_info_bar,
    "close",
    G_CALLBACK(handle_warning_info_bar_close),
    NULL
  );

  g_signal_connect(
    handle->warning_info_bar,
    "response",
    G_CALLBACK(handle_warning_info_bar_response),
    NULL
  );

  handle->stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "new_lobby_stack")
  );

  handle->generate_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "generate_box")
  );

  handle->copy_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "copy_box")
  );

  handle->expiration_combo_box = GTK_COMBO_BOX(
    gtk_builder_get_object(handle->builder, "expiration_combo_box")
  );

  handle->preview_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "preview_stack")
  );

  handle->fail_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "fail_box")
  );

  handle->loading_spinner = GTK_SPINNER(
    gtk_builder_get_object(handle->builder, "loading_spinner")
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

  handle->cancel_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "cancel_button")
  );

  g_signal_connect(
    handle->cancel_button,
    "clicked",
    G_CALLBACK(handle_cancel_button_click),
    handle->dialog
  );

  handle->generate_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "generate_button")
  );

  g_signal_connect(
    handle->generate_button,
    "clicked",
    G_CALLBACK(handle_generate_button_click),
    app
  );

  handle->copy_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "copy_button")
  );

  g_signal_connect(
    handle->copy_button,
    "clicked",
    G_CALLBACK(handle_copy_button_click),
    app
  );

  g_signal_connect(
    handle->dialog,
    "destroy",
    G_CALLBACK(handle_dialog_destroy),
    handle
  );
}

void
ui_new_lobby_dialog_cleanup(UI_NEW_LOBBY_Handle *handle)
{
  g_assert(handle);

  g_signal_handler_disconnect(
    handle->id_drawing_area,
    handle->id_draw_signal
  );

  g_object_unref(handle->builder);

  if (handle->qr)
    QRcode_free(handle->qr);

  memset(handle, 0, sizeof(*handle));
}
