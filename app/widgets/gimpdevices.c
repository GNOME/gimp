/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpbase/gimpbase.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpdatafactory.h"
#include "core/gimpgradient.h"
#include "core/gimplist.h"
#include "core/gimppattern.h"
#include "core/gimptoolinfo.h"

#include "config/gimpconfig.h"

#include "gimpdeviceinfo.h"
#include "gimpdevices.h"

#include "gimprc.h"


#define GIMP_DEVICE_MANAGER_DATA_KEY "gimp-device-manager"


typedef struct _GimpDeviceManager GimpDeviceManager;

struct _GimpDeviceManager
{
  GimpContainer          *device_info_list;
  GdkDevice              *current_device;
  GimpDeviceChangeNotify  change_notify;
};


/*  local function prototypes  */

static GimpDeviceManager * gimp_device_manager_get (Gimp *gimp);


/*  public functions  */

void 
gimp_devices_init (Gimp                   *gimp,
                   GimpDeviceChangeNotify  change_notify)
{
  GimpDeviceManager *manager;
  GdkDevice         *device;
  GimpDeviceInfo    *device_info;
  GList             *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  g_return_if_fail (gimp_device_manager_get (gimp) == NULL);

  manager = g_new0 (GimpDeviceManager, 1);

  g_object_set_data_full (G_OBJECT (gimp),
                          GIMP_DEVICE_MANAGER_DATA_KEY, manager,
                          (GDestroyNotify) g_free);

  manager->device_info_list = gimp_list_new (GIMP_TYPE_DEVICE_INFO,
                                             GIMP_CONTAINER_POLICY_STRONG);
  manager->current_device   = gdk_device_get_core_pointer ();
  manager->change_notify    = change_notify;

  /*  create device info structures for present devices */
  for (list = gdk_devices_list (); list; list = g_list_next (list))
    {
      device = (GdkDevice *) list->data;

      device_info = gimp_device_info_new (gimp, device->name);
      gimp_container_add (manager->device_info_list, GIMP_OBJECT (device_info));
      g_object_unref (G_OBJECT (device_info));

      gimp_device_info_set_from_device (device_info, device);
    }
}

void
gimp_devices_exit (Gimp *gimp)
{
  GimpDeviceManager *manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  manager = gimp_device_manager_get (gimp);

  g_return_if_fail (manager != NULL);

  g_object_unref (G_OBJECT (manager->device_info_list));
  g_object_set_data (G_OBJECT (gimp), GIMP_DEVICE_MANAGER_DATA_KEY, NULL);
}

void
gimp_devices_restore (Gimp *gimp)
{
  GimpDeviceManager *manager;
  GimpDeviceInfo    *device_info;
  GimpContext       *user_context;
  gchar             *filename;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  manager = gimp_device_manager_get (gimp);

  g_return_if_fail (manager != NULL);

  /* Augment with information from rc file */
  filename = gimp_personal_rc_file ("devicerc");
  gimprc_parse_file (filename);
  g_free (filename);

  device_info = gimp_device_info_get_by_device (manager->current_device);

  g_return_if_fail (GIMP_IS_DEVICE_INFO (device_info));

  user_context = gimp_get_user_context (gimp);

  gimp_context_copy_properties (GIMP_CONTEXT (device_info), user_context,
				GIMP_DEVICE_INFO_CONTEXT_MASK);
  gimp_context_set_parent (GIMP_CONTEXT (device_info), user_context);
}

void
gimp_devices_save (Gimp *gimp)
{
  GimpDeviceManager *manager;
  gchar             *filename;
  FILE              *fp;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  manager = gimp_device_manager_get (gimp);

  g_return_if_fail (manager != NULL);

  filename = gimp_personal_rc_file ("devicerc");
  fp = fopen (filename, "wb");

  if (fp)
    {
      gimp_container_foreach (manager->device_info_list,
                              (GFunc) gimp_device_info_save, fp);
      fclose (fp);
    }
  else
    {
      g_warning ("%s: could not open \"%s\" for writing: %s",
                 G_STRLOC, filename, g_strerror (errno));
    }

  g_free (filename);
}

void
gimp_devices_restore_test (Gimp *gimp)
{
  GimpDeviceManager *manager;
  GimpDeviceInfo    *device_info;
  GimpContext       *user_context;
  gchar             *filename;
  GError            *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  manager = gimp_device_manager_get (gimp);

  g_return_if_fail (manager != NULL);

  filename = gimp_personal_rc_file ("test-devicerc");

  if (! gimp_config_deserialize (G_OBJECT (manager->device_info_list),
                                 filename,
                                 gimp,
                                 &error))
    {
      g_message ("Could not read test-devicerc: %s", error->message);
      g_clear_error (&error);

      g_free (filename);
      return;
    }

  g_free (filename);

  device_info = gimp_device_info_get_by_device (manager->current_device);

  g_return_if_fail (GIMP_IS_DEVICE_INFO (device_info));

  user_context = gimp_get_user_context (gimp);

  gimp_context_copy_properties (GIMP_CONTEXT (device_info), user_context,
				GIMP_DEVICE_INFO_CONTEXT_MASK);
  gimp_context_set_parent (GIMP_CONTEXT (device_info), user_context);
}

