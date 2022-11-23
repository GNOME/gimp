/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmamotionbuffer.h
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

#ifndef __LIGMA_MOTION_BUFFER_H__
#define __LIGMA_MOTION_BUFFER_H__


#include "core/ligmaobject.h"


#define LIGMA_TYPE_MOTION_BUFFER            (ligma_motion_buffer_get_type ())
#define LIGMA_MOTION_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_MOTION_BUFFER, LigmaMotionBuffer))
#define LIGMA_MOTION_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_MOTION_BUFFER, LigmaMotionBufferClass))
#define LIGMA_IS_MOTION_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_MOTION_BUFFER))
#define LIGMA_IS_MOTION_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_MOTION_BUFFER))
#define LIGMA_MOTION_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_MOTION_BUFFER, LigmaMotionBufferClass))


typedef struct _LigmaMotionBufferClass LigmaMotionBufferClass;

struct _LigmaMotionBuffer
{
  LigmaObject  parent_instance;

  guint32     last_read_motion_time;

  guint32     last_motion_time; /*  previous time of a forwarded motion event  */
  gdouble     last_motion_delta_time;
  gdouble     last_motion_delta_x;
  gdouble     last_motion_delta_y;
  gdouble     last_motion_distance;

  LigmaCoords  last_coords;      /* last motion event                   */

  GArray     *event_history;
  GArray     *event_queue;
  gboolean    event_delay;      /* TRUE if there's an unsent event in
                                 *  the history buffer
                                 */

  gint               event_delay_timeout;
  GdkModifierType    last_active_state;
};

struct _LigmaMotionBufferClass
{
  LigmaObjectClass  parent_class;

  void (* stroke) (LigmaMotionBuffer *buffer,
                   const LigmaCoords *coords,
                   guint32           time,
                   GdkModifierType   state);
  void (* hover)  (LigmaMotionBuffer *buffer,
                   const LigmaCoords *coords,
                   GdkModifierType   state,
                   gboolean          proximity);
};


GType              ligma_motion_buffer_get_type     (void) G_GNUC_CONST;

LigmaMotionBuffer * ligma_motion_buffer_new          (void);

void       ligma_motion_buffer_begin_stroke         (LigmaMotionBuffer *buffer,
                                                    guint32           time,
                                                    LigmaCoords       *last_motion);
void       ligma_motion_buffer_end_stroke           (LigmaMotionBuffer *buffer);

gboolean   ligma_motion_buffer_motion_event         (LigmaMotionBuffer *buffer,
                                                    LigmaCoords       *coords,
                                                    guint32           time,
                                                    gboolean          event_fill);
guint32    ligma_motion_buffer_get_last_motion_time (LigmaMotionBuffer *buffer);

void       ligma_motion_buffer_request_stroke       (LigmaMotionBuffer *buffer,
                                                    GdkModifierType   state,
                                                    guint32           time);
void       ligma_motion_buffer_request_hover        (LigmaMotionBuffer *buffer,
                                                    GdkModifierType   state,
                                                    gboolean          proximity);


#endif /* __LIGMA_MOTION_BUFFER_H__ */
