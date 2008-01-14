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

#include "libgimpmath/gimpmath.h"

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

  if (gdk_device_get_axis (device,
                           event->axes, GDK_AXIS_PRESSURE, &coords->pressure))
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

/**
 * gimp_display_shell_eval_event:
 * @shell:
 * @coords:
 * @inertia_factor:
 * @time:
 *
 * This function evaluates the event to decide if the change is
 * big enough to need handling and returns FALSE, if change is less
 * than one image pixel or when smoothed event distance covers less
 * than one pixel taking a whole lot of load off any draw tools that
 * have no use for these sub-pixel events anyway. If the event is
 * seen fit at first look, it is evaluated for speed and smoothed.
 * Due to lousy time resolution of events pretty strong smoothing is
 * applied to timestamps for sensible speed result. This function is
 * also ideal for other event adjustment like pressure curve or
 * calculating other derived dynamics factors like angular velocity
 * calculation from tilt values, to allow for even more dynamic
 * brushes. Calculated distance to last event is stored in GimpCoords
 * because its a sideproduct of velocity calculation and is currently
 * calculated in each tool. If they were to use this distance, more
 * resouces on recalculating the same value would be saved.
 *
 * Return value:
 **/
gboolean
gimp_display_shell_eval_event (GimpDisplayShell *shell,
                               GimpCoords       *coords,
			       gdouble           inertia_factor,
			       guint32           time)
{
  guint32        thistime      = time;
  gdouble        dist;
  gdouble        xy_sfactor    = 0;
  const gdouble  smooth_factor = 0.3;

  if (shell->last_disp_motion_time == 0)
    {
      /* First pair is invalid to do any velocity calculation,
       * so we apply constant values.
       */
      coords->velocity   = 100;
      coords->delta_time = 0.001;
      coords->distance   = 0;

      shell->last_coords = *coords;
    }
  else
    {
      gdouble dx = coords->delta_x = shell->last_coords.x - coords->x;
      gdouble dy = coords->delta_y = shell->last_coords.y - coords->y;

      /* Events with distances less than 1 in either motion direction
       * are not worth handling.
       */
      if (fabs (dx) < 1.0 && fabs (dy) < 1.0)
        return FALSE;

      coords->delta_time = thistime - shell->last_disp_motion_time;
      coords->delta_time = (shell->last_coords.delta_time * (1 - smooth_factor)
                            + coords->delta_time * smooth_factor);
      coords->distance = dist = sqrt (SQR (dx) + SQR (dy));

      /* If even smoothed time resolution does not allow to guess for speed,
       * use last velocity.
       */
      if ((coords->delta_time == 0))
        {
          coords->velocity = shell->last_coords.velocity;
        }
      else
        {

          coords->velocity = (coords->distance / (gdouble) coords->delta_time) / 10;

          /* A little smooth on this too, feels better in tools this way. */
          coords->velocity = (shell->last_coords.velocity * (1 - smooth_factor)
                              +  coords->velocity * smooth_factor);
	  /* Speed needs upper limit */
          coords->velocity=MIN(coords->velocity,1.0);
        }

      if (inertia_factor > 0)
        {
          /* Apply smoothing to X and Y.
           *
           * The very high velocity values (yes, thats around 25
           * according to tests) happen at great zoom outs and scream
           * for heavyhanded smooth so I made it automatic. This
           * should be bound to zoom level once theres a GUI foob to
           * adjust smooth or disabled completely.
           */

           /*Apply smoothing to X and Y.
             Smooth is dampened for high speeds to minimize the whip effect.*/
           xy_sfactor = inertia_factor - inertia_factor * fabs (coords->velocity);
           coords->x = shell->last_coords.x - (shell->last_coords.delta_x * xy_sfactor
                                               + coords->delta_x * (1 - xy_sfactor));
           coords->y = shell->last_coords.y - (shell->last_coords.delta_y * xy_sfactor
                                               + coords->delta_y * (1 - xy_sfactor));

           /* Recalculate distance */
           coords->distance = sqrt ((shell->last_coords.x - coords->x) *
                                    (shell->last_coords.x - coords->x) +
                                    (shell->last_coords.y - coords->y) *
                                    (shell->last_coords.y - coords->y));

        }

#ifdef VERBOSE
      g_printerr ("DIST: %f, DT:%f, Vel:%f, Press:%f,smooth_dd:%f, sf %f\n",
                  coords->distance,
                  coords->delta_time,
                  shell->last_coords.velocity,
                  coords->pressure,
                  coords->distance - dist,
                  xy_sfactor);
#endif

    }

  return TRUE;
}
