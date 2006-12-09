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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "display-types.h"

#include "gimpdisplayshell.h"
#include "gimpdisplayshell-coords.h"


/*  public functions  */

gboolean
gimp_display_shell_get_event_coords (GimpDisplayShell *shell,
                                     GdkEvent         *event,
                                     GdkDevice        *device,
                                     GimpCoords       *coords)
{
  if (gdk_event_get_axis (event, GDK_AXIS_X, &coords->x))
    {
      gdk_event_get_axis (event, GDK_AXIS_Y, &coords->y);

      /*  CLAMP() the return value of each *_get_axis() call to be safe
       *  against buggy XInput drivers. Provide default values if the
       *  requested axis does not exist.
       */

      if (gdk_event_get_axis (event, GDK_AXIS_PRESSURE, &coords->pressure))
        coords->pressure = CLAMP (coords->pressure, GIMP_COORDS_MIN_PRESSURE,
                                  GIMP_COORDS_MAX_PRESSURE);
      else
        coords->pressure = GIMP_COORDS_DEFAULT_PRESSURE;

      if (gdk_event_get_axis (event, GDK_AXIS_XTILT, &coords->xtilt))
        coords->xtilt = CLAMP (coords->xtilt, GIMP_COORDS_MIN_TILT,
                               GIMP_COORDS_MAX_TILT);
      else
        coords->xtilt = GIMP_COORDS_DEFAULT_TILT;

      if (gdk_event_get_axis (event, GDK_AXIS_YTILT, &coords->ytilt))
        coords->ytilt = CLAMP (coords->ytilt, GIMP_COORDS_MIN_TILT,
                               GIMP_COORDS_MAX_TILT);
      else
        coords->ytilt = GIMP_COORDS_DEFAULT_TILT;

      if (gdk_event_get_axis (event, GDK_AXIS_WHEEL, &coords->wheel))
        coords->wheel = CLAMP (coords->wheel, GIMP_COORDS_MIN_WHEEL,
                               GIMP_COORDS_MAX_WHEEL);
      else
        coords->wheel = GIMP_COORDS_DEFAULT_WHEEL;

      return TRUE;
    }

  gimp_display_shell_get_device_coords (shell, device, coords);

  return FALSE;
}

void
gimp_display_shell_get_device_coords (GimpDisplayShell *shell,
                                      GdkDevice        *device,
                                      GimpCoords       *coords)
{
  gdouble axes[GDK_AXIS_LAST];

  gdk_device_get_state (device, shell->canvas->window, axes, NULL);

  gdk_device_get_axis (device, axes, GDK_AXIS_X, &coords->x);
  gdk_device_get_axis (device, axes, GDK_AXIS_Y, &coords->y);

  /*  CLAMP() the return value of each *_get_axis() call to be safe
   *  against buggy XInput drivers. Provide default values if the
   *  requested axis does not exist.
   */

  if (gdk_device_get_axis (device, axes, GDK_AXIS_PRESSURE, &coords->pressure))
    coords->pressure = CLAMP (coords->pressure, GIMP_COORDS_MIN_PRESSURE,
                              GIMP_COORDS_MAX_PRESSURE);
  else
    coords->pressure = GIMP_COORDS_DEFAULT_PRESSURE;

  if (gdk_device_get_axis (device, axes, GDK_AXIS_XTILT, &coords->xtilt))
    coords->xtilt = CLAMP (coords->xtilt, GIMP_COORDS_MIN_TILT,
                           GIMP_COORDS_MAX_TILT);
  else
    coords->xtilt = GIMP_COORDS_DEFAULT_TILT;

  if (gdk_device_get_axis (device, axes, GDK_AXIS_YTILT, &coords->ytilt))
    coords->ytilt = CLAMP (coords->ytilt, GIMP_COORDS_MIN_TILT,
                           GIMP_COORDS_MAX_TILT);
  else
    coords->ytilt = GIMP_COORDS_DEFAULT_TILT;

  if (gdk_device_get_axis (device, axes, GDK_AXIS_WHEEL, &coords->wheel))
    coords->wheel = CLAMP (coords->wheel, GIMP_COORDS_MIN_WHEEL,
                           GIMP_COORDS_MAX_WHEEL);
  else
    coords->wheel = GIMP_COORDS_DEFAULT_WHEEL;
}

void
gimp_display_shell_get_time_coords (GimpDisplayShell *shell,
                                    GdkDevice        *device,
                                    GdkTimeCoord     *event,
                                    GimpCoords       *coords)
{
  gdk_device_get_axis (device, event->axes, GDK_AXIS_X, &coords->x);
  gdk_device_get_axis (device, event->axes, GDK_AXIS_Y, &coords->y);

  /*  CLAMP() the return value of each *_get_axis() call to be safe
   *  against buggy XInput drivers. Provide default values if the
   *  requested axis does not exist.
   */

  if (gdk_device_get_axis (device, event->axes, GDK_AXIS_PRESSURE, &coords->pressure))
    coords->pressure = CLAMP (coords->pressure, GIMP_COORDS_MIN_PRESSURE,
                              GIMP_COORDS_MAX_PRESSURE);
  else
    coords->pressure = GIMP_COORDS_DEFAULT_PRESSURE;

  if (gdk_device_get_axis (device, event->axes, GDK_AXIS_XTILT, &coords->xtilt))
    coords->xtilt = CLAMP (coords->xtilt, GIMP_COORDS_MIN_TILT,
                           GIMP_COORDS_MAX_TILT);
  else
    coords->xtilt = GIMP_COORDS_DEFAULT_TILT;

  if (gdk_device_get_axis (device, event->axes, GDK_AXIS_YTILT, &coords->ytilt))
    coords->ytilt = CLAMP (coords->ytilt, GIMP_COORDS_MIN_TILT,
                           GIMP_COORDS_MAX_TILT);
  else
    coords->ytilt = GIMP_COORDS_DEFAULT_TILT;

  if (gdk_device_get_axis (device, event->axes, GDK_AXIS_WHEEL, &coords->wheel))
    coords->wheel = CLAMP (coords->wheel, GIMP_COORDS_MIN_WHEEL,
                           GIMP_COORDS_MAX_WHEEL);
  else
    coords->wheel = GIMP_COORDS_DEFAULT_WHEEL;
}

gboolean
gimp_display_shell_get_event_state (GimpDisplayShell *shell,
                                    GdkEvent         *event,
                                    GdkDevice        *device,
                                    GdkModifierType  *state)
{
  if (gdk_event_get_state (event, state))
    return TRUE;

  gimp_display_shell_get_device_state (shell, device, state);

  return FALSE;
}

void
gimp_display_shell_get_device_state (GimpDisplayShell *shell,
                                     GdkDevice        *device,
                                     GdkModifierType  *state)
{
  gdk_device_get_state (device, shell->canvas->window, NULL, state);
}
