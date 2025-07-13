/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * brush_generated module Copyright 1998 Jay Cox <jaycox@earthlink.net>
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

#include "gimpbrush.h"


#define GIMP_TYPE_BRUSH_GENERATED            (gimp_brush_generated_get_type ())
#define GIMP_BRUSH_GENERATED(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BRUSH_GENERATED, GimpBrushGenerated))
#define GIMP_BRUSH_GENERATED_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BRUSH_GENERATED, GimpBrushGeneratedClass))
#define GIMP_IS_BRUSH_GENERATED(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BRUSH_GENERATED))
#define GIMP_IS_BRUSH_GENERATED_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BRUSH_GENERATED))
#define GIMP_BRUSH_GENERATED_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BRUSH_GENERATED, GimpBrushGeneratedClass))


/* When changing these values, also update it in pdb/groups/brush.pdb */
#define GIMP_BRUSH_GENERATED_MIN_RADIUS         0.1
#define GIMP_BRUSH_GENERATED_MAX_RADIUS      4000.0

typedef struct _GimpBrushGeneratedClass GimpBrushGeneratedClass;

struct _GimpBrushGenerated
{
  GimpBrush               parent_instance;

  GimpBrushGeneratedShape shape;
  gfloat                  radius;
  gint                    spikes;       /* 2 - 20     */
  gfloat                  hardness;     /* 0.0 - 1.0  */
  gfloat                  aspect_ratio; /* y/x        */
  gfloat                  angle;        /* in degrees */
};

struct _GimpBrushGeneratedClass
{
  GimpBrushClass  parent_class;
};


GType       gimp_brush_generated_get_type     (void) G_GNUC_CONST;

GimpData  * gimp_brush_generated_new          (const gchar             *name,
                                               GimpBrushGeneratedShape  shape,
                                               gfloat                   radius,
                                               gint                     spikes,
                                               gfloat                   hardness,
                                               gfloat                   aspect_ratio,
                                               gfloat                   angle);

GimpBrushGeneratedShape
        gimp_brush_generated_set_shape        (GimpBrushGenerated      *brush,
                                               GimpBrushGeneratedShape  shape);
gfloat  gimp_brush_generated_set_radius       (GimpBrushGenerated      *brush,
                                               gfloat                   radius);
gint    gimp_brush_generated_set_spikes       (GimpBrushGenerated      *brush,
                                               gint                     spikes);
gfloat  gimp_brush_generated_set_hardness     (GimpBrushGenerated      *brush,
                                               gfloat                   hardness);
gfloat  gimp_brush_generated_set_aspect_ratio (GimpBrushGenerated      *brush,
                                               gfloat                   ratio);
gfloat  gimp_brush_generated_set_angle        (GimpBrushGenerated      *brush,
                                               gfloat                   angle);

GimpBrushGeneratedShape
        gimp_brush_generated_get_shape        (GimpBrushGenerated      *brush);
gfloat  gimp_brush_generated_get_radius       (GimpBrushGenerated      *brush);
gint    gimp_brush_generated_get_spikes       (GimpBrushGenerated      *brush);
gfloat  gimp_brush_generated_get_hardness     (GimpBrushGenerated      *brush);
gfloat  gimp_brush_generated_get_aspect_ratio (GimpBrushGenerated      *brush);
gfloat  gimp_brush_generated_get_angle        (GimpBrushGenerated      *brush);
