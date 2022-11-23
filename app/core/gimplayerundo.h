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

#ifndef __LIGMA_LAYER_UNDO_H__
#define __LIGMA_LAYER_UNDO_H__


#include "ligmaitemundo.h"


#define LIGMA_TYPE_LAYER_UNDO            (ligma_layer_undo_get_type ())
#define LIGMA_LAYER_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_LAYER_UNDO, LigmaLayerUndo))
#define LIGMA_LAYER_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_LAYER_UNDO, LigmaLayerUndoClass))
#define LIGMA_IS_LAYER_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_LAYER_UNDO))
#define LIGMA_IS_LAYER_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_LAYER_UNDO))
#define LIGMA_LAYER_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_LAYER_UNDO, LigmaLayerUndoClass))


typedef struct _LigmaLayerUndo      LigmaLayerUndo;
typedef struct _LigmaLayerUndoClass LigmaLayerUndoClass;

struct _LigmaLayerUndo
{
  LigmaItemUndo  parent_instance;

  LigmaLayer    *prev_parent;
  gint          prev_position;   /*  former position in list  */
  GList        *prev_layers;     /*  previous selected layers */
};

struct _LigmaLayerUndoClass
{
  LigmaItemUndoClass  parent_class;
};


GType   ligma_layer_undo_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_LAYER_UNDO_H__ */
