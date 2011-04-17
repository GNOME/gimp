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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "core/gimpcoords.h"
#include "core/gimpcoords-interpolate.h"
#include "core/gimpmarshal.h"

#include "gimpmotionbuffer.h"


/* Velocity unit is screen pixels per millisecond we pass to tools as 1. */
#define VELOCITY_UNIT        3.0
#define EVENT_FILL_PRECISION 6.0
#define DIRECTION_RADIUS     (1.5 / MAX (scale_x, scale_y))
#define SMOOTH_FACTOR        0.3


enum
{
  PROP_0
};

enum
{
  MOTION,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void   gimp_motion_buffer_constructed        (GObject          *object);
static void   gimp_motion_buffer_dispose            (GObject          *object);
static void   gimp_motion_buffer_finalize           (GObject          *object);
static void   gimp_motion_buffer_set_property       (GObject          *object,
                                                     guint             property_id,
                                                     const GValue     *value,
                                                     GParamSpec       *pspec);
static void   gimp_motion_buffer_get_property       (GObject          *object,
                                                     guint             property_id,
                                                     GValue           *value,
                                                     GParamSpec       *pspec);

static void   gimp_motion_buffer_interpolate_stroke (GimpMotionBuffer *buffer,
                                                     GimpCoords       *coords);


G_DEFINE_TYPE (GimpMotionBuffer, gimp_motion_buffer, GIMP_TYPE_OBJECT)

#define parent_class gimp_motion_buffer_parent_class

static guint motion_buffer_signals[LAST_SIGNAL] = { 0 };

static const GimpCoords default_coords = GIMP_COORDS_DEFAULT_VALUES;


static void
gimp_motion_buffer_class_init (GimpMotionBufferClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  motion_buffer_signals[MOTION] =
    g_signal_new ("motion",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpMotionBufferClass, motion),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->constructed  = gimp_motion_buffer_constructed;
  object_class->dispose      = gimp_motion_buffer_dispose;
  object_class->finalize     = gimp_motion_buffer_finalize;
  object_class->set_property = gimp_motion_buffer_set_property;
  object_class->get_property = gimp_motion_buffer_get_property;
}

static void
gimp_motion_buffer_init (GimpMotionBuffer *buffer)
{
  buffer->event_history = g_array_new (FALSE, FALSE, sizeof (GimpCoords));
  buffer->event_queue   = g_array_new (FALSE, FALSE, sizeof (GimpCoords));
}

static void
gimp_motion_buffer_constructed (GObject *object)
{
  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);
}