void
gimp_devices_save_test (Gimp *gimp)
{
  GimpDeviceManager *manager;
  gchar             *filename;
  GError            *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  manager = gimp_device_manager_get (gimp);

  g_return_if_fail (manager != NULL);

  filename = gimp_personal_rc_file ("test-devicerc");

  if (! gimp_config_serialize (G_OBJECT (manager->device_info_list),
                               filename,
                               "# test-devicerc\n",
                               "# end test-devicerc",
                               NULL,
                               &error))
    {
      g_message ("Could not write test-devicerc: %s", error->message);
      g_clear_error (&error);
    }

  g_free (filename);
}

GdkDevice *
gimp_devices_get_current (Gimp *gimp)
{
  GimpDeviceManager *manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  manager = gimp_device_manager_get (gimp);

  g_return_val_if_fail (manager != NULL, NULL);

  return manager->current_device;
}

void
gimp_devices_select_device (Gimp      *gimp,
                            GdkDevice *new_device)
{
  GimpDeviceManager *manager;
  GimpDeviceInfo    *current_device_info;
  GimpDeviceInfo    *new_device_info;
  GimpContext       *user_context;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GDK_IS_DEVICE (new_device));

  manager = gimp_device_manager_get (gimp);

  g_return_if_fail (manager != NULL);

  current_device_info = gimp_device_info_get_by_device (manager->current_device);
  new_device_info     = gimp_device_info_get_by_device (new_device);

  g_return_if_fail (GIMP_IS_DEVICE_INFO (current_device_info));
  g_return_if_fail (GIMP_IS_DEVICE_INFO (new_device_info));

  gimp_context_unset_parent (GIMP_CONTEXT (current_device_info));

  manager->current_device = new_device;

  user_context = gimp_get_user_context (gimp);

  gimp_context_copy_properties (GIMP_CONTEXT (new_device_info), user_context,
				GIMP_DEVICE_INFO_CONTEXT_MASK);
  gimp_context_set_parent (GIMP_CONTEXT (new_device_info), user_context);

  if (manager->change_notify)
    manager->change_notify (gimp);
}

gboolean
gimp_devices_check_change (Gimp     *gimp,
                           GdkEvent *event)
{
  GimpDeviceManager *manager;
  GdkDevice         *device;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  manager = gimp_device_manager_get (gimp);

  g_return_val_if_fail (manager != NULL, FALSE);

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
      device = manager->current_device;
      break;
    }

  if (device != manager->current_device)
    {
      gimp_devices_select_device (gimp, device);
      return TRUE;
    }

  return FALSE;
}

void
gimp_devices_rc_update (Gimp               *gimp,
                        const gchar        *name, 
                        GimpDeviceValues    values,
                        GdkInputMode        mode, 
                        gint                num_axes,
                        const GdkAxisUse   *axes, 
                        gint                num_keys, 
                        const GdkDeviceKey *keys,
                        const gchar        *tool_name,
                        const GimpRGB      *foreground,
                        const GimpRGB      *background,
                        const gchar        *brush_name, 
                        const gchar        *pattern_name,
                        const gchar        *gradient_name)
{
  GimpDeviceManager *manager;
  GimpDeviceInfo    *device_info = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (name != NULL);

  manager = gimp_device_manager_get (gimp);

  g_return_if_fail (manager != NULL);

  /*  Find device if we have it  */
  device_info = (GimpDeviceInfo *)
    gimp_container_get_child_by_name (manager->device_info_list, name);

  if (! device_info)
    {
      device_info = gimp_device_info_new (gimp, name);
      gimp_container_add (manager->device_info_list, GIMP_OBJECT (device_info));
      g_object_unref (G_OBJECT (device_info));
    }
  else if (! device_info->device)
    {
      g_warning ("%s: called multiple times for not present device",
                 G_STRLOC);
      return;
    }

  gimp_device_info_set_from_rc (device_info,
                                values,
                                mode,
                                num_axes, axes,
                                num_keys, keys,
                                tool_name,
                                foreground, background,
                                brush_name,
                                pattern_name,
                                gradient_name);
}


/*  private functions  */

static GimpDeviceManager *
gimp_device_manager_get (Gimp *gimp)
{
  return g_object_get_data (G_OBJECT (gimp), GIMP_DEVICE_MANAGER_DATA_KEY);
}
