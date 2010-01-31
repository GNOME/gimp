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

/* Velocity unit is screen pixels per millisecond we pass to tools as 1. */
#define VELOCITY_UNIT 3.0

static const GimpCoords default_coords = GIMP_COORDS_DEFAULT_VALUES;


/*  public functions  */

gboolean
gimp_display_shell_get_event_coords (GimpDisplayShell *shell,
                                     GdkEvent         *event,
                                     GdkDevice        *device,
                                     GimpCoords       *coords)
{
  gdouble x;

  if (gdk_event_get_axis (event, GDK_AXIS_X, &x))
    {
      *coords = default_coords;

      coords->x = x;
      gdk_event_get_axis (event, GDK_AXIS_Y, &coords->y);

      /*  CLAMP() the return value of each *_get_axis() call to be safe
       *  against buggy XInput drivers.
       */

      if (gdk_event_get_axis (event, GDK_AXIS_PRESSURE, &coords->pressure))
        coords->pressure = CLAMP (coords->pressure, GIMP_COORDS_MIN_PRESSURE,
                                  GIMP_COORDS_MAX_PRESSURE);

      if (gdk_event_get_axis (event, GDK_AXIS_XTILT, &coords->xtilt))
        coords->xtilt = CLAMP (coords->xtilt, GIMP_COORDS_MIN_TILT,
                               GIMP_COORDS_MAX_TILT);

      if (gdk_event_get_axis (event, GDK_AXIS_YTILT, &coords->ytilt))
        coords->ytilt = CLAMP (coords->ytilt, GIMP_COORDS_MIN_TILT,
                               GIMP_COORDS_MAX_TILT);

      if (gdk_event_get_axis (event, GDK_AXIS_WHEEL, &coords->wheel))
        coords->wheel = CLAMP (coords->wheel, GIMP_COORDS_MIN_WHEEL,
                               GIMP_COORDS_MAX_WHEEL);

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

  *coords = default_coords;

  gdk_device_get_state (device, shell->canvas->window, axes, NULL);

  gdk_device_get_axis (device, axes, GDK_AXIS_X, &coords->x);
  gdk_device_get_axis (device, axes, GDK_AXIS_Y, &coords->y);

  /*  CLAMP() the return value of each *_get_axis() call to be safe
   *  against buggy XInput drivers.
   */

  if (gdk_device_get_axis (device, axes, GDK_AXIS_PRESSURE, &coords->pressure))
    coords->pressure = CLAMP (coords->pressure, GIMP_COORDS_MIN_PRESSURE,
                              GIMP_COORDS_MAX_PRESSURE);

  if (gdk_device_get_axis (device, axes, GDK_AXIS_XTILT, &coords->xtilt))
    coords->xtilt = CLAMP (coords->xtilt, GIMP_COORDS_MIN_TILT,
                           GIMP_COORDS_MAX_TILT);

  if (gdk_device_get_axis (device, axes, GDK_AXIS_YTILT, &coords->ytilt))
    coords->ytilt = CLAMP (coords->ytilt, GIMP_COORDS_MIN_TILT,
                           GIMP_COORDS_MAX_TILT);

  if (gdk_device_get_axis (device, axes, GDK_AXIS_WHEEL, &coords->wheel))
    coords->wheel = CLAMP (coords->wheel, GIMP_COORDS_MIN_WHEEL,
                           GIMP_COORDS_MAX_WHEEL);
}

void
gimp_display_shell_get_time_coords (GimpDisplayShell *shell,
                                    GdkDevice        *device,
                                    GdkTimeCoord     *event,
                                    GimpCoords       *coords)
{
  *coords = default_coords;

  gdk_device_get_axis (device, event->axes, GDK_AXIS_X, &coords->x);
  gdk_device_get_axis (device, event->axes, GDK_AXIS_Y, &coords->y);

  /*  CLAMP() the return value of each *_get_axis() call to be safe
   *  against buggy XInput drivers.
   */

  if (gdk_device_get_axis (device,
                           event->axes, GDK_AXIS_PRESSURE, &coords->pressure))
    coords->pressure = CLAMP (coords->pressure, GIMP_COORDS_MIN_PRESSURE,
                              GIMP_COORDS_MAX_PRESSURE);

  if (gdk_device_get_axis (device, event->axes, GDK_AXIS_XTILT, &coords->xtilt))
    coords->xtilt = CLAMP (coords->xtilt, GIMP_COORDS_MIN_TILT,
                           GIMP_COORDS_MAX_TILT);

  if (gdk_device_get_axis (device, event->axes, GDK_AXIS_YTILT, &coords->ytilt))
    coords->ytilt = CLAMP (coords->ytilt, GIMP_COORDS_MIN_TILT,
                           GIMP_COORDS_MAX_TILT);

  if (gdk_device_get_axis (device, event->axes, GDK_AXIS_WHEEL, &coords->wheel))
    coords->wheel = CLAMP (coords->wheel, GIMP_COORDS_MIN_WHEEL,
                           GIMP_COORDS_MAX_WHEEL);
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
 * than set filter level taking a whole lot of load off any draw tools
 * that have no use for these events anyway. If the event is
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
  gdouble delta_time = 0.001;
  gdouble delta_x    = 0.0;
  gdouble delta_y    = 0.0;
  gdouble distance   = 1.0;

  /* Smoothing causes problems with cursor tracking
   * when zoomed above screen resolution so we need to supress it.
   */
  if (shell->scale_x > 1.0 || shell->scale_y > 1.0)
    {
      inertia_factor = 0.0;
    }

  if (shell->last_motion_time == 0)
    {
      /* First pair is invalid to do any velocity calculation,
       * so we apply a constant value.
       */
      coords->velocity = 1.0;
    }
  else
    {
      gdouble filter;
      gdouble dist;

      delta_x = shell->last_coords.x - coords->x;
      delta_y = shell->last_coords.y - coords->y;

#define SMOOTH_FACTOR 0.3

      /* Events with distances less than the screen resolution are not
       * worth handling.
       */
      filter = MIN (1 / shell->scale_x, 1 / shell->scale_y) / 2.0;

      if (fabs (delta_x) < filter && fabs (delta_y) < filter)
        return FALSE;

      delta_time = (shell->last_motion_delta_time * (1 - SMOOTH_FACTOR)
                    +  (time - shell->last_motion_time) * SMOOTH_FACTOR);

      distance = dist = sqrt (SQR (delta_x) + SQR (delta_y));

      /* If even smoothed time resolution does not allow to guess for speed,
       * use last velocity.
       */
      if (delta_time == 0)
        {
          coords->velocity = shell->last_coords.velocity;
        }
      else
        {
          /* We need to calculate the velocity in screen coordinates
           * for human interaction
           */
          gdouble screen_distance = (distance *
                                     MIN (shell->scale_x, shell->scale_y));

          /* Calculate raw valocity */
          coords->velocity = ((screen_distance / delta_time) / VELOCITY_UNIT);

          /* Adding velocity dependent smoothing, feels better in tools. */
          coords->velocity = (shell->last_coords.velocity *
                              (1 - MIN (SMOOTH_FACTOR, coords->velocity)) +
                              coords->velocity *
                              MIN (SMOOTH_FACTOR, coords->velocity));

          /* Speed needs upper limit */
          coords->velocity = MIN (coords->velocity, 1.0);
        }

      /* High speed -> less smooth*/
      inertia_factor *= (1 - coords->velocity);

      if (inertia_factor > 0 && distance > 0)
        {
          /* Apply smoothing to X and Y. */

          /* This tells how far from the pointer can stray from the line */
          gdouble max_deviation = SQR (20 * inertia_factor);
          gdouble cur_deviation = max_deviation;
          gdouble sin_avg;
          gdouble sin_old;
          gdouble sin_new;
          gdouble cos_avg;
          gdouble cos_old;
          gdouble cos_new;
          gdouble new_x;
          gdouble new_y;

          sin_new = delta_x / distance;
          sin_old = shell->last_motion_delta_x / shell->last_motion_distance;
          sin_avg = sin (asin (sin_old) * inertia_factor +
                         asin (sin_new) * (1 - inertia_factor));

          cos_new = delta_y / distance;
          cos_old = shell->last_motion_delta_y / shell->last_motion_distance;
          cos_avg = cos (acos (cos_old) * inertia_factor +
                         acos (cos_new) * (1 - inertia_factor));

          delta_x = sin_avg * distance;
          delta_y = cos_avg * distance;

          new_x = (shell->last_coords.x - delta_x) * 0.5 + coords->x * 0.5;
          new_y = (shell->last_coords.y - delta_y) * 0.5 + coords->y * 0.5;

          cur_deviation = SQR (coords->x - new_x) + SQR (coords->y - new_y);

          while (cur_deviation >= max_deviation)
            {
              new_x = new_x * 0.8 + coords->x * 0.2;
              new_y = new_y * 0.8 + coords->y * 0.2;

              cur_deviation = (SQR (coords->x - new_x) +
                               SQR (coords->y - new_y));
            }

          coords->x = new_x;
          coords->y = new_y;

          delta_x = shell->last_coords.x - coords->x;
          delta_y = shell->last_coords.y - coords->y;

          /* Recalculate distance */
          distance = sqrt (SQR (delta_x) + SQR (delta_y));
        }

#ifdef VERBOSE
      g_printerr ("DIST: %f, DT:%f, Vel:%f, Press:%f,smooth_dd:%f, sf %f\n",
                  distance,
                  delta_time,
                  shell->last_coords.velocity,
                  coords->pressure,
                  coords->distance - dist,
                  inertia_factor);
#endif
    }

  shell->last_coords            = *coords;

  shell->last_motion_time       = time;
  shell->last_motion_delta_time = delta_time;
  shell->last_motion_delta_x    = delta_x;
  shell->last_motion_delta_y    = delta_y;
  shell->last_motion_distance   = distance;

  return TRUE;
}
