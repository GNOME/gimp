/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpDBusService
 * Copyright (C) 2007 Sven Neumann <sven@gimp.org>
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


static void  gimp_dbus_service_class_init (GimpDBusServiceClass *klass);
static void  gimp_dbus_service_init       (GimpDBusService      *service);

G_DEFINE_TYPE (GimpDBusService, gimp_dbus_service, G_TYPE_OBJECT)


static void
gimp_dbus_service_class_init (GimpDBusServiceClass *klass)
{
  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
                                   &dbus_glib_gimp_object_info);
}

static void
gimp_dbus_service_init (GimpDBusService *service)
{
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

typedef struct
{
  Gimp     *gimp;
  gchar    *uri;
  gboolean  as_new;
} IdleData;

static IdleData *
gimp_dbus_service_open_idle_new (GimpDBusService *service,
                                 const gchar     *uri,
                                 gboolean         as_new)
{
  IdleData *data = g_slice_new (IdleData);

  data->gimp   = g_object_ref (service->gimp);
  data->uri    = g_strdup (uri);
  data->as_new = as_new;

  return data;
}

static void
gimp_dbus_service_open_idle_free (IdleData *data)
{
  g_object_unref (data->gimp);
  g_free (data->uri);

  g_slice_free (IdleData, data);
}

static gboolean
gimp_dbus_service_open_idle (IdleData *data)
{
  file_open_from_command_line (data->gimp, data->uri, data->as_new);

  return FALSE;
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

  g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                   (GSourceFunc) gimp_dbus_service_open_idle,
                   gimp_dbus_service_open_idle_new (service, uri, FALSE),
                   (GDestroyNotify) gimp_dbus_service_open_idle_free);

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

  g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                   (GSourceFunc) gimp_dbus_service_open_idle,
                   gimp_dbus_service_open_idle_new (service, uri, TRUE),
                   (GDestroyNotify) gimp_dbus_service_open_idle_free);

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


#endif /* HAVE_DBUS_GLIB */
