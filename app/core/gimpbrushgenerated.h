/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_BRUSH_GENERATED_H__
#define __LIGMA_BRUSH_GENERATED_H__


#include "ligmabrush.h"


#define LIGMA_TYPE_BRUSH_GENERATED            (ligma_brush_generated_get_type ())
#define LIGMA_BRUSH_GENERATED(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BRUSH_GENERATED, LigmaBrushGenerated))
#define LIGMA_BRUSH_GENERATED_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BRUSH_GENERATED, LigmaBrushGeneratedClass))
#define LIGMA_IS_BRUSH_GENERATED(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BRUSH_GENERATED))
#define LIGMA_IS_BRUSH_GENERATED_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BRUSH_GENERATED))
#define LIGMA_BRUSH_GENERATED_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BRUSH_GENERATED, LigmaBrushGeneratedClass))


typedef struct _LigmaBrushGeneratedClass LigmaBrushGeneratedClass;

struct _LigmaBrushGenerated
{
  LigmaBrush               parent_instance;

  LigmaBrushGeneratedShape shape;
  gfloat                  radius;
  gint                    spikes;       /* 2 - 20     */
  gfloat                  hardness;     /* 0.0 - 1.0  */
  gfloat                  aspect_ratio; /* y/x        */
  gfloat                  angle;        /* in degrees */
};

struct _LigmaBrushGeneratedClass
{
  LigmaBrushClass  parent_class;
};


GType       ligma_brush_generated_get_type     (void) G_GNUC_CONST;

LigmaData  * ligma_brush_generated_new          (const gchar             *name,
                                               LigmaBrushGeneratedShape  shape,
                                               gfloat                   radius,
                                               gint                     spikes,
                                               gfloat                   hardness,
                                               gfloat                   aspect_ratio,
                                               gfloat                   angle);

LigmaBrushGeneratedShape
        ligma_brush_generated_set_shape        (LigmaBrushGenerated      *brush,
                                               LigmaBrushGeneratedShape  shape);
gfloat  ligma_brush_generated_set_radius       (LigmaBrushGenerated      *brush,
                                               gfloat                   radius);
gint    ligma_brush_generated_set_spikes       (LigmaBrushGenerated      *brush,
                                               gint                     spikes);
gfloat  ligma_brush_generated_set_hardness     (LigmaBrushGenerated      *brush,
                                               gfloat                   hardness);
gfloat  ligma_brush_generated_set_aspect_ratio (LigmaBrushGenerated      *brush,
                                               gfloat                   ratio);
gfloat  ligma_brush_generated_set_angle        (LigmaBrushGenerated      *brush,
                                               gfloat                   angle);

LigmaBrushGeneratedShape
        ligma_brush_generated_get_shape        (LigmaBrushGenerated      *brush);
gfloat  ligma_brush_generated_get_radius       (LigmaBrushGenerated      *brush);
gint    ligma_brush_generated_get_spikes       (LigmaBrushGenerated      *brush);
gfloat  ligma_brush_generated_get_hardness     (LigmaBrushGenerated      *brush);
gfloat  ligma_brush_generated_get_aspect_ratio (LigmaBrushGenerated      *brush);
gfloat  ligma_brush_generated_get_angle        (LigmaBrushGenerated      *brush);


#endif  /*  __LIGMA_BRUSH_GENERATED_H__  */
