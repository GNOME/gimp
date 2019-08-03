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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#define DIRECTION_RADIUS     (1.0 / MAX (scale_x, scale_y))
#define SMOOTH_FACTOR        0.3


enum
{
  PROP_0
};

enum
{
  STROKE,
  HOVER,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void     gimp_motion_buffer_dispose             (GObject          *object);
static void     gimp_motion_buffer_finalize            (GObject          *object);
static void     gimp_motion_buffer_set_property        (GObject          *object,
                                                        guint             property_id,
                                                        const GValue     *value,
                                                        GParamSpec       *pspec);
static void     gimp_motion_buffer_get_property        (GObject          *object,
                                                        guint             property_id,
                                                        GValue           *value,
                                                        GParamSpec       *pspec);

static void     gimp_motion_buffer_push_event_history  (GimpMotionBuffer *buffer,
                                                        const GimpCoords *coords);
static void     gimp_motion_buffer_pop_event_queue     (GimpMotionBuffer *buffer,
                                                        GimpCoords       *coords);

static void     gimp_motion_buffer_interpolate_stroke  (GimpMotionBuffer *buffer,
                                                        GimpCoords       *coords);
static gboolean gimp_motion_buffer_event_queue_timeout (GimpMotionBuffer *buffer);


G_DEFINE_TYPE (GimpMotionBuffer, gimp_motion_buffer, GIMP_TYPE_OBJECT)

#define parent_class gimp_motion_buffer_parent_class

static guint motion_buffer_signals[LAST_SIGNAL] = { 0 };


static void
gimp_motion_buffer_class_init (GimpMotionBufferClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  motion_buffer_signals[STROKE] =
    g_signal_new ("stroke",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpMotionBufferClass, stroke),
                  NULL, NULL,
                  gimp_marshal_VOID__POINTER_UINT_FLAGS,
                  G_TYPE_NONE, 3,
                  G_TYPE_POINTER,
                  G_TYPE_UINT,
                  GDK_TYPE_MODIFIER_TYPE);

