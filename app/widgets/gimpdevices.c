/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmabase/ligmabase.h"

#include "widgets-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmadatafactory.h"
#include "core/ligmaerror.h"
#include "core/ligmagradient.h"
#include "core/ligmalist.h"
#include "core/ligmapattern.h"
#include "core/ligmatoolinfo.h"

#include "ligmadeviceinfo.h"
#include "ligmadevicemanager.h"
#include "ligmadevices.h"

#include "ligma-intl.h"


#define LIGMA_DEVICE_MANAGER_DATA_KEY "ligma-device-manager"


static gboolean devicerc_deleted = FALSE;


/*  public functions  */

void
ligma_devices_init (Ligma *ligma)
{
  LigmaDeviceManager *manager;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  manager = g_object_get_data (G_OBJECT (ligma), LIGMA_DEVICE_MANAGER_DATA_KEY);

  g_return_if_fail (manager == NULL);

  manager = ligma_device_manager_new (ligma);

  g_object_set_data_full (G_OBJECT (ligma),
                          LIGMA_DEVICE_MANAGER_DATA_KEY, manager,
                          (GDestroyNotify) g_object_unref);
}

void
ligma_devices_exit (Ligma *ligma)
{
  LigmaDeviceManager *manager;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  manager = ligma_devices_get_manager (ligma);

  g_return_if_fail (LIGMA_IS_DEVICE_MANAGER (manager));

  g_object_set_data (G_OBJECT (ligma), LIGMA_DEVICE_MANAGER_DATA_KEY, NULL);
}

void
ligma_devices_restore (Ligma *ligma)
{
  LigmaDeviceManager *manager;
  GList             *list;
  GFile             *file;
  GError            *error = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  manager = ligma_devices_get_manager (ligma);

  g_return_if_fail (LIGMA_IS_DEVICE_MANAGER (manager));

  for (list = LIGMA_LIST (manager)->queue->head;
       list;
       list = g_list_next (list))
    {
      LigmaDeviceInfo *device_info = list->data;

      ligma_device_info_save_tool (device_info);
      ligma_device_info_set_default_tool (device_info);
    }

  file = ligma_directory_file ("devicerc", NULL);

  if (ligma->be_verbose)
    g_print ("Parsing '%s'\n", ligma_file_get_utf8_name (file));

  if (! ligma_config_deserialize_file (LIGMA_CONFIG (manager),
                                      file,
                                      ligma,
                                      &error))
    {
      if (error->code != LIGMA_CONFIG_ERROR_OPEN_ENOENT)
        ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);

      g_error_free (error);
      /* don't bail out here */
    }

  g_object_unref (file);

  for (list = LIGMA_LIST (manager)->queue->head;
       list;
       list = g_list_next (list))
    {
      LigmaDeviceInfo *device_info = list->data;

      if (! LIGMA_TOOL_PRESET (device_info)->tool_options)
        {
          ligma_device_info_save_tool (device_info);

          g_printerr ("%s: set default tool on loaded LigmaDeviceInfo without tool options: %s\n",
                      G_STRFUNC, ligma_object_get_name (device_info));
        }
    }

  if (! LIGMA_GUI_CONFIG (ligma->config)->devices_share_tool)
    {
      LigmaDeviceInfo *current_device;

      current_device = ligma_device_manager_get_current_device (manager);

      ligma_device_info_restore_tool (current_device);
    }
}

void
ligma_devices_save (Ligma     *ligma,
                   gboolean  always_save)
{
  LigmaDeviceManager *manager;
  GFile             *file;
  GError            *error = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  manager = ligma_devices_get_manager (ligma);

  g_return_if_fail (LIGMA_IS_DEVICE_MANAGER (manager));

  if (devicerc_deleted && ! always_save)
    return;

  file = ligma_directory_file ("devicerc", NULL);

  if (ligma->be_verbose)
    g_print ("Writing '%s'\n", ligma_file_get_utf8_name (file));

  if (! LIGMA_GUI_CONFIG (ligma->config)->devices_share_tool)
    {
      LigmaDeviceInfo *current_device;

      current_device = ligma_device_manager_get_current_device (manager);

      ligma_device_info_save_tool (current_device);
    }

  if (! ligma_config_serialize_to_file (LIGMA_CONFIG (manager),
                                       file,
                                       "LIGMA devicerc",
                                       "end of devicerc",
                                       NULL,
                                       &error))
    {
      ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }

  g_object_unref (file);

  devicerc_deleted = FALSE;
}

