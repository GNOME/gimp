/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_GRADIENT_H__
#define __LIGMA_GRADIENT_H__


#include "ligmadata.h"


#define LIGMA_GRADIENT_DEFAULT_SAMPLE_SIZE 40


struct _LigmaGradientSegment
{
  gdouble                  left, middle, right;

  LigmaGradientColor        left_color_type;
  LigmaRGB                  left_color;
  LigmaGradientColor        right_color_type;
  LigmaRGB                  right_color;

  LigmaGradientSegmentType  type;          /*  Segment's blending function  */
  LigmaGradientSegmentColor color;         /*  Segment's coloring type      */

  LigmaGradientSegment     *prev;
  LigmaGradientSegment     *next;
};


#define LIGMA_TYPE_GRADIENT            (ligma_gradient_get_type ())
#define LIGMA_GRADIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_GRADIENT, LigmaGradient))
#define LIGMA_GRADIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_GRADIENT, LigmaGradientClass))
#define LIGMA_IS_GRADIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_GRADIENT))
#define LIGMA_IS_GRADIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_GRADIENT))
#define LIGMA_GRADIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_GRADIENT, LigmaGradientClass))


typedef struct _LigmaGradientClass LigmaGradientClass;

struct _LigmaGradient
{
  LigmaData             parent_instance;

  LigmaGradientSegment *segments;
};

struct _LigmaGradientClass
{
  LigmaDataClass  parent_class;
};


GType                 ligma_gradient_get_type       (void) G_GNUC_CONST;

LigmaData            * ligma_gradient_new            (LigmaContext   *context,
                                                    const gchar   *name);
LigmaData            * ligma_gradient_get_standard   (LigmaContext   *context);

LigmaGradientSegment * ligma_gradient_get_color_at   (LigmaGradient        *gradient,
                                                    LigmaContext         *context,
                                                    LigmaGradientSegment *seg,
                                                    gdouble              pos,
                                                    gboolean             reverse,
                                                    LigmaGradientBlendColorSpace blend_color_space,
                                                    LigmaRGB             *color);
LigmaGradientSegment * ligma_gradient_get_segment_at (LigmaGradient  *grad,
                                                    gdouble        pos);
void                  ligma_gradient_split_at       (LigmaGradient         *gradient,
                                                    LigmaContext          *context,
                                                    LigmaGradientSegment  *seg,
                                                    gdouble               pos,
                                                    LigmaGradientBlendColorSpace blend_color_space,
                                                    LigmaGradientSegment **newl,
                                                    LigmaGradientSegment **newr);

gboolean          ligma_gradient_has_fg_bg_segments (LigmaGradient  *gradient);
LigmaGradient    * ligma_gradient_flatten            (LigmaGradient  *gradient,
                                                    LigmaContext   *context);


/*  gradient segment functions  */

LigmaGradientSegment * ligma_gradient_segment_new       (void);
LigmaGradientSegment * ligma_gradient_segment_get_last  (LigmaGradientSegment *seg);
LigmaGradientSegment * ligma_gradient_segment_get_first (LigmaGradientSegment *seg);
LigmaGradientSegment * ligma_gradient_segment_get_nth   (LigmaGradientSegment *seg,
                                                       gint                 index);

void                  ligma_gradient_segment_free      (LigmaGradientSegment *seg);
void                  ligma_gradient_segments_free     (LigmaGradientSegment *seg);

void    ligma_gradient_segment_split_midpoint  (LigmaGradient         *gradient,
                                               LigmaContext          *context,
                                               LigmaGradientSegment  *lseg,
                                               LigmaGradientBlendColorSpace blend_color_space,
                                               LigmaGradientSegment **newl,
                                               LigmaGradientSegment **newr);
void    ligma_gradient_segment_split_uniform   (LigmaGradient         *gradient,
                                               LigmaContext          *context,
                                               LigmaGradientSegment  *lseg,
                                               gint                  parts,
                                               LigmaGradientBlendColorSpace blend_color_space,
                                               LigmaGradientSegment **newl,
                                               LigmaGradientSegment **newr);

/* Colors Setting/Getting Routines */
void    ligma_gradient_segment_get_left_color  (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *seg,
                                               LigmaRGB              *color);

void    ligma_gradient_segment_set_left_color  (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *seg,
                                               const LigmaRGB        *color);


void    ligma_gradient_segment_get_right_color (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *seg,
                                               LigmaRGB              *color);

void    ligma_gradient_segment_set_right_color (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *seg,
                                               const LigmaRGB        *color);


LigmaGradientColor
ligma_gradient_segment_get_left_color_type     (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *seg);

void
ligma_gradient_segment_set_left_color_type     (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *seg,
                                               LigmaGradientColor     color_type);


LigmaGradientColor
ligma_gradient_segment_get_right_color_type    (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *seg);

