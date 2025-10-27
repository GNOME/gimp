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

#pragma once

#include "gimplayermask.h"


#define GIMP_TYPE_LAYER_VECTOR_MASK            (gimp_layer_vector_mask_get_type ())
#define GIMP_LAYER_VECTOR_MASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LAYER_VECTOR_MASK, GimpLayerVectorMask))
#define GIMP_LAYER_VECTOR_MASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LAYER_VECTOR_MASK, GimpLayerVectorMaskClass))
#define GIMP_IS_LAYER_VECTOR_MASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LAYER_VECTOR_MASK))
#define GIMP_IS_LAYER_VECTOR_MASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LAYER_VECTOR_MASK))
#define GIMP_LAYER_VECTOR_MASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_LAYER_VECTOR_MASK, GimpLayerVectorMaskClass))


typedef struct _GimpLayerVectorMaskClass  GimpLayerVectorMaskClass;

struct _GimpLayerVectorMask
{
  GimpLayerMask  parent_instance;

  GimpLayer     *layer;
  GimpPath      *path;
};

struct _GimpLayerVectorMaskClass
{
  GimpLayerMaskClass  parent_class;
};


GType           gimp_layer_vector_mask_get_type  (void) G_GNUC_CONST;

GimpLayerVectorMask * gimp_layer_vector_mask_new (GimpImage           *image,
                                                  GimpPath            *path,
                                                  gint                 width,
                                                  gint                 height,
                                                  const gchar         *name);

gboolean gimp_layer_vector_mask_render           (GimpLayerVectorMask *layer_mask);

void            gimp_layer_vector_mask_set_path  (GimpLayerVectorMask *layer_mask,
                                                  GimpPath            *layer);
GimpPath      * gimp_layer_vector_mask_get_path  (GimpLayerVectorMask *layer_mask);
