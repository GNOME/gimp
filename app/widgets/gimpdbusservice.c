/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpDBusService
 * Copyright (C) 2007, 2008 Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#if HAVE_DBUS_GLIB

#include <dbus/dbus-glib.h>

#include "core/core-types.h"

#include "core/gimp.h"

#include "file/file-open.h"

#include "gimpdbusservice.h"
#include "gimpdbusservice-glue.h"
#include "gimpuimanager.c"


typedef struct
{
  gchar    *uri;
  gboolean  as_new;
} OpenData;


static void  gimp_dbus_service_class_init (GimpDBusServiceClass *klass);
static void  gimp_dbus_service_init       (GimpDBusService      *service);
static void  gimp_dbus_service_dispose    (GObject              *object);
static void  gimp_dbus_service_finalize   (GObject              *object);

static void  gimp_dbus_service_queue_open (GimpDBusService      *service,
                                           const gchar          *uri,
                                           gboolean              as_new);

static OpenData * gimp_dbus_service_open_data_new  (GimpDBusService *service,
                                                    const gchar     *uri,
                                                    gboolean         as_new);
static void       gimp_dbus_service_open_data_free (OpenData        *data);


G_DEFINE_TYPE (GimpDBusService, gimp_dbus_service, G_TYPE_OBJECT)

#define parent_class gimp_dbus_service_parent_class


static void
gimp_dbus_service_class_init (GimpDBusServiceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose  = gimp_dbus_service_dispose;
  object_class->finalize = gimp_dbus_service_finalize;

  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
                                   &dbus_glib_gimp_object_info);
}

static void
gimp_dbus_service_init (GimpDBusService *service)
{
  service->queue = g_queue_new ();
}

GObject *
gimp_dbus_service_new (Gimp *gimp)
{
  GimpDBusService *service;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  service = g_object_new (GIMP_TYPE_DBUS_SERVICE, NULL);

  service->gimp = gimp;

  return G_OBJECT (service);
}

static void
gimp_dbus_service_dispose (GObject *object)
{
  GimpDBusService *service = GIMP_DBUS_SERVICE (object);

  if (service->source)
    {
      g_source_remove (g_source_get_id (service->source));
      service->source = NULL;
    }

  while (! g_queue_is_empty (service->queue))
    {
      gimp_dbus_service_open_data_free (g_queue_pop_head (service->queue));
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_dbus_service_finalize (GObject *object)
{
  GimpDBusService *service = GIMP_DBUS_SERVICE (object);

  if (service->queue)
    {
      g_queue_free (service->queue);
      service->queue = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

gboolean
gimp_dbus_service_open (GimpDBusService  *service,
                        const gchar      *uri,
                        gboolean         *success,
                        GError          **dbus_error)
{
  g_return_val_if_fail (GIMP_IS_DBUS_SERVICE (service), FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);
  g_return_val_if_fail (success != NULL, FALSE);

  gimp_dbus_service_queue_open (service, uri, FALSE);

  /*  The call always succeeds as it is handled in one way or another.
   *  Even presenting an error message is considered success ;-)
   */
  *success = TRUE;

  return TRUE;
}

gboolean
gimp_dbus_service_open_as_new (GimpDBusService  *service,
                               const gchar      *uri,
                               gboolean         *success,
                               GError          **dbus_error)
{
  g_return_val_if_fail (GIMP_IS_DBUS_SERVICE (service), FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);
  g_return_val_if_fail (success != NULL, FALSE);

  gimp_dbus_service_queue_open (service, uri, TRUE);

  /*  The call always succeeds as it is handled in one way or another.
   *  Even presenting an error message is considered success ;-)
   */
  *success = TRUE;

  return TRUE;
}

gboolean
gimp_dbus_service_activate (GimpDBusService  *service,
                            GError          **dbus_error)
{
  const GList *managers;

  g_return_val_if_fail (GIMP_IS_DBUS_SERVICE (service), FALSE);

  /* raise the toolbox */
  managers = gimp_ui_managers_from_name ("<Image>");

  if (managers)
    gimp_ui_manager_activate_action (managers->data,
                                     "dialogs", "dialogs-toolbox");

  return TRUE;
}

static OpenData *
gimp_dbus_service_open_data_new (GimpDBusService *service,
                                 const gchar     *uri,
                                 gboolean         as_new)
{
  OpenData *data = g_slice_new (OpenData);

  data->uri    = g_strdup (uri);
  data->as_new = as_new;

  return data;
}

static void
gimp_dbus_service_open_data_free (OpenData *data)
{
  g_free (data->uri);
  g_slice_free (OpenData, data);
}

static gboolean
gimp_dbus_service_open_idle (GimpDBusService *service)
{
  OpenData *data = g_queue_pop_tail (service->queue);

  if (data)
    {
      file_open_from_command_line (service->gimp, data->uri, data->as_new);

      gimp_dbus_service_open_data_free (data);

      return TRUE;
    }

  service->source = NULL;

  return FALSE;
}

static void
gimp_dbus_service_queue_open (GimpDBusService *service,
                              const gchar     *uri,
                              gboolean         as_new)
{
  g_queue_push_tail (service->queue,
                     gimp_dbus_service_open_data_new (service, uri, as_new));

  if (! service->source)
    {
      service->source = g_idle_source_new ();

      g_source_set_callback (service->source,
                             (GSourceFunc) gimp_dbus_service_open_idle, service,
                             NULL);
      g_source_attach (service->source, NULL);
      g_source_unref (service->source);
    }
}

#endif /* HAVE_DBUS_GLIB */
