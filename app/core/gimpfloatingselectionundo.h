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

#ifndef __LIGMA_FLOATING_SELECTION_UNDO_H__
#define __LIGMA_FLOATING_SELECTION_UNDO_H__


#include "ligmaitemundo.h"


#define LIGMA_TYPE_FLOATING_SELECTION_UNDO            (ligma_floating_selection_undo_get_type ())
#define LIGMA_FLOATING_SELECTION_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_FLOATING_SELECTION_UNDO, LigmaFloatingSelectionUndo))
#define LIGMA_FLOATING_SELECTION_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_FLOATING_SELECTION_UNDO, LigmaFloatingSelectionUndoClass))
#define LIGMA_IS_FLOATING_SELECTION_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_FLOATING_SELECTION_UNDO))
#define LIGMA_IS_FLOATING_SELECTION_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_FLOATING_SELECTION_UNDO))
#define LIGMA_FLOATING_SELECTION_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_FLOATING_SELECTION_UNDO, LigmaFloatingSelectionUndoClass))


typedef struct _LigmaFloatingSelectionUndo      LigmaFloatingSelectionUndo;
typedef struct _LigmaFloatingSelectionUndoClass LigmaFloatingSelectionUndoClass;

struct _LigmaFloatingSelectionUndo
{
  LigmaItemUndo  parent_instance;

  LigmaDrawable *drawable;
};

struct _LigmaFloatingSelectionUndoClass
{
  LigmaItemUndoClass  parent_class;
};


GType   ligma_floating_selection_undo_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_FLOATING_SELECTION_UNDO_H__ */
