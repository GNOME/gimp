/* GIMP - The GNU Image Manipulation Program
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

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
#include "gimpdevices.h"

#include "gimp-intl.h"


#define GIMP_DEVICE_MANAGER_DATA_KEY "gimp-device-manager"


typedef struct _GimpDeviceManager GimpDeviceManager;

struct _GimpDeviceManager
{
  Gimp                   *gimp;
  GimpContainer          *device_info_list;
  GdkDevice              *current_device;
  GimpDeviceChangeNotify  change_notify;
  gboolean                devicerc_deleted;
};


/*  local function prototypes  */

static GimpDeviceManager * gimp_device_manager_get  (Gimp              *gimp);
static void                gimp_device_manager_free (GimpDeviceManager *manager);

static void   gimp_devices_display_opened (GdkDisplayManager *disp_manager,
                                           GdkDisplay        *display,
                                           GimpDeviceManager *manager);
static void   gimp_devices_display_closed (GdkDisplay        *display,
                                           gboolean           is_error,
                                           GimpDeviceManager *manager);


/*  public functions  */

void
gimp_devices_init (Gimp                   *gimp,
                   GimpDeviceChangeNotify  change_notify)
{
  GdkDisplayManager *disp_manager = gdk_display_manager_get ();
  GimpDeviceManager *manager;
  GSList            *displays;
  GSList            *list;
  GdkDisplay        *gdk_display;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (gimp_device_manager_get (gimp) == NULL);

  manager = g_slice_new0 (GimpDeviceManager);

  g_object_set_data_full (G_OBJECT (gimp),
                          GIMP_DEVICE_MANAGER_DATA_KEY, manager,
                          (GDestroyNotify) gimp_device_manager_free);

  gdk_display = gdk_display_get_default ();

  manager->gimp             = gimp;
  manager->device_info_list = gimp_list_new (GIMP_TYPE_DEVICE_INFO, FALSE);
  manager->current_device   = gdk_display_get_core_pointer (gdk_display);
  manager->change_notify    = change_notify;

  displays = gdk_display_manager_list_displays (disp_manager);

  /*  present displays in the order in which they were opened  */
  displays = g_slist_reverse (displays);

  for (list = displays; list; list = g_slist_next (list))
    {
      gimp_devices_display_opened (disp_manager, list->data, manager);
    }

  g_slist_free (displays);

  g_signal_connect (disp_manager, "display-opened",
                    G_CALLBACK (gimp_devices_display_opened),
                    manager);
}

void
gimp_devices_exit (Gimp *gimp)
{
  GimpDeviceManager *manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  manager = gimp_device_manager_get (gimp);

  g_return_if_fail (manager != NULL);

  g_signal_handlers_disconnect_by_func (gdk_display_manager_get (),
                                        gimp_devices_display_opened,
                                        manager);

  g_object_set_data (G_OBJECT (gimp), GIMP_DEVICE_MANAGER_DATA_KEY, NULL);
}

void
gimp_devices_restore (Gimp *gimp)
{
  GimpDeviceManager *manager;
  GimpDeviceInfo    *device_info;
  GimpContext       *user_context;
  GList             *list;
  gchar             *filename;
  GError            *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  manager = gimp_device_manager_get (gimp);

  g_return_if_fail (manager != NULL);

  user_context = gimp_get_user_context (gimp);

  for (list = GIMP_LIST (manager->device_info_list)->list;
       list;
       list = g_list_next (list))
    {
      GimpDeviceInfo *device_info = list->data;

      gimp_context_copy_properties (user_context, GIMP_CONTEXT (device_info),
                                    GIMP_DEVICE_INFO_CONTEXT_MASK);
    }

  filename = gimp_personal_rc_file ("devicerc");

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_filename_to_utf8 (filename));

  if (! gimp_config_deserialize_file (GIMP_CONFIG (manager->device_info_list),
                                      filename,
                                      gimp,
                                      &error))
    {
      if (error->code != GIMP_CONFIG_ERROR_OPEN_ENOENT)
        gimp_message (gimp, NULL, GIMP_MESSAGE_ERROR, "%s", error->message);

      g_error_free (error);
      /* don't bail out here */
    }

  g_free (filename);

  gimp_list_reverse (GIMP_LIST (manager->device_info_list));

  device_info = gimp_device_info_get_by_device (manager->current_device);

  g_return_if_fail (GIMP_IS_DEVICE_INFO (device_info));

  gimp_context_copy_properties (GIMP_CONTEXT (device_info), user_context,
                                GIMP_DEVICE_INFO_CONTEXT_MASK);
  gimp_context_set_parent (GIMP_CONTEXT (device_info), user_context);
}

