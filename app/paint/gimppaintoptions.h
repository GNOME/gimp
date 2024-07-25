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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_PAINT_OPTIONS_H__
#define __GIMP_PAINT_OPTIONS_H__


#include "core/gimptooloptions.h"


#define GIMP_PAINT_OPTIONS_CONTEXT_MASK GIMP_CONTEXT_PROP_MASK_FOREGROUND | \
                                        GIMP_CONTEXT_PROP_MASK_BACKGROUND | \
                                        GIMP_CONTEXT_PROP_MASK_OPACITY    | \
                                        GIMP_CONTEXT_PROP_MASK_PAINT_MODE | \
                                        GIMP_CONTEXT_PROP_MASK_BRUSH      | \
                                        GIMP_CONTEXT_PROP_MASK_DYNAMICS   | \
                                        GIMP_CONTEXT_PROP_MASK_PALETTE


typedef struct _GimpJitterOptions        GimpJitterOptions;
typedef struct _GimpFadeOptions          GimpFadeOptions;
typedef struct _GimpGradientPaintOptions GimpGradientPaintOptions;
typedef struct _GimpSmoothingOptions     GimpSmoothingOptions;

struct _GimpJitterOptions
{
  gboolean  use_jitter;
  gdouble   jitter_amount;
};

struct _GimpFadeOptions
{
  gboolean        fade_reverse;
  gdouble         fade_length;
  GimpUnit       *fade_unit;
  GimpRepeatMode  fade_repeat;
};

struct _GimpGradientPaintOptions
{
  gboolean                    gradient_reverse;
  GimpGradientBlendColorSpace gradient_blend_color_space;
  GimpRepeatMode              gradient_repeat; /* only used by gradient tool */
};

struct _GimpSmoothingOptions
{
  gboolean use_smoothing;
  gint     smoothing_quality;
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

  gboolean                  use_applicator;

  GimpBrush                *brush; /* weak-refed storage for the GUI */

  gdouble                   brush_size;
  gdouble                   brush_angle;
  gdouble                   brush_aspect_ratio;
  gdouble                   brush_spacing;
  gdouble                   brush_hardness;
  gdouble                   brush_force;

  gboolean                  brush_link_size;
  gboolean                  brush_link_aspect_ratio;
  gboolean                  brush_link_angle;
  gboolean                  brush_link_spacing;
  gboolean                  brush_link_hardness;

  gboolean                  brush_lock_to_view;

  GimpPaintApplicationMode  application_mode;
  GimpPaintApplicationMode  application_mode_save;

  gboolean                  hard;

  gboolean                  expand_use;
  gdouble                   expand_amount;
  GimpFillType              expand_fill_type;
  GimpAddMaskType           expand_mask_fill_type;

  GimpJitterOptions        *jitter_options;

  gboolean                  dynamics_enabled;
  GimpFadeOptions          *fade_options;
  GimpGradientPaintOptions *gradient_options;
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


GType    gimp_paint_options_get_type (void) G_GNUC_CONST;

GimpPaintOptions *
         gimp_paint_options_new                (GimpPaintInfo       *paint_info);

gdouble  gimp_paint_options_get_fade           (GimpPaintOptions    *options,
                                                GimpImage           *image,
                                                gdouble              pixel_dist);

gdouble  gimp_paint_options_get_jitter         (GimpPaintOptions    *options,
                                                GimpImage           *image);

gboolean gimp_paint_options_get_gradient_color (GimpPaintOptions    *options,
                                                GimpImage           *image,
                                                gdouble              grad_point,
                                                gdouble              pixel_dist,
                                                GeglColor          **color);

GimpBrushApplicationMode
         gimp_paint_options_get_brush_mode     (GimpPaintOptions    *options);

void     gimp_paint_options_set_default_brush_size
                                               (GimpPaintOptions    *options,
                                                GimpBrush           *brush);
void     gimp_paint_options_set_default_brush_angle
                                               (GimpPaintOptions    *options,
                                                GimpBrush           *brush);
void     gimp_paint_options_set_default_brush_aspect_ratio
                                               (GimpPaintOptions    *options,
                                                GimpBrush           *brush);
void     gimp_paint_options_set_default_brush_spacing
                                               (GimpPaintOptions    *options,
                                                GimpBrush           *brush);

void     gimp_paint_options_set_default_brush_hardness
                                               (GimpPaintOptions    *options,
                                                GimpBrush           *brush);

gboolean gimp_paint_options_are_dynamics_enabled (GimpPaintOptions *paint_options);
void     gimp_paint_options_enable_dynamics      (GimpPaintOptions *paint_options,
                                                  gboolean          enable);

gboolean gimp_paint_options_is_prop            (const gchar         *prop_name,
                                                GimpContextPropMask  prop_mask);
void     gimp_paint_options_copy_props         (GimpPaintOptions    *src,
                                                GimpPaintOptions    *dest,
                                                GimpContextPropMask  prop_mask);


#endif  /*  __GIMP_PAINT_OPTIONS_H__  */
