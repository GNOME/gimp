/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmotionbuffer.h
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

#pragma once

#include "core/gimpobject.h"


#define GIMP_TYPE_MOTION_BUFFER            (gimp_motion_buffer_get_type ())
#define GIMP_MOTION_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MOTION_BUFFER, GimpMotionBuffer))
#define GIMP_MOTION_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MOTION_BUFFER, GimpMotionBufferClass))
#define GIMP_IS_MOTION_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MOTION_BUFFER))
#define GIMP_IS_MOTION_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MOTION_BUFFER))
#define GIMP_MOTION_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MOTION_BUFFER, GimpMotionBufferClass))


typedef struct _GimpMotionBufferClass GimpMotionBufferClass;

struct _GimpMotionBuffer
{
  GimpObject  parent_instance;

  guint32     last_read_motion_time;

  guint32     last_motion_time; /*  previous time of a forwarded motion event  */
  gdouble     last_motion_delta_time;
  gdouble     last_motion_delta_x;
  gdouble     last_motion_delta_y;
  gdouble     last_motion_distance;

  GimpCoords  last_coords;      /* last motion event                   */

  GArray     *event_history;
  GArray     *event_queue;
  gboolean    event_delay;      /* TRUE if there's an unsent event in
                                 *  the history buffer
                                 */

  gint               event_delay_timeout;
  GdkModifierType    last_active_state;
};

struct _GimpMotionBufferClass
{
  GimpObjectClass  parent_class;

  void (* stroke) (GimpMotionBuffer *buffer,
                   const GimpCoords *coords,
                   guint32           time,
                   GdkModifierType   state);
  void (* hover)  (GimpMotionBuffer *buffer,
                   const GimpCoords *coords,
                   GdkModifierType   state,
                   gboolean          proximity);
};


GType              gimp_motion_buffer_get_type     (void) G_GNUC_CONST;

GimpMotionBuffer * gimp_motion_buffer_new          (void);

void       gimp_motion_buffer_begin_stroke         (GimpMotionBuffer *buffer,
                                                    guint32           time,
                                                    GimpCoords       *last_motion);
void       gimp_motion_buffer_end_stroke           (GimpMotionBuffer *buffer);

gboolean   gimp_motion_buffer_motion_event         (GimpMotionBuffer *buffer,
                                                    GimpCoords       *coords,
                                                    guint32           time,
                                                    gboolean          event_fill);
guint32    gimp_motion_buffer_get_last_motion_time (GimpMotionBuffer *buffer);

void       gimp_motion_buffer_request_stroke       (GimpMotionBuffer *buffer,
                                                    GdkModifierType   state,
                                                    guint32           time);
void       gimp_motion_buffer_request_hover        (GimpMotionBuffer *buffer,
                                                    GdkModifierType   state,
                                                    gboolean          proximity);
