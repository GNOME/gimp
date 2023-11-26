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

#ifndef __GIMP_GRADIENT_H__
#define __GIMP_GRADIENT_H__


#include "gimpdata.h"


#define GIMP_GRADIENT_DEFAULT_SAMPLE_SIZE 40


struct _GimpGradientSegment
{
  gdouble                  left, middle, right;

  GimpGradientColor        left_color_type;
  GeglColor               *left_color;
  GimpGradientColor        right_color_type;
  GeglColor               *right_color;

  GimpGradientSegmentType  type;          /*  Segment's blending function  */
  GimpGradientSegmentColor color;         /*  Segment's coloring type      */

  GimpGradientSegment     *prev;
  GimpGradientSegment     *next;
};


#define GIMP_TYPE_GRADIENT            (gimp_gradient_get_type ())
#define GIMP_GRADIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_GRADIENT, GimpGradient))
#define GIMP_GRADIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_GRADIENT, GimpGradientClass))
#define GIMP_IS_GRADIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_GRADIENT))
#define GIMP_IS_GRADIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_GRADIENT))
#define GIMP_GRADIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_GRADIENT, GimpGradientClass))


typedef struct _GimpGradientClass GimpGradientClass;

struct _GimpGradient
{
  GimpData             parent_instance;

  GimpGradientSegment *segments;
};

struct _GimpGradientClass
{
  GimpDataClass  parent_class;
};


GType                 gimp_gradient_get_type       (void) G_GNUC_CONST;

GimpData            * gimp_gradient_new            (GimpContext   *context,
                                                    const gchar   *name);
GimpData            * gimp_gradient_get_standard   (GimpContext   *context);

GimpGradientSegment * gimp_gradient_get_color_at   (GimpGradient                 *gradient,
                                                    GimpContext                  *context,
                                                    GimpGradientSegment          *seg,
                                                    gdouble                       pos,
                                                    gboolean                      reverse,
                                                    GimpGradientBlendColorSpace   blend_color_space,
                                                    GeglColor                   **color);
GimpGradientSegment * gimp_gradient_get_segment_at (GimpGradient                 *grad,
                                                    gdouble                       pos);
void                  gimp_gradient_split_at       (GimpGradient                 *gradient,
                                                    GimpContext                  *context,
                                                    GimpGradientSegment          *seg,
                                                    gdouble                       pos,
                                                    GimpGradientBlendColorSpace   blend_color_space,
                                                    GimpGradientSegment         **newl,
                                                    GimpGradientSegment         **newr);

gboolean          gimp_gradient_has_fg_bg_segments (GimpGradient  *gradient);
GimpGradient    * gimp_gradient_flatten            (GimpGradient  *gradient,
                                                    GimpContext   *context);


/*  gradient segment functions  */

GimpGradientSegment * gimp_gradient_segment_new       (void);
GimpGradientSegment * gimp_gradient_segment_get_last  (GimpGradientSegment *seg);
GimpGradientSegment * gimp_gradient_segment_get_first (GimpGradientSegment *seg);
GimpGradientSegment * gimp_gradient_segment_get_nth   (GimpGradientSegment *seg,
                                                       gint                 index);

void                  gimp_gradient_segment_free      (GimpGradientSegment *seg);
void                  gimp_gradient_segments_free     (GimpGradientSegment *seg);

void                  gimp_gradient_segment_split_midpoint  (GimpGradient         *gradient,
                                                             GimpContext          *context,
                                                             GimpGradientSegment  *lseg,
                                                             GimpGradientBlendColorSpace blend_color_space,
                                                             GimpGradientSegment **newl,
                                                             GimpGradientSegment **newr);
void                  gimp_gradient_segment_split_uniform   (GimpGradient         *gradient,
                                                             GimpContext          *context,
                                                             GimpGradientSegment  *lseg,
                                                             gint                  parts,
                                                             GimpGradientBlendColorSpace blend_color_space,
                                                             GimpGradientSegment **newl,
                                                             GimpGradientSegment **newr);

/* Colors Setting/Getting Routines */
GeglColor           * gimp_gradient_segment_get_left_color  (GimpGradient         *gradient,
                                                             GimpGradientSegment  *seg);

void                  gimp_gradient_segment_set_left_color  (GimpGradient         *gradient,
                                                             GimpGradientSegment  *seg,
                                                             GeglColor            *color);


GeglColor           * gimp_gradient_segment_get_right_color (GimpGradient         *gradient,
                                                             GimpGradientSegment  *seg);

void                  gimp_gradient_segment_set_right_color (GimpGradient         *gradient,
                                                             GimpGradientSegment  *seg,
                                                             GeglColor            *color);


GimpGradientColor
gimp_gradient_segment_get_left_color_type     (GimpGradient         *gradient,
                                               GimpGradientSegment  *seg);

void
gimp_gradient_segment_set_left_color_type     (GimpGradient         *gradient,
                                               GimpGradientSegment  *seg,
                                               GimpGradientColor     color_type);


GimpGradientColor
gimp_gradient_segment_get_right_color_type    (GimpGradient         *gradient,
                                               GimpGradientSegment  *seg);

void
gimp_gradient_segment_set_right_color_type    (GimpGradient         *gradient,
                                               GimpGradientSegment  *seg,
                                               GimpGradientColor     color_type);


GeglColor *
gimp_gradient_segment_get_left_flat_color     (GimpGradient         *gradient,
                                               GimpContext          *context,
                                               GimpGradientSegment  *seg);


