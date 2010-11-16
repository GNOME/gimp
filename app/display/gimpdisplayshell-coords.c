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

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "gimpdisplayshell.h"
#include "gimpdisplayshell-coords.h"

#include "core/gimpcoords.h"
#include "core/gimpcoords-interpolate.h"

/* Velocity unit is screen pixels per millisecond we pass to tools as 1. */
#define VELOCITY_UNIT 3.0

#define EVENT_FILL_PRECISION 6.0

#define DIRECTION_RADIUS  (1.5 / MAX (shell->scale_x, shell->scale_y))

#define SMOOTH_FACTOR 0.3


static void gimp_display_shell_interpolate_stroke (GimpDisplayShell *shell,
                                                   GimpCoords       *coords);


static const GimpCoords default_coords = GIMP_COORDS_DEFAULT_VALUES;


/*  public functions  */

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
  gdouble  delta_time  = 0.001;
  gdouble  delta_x     = 0.0;
  gdouble  delta_y     = 0.0;
  gdouble  dir_delta_x = 0.0;
  gdouble  dir_delta_y = 0.0;
  gdouble  distance    = 1.0;
  gboolean event_fill  = (inertia_factor > 0);
  gdouble  delta_pressure = 0.0;

  /* Smoothing causes problems with cursor tracking
   * when zoomed above screen resolution so we need to supress it.
   */
  if (shell->scale_x > 1.0 || shell->scale_y > 1.0)
    {
      inertia_factor = 0.0;
    }


  delta_time = (shell->last_motion_delta_time * (1 - SMOOTH_FACTOR)
                +  (time - shell->last_motion_time) * SMOOTH_FACTOR);

  delta_pressure = coords->pressure - shell->last_coords.pressure;

  /* Try to detect a pen lift */
  if ((delta_time < 50) && (fabs(delta_pressure) > 0.04) && (delta_pressure < 0.0))
    {
      return FALSE;
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
      gdouble delta_dir;
      GimpCoords last_dir_event = shell->last_coords;

      delta_x = last_dir_event.x - coords->x;
      delta_y = last_dir_event.y - coords->y;

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

      if (((fabs (delta_x) > DIRECTION_RADIUS) &&
           (fabs (delta_y) > DIRECTION_RADIUS)) ||
          (shell->event_history->len < 4))
        {
          dir_delta_x = delta_x;
          dir_delta_y = delta_y;
        }
      else
        {
          gint x = 3;

          while (((fabs (dir_delta_x) < DIRECTION_RADIUS) ||
                  (fabs (dir_delta_y) < DIRECTION_RADIUS)) &&
                  (x >= 0))
            {
              last_dir_event = g_array_index (shell->event_history,
                                                        GimpCoords, x);

              dir_delta_x = last_dir_event.x - coords->x;
              dir_delta_y = last_dir_event.y - coords->y;

              x--;
            }
        }

      if ((fabs (dir_delta_x) < DIRECTION_RADIUS) ||
          (fabs (dir_delta_y) < DIRECTION_RADIUS))
        {
          coords->direction = shell->last_coords.direction;
        }
      else
        {
          coords->direction = gimp_coords_direction (&last_dir_event, coords);
        }

      coords->direction = coords->direction - floor (coords->direction);

      delta_dir = coords->direction - shell->last_coords.direction;

      if ((fabs (delta_dir) > 0.5) && (delta_dir < 0.0))
        {
          coords->direction = (0.5 * coords->direction +
                               0.5 * (shell->last_coords.direction - 1.0));
        }
      else if ((fabs (delta_dir) > 0.5) && (delta_dir > 0.0))
        {
          coords->direction = (0.5 * coords->direction +
                               0.5 * (shell->last_coords.direction + 1.0));
        }
      else
        {
          coords->direction = (0.5 * coords->direction +
                               0.5 * shell->last_coords.direction);
        }

      coords->direction = coords->direction - floor (coords->direction);

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

        /* do event fill for devices that do not provide enough events*/
        if (distance >= EVENT_FILL_PRECISION &&
            event_fill                       &&
            shell->event_history->len >= 2)
          {
            if (shell->event_delay)
              {
                gimp_display_shell_interpolate_stroke (shell,
                                                       coords);
              }
            else
              {
                shell->event_delay = TRUE;
                gimp_display_shell_push_event_history (shell, coords);
              }
          }
        else
          {
            if (shell->event_delay)
              {
                shell->event_delay = FALSE;
              }

            gimp_display_shell_push_event_history (shell, coords);
          }

#ifdef VERBOSE
      g_printerr ("DIST: %f, DT:%f, Vel:%f, Press:%f,smooth_dd:%f, sf %f\n",
                  distance,
                  delta_time,
                  shell->last_coords.velocity,
                  coords->pressure,
                  distance - dist,
                  inertia_factor);
#endif
    }

  g_array_append_val (shell->event_queue, *coords);

  shell->last_coords            = *coords;
  shell->last_motion_time       = time;
  shell->last_motion_delta_time = delta_time;
  shell->last_motion_delta_x    = delta_x;
  shell->last_motion_delta_y    = delta_y;
  shell->last_motion_distance   = distance;

  return TRUE;
}


/* Helper function fo managing event history */
void
gimp_display_shell_push_event_history (GimpDisplayShell *shell,
                                       GimpCoords       *coords)
{
   if (shell->event_history->len == 4)
     g_array_remove_index (shell->event_history, 0);

   g_array_append_val (shell->event_history, *coords);
}

static void
gimp_display_shell_interpolate_stroke (GimpDisplayShell *shell,
                                       GimpCoords       *coords)
{
  GArray *ret_coords;
  gint    i = shell->event_history->len - 1;

  /* Note that there must be exactly one event in buffer or bad things
   * can happen. This should never get called under other circumstances.
   */
  ret_coords = g_array_new (FALSE, FALSE, sizeof (GimpCoords));

  gimp_coords_interpolate_catmull (g_array_index (shell->event_history,
                                                  GimpCoords, i - 1),
                                   g_array_index (shell->event_history,
                                                  GimpCoords, i),
                                   g_array_index (shell->event_queue,
                                                  GimpCoords, 0),
                                   *coords,
                                   EVENT_FILL_PRECISION / 2,
                                   &ret_coords,
                                   NULL);

  /* Push the last actual event in history */
  gimp_display_shell_push_event_history (shell,
                                         &g_array_index (shell->event_queue,
                                                         GimpCoords, 0));

  g_array_set_size (shell->event_queue, 0);

  g_array_append_vals (shell->event_queue,
                       &g_array_index (ret_coords, GimpCoords, 0),
                       ret_coords->len);

  g_array_free (ret_coords, TRUE);
}
