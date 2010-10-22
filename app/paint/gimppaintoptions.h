/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_PAINT_OPTIONS_H__
#define __GIMP_PAINT_OPTIONS_H__


#include "core/gimptooloptions.h"


#define GIMP_PAINT_OPTIONS_CONTEXT_MASK GIMP_CONTEXT_FOREGROUND_MASK | \
                                        GIMP_CONTEXT_BACKGROUND_MASK | \
                                        GIMP_CONTEXT_OPACITY_MASK    | \
                                        GIMP_CONTEXT_PAINT_MODE_MASK | \
                                        GIMP_CONTEXT_BRUSH_MASK      | \
                                        GIMP_CONTEXT_DYNAMICS_MASK

typedef struct _GimpCircularQueue   GimpCircularQueue;
struct _GimpCircularQueue
{
  guint    element_size;
  guint    queue_size;
  guint    start;
  guint    end;
  gpointer data;
};
GimpCircularQueue* gimp_circular_queue_new(guint element_size, guint queue_size);
void gimp_circular_queue_free(GimpCircularQueue* queue);
void gimp_circular_queue_enqueue_data(GimpCircularQueue* queue, gpointer data);
gpointer gimp_circular_queue_get_nth_offset(GimpCircularQueue* queue, guint index);
gpointer gimp_circular_queue_get_last_offset(GimpCircularQueue* queue);
#define gimp_circular_queue_length(q) ((q)->end - (q)->start)
#define gimp_circular_queue_enqueue(q, a) gimp_circular_queue_enqueue_data(q, (void*)(&(a)))
#define gimp_circular_queue_index(q, type, i) (*(type*)gimp_circular_queue_get_nth_offset(q, i))
#define gimp_circular_queue_last(q, type) (*(type*)gimp_circular_queue_get_last_offset(q))


typedef struct _GimpJitterOptions   GimpJitterOptions;
typedef struct _GimpFadeOptions     GimpFadeOptions;
typedef struct _GimpGradientOptions GimpGradientOptions;
typedef struct _GimpSmoothingOptions GimpSmoothingOptions;

struct _GimpJitterOptions
{
  gboolean  use_jitter;
  gdouble   jitter_amount;
};

struct _GimpFadeOptions
{
  gboolean        fade_reverse;
  gdouble         fade_length;
  GimpUnit        fade_unit;
  GimpRepeatMode  fade_repeat;
};

struct _GimpGradientOptions
{
  gboolean        gradient_reverse;
  GimpRepeatMode  gradient_repeat;
};

struct _GimpSmoothingOptions
{
  gboolean use_smoothing;
  gint     smoothing_history;
  gdouble  smoothing_factor;
};


#define GIMP_TYPE_PAINT_OPTIONS            (gimp_paint_options_get_type ())
#define GIMP_PAINT_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PAINT_OPTIONS, GimpPaintOptions))
#define GIMP_PAINT_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PAINT_OPTIONS, GimpPaintOptionsClass))
#define GIMP_IS_PAINT_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PAINT_OPTIONS))
#define GIMP_IS_PAINT_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PAINT_OPTIONS))
#define GIMP_PAINT_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PAINT_OPTIONS, GimpPaintOptionsClass))


typedef struct _GimpPaintOptionsClass GimpPaintOptionsClass;

struct _GimpPaintOptions
{
  GimpToolOptions           parent_instance;

  GimpPaintInfo            *paint_info;

  gdouble                   brush_size;
  gdouble                   brush_angle;
  gdouble                   brush_aspect_ratio;

  GimpPaintApplicationMode  application_mode;
  GimpPaintApplicationMode  application_mode_save;

  gboolean                  hard;

  GimpJitterOptions        *jitter_options;

  gboolean                  dynamics_expanded;
  GimpFadeOptions          *fade_options;
  GimpGradientOptions      *gradient_options;
  GimpSmoothingOptions     *smoothing_options;

  GimpViewType              brush_view_type;
  GimpViewSize              brush_view_size;
  GimpViewType              dynamics_view_type;
  GimpViewSize              dynamics_view_size;
  GimpViewType              pattern_view_type;
  GimpViewSize              pattern_view_size;
  GimpViewType              gradient_view_type;
  GimpViewSize              gradient_view_size;
};

struct _GimpPaintOptionsClass
{
  GimpToolOptionsClass  parent_instance;
};


GType              gimp_paint_options_get_type (void) G_GNUC_CONST;

GimpPaintOptions * gimp_paint_options_new      (GimpPaintInfo    *paint_info);

gdouble            gimp_paint_options_get_fade (GimpPaintOptions *paint_options,
                                                GimpImage        *image,
                                                gdouble           pixel_dist);

gdouble          gimp_paint_options_get_jitter (GimpPaintOptions *paint_options,
                                                GimpImage        *image);

gboolean gimp_paint_options_get_gradient_color (GimpPaintOptions *paint_options,
                                                GimpImage        *image,
                                                gdouble           grad_point,
                                                gdouble           pixel_dist,
                                                GimpRGB          *color);

GimpCoords gimp_paint_options_get_smoothed_coords (GimpPaintOptions  *paint_options,
                                                   const GimpCoords *original_coords,
                                                   GimpCircularQueue *history);

GimpBrushApplicationMode
             gimp_paint_options_get_brush_mode (GimpPaintOptions *paint_options);



#endif  /*  __GIMP_PAINT_OPTIONS_H__  */
