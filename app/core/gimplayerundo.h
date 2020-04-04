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

#ifndef __GIMP_LAYER_UNDO_H__
#define __GIMP_LAYER_UNDO_H__


#include "gimpitemundo.h"


#define GIMP_TYPE_LAYER_UNDO            (gimp_layer_undo_get_type ())
#define GIMP_LAYER_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LAYER_UNDO, GimpLayerUndo))
#define GIMP_LAYER_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LAYER_UNDO, GimpLayerUndoClass))
#define GIMP_IS_LAYER_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LAYER_UNDO))
#define GIMP_IS_LAYER_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LAYER_UNDO))
#define GIMP_LAYER_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_LAYER_UNDO, GimpLayerUndoClass))


typedef struct _GimpLayerUndo      GimpLayerUndo;
typedef struct _GimpLayerUndoClass GimpLayerUndoClass;

struct _GimpLayerUndo
{
  GimpItemUndo  parent_instance;

  GimpLayer    *prev_parent;
  gint          prev_position;   /*  former position in list  */
  GList        *prev_layers;     /*  previous selected layers */
};

struct _GimpLayerUndoClass
{
  GimpItemUndoClass  parent_class;
};


GType   gimp_layer_undo_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_LAYER_UNDO_H__ */