void
gimp_devices_save (Gimp     *gimp,
                   gboolean  always_save)
{
  GimpDeviceManager *manager;
  gchar             *filename;
  GError            *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  manager = gimp_device_manager_get (gimp);

  g_return_if_fail (manager != NULL);

  if (manager->devicerc_deleted && ! always_save)
    return;

  filename = gimp_personal_rc_file ("devicerc");

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_filename_to_utf8 (filename));

  if (! gimp_config_serialize_to_file (GIMP_CONFIG (manager->device_info_list),
                                       filename,
                                       "GIMP devicerc",
                                       "end of devicerc",
                                       NULL,
                                       &error))
    {
      gimp_message (gimp, NULL, GIMP_MESSAGE_ERROR, "%s", error->message);
      g_error_free (error);
    }

  g_free (filename);

  manager->devicerc_deleted = FALSE;
}

gboolean
gimp_devices_clear (Gimp    *gimp,
                    GError **error)
{
  GimpDeviceManager *manager;
  gchar             *filename;
  gboolean           success = TRUE;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  manager = gimp_device_manager_get (gimp);

  g_return_val_if_fail (manager != NULL, FALSE);

  filename = gimp_personal_rc_file ("devicerc");

  if (g_unlink (filename) != 0 && errno != ENOENT)
    {
      g_set_error (error, 0, 0, _("Deleting \"%s\" failed: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      success = FALSE;
    }
  else
    {
      manager->devicerc_deleted = TRUE;
    }

  g_free (filename);

  return success;
}

GimpContainer *
gimp_devices_get_list (Gimp *gimp)
{
  GimpDeviceManager *manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  manager = gimp_device_manager_get (gimp);

  g_return_val_if_fail (manager != NULL, NULL);

  return manager->device_info_list;
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

  gimp_context_set_parent (GIMP_CONTEXT (current_device_info), NULL);

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
  GtkWidget         *source;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  manager = gimp_device_manager_get (gimp);

  g_return_val_if_fail (manager != NULL, FALSE);

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


/*  private functions  */

static GimpDeviceManager *
gimp_device_manager_get (Gimp *gimp)
{
  return g_object_get_data (G_OBJECT (gimp), GIMP_DEVICE_MANAGER_DATA_KEY);
}

static void
gimp_device_manager_free (GimpDeviceManager *manager)
{
  if (manager->device_info_list)
    g_object_unref (manager->device_info_list);

  g_slice_free (GimpDeviceManager, manager);
}

static void
gimp_devices_display_opened (GdkDisplayManager *disp_manager,
                             GdkDisplay        *gdk_display,
                             GimpDeviceManager *manager)
{
  GList *list;

  /*  create device info structures for present devices */
  for (list = gdk_display_list_devices (gdk_display); list; list = list->next)
    {
      GdkDevice      *device = list->data;
      GimpDeviceInfo *device_info;

      device_info = gimp_device_info_new (manager->gimp, device->name);
      gimp_device_info_set_from_device (device_info, device, gdk_display);

      gimp_container_add (manager->device_info_list, GIMP_OBJECT (device_info));
      g_object_unref (device_info);

    }

  g_signal_connect (gdk_display, "closed",
                    G_CALLBACK (gimp_devices_display_closed),
                    manager);
}

static void
gimp_devices_display_closed (GdkDisplay        *gdk_display,
                             gboolean           is_error,
                             GimpDeviceManager *manager)
{
}
