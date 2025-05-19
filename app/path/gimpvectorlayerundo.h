/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   gimpvectorlayerundo.h
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

#ifndef __GIMP_VECTOR_LAYER_UNDO_H__
#define __GIMP_VECTOR_LAYER_UNDO_H__


#include "core/gimpitemundo.h"


#define GIMP_TYPE_VECTOR_LAYER_UNDO            (gimp_vector_layer_undo_get_type ())
#define GIMP_VECTOR_LAYER_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VECTOR_LAYER_UNDO, GimpVectorLayerUndo))
#define GIMP_VECTOR_LAYER_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VECTOR_LAYER_UNDO, GimpVectorLayerUndoClass))
#define GIMP_IS_VECTOR_LAYER_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_VECTOR_LAYER_UNDO))
#define GIMP_IS_VECTOR_LAYER_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VECTOR_LAYER_UNDO))
#define GIMP_VECTOR_LAYER_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VECTOR_LAYER_UNDO, GimpVectorLayerUndoClass))


typedef struct _GimpVectorLayerUndoClass GimpVectorLayerUndoClass;

struct _GimpVectorLayerUndo
{
  GimpItemUndo            parent_instance;

  GimpVectorLayerOptions *vector_layer_options;
  const GParamSpec       *pspec;
  GValue                 *value;
  gboolean                modified;
};

struct _GimpVectorLayerUndoClass
{
  GimpItemClass  parent_class;
};


GType      gimp_vector_layer_undo_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_VECTOR_LAYER_UNDO_H__ */