GeglColor *
gimp_gradient_segment_get_right_flat_color    (GimpGradient         *gradient,
                                               GimpContext          *context,
                                               GimpGradientSegment  *seg);


/* Position Setting/Getting Routines */
/* (Setters return the position after it was set) */
gdouble gimp_gradient_segment_get_left_pos    (GimpGradient         *gradient,
                                               GimpGradientSegment  *seg);
gdouble gimp_gradient_segment_set_left_pos    (GimpGradient         *gradient,
                                               GimpGradientSegment  *seg,
                                               gdouble               pos);

gdouble gimp_gradient_segment_get_right_pos   (GimpGradient         *gradient,
                                               GimpGradientSegment  *seg);
gdouble gimp_gradient_segment_set_right_pos   (GimpGradient         *gradient,
                                               GimpGradientSegment  *seg,
                                               gdouble               pos);

gdouble gimp_gradient_segment_get_middle_pos  (GimpGradient         *gradient,
                                               GimpGradientSegment  *seg);
gdouble gimp_gradient_segment_set_middle_pos  (GimpGradient         *gradient,
                                               GimpGradientSegment  *seg,
                                               gdouble               pos);

/* Getting/Setting the Blending Function/Coloring Type */
GimpGradientSegmentType
gimp_gradient_segment_get_blending_function   (GimpGradient         *gradient,
                                               GimpGradientSegment  *seg);
GimpGradientSegmentColor
gimp_gradient_segment_get_coloring_type       (GimpGradient         *gradient,
                                               GimpGradientSegment  *seg);

/*
 * If the second segment is NULL, these functions will process
 * until the end of the string.
 * */
gint    gimp_gradient_segment_range_get_n_segments
                                              (GimpGradient         *gradient,
                                               GimpGradientSegment  *range_l,
                                               GimpGradientSegment  *range_r);

void    gimp_gradient_segment_range_compress  (GimpGradient         *gradient,
                                               GimpGradientSegment  *range_l,
                                               GimpGradientSegment  *range_r,
                                               gdouble               new_l,
                                               gdouble               new_r);
void    gimp_gradient_segment_range_blend     (GimpGradient         *gradient,
                                               GimpGradientSegment  *lseg,
                                               GimpGradientSegment  *rseg,
                                               GeglColor            *color1,
                                               GeglColor            *color2,
                                               gboolean              blend_colors,
                                               gboolean              blend_opacity);

void    gimp_gradient_segment_range_set_blending_function
                                              (GimpGradient         *gradient,
                                               GimpGradientSegment  *start_seg,
                                               GimpGradientSegment  *end_seg,
                                               GimpGradientSegmentType new_type);

void    gimp_gradient_segment_range_set_coloring_type
                                              (GimpGradient         *gradient,
                                               GimpGradientSegment  *start_seg,
                                               GimpGradientSegment  *end_seg,
                                               GimpGradientSegmentColor new_color);

void    gimp_gradient_segment_range_flip      (GimpGradient         *gradient,
                                               GimpGradientSegment  *start_seg,
                                               GimpGradientSegment  *end_seg,
                                               GimpGradientSegment **final_start_seg,
                                               GimpGradientSegment **final_end_seg);

void    gimp_gradient_segment_range_replicate (GimpGradient         *gradient,
                                               GimpGradientSegment  *start_seg,
                                               GimpGradientSegment  *end_seg,
                                               gint                  replicate_times,
                                               GimpGradientSegment **final_start_seg,
                                               GimpGradientSegment **final_end_seg);

void    gimp_gradient_segment_range_split_midpoint
                                              (GimpGradient         *gradient,
                                               GimpContext          *context,
                                               GimpGradientSegment  *start_seg,
                                               GimpGradientSegment  *end_seg,
                                               GimpGradientBlendColorSpace blend_color_space,
                                               GimpGradientSegment **final_start_seg,
                                               GimpGradientSegment **final_end_seg);

void    gimp_gradient_segment_range_split_uniform
                                              (GimpGradient         *gradient,
                                               GimpContext          *context,
                                               GimpGradientSegment  *start_seg,
                                               GimpGradientSegment  *end_seg,
                                               gint                  parts,
                                               GimpGradientBlendColorSpace blend_color_space,
                                               GimpGradientSegment **final_start_seg,
                                               GimpGradientSegment **final_end_seg);

void    gimp_gradient_segment_range_delete    (GimpGradient         *gradient,
                                               GimpGradientSegment  *start_seg,
                                               GimpGradientSegment  *end_seg,
                                               GimpGradientSegment **final_start_seg,
                                               GimpGradientSegment **final_end_seg);

void    gimp_gradient_segment_range_merge     (GimpGradient         *gradient,
                                               GimpGradientSegment  *start_seg,
                                               GimpGradientSegment  *end_seg,
                                               GimpGradientSegment **final_start_seg,
                                               GimpGradientSegment **final_end_seg);

void    gimp_gradient_segment_range_recenter_handles
                                              (GimpGradient         *gradient,
                                               GimpGradientSegment  *start_seg,
                                               GimpGradientSegment  *end_seg);
void    gimp_gradient_segment_range_redistribute_handles
                                              (GimpGradient         *gradient,
                                               GimpGradientSegment  *start_seg,
                                               GimpGradientSegment  *end_seg);

gdouble gimp_gradient_segment_range_move      (GimpGradient         *gradient,
                                               GimpGradientSegment  *range_l,
                                               GimpGradientSegment  *range_r,
                                               gdouble               delta,
                                               gboolean              control_compress);


#endif /* __GIMP_GRADIENT_H__ */