  motion_buffer_signals[HOVER] =
    g_signal_new ("hover",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpMotionBufferClass, hover),
                  NULL, NULL,
                  gimp_marshal_VOID__POINTER_FLAGS_BOOLEAN,
                  G_TYPE_NONE, 3,
                  G_TYPE_POINTER,
                  GDK_TYPE_MODIFIER_TYPE,
                  G_TYPE_BOOLEAN);

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
gimp_motion_buffer_dispose (GObject *object)
{
  GimpMotionBuffer *buffer = GIMP_MOTION_BUFFER (object);

  if (buffer->event_delay_timeout)
    {
      g_source_remove (buffer->event_delay_timeout);
      buffer->event_delay_timeout = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_motion_buffer_finalize (GObject *object)
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

void
gimp_motion_buffer_begin_stroke (GimpMotionBuffer *buffer,
                                 guint32           time,
                                 GimpCoords       *last_motion)
{
  g_return_if_fail (GIMP_IS_MOTION_BUFFER (buffer));
  g_return_if_fail (last_motion != NULL);

  buffer->last_read_motion_time = time;

  *last_motion = buffer->last_coords;
}

void
gimp_motion_buffer_end_stroke (GimpMotionBuffer *buffer)
{
  g_return_if_fail (GIMP_IS_MOTION_BUFFER (buffer));

  if (buffer->event_delay_timeout)
    {
      g_source_remove (buffer->event_delay_timeout);
      buffer->event_delay_timeout = 0;
    }

  gimp_motion_buffer_event_queue_timeout (buffer);
}

/**
 * gimp_motion_buffer_motion_event:
 * @buffer:
 * @coords:
 * @time:
 * @event_fill:
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
 * each tool. If they were to use this distance, more resources on
 * recalculating the same value would be saved.
 *
 * Returns: %TRUE if the motion was significant enough to be
 *               processed, %FALSE otherwise.
 **/
gboolean
gimp_motion_buffer_motion_event (GimpMotionBuffer *buffer,
                                 GimpCoords       *coords,
                                 guint32           time,
                                 gboolean          event_fill)
{
  gdouble  delta_time  = 0.001;
  gdouble  delta_x     = 0.0;
  gdouble  delta_y     = 0.0;
  gdouble  distance    = 1.0;
  gdouble  scale_x     = coords->xscale;
  gdouble  scale_y     = coords->yscale;

  g_return_val_if_fail (GIMP_IS_MOTION_BUFFER (buffer), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);

  /*  the last_read_motion_time most be set unconditionally, so set
   *  it early
   */
  buffer->last_read_motion_time = time;

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
      gdouble    dir_delta_x = 0.0;
      gdouble    dir_delta_y = 0.0;

      delta_x = last_dir_event.x - coords->x;
      delta_y = last_dir_event.y - coords->y;

      /*  Events with distances less than the screen resolution are
       *  not worth handling.
       */
      filter = MIN (1.0 / scale_x, 1.0 / scale_y) / 2.0;

      if (fabs (delta_x) < filter &&
          fabs (delta_y) < filter)
        {
          return FALSE;
        }

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
          gint x = CLAMP ((buffer->event_history->len - 1), 3, 15);

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

      if (delta_dir < -0.5)
        {
          coords->direction = (0.5 * coords->direction +
                               0.5 * (buffer->last_coords.direction - 1.0));
        }
      else if (delta_dir > 0.5)
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

guint32
gimp_motion_buffer_get_last_motion_time (GimpMotionBuffer *buffer)
{
  g_return_val_if_fail (GIMP_IS_MOTION_BUFFER (buffer), 0);

  return buffer->last_read_motion_time;
}

void
gimp_motion_buffer_request_stroke (GimpMotionBuffer *buffer,
                                   GdkModifierType   state,
                                   guint32           time)
{
  GdkModifierType  event_state;
  gint             keep = 0;

  g_return_if_fail (GIMP_IS_MOTION_BUFFER (buffer));

  if (buffer->event_delay)
    {
      /* If we are in delay we use LAST state, not current */
      event_state = buffer->last_active_state;

      keep = 1; /* Holding one event in buf */
    }
  else
    {
      /* Save the state */
      event_state = state;
    }

  if (buffer->event_delay_timeout)
    {
      g_source_remove (buffer->event_delay_timeout);
      buffer->event_delay_timeout = 0;
    }

  buffer->last_active_state = state;

  while (buffer->event_queue->len > keep)
    {
      GimpCoords buf_coords;

      gimp_motion_buffer_pop_event_queue (buffer, &buf_coords);

      g_signal_emit (buffer, motion_buffer_signals[STROKE], 0,
                     &buf_coords, time, event_state);
    }

  if (buffer->event_delay)
    {
      buffer->event_delay_timeout =
        g_timeout_add (50,
                       (GSourceFunc) gimp_motion_buffer_event_queue_timeout,
                       buffer);
    }
}

void
gimp_motion_buffer_request_hover (GimpMotionBuffer *buffer,
                                  GdkModifierType   state,
                                  gboolean          proximity)
{
  g_return_if_fail (GIMP_IS_MOTION_BUFFER (buffer));

  if (buffer->event_queue->len > 0)
    {
      GimpCoords buf_coords = g_array_index (buffer->event_queue,
                                             GimpCoords,
                                             buffer->event_queue->len - 1);

      g_signal_emit (buffer, motion_buffer_signals[HOVER], 0,
                     &buf_coords, state, proximity);

      g_array_set_size (buffer->event_queue, 0);
    }
}


/*  private functions  */

static void
gimp_motion_buffer_push_event_history (GimpMotionBuffer *buffer,
                                       const GimpCoords *coords)
{
  if (buffer->event_history->len == 4)
    g_array_remove_index (buffer->event_history, 0);

  g_array_append_val (buffer->event_history, *coords);
}

static void
gimp_motion_buffer_pop_event_queue (GimpMotionBuffer *buffer,
                                    GimpCoords       *coords)
{
  *coords = g_array_index (buffer->event_queue, GimpCoords, 0);

  g_array_remove_index (buffer->event_queue, 0);
}

static void
gimp_motion_buffer_interpolate_stroke (GimpMotionBuffer *buffer,
                                       GimpCoords       *coords)
{
  GimpCoords  catmull[4];
  GArray     *ret_coords;
  gint        i = buffer->event_history->len - 1;

  /* Note that there must be exactly one event in buffer or bad things
   * can happen. This must never get called under other circumstances.
   */
  ret_coords = g_array_new (FALSE, FALSE, sizeof (GimpCoords));

  catmull[0] = g_array_index (buffer->event_history, GimpCoords, i - 1);
  catmull[1] = g_array_index (buffer->event_history, GimpCoords, i);
  catmull[2] = g_array_index (buffer->event_queue,   GimpCoords, 0);
  catmull[3] = *coords;

  gimp_coords_interpolate_catmull (catmull, EVENT_FILL_PRECISION / 2,
                                   ret_coords, NULL);

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

static gboolean
gimp_motion_buffer_event_queue_timeout (GimpMotionBuffer *buffer)
{
  buffer->event_delay         = FALSE;
  buffer->event_delay_timeout = 0;

  if (buffer->event_queue->len > 0)
    {
      GimpCoords last_coords = g_array_index (buffer->event_queue,
                                              GimpCoords,
                                              buffer->event_queue->len - 1);

      gimp_motion_buffer_push_event_history (buffer, &last_coords);

      gimp_motion_buffer_request_stroke (buffer,
                                         buffer->last_active_state,
                                         buffer->last_read_motion_time);
    }

  return FALSE;
}
