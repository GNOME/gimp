/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_GRADIENT_H__
#define __GIMP_GRADIENT_H__


#include "gimpdata.h"


#define GIMP_GRADIENT_FILE_EXTENSION ".ggr"


typedef enum
{
  GRAD_LINEAR = 0,
  GRAD_CURVED,
  GRAD_SINE,
  GRAD_SPHERE_INCREASING,
  GRAD_SPHERE_DECREASING
} GimpGradientSegmentType;

typedef enum
{
  GRAD_RGB = 0,  /* normal RGB */
  GRAD_HSV_CCW,  /* counterclockwise hue */
  GRAD_HSV_CW    /* clockwise hue */
} GimpGradientSegmentColor;


typedef struct _GimpGradientSegment GimpGradientSegment;

struct _GimpGradientSegment
{
  gdouble                  left, middle, right;
  GimpRGB                  left_color;
  GimpRGB                  right_color;
  GimpGradientSegmentType  type;          /*  Segment's blending function  */
  GimpGradientSegmentColor color;         /*  Segment's coloring type      */

  GimpGradientSegment     *prev;
  GimpGradientSegment     *next;
};


#define GIMP_TYPE_GRADIENT            (gimp_gradient_get_type ())
#define GIMP_GRADIENT(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_GRADIENT, GimpGradient))
#define GIMP_GRADIENT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_GRADIENT, GimpGradientClass))
#define GIMP_IS_GRADIENT(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_GRADIENT))
#define GIMP_IS_GRADIENT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_GRADIENT))


typedef struct _GimpGradientClass GimpGradientClass;

struct _GimpGradient
{
  GimpData             parent_instance;

  GimpGradientSegment *segments;

  /*< private >*/
  GimpGradientSegment *last_visited;
};

struct _GimpGradientClass
{
  GimpDataClass  parent_class;
};


GtkType               gimp_gradient_get_type         (void);
GimpGradient        * gimp_gradient_new              (const gchar   *name);

GimpGradient        * gimp_gradient_get_standard     (void);

GimpGradient        * gimp_gradient_load             (const gchar   *filename);

void                  gimp_gradient_get_color_at     (GimpGradient  *gradient,
						      gdouble        pos,
						      GimpRGB       *color);
GimpGradientSegment * gimp_gradient_get_segment_at   (GimpGradient  *grad,
						      gdouble        pos);


/*  gradient segment functions  */

GimpGradientSegment * gimp_gradient_segment_new      (void);
GimpGradientSegment * gimp_gradient_segment_get_last (GimpGradientSegment *seg);

void                  gimp_gradient_segment_free     (GimpGradientSegment *seg);
void                  gimp_gradient_segments_free    (GimpGradientSegment *seg);


#endif /* __GIMP_GRADIENT_H__ */
