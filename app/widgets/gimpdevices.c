/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#undef GSEAL_ENABLE

#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpbase/gimpbase.h"

#ifdef G_OS_WIN32
#include "libgimpbase/gimpwin32-io.h"
#endif

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpdatafactory.h"
#include "core/gimpgradient.h"
#include "core/gimplist.h"
#include "core/gimppattern.h"
#include "core/gimptoolinfo.h"

#include "gimpdeviceinfo.h"
#include "gimpdevicemanager.h"
#include "gimpdevices.h"

#include "gimp-intl.h"


#define GIMP_DEVICE_MANAGER_DATA_KEY "gimp-device-manager"


static gboolean devicerc_deleted = FALSE;


/*  public functions  */

void
gimp_devices_init (Gimp *gimp)
{
  GimpDeviceManager *manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  manager = g_object_get_data (G_OBJECT (gimp), GIMP_DEVICE_MANAGER_DATA_KEY);

  g_return_if_fail (manager == NULL);

  manager = gimp_device_manager_new (gimp);

  g_object_set_data_full (G_OBJECT (gimp),
                          GIMP_DEVICE_MANAGER_DATA_KEY, manager,
                          (GDestroyNotify) g_object_unref);
}

void
gimp_devices_exit (Gimp *gimp)
{
  GimpDeviceManager *manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  manager = gimp_devices_get_manager (gimp);

  g_return_if_fail (GIMP_IS_DEVICE_MANAGER (manager));

  g_object_set_data (G_OBJECT (gimp), GIMP_DEVICE_MANAGER_DATA_KEY, NULL);
}

void
gimp_devices_restore (Gimp *gimp)
{
  GimpDeviceManager *manager;
  GimpContext       *user_context;
  GimpDeviceInfo    *current_device;
  GList             *list;
  gchar             *filename;
  GError            *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  manager = gimp_devices_get_manager (gimp);

  g_return_if_fail (GIMP_IS_DEVICE_MANAGER (manager));

  user_context = gimp_get_user_context (gimp);

  for (list = GIMP_LIST (manager)->list;
       list;
       list = g_list_next (list))
    {
      GimpDeviceInfo *device_info = list->data;

      gimp_context_copy_properties (user_context, GIMP_CONTEXT (device_info),
                                    GIMP_DEVICE_INFO_CONTEXT_MASK);

      gimp_device_info_set_default_tool (device_info);
    }

  filename = gimp_personal_rc_file ("devicerc");

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_filename_to_utf8 (filename));

  if (! gimp_config_deserialize_file (GIMP_CONFIG (manager),
                                      filename,
                                      gimp,
                                      &error))
    {
      if (error->code != GIMP_CONFIG_ERROR_OPEN_ENOENT)
        gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);

      g_error_free (error);
      /* don't bail out here */
    }

  g_free (filename);

  current_device = gimp_device_manager_get_current_device (manager);

  gimp_context_copy_properties (GIMP_CONTEXT (current_device), user_context,
                                GIMP_DEVICE_INFO_CONTEXT_MASK);
  gimp_context_set_parent (GIMP_CONTEXT (current_device), user_context);
}

void
gimp_devices_save (Gimp     *gimp,
                   gboolean  always_save)
{
  GimpDeviceManager *manager;
  gchar             *filename;
  GError            *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  manager = gimp_devices_get_manager (gimp);

  g_return_if_fail (GIMP_IS_DEVICE_MANAGER (manager));

  if (devicerc_deleted && ! always_save)
    return;

  filename = gimp_personal_rc_file ("devicerc");

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_filename_to_utf8 (filename));

  if (! gimp_config_serialize_to_file (GIMP_CONFIG (manager),
                                       filename,
                                       "GIMP devicerc",
                                       "end of devicerc",
                                       NULL,
                                       &error))
    {
      gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }

  g_free (filename);

  devicerc_deleted = FALSE;
}

gboolean
gimp_devices_clear (Gimp    *gimp,
                    GError **error)
{
  GimpDeviceManager *manager;
  gchar             *filename;
  gboolean           success = TRUE;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  manager = gimp_devices_get_manager (gimp);

  g_return_val_if_fail (GIMP_IS_DEVICE_MANAGER (manager), FALSE);

  filename = gimp_personal_rc_file ("devicerc");

  if (g_unlink (filename) != 0 && errno != ENOENT)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
		   _("Deleting \"%s\" failed: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      success = FALSE;
    }
  else
    {
      devicerc_deleted = TRUE;
    }

  g_free (filename);

  return success;
}

GimpDeviceManager *
gimp_devices_get_manager (Gimp *gimp)
{
  GimpDeviceManager *manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  manager = g_object_get_data (G_OBJECT (gimp), GIMP_DEVICE_MANAGER_DATA_KEY);

  g_return_val_if_fail (GIMP_IS_DEVICE_MANAGER (manager), NULL);

  return manager;
}

void
gimp_devices_add_widget (Gimp      *gimp,
                         GtkWidget *widget)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gtk_widget_set_extension_events (widget, GDK_EXTENSION_EVENTS_ALL);

  g_signal_connect (widget, "motion-notify-event",
                    G_CALLBACK (gimp_devices_check_callback),
                    gimp);
}

gboolean
gimp_devices_check_callback (GtkWidget *widget,
                             GdkEvent  *event,
                             Gimp      *gimp)
{
  g_return_val_if_fail (event != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  if (! gimp->busy)
    gimp_devices_check_change (gimp, event);

  return FALSE;
}

gboolean
gimp_devices_check_change (Gimp     *gimp,
                           GdkEvent *event)
{
  GimpDeviceManager *manager;
  GdkDevice         *device;
  GimpDeviceInfo    *device_info;
  GtkWidget         *source;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  manager = gimp_devices_get_manager (gimp);

  g_return_val_if_fail (GIMP_IS_DEVICE_MANAGER (manager), FALSE);

  /* It is possible that the event was propagated from a widget that does not
     want extension events and therefore always sends core pointer events.
     This can cause a false switch to the core pointer device. */

  source = gtk_get_event_widget (event);

  if (source &&
      gtk_widget_get_extension_events (source) == GDK_EXTENSION_EVENTS_NONE)
    return FALSE;

  switch (event->type)
    {
    case GDK_MOTION_NOTIFY:
      device = ((GdkEventMotion *) event)->device;
      break;

    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
    case GDK_3BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
      device = ((GdkEventButton *) event)->device;
      break;

    case GDK_PROXIMITY_IN:
    case GDK_PROXIMITY_OUT:
      device = ((GdkEventProximity *) event)->device;
      break;

    case GDK_SCROLL:
      device = ((GdkEventScroll *) event)->device;
      break;

    default:
      device = gimp_device_manager_get_current_device (manager)->device;
      break;
    }

  device_info = gimp_device_info_get_by_device (device);

  if (device_info != gimp_device_manager_get_current_device (manager))
    {
      gimp_device_manager_set_current_device (manager, device_info);
      return TRUE;
    }

  return FALSE;
}