void
ligma_gradient_segment_set_right_color_type    (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *seg,
                                               LigmaGradientColor     color_type);


void
ligma_gradient_segment_get_left_flat_color     (LigmaGradient         *gradient,
                                               LigmaContext          *context,
                                               LigmaGradientSegment  *seg,
                                               LigmaRGB              *color);


void
ligma_gradient_segment_get_right_flat_color    (LigmaGradient         *gradient,
                                               LigmaContext          *context,
                                               LigmaGradientSegment  *seg,
                                               LigmaRGB              *color);


/* Position Setting/Getting Routines */
/* (Setters return the position after it was set) */
gdouble ligma_gradient_segment_get_left_pos    (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *seg);
gdouble ligma_gradient_segment_set_left_pos    (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *seg,
                                               gdouble               pos);

gdouble ligma_gradient_segment_get_right_pos   (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *seg);
gdouble ligma_gradient_segment_set_right_pos   (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *seg,
                                               gdouble               pos);

gdouble ligma_gradient_segment_get_middle_pos  (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *seg);
gdouble ligma_gradient_segment_set_middle_pos  (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *seg,
                                               gdouble               pos);

/* Getting/Setting the Blending Function/Coloring Type */
LigmaGradientSegmentType
ligma_gradient_segment_get_blending_function   (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *seg);
LigmaGradientSegmentColor
ligma_gradient_segment_get_coloring_type       (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *seg);

/*
 * If the second segment is NULL, these functions will process
 * until the end of the string.
 * */
gint    ligma_gradient_segment_range_get_n_segments
                                              (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *range_l,
                                               LigmaGradientSegment  *range_r);

void    ligma_gradient_segment_range_compress  (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *range_l,
                                               LigmaGradientSegment  *range_r,
                                               gdouble               new_l,
                                               gdouble               new_r);
void    ligma_gradient_segment_range_blend     (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *lseg,
                                               LigmaGradientSegment  *rseg,
                                               const LigmaRGB        *rgb1,
                                               const LigmaRGB        *rgb2,
                                               gboolean              blend_colors,
                                               gboolean              blend_opacity);

void    ligma_gradient_segment_range_set_blending_function
                                              (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *start_seg,
                                               LigmaGradientSegment  *end_seg,
                                               LigmaGradientSegmentType new_type);

void    ligma_gradient_segment_range_set_coloring_type
                                              (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *start_seg,
                                               LigmaGradientSegment  *end_seg,
                                               LigmaGradientSegmentColor new_color);

void    ligma_gradient_segment_range_flip      (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *start_seg,
                                               LigmaGradientSegment  *end_seg,
                                               LigmaGradientSegment **final_start_seg,
                                               LigmaGradientSegment **final_end_seg);

void    ligma_gradient_segment_range_replicate (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *start_seg,
                                               LigmaGradientSegment  *end_seg,
                                               gint                  replicate_times,
                                               LigmaGradientSegment **final_start_seg,
                                               LigmaGradientSegment **final_end_seg);

void    ligma_gradient_segment_range_split_midpoint
                                              (LigmaGradient         *gradient,
                                               LigmaContext          *context,
                                               LigmaGradientSegment  *start_seg,
                                               LigmaGradientSegment  *end_seg,
                                               LigmaGradientBlendColorSpace blend_color_space,
                                               LigmaGradientSegment **final_start_seg,
                                               LigmaGradientSegment **final_end_seg);

void    ligma_gradient_segment_range_split_uniform
                                              (LigmaGradient         *gradient,
                                               LigmaContext          *context,
                                               LigmaGradientSegment  *start_seg,
                                               LigmaGradientSegment  *end_seg,
                                               gint                  parts,
                                               LigmaGradientBlendColorSpace blend_color_space,
                                               LigmaGradientSegment **final_start_seg,
                                               LigmaGradientSegment **final_end_seg);

void    ligma_gradient_segment_range_delete    (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *start_seg,
                                               LigmaGradientSegment  *end_seg,
                                               LigmaGradientSegment **final_start_seg,
                                               LigmaGradientSegment **final_end_seg);

void    ligma_gradient_segment_range_merge     (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *start_seg,
                                               LigmaGradientSegment  *end_seg,
                                               LigmaGradientSegment **final_start_seg,
                                               LigmaGradientSegment **final_end_seg);

void    ligma_gradient_segment_range_recenter_handles
                                              (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *start_seg,
                                               LigmaGradientSegment  *end_seg);
void    ligma_gradient_segment_range_redistribute_handles
                                              (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *start_seg,
                                               LigmaGradientSegment  *end_seg);

gdouble ligma_gradient_segment_range_move      (LigmaGradient         *gradient,
                                               LigmaGradientSegment  *range_l,
                                               LigmaGradientSegment  *range_r,
                                               gdouble               delta,
                                               gboolean              control_compress);


#endif /* __LIGMA_GRADIENT_H__ */