static void
gimp_motion_buffer_dispose (GObject *object)
{
  GimpMotionBuffer *buffer = GIMP_MOTION_BUFFER (object);

  if (buffer->event_history)
    {
      g_array_free (buffer->event_history, TRUE);
      buffer->event_history = NULL;
    }

  if (buffer->event_queue)
    {
      g_array_free (buffer->event_queue, TRUE);
      buffer->event_queue = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_motion_buffer_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_motion_buffer_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_motion_buffer_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

GimpMotionBuffer *
gimp_motion_buffer_new (void)
{
  return g_object_new (GIMP_TYPE_MOTION_BUFFER,
                       NULL);
}

/**
 * gimp_display_shell_eval_event:
 * @shell:
 * @coords:
 * @inertia_factor:
 * @time:
 *
 * This function evaluates the event to decide if the change is big
 * enough to need handling and returns FALSE, if change is less than
 * set filter level taking a whole lot of load off any draw tools that
 * have no use for these events anyway. If the event is seen fit at
 * first look, it is evaluated for speed and smoothed.  Due to lousy
 * time resolution of events pretty strong smoothing is applied to
 * timestamps for sensible speed result. This function is also ideal
 * for other event adjustment like pressure curve or calculating other
 * derived dynamics factors like angular velocity calculation from
 * tilt values, to allow for even more dynamic brushes. Calculated
 * distance to last event is stored in GimpCoords because its a
 * sideproduct of velocity calculation and is currently calculated in
 * each tool. If they were to use this distance, more resouces on
 * recalculating the same value would be saved.
 *
 * Return value:
 **/
gboolean
gimp_motion_buffer_eval_event (GimpMotionBuffer *buffer,
                               gdouble           scale_x,
                               gdouble           scale_y,
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
  gboolean event_fill  = (inertia_factor > 0.0);

  /*  Smoothing causes problems with cursor tracking when zoomed above
   *  screen resolution so we need to supress it.
   */
  if (scale_x > 1.0 || scale_y > 1.0)
    {
      inertia_factor = 0.0;
    }

  delta_time = (buffer->last_motion_delta_time * (1 - SMOOTH_FACTOR) +
                (time - buffer->last_motion_time) * SMOOTH_FACTOR);

  if (buffer->last_motion_time == 0)
    {
      /*  First pair is invalid to do any velocity calculation, so we
       *  apply a constant value.
       */
      coords->velocity = 1.0;
    }
  else
    {
      GimpCoords last_dir_event = buffer->last_coords;
      gdouble    filter;
      gdouble    dist;
      gdouble    delta_dir;

      delta_x = last_dir_event.x - coords->x;
      delta_y = last_dir_event.y - coords->y;

      /*  Events with distances less than the screen resolution are
       *  not worth handling.
       */
      filter = MIN (1 / scale_x, 1 / scale_y) / 2.0;

      if (fabs (delta_x) < filter && fabs (delta_y) < filter)
        return FALSE;

      distance = dist = sqrt (SQR (delta_x) + SQR (delta_y));

      /*  If even smoothed time resolution does not allow to guess for
       *  speed, use last velocity.
       */
      if (delta_time == 0)
        {
          coords->velocity = buffer->last_coords.velocity;
        }
      else
        {
          /*  We need to calculate the velocity in screen coordinates
           *  for human interaction
           */
          gdouble screen_distance = (distance * MIN (scale_x, scale_y));

          /* Calculate raw valocity */
          coords->velocity = ((screen_distance / delta_time) / VELOCITY_UNIT);

          /* Adding velocity dependent smoothing, feels better in tools. */
          coords->velocity = (buffer->last_coords.velocity *
                              (1 - MIN (SMOOTH_FACTOR, coords->velocity)) +
                              coords->velocity *
                              MIN (SMOOTH_FACTOR, coords->velocity));

          /* Speed needs upper limit */
          coords->velocity = MIN (coords->velocity, 1.0);
        }

      if (((fabs (delta_x) > DIRECTION_RADIUS) &&
           (fabs (delta_y) > DIRECTION_RADIUS)) ||
          (buffer->event_history->len < 4))
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
              last_dir_event = g_array_index (buffer->event_history,
                                              GimpCoords, x);

              dir_delta_x = last_dir_event.x - coords->x;
              dir_delta_y = last_dir_event.y - coords->y;

              x--;
            }
        }

      if ((fabs (dir_delta_x) < DIRECTION_RADIUS) ||
          (fabs (dir_delta_y) < DIRECTION_RADIUS))
        {
          coords->direction = buffer->last_coords.direction;
        }
      else
        {
          coords->direction = gimp_coords_direction (&last_dir_event, coords);
        }

      coords->direction = coords->direction - floor (coords->direction);

      delta_dir = coords->direction - buffer->last_coords.direction;

      if ((fabs (delta_dir) > 0.5) && (delta_dir < 0.0))
        {
          coords->direction = (0.5 * coords->direction +
                               0.5 * (buffer->last_coords.direction - 1.0));
        }
      else if ((fabs (delta_dir) > 0.5) && (delta_dir > 0.0))
        {
          coords->direction = (0.5 * coords->direction +
                               0.5 * (buffer->last_coords.direction + 1.0));
        }
      else
        {
          coords->direction = (0.5 * coords->direction +
                               0.5 * buffer->last_coords.direction);
        }

      coords->direction = coords->direction - floor (coords->direction);

      /* do event fill for devices that do not provide enough events */
      if (distance >= EVENT_FILL_PRECISION &&
          event_fill                       &&
          buffer->event_history->len >= 2)
        {
          if (buffer->event_delay)
            {
              gimp_motion_buffer_interpolate_stroke (buffer, coords);
            }
          else
            {
              buffer->event_delay = TRUE;
              gimp_motion_buffer_push_event_history (buffer, coords);
            }
        }
      else
        {
          if (buffer->event_delay)
            buffer->event_delay = FALSE;

          gimp_motion_buffer_push_event_history (buffer, coords);
        }

#ifdef EVENT_VERBOSE
      g_printerr ("DIST: %f, DT:%f, Vel:%f, Press:%f,smooth_dd:%f, POS: (%f, %f)\n",
                  distance,
                  delta_time,
                  buffer->last_coords.velocity,
                  coords->pressure,
                  distance - dist,
                  coords->x,
                  coords->y);
#endif
    }

  g_array_append_val (buffer->event_queue, *coords);

  buffer->last_coords            = *coords;
  buffer->last_motion_time       = time;
  buffer->last_motion_delta_time = delta_time;
  buffer->last_motion_delta_x    = delta_x;
  buffer->last_motion_delta_y    = delta_y;
  buffer->last_motion_distance   = distance;

  return TRUE;
}

void
gimp_motion_buffer_push_event_history (GimpMotionBuffer *buffer,
                                       GimpCoords       *coords)
{
  if (buffer->event_history->len == 4)
    g_array_remove_index (buffer->event_history, 0);

  g_array_append_val (buffer->event_history, *coords);
}


/*  private functions  */

static void
gimp_motion_buffer_interpolate_stroke (GimpMotionBuffer *buffer,
                                       GimpCoords       *coords)
{
  GArray *ret_coords;
  gint    i = buffer->event_history->len - 1;

  /* Note that there must be exactly one event in buffer or bad things
   * can happen. This must never get called under other circumstances.
   */
  ret_coords = g_array_new (FALSE, FALSE, sizeof (GimpCoords));

  gimp_coords_interpolate_catmull (g_array_index (buffer->event_history,
                                                  GimpCoords, i - 1),
                                   g_array_index (buffer->event_history,
                                                  GimpCoords, i),
                                   g_array_index (buffer->event_queue,
                                                  GimpCoords, 0),
                                   *coords,
                                   EVENT_FILL_PRECISION / 2,
                                   &ret_coords,
                                   NULL);

  /* Push the last actual event in history */
  gimp_motion_buffer_push_event_history (buffer,
                                         &g_array_index (buffer->event_queue,
                                                         GimpCoords, 0));

  g_array_set_size (buffer->event_queue, 0);

  g_array_append_vals (buffer->event_queue,
                       &g_array_index (ret_coords, GimpCoords, 0),
                       ret_coords->len);

  g_array_free (ret_coords, TRUE);
}
