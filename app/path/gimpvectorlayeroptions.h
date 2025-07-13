/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   gimpvectorlayeroptions.h
 *
 *   Copyright 2006 Hendrik Boom
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_VECTOR_LAYER_OPTIONS_H__
#define __GIMP_VECTOR_LAYER_OPTIONS_H__


#include "core/gimpobject.h"


#define GIMP_TYPE_VECTOR_LAYER_OPTIONS            (gimp_vector_layer_options_get_type ())
#define GIMP_VECTOR_LAYER_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VECTOR_LAYER_OPTIONS, GimpVectorLayerOptions))
#define GIMP_VECTOR_LAYER_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VECTOR_LAYER_OPTIONS, GimpVectorLayerOptionsClass))
#define GIMP_IS_VECTOR_LAYER_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_VECTOR_LAYER_OPTIONS))
#define GIMP_IS_VECTOR_LAYER_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VECTOR_LAYER_OPTIONS))
#define GIMP_VECTOR_LAYER_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VECTOR_LAYER_OPTIONS, GimpVectorLayerOptionsClass))


typedef struct _GimpVectorLayerOptionsClass GimpVectorLayerOptionsClass;

struct _GimpVectorLayerOptions
{
  GimpObject         parent_instance;

  Gimp              *gimp;

  GimpTattoo         path_tattoo;
  GimpPath          *path;

  gboolean           enable_fill;
  gboolean           enable_stroke;
  GimpFillOptions   *fill_options;
  GimpStrokeOptions *stroke_options;
};

struct _GimpVectorLayerOptionsClass
{
  GimpObjectClass  parent_class;
};

GType                    gimp_vector_layer_options_get_type (void) G_GNUC_CONST;

GimpVectorLayerOptions * gimp_vector_layer_options_new      (GimpImage   *image,
                                                             GimpPath    *path,
                                                             GimpContext *context);


#endif /* __GIMP_VECTOR_LAYER_OPTIONS_H__ */
