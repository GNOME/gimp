/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_PAINT_OPTIONS_H__
#define __LIGMA_PAINT_OPTIONS_H__


#include "core/ligmatooloptions.h"


#define LIGMA_PAINT_OPTIONS_CONTEXT_MASK LIGMA_CONTEXT_PROP_MASK_FOREGROUND | \
                                        LIGMA_CONTEXT_PROP_MASK_BACKGROUND | \
                                        LIGMA_CONTEXT_PROP_MASK_OPACITY    | \
                                        LIGMA_CONTEXT_PROP_MASK_PAINT_MODE | \
                                        LIGMA_CONTEXT_PROP_MASK_BRUSH      | \
                                        LIGMA_CONTEXT_PROP_MASK_DYNAMICS   | \
                                        LIGMA_CONTEXT_PROP_MASK_PALETTE


typedef struct _LigmaJitterOptions        LigmaJitterOptions;
typedef struct _LigmaFadeOptions          LigmaFadeOptions;
typedef struct _LigmaGradientPaintOptions LigmaGradientPaintOptions;
typedef struct _LigmaSmoothingOptions     LigmaSmoothingOptions;

struct _LigmaJitterOptions
{
  gboolean  use_jitter;
  gdouble   jitter_amount;
};

struct _LigmaFadeOptions
{
  gboolean        fade_reverse;
  gdouble         fade_length;
  LigmaUnit        fade_unit;
  LigmaRepeatMode  fade_repeat;
};

struct _LigmaGradientPaintOptions
{
  gboolean                    gradient_reverse;
  LigmaGradientBlendColorSpace gradient_blend_color_space;
  LigmaRepeatMode              gradient_repeat; /* only used by gradient tool */
};

struct _LigmaSmoothingOptions
{
  gboolean use_smoothing;
  gint     smoothing_quality;
  gdouble  smoothing_factor;
};


#define LIGMA_TYPE_PAINT_OPTIONS            (ligma_paint_options_get_type ())
#define LIGMA_PAINT_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PAINT_OPTIONS, LigmaPaintOptions))
#define LIGMA_PAINT_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PAINT_OPTIONS, LigmaPaintOptionsClass))
#define LIGMA_IS_PAINT_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PAINT_OPTIONS))
#define LIGMA_IS_PAINT_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PAINT_OPTIONS))
#define LIGMA_PAINT_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PAINT_OPTIONS, LigmaPaintOptionsClass))


typedef struct _LigmaPaintOptionsClass LigmaPaintOptionsClass;

struct _LigmaPaintOptions
{
  LigmaToolOptions           parent_instance;

  LigmaPaintInfo            *paint_info;

  gboolean                  use_applicator;

  LigmaBrush                *brush; /* weak-refed storage for the GUI */

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

  LigmaPaintApplicationMode  application_mode;
  LigmaPaintApplicationMode  application_mode_save;

  gboolean                  hard;

  LigmaJitterOptions        *jitter_options;

  gboolean                  dynamics_enabled;
  LigmaFadeOptions          *fade_options;
  LigmaGradientPaintOptions *gradient_options;
  LigmaSmoothingOptions     *smoothing_options;

  LigmaViewType              brush_view_type;
  LigmaViewSize              brush_view_size;
  LigmaViewType              dynamics_view_type;
  LigmaViewSize              dynamics_view_size;
  LigmaViewType              pattern_view_type;
  LigmaViewSize              pattern_view_size;
  LigmaViewType              gradient_view_type;
  LigmaViewSize              gradient_view_size;
};

struct _LigmaPaintOptionsClass
{
  LigmaToolOptionsClass  parent_instance;
};


GType    ligma_paint_options_get_type (void) G_GNUC_CONST;

LigmaPaintOptions *
         ligma_paint_options_new                (LigmaPaintInfo       *paint_info);

gdouble  ligma_paint_options_get_fade           (LigmaPaintOptions    *options,
                                                LigmaImage           *image,
                                                gdouble              pixel_dist);

gdouble  ligma_paint_options_get_jitter         (LigmaPaintOptions    *options,
                                                LigmaImage           *image);

gboolean ligma_paint_options_get_gradient_color (LigmaPaintOptions    *options,
                                                LigmaImage           *image,
                                                gdouble              grad_point,
                                                gdouble              pixel_dist,
                                                LigmaRGB             *color);

LigmaBrushApplicationMode
         ligma_paint_options_get_brush_mode     (LigmaPaintOptions    *options);

void     ligma_paint_options_set_default_brush_size
                                               (LigmaPaintOptions    *options,
                                                LigmaBrush           *brush);
void     ligma_paint_options_set_default_brush_angle
                                               (LigmaPaintOptions    *options,
                                                LigmaBrush           *brush);
void     ligma_paint_options_set_default_brush_aspect_ratio
                                               (LigmaPaintOptions    *options,
                                                LigmaBrush           *brush);
void     ligma_paint_options_set_default_brush_spacing
                                               (LigmaPaintOptions    *options,
                                                LigmaBrush           *brush);

void     ligma_paint_options_set_default_brush_hardness
                                               (LigmaPaintOptions    *options,
                                                LigmaBrush           *brush);

gboolean ligma_paint_options_are_dynamics_enabled (LigmaPaintOptions *paint_options);
void     ligma_paint_options_enable_dynamics      (LigmaPaintOptions *paint_options,
                                                  gboolean          enable);

gboolean ligma_paint_options_is_prop            (const gchar         *prop_name,
                                                LigmaContextPropMask  prop_mask);
void     ligma_paint_options_copy_props         (LigmaPaintOptions    *src,
                                                LigmaPaintOptions    *dest,
                                                LigmaContextPropMask  prop_mask);


#endif  /*  __LIGMA_PAINT_OPTIONS_H__  */
