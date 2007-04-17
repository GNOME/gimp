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

gboolean
gimp_dbus_service_open (GimpDBusService  *service,
                        const gchar      *filename,
                        gboolean         *success,
                        GError          **dbus_error)
{
  g_return_val_if_fail (GIMP_IS_DBUS_SERVICE (service), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (success != NULL, FALSE);

  *success = file_open_from_command_line (service->gimp, filename, FALSE);

  return TRUE;
}

gboolean
gimp_dbus_service_open_as_new (GimpDBusService  *service,
                               const gchar      *filename,
                               gboolean         *success,
                               GError          **dbus_error)
{
  g_return_val_if_fail (GIMP_IS_DBUS_SERVICE (service), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (success != NULL, FALSE);

  *success = file_open_from_command_line (service->gimp, filename, TRUE);

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