gboolean
ligma_devices_clear (Ligma    *ligma,
                    GError **error)
{
  LigmaDeviceManager *manager;
  GFile             *file;
  GError            *my_error = NULL;
  gboolean           success  = TRUE;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);

  manager = ligma_devices_get_manager (ligma);

  g_return_val_if_fail (LIGMA_IS_DEVICE_MANAGER (manager), FALSE);

  file = ligma_directory_file ("devicerc", NULL);

  if (! g_file_delete (file, NULL, &my_error) &&
      my_error->code != G_IO_ERROR_NOT_FOUND)
    {
      success = FALSE;

      g_set_error (error, LIGMA_ERROR, LIGMA_FAILED,
                   _("Deleting \"%s\" failed: %s"),
                   ligma_file_get_utf8_name (file), my_error->message);
    }
  else
    {
      devicerc_deleted = TRUE;
    }

  g_clear_error (&my_error);
  g_object_unref (file);

  return success;
}

LigmaDeviceManager *
ligma_devices_get_manager (Ligma *ligma)
{
  LigmaDeviceManager *manager;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  manager = g_object_get_data (G_OBJECT (ligma), LIGMA_DEVICE_MANAGER_DATA_KEY);

  g_return_val_if_fail (LIGMA_IS_DEVICE_MANAGER (manager), NULL);

  return manager;
}

GdkDevice *
ligma_devices_get_from_event (Ligma            *ligma,
                             const GdkEvent  *event,
                             GdkDevice      **grab_device)
{
  GdkDevice *device;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (event != NULL, NULL);

  device = gdk_event_get_source_device (event);

  /*  initialize the default grab device to the event's device,
   *  because that is always either a master or a floating device,
   *  which is the types of devices we can make grabs on without
   *  disturbing side effects.
   */
  if (grab_device)
    *grab_device = gdk_event_get_device (event);

  if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
    {
      switch (gdk_device_get_device_type (device))
        {
        case GDK_DEVICE_TYPE_MASTER:
          /*  this happens on focus synthesized focus changed events,
           *  and we can't do anything with the returned device, so
           *  return NULL
           */
          return NULL;

        case GDK_DEVICE_TYPE_SLAVE:
          /*  it makes no sense for us to distinguigh between
           *  different slave keyboards, so just always return
           *  their respective master
           */
          return gdk_device_get_associated_device (device);

        case GDK_DEVICE_TYPE_FLOATING:
          /*  we have no way of explicitly enabling floating
           *  keyboards, so we cannot get their events
           */
          g_return_val_if_reached (device);
       }
    }
  else
    {
      switch (gdk_device_get_device_type (device))
        {
        case GDK_DEVICE_TYPE_MASTER:
          /*  this can only happen for synthesized events which have
           *  no actual source, so return NULL to indicate that we
           *  cannot do anything with the event's device information
           */
          return NULL;

        case GDK_DEVICE_TYPE_SLAVE:
          /*  this is the tricky part: we do want to distingiugh slave
           *  devices, but only if we actually enabled them ourselves
           *  explicitly (like the pens of a tablet); however we
           *  usually don't enable the different incarnations of the
           *  mouse itself (like touchpad, trackpoint, usb mouse
           *  etc.), so for these return their respective master so
           *  its settings are used
           */
          if (gdk_device_get_mode (device) == GDK_MODE_DISABLED)
            {
              return gdk_device_get_associated_device (device);
            }

          return device;

        case GDK_DEVICE_TYPE_FLOATING:
          /*  we only get events for floating devices which have
           *  enabled ourselves, so return the floating device
           */
          return device;
        }
    }

  g_return_val_if_reached (device);
}

void
ligma_devices_add_widget (Ligma      *ligma,
                         GtkWidget *widget)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  g_signal_connect (widget, "motion-notify-event",
                    G_CALLBACK (ligma_devices_check_callback),
                    ligma);
}

gboolean
ligma_devices_check_callback (GtkWidget *widget,
                             GdkEvent  *event,
                             Ligma      *ligma)
{
  g_return_val_if_fail (event != NULL, FALSE);
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);

  if (! ligma->busy)
    {
      GdkDevice *device;

      device = ligma_devices_get_from_event (ligma, event, NULL);

      if (device)
        ligma_devices_check_change (ligma, device);
    }

  return FALSE;
}

gboolean
ligma_devices_check_change (Ligma      *ligma,
                           GdkDevice *device)
{
  LigmaDeviceManager *manager;
  LigmaDeviceInfo    *device_info;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (GDK_IS_DEVICE (device), FALSE);

  manager = ligma_devices_get_manager (ligma);

  g_return_val_if_fail (LIGMA_IS_DEVICE_MANAGER (manager), FALSE);

  device_info = ligma_device_info_get_by_device (device);

  if (! device_info)
    device_info = ligma_device_manager_get_current_device (manager);

  if (device_info &&
      device_info != ligma_device_manager_get_current_device (manager))
    {
      ligma_device_manager_set_current_device (manager, device_info);
      return TRUE;
    }

  return FALSE;
}
