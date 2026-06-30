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
 * @file media.c
 */

#include "media.h"

#include <glib-2.0/glib.h>
#include <pipewire/impl.h>

#include "application.h"

static void
on_core_done(void *data,
             UNUSED uint32_t id,
             int seq)
{
  g_assert(data);

	MESSENGER_MediaInfo *media = (MESSENGER_MediaInfo*) data;
  MESSENGER_Application *app = media->app;

	if ((seq == media->pw.pending) && (app->pw.main_loop))
		pw_main_loop_quit(app->pw.main_loop);
}

static void
on_core_error(void *data,
              UNUSED uint32_t id,
              UNUSED int seq,
              int res,
              const char *message)
{
  g_assert((data) && (message));

	MESSENGER_MediaInfo *media = (MESSENGER_MediaInfo*) data;
  MESSENGER_Application *app = media->app;

  g_printerr("ERROR: %s\n", message);

	if ((id == PW_ID_CORE) && (res == -EPIPE) && (app->pw.main_loop))
		pw_main_loop_quit(app->pw.main_loop);
}

static const struct pw_core_events remote_core_events = {
	PW_VERSION_CORE_EVENTS,
	.done = on_core_done,
	.error = on_core_error,
};

static void
registry_event_global(void *data,
                      uint32_t id,
                      uint32_t permissions,
                      const char *type,
                      uint32_t version,
                      const struct spa_dict *props)
{
  g_assert(data);

  MESSENGER_MediaInfo *media = (MESSENGER_MediaInfo*) data;

	if (!props)
    return;

  struct pw_properties *properties = pw_properties_new_dict(props);
  if (!properties)
    return;

  size_t size = pw_map_get_size(&(media->pw.globals));
	while (id > size)
		pw_map_insert_at(&(media->pw.globals), size++, NULL);

	pw_map_insert_at(&(media->pw.globals), id, properties);

  media->pw.pending = media->pw.core? pw_core_sync(media->pw.core, 0, 0) : 0;
}

static void
registry_event_global_remove(void *data,
                             uint32_t id)
{
  g_assert(data);

  MESSENGER_MediaInfo *media = (MESSENGER_MediaInfo*) data;

  struct pw_properties *properties = pw_map_lookup(&(media->pw.globals), id);
  if (!properties)
    return;

  pw_map_insert_at(&(media->pw.globals), id, NULL);
	pw_properties_free(properties);
}

static const struct pw_registry_events registry_events = {
	PW_VERSION_REGISTRY_EVENTS,
	.global = registry_event_global,
	.global_remove = registry_event_global_remove,
};

void
media_pw_init(MESSENGER_MediaInfo *media,
              MESSENGER_Application *app,
              int fd)
{
  g_assert((media) && (app));

  media_pw_cleanup(media);
  media->app = app;

  if (app->pw.context)
  {
    if (-1 != fd)
      media->pw.core = pw_context_connect_fd(
        app->pw.context,
        fd,
        NULL,
        0
      );
    else
      media->pw.core = pw_context_connect(app->pw.context, NULL, 0);
  }
  else
    media->pw.core = NULL;

  media->pw.registry = media->pw.core? 
    pw_core_get_registry(media->pw.core, PW_VERSION_REGISTRY, 0) : NULL;

  pw_map_init(&(media->pw.globals), 64, 16);

  if (media->pw.core)
    pw_core_add_listener(
      media->pw.core,
      &(media->pw.core_listener),
      &remote_core_events,
      media
    );

  if (media->pw.registry)
    pw_registry_add_listener(
      media->pw.registry,
      &(media->pw.registry_listener),
      &registry_events,
      media
    );
}

void
media_init_camera_capturing(MESSENGER_MediaInfo *media,
                            MESSENGER_Application *app)
{
  g_assert((media) && (app));
  int fd = -1;

#ifndef MESSENGER_APPLICATION_NO_PORTAL
  if ((app->portal) && (xdp_portal_is_camera_present(app->portal)))
    fd = xdp_portal_open_pipewire_remote_for_camera(app->portal);
#endif
  
  media_pw_init(media, app, fd);
}

void
media_init_screen_sharing(MESSENGER_MediaInfo *media,
                          MESSENGER_Application *app)
{
  g_assert((media) && (app));
  int fd = -1;

#ifndef MESSENGER_APPLICATION_NO_PORTAL
  fd = application_get_active_session_remote(app);
#endif

  media_pw_init(media, app, fd);
}

static int
destroy_global(void *obj,
               UNUSED void *data)
{
  struct pw_properties *properties = (struct pw_properties*) obj;

  if (!properties)
    return 0;

	pw_properties_free(properties);
	return 0;
}

void
media_pw_cleanup(MESSENGER_MediaInfo *media)
{
  g_assert(media);

  if (media->pw.registry)
    pw_proxy_destroy((struct pw_proxy*) media->pw.registry);

  media->pw.registry = NULL;

  pw_map_for_each(&(media->pw.globals), destroy_global, NULL);
  pw_map_clear(&(media->pw.globals));

  if (media->pw.core)
    pw_core_disconnect(media->pw.core);

  media->pw.core = NULL;
  media->app = NULL;
}

void
media_pw_main_loop_run(MESSENGER_MediaInfo *media)
{
  g_assert(media);

  if (!(media->pw.core))
    return;

  media->pw.pending = media->pw.core? pw_core_sync(media->pw.core, 0, 0) : 0;

  application_pw_main_loop_run(media->app);
}

typedef struct IterateGlobalClosure
{
  MESSENGER_MediaNodeIterator iterator;
  void *cls;
} IterateGlobalClosure;

static int
iterate_global(void *obj,
               void *data)
{
  g_assert(data);

  IterateGlobalClosure *closure = (IterateGlobalClosure*) data;
  struct pw_properties *properties = (struct pw_properties*) obj;

  if (!properties)
    return 0;

  struct spa_dict *props = &(properties->dict);

  if ((!props) || (!props->n_items))
    return 0;

  const char *name = NULL;
  const char *description = NULL;
  const char *media_role = NULL;
  const char *media_class = NULL;

  const struct spa_dict_item *item;
  spa_dict_for_each(item, props)
  {
    if (0 == g_strcmp0(item->key, "node.name"))
      name = item->value;

    if (0 == g_strcmp0(item->key, "node.description"))
      description = item->value;

    if (0 == g_strcmp0(item->key, "media.class"))
      media_class = item->value;

    if (0 == g_strcmp0(item->key, "media.role"))
      media_role = item->value;
	}

  if ((!name) || (!media_class))
    return 0;

  closure->iterator(
    closure->cls,
    name,
    description,
    media_class,
    media_role
  );
  
	return 0;
}

void
media_pw_iterate_nodes(MESSENGER_MediaInfo *media,
                       MESSENGER_MediaNodeIterator it,
                       void *cls)
{
  g_assert(media);

  if (!it)
    return;

  IterateGlobalClosure closure;
  closure.iterator = it;
  closure.cls = cls;

  pw_map_for_each(&(media->pw.globals), iterate_global, &closure);
}
