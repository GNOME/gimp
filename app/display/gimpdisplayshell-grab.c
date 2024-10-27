/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdisplayshell-grab.c
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

#include "display-types.h"

#include "widgets/gimpdevices.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-grab.h"


#if 0
static GdkDevice *
get_associated_pointer (GdkDevice *device)
{
  switch (gdk_device_get_device_type (device))
    {
    case GDK_DEVICE_TYPE_SLAVE:
      device = gdk_device_get_associated_device (device);
      break;

    case GDK_DEVICE_TYPE_FLOATING:
      {
        GdkDisplay *display = gdk_device_get_display (device);
        GdkSeat    *seat    = gdk_display_get_default_seat (display);

        return gdk_seat_get_pointer (seat);
      }
      break;

    default:
      break;
    }

  if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
    device = gdk_device_get_associated_device (device);

  return device;
}
#endif

gboolean
gimp_display_shell_pointer_grab (GimpDisplayShell *shell,
                                 const GdkEvent   *event,
                                 GdkEventMask      event_mask)
{
  GdkDisplay    *display;
  GdkSeat       *seat;
  GdkDevice     *device;
  GdkDevice     *source_device;
  GdkGrabStatus  status;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  g_return_val_if_fail (shell->grab_seat == NULL, FALSE);

  source_device = gimp_devices_get_from_event (shell->display->gimp,
                                               event, &device);
  display = gdk_device_get_display (device);
  seat    = gdk_display_get_default_seat (display);

  if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
    {
      /* When the source grab event was a keyboard, we want to accept
       * pointer events from any pointing device. This is especially
       * true on Wayland where each pointing device can be really
       * independant whereas on X11, they all appear as "Virtual core
       * pointer".
       */
      device        = NULL;
      source_device = NULL;
    }

  status = gdk_seat_grab (seat,
                          gtk_widget_get_window (shell->canvas),
                          GDK_SEAT_CAPABILITY_ALL_POINTING,
                          FALSE, NULL,
                          event, NULL, NULL);

  if (status == GDK_GRAB_SUCCESS)
    {
      shell->grab_seat           = seat;
      shell->grab_pointer        = device;
      shell->grab_pointer_source = source_device;

      return TRUE;
    }

  g_printerr ("%s: gdk_seat_grab(%s) failed with status %d\n",
              G_STRFUNC, gdk_device_get_name (device), status);

  return FALSE;
}

void
gimp_display_shell_pointer_ungrab (GimpDisplayShell *shell,
                                   const GdkEvent   *event)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (event != NULL);
  g_return_if_fail (shell->grab_seat != NULL);

  gdk_seat_ungrab (shell->grab_seat);

  shell->grab_seat           = NULL;
  shell->grab_pointer        = NULL;
  shell->grab_pointer_source = NULL;
}
