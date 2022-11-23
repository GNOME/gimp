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

#if 0

#ifndef __LIGMA_FOREGROUND_SELECT_TOOL_UNDO_H__
#define __LIGMA_FOREGROUND_SELECT_TOOL_UNDO_H__


#include "core/ligmaundo.h"


#define LIGMA_TYPE_FOREGROUND_SELECT_TOOL_UNDO            (ligma_foreground_select_tool_undo_get_type ())
#define LIGMA_FOREGROUND_SELECT_TOOL_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_FOREGROUND_SELECT_TOOL_UNDO, LigmaForegroundSelectToolUndo))
#define LIGMA_FOREGROUND_SELECT_TOOL_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_FOREGROUND_SELECT_TOOL_UNDO, LigmaForegroundSelectToolUndoClass))
#define LIGMA_IS_FOREGROUND_SELECT_TOOL_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_FOREGROUND_SELECT_TOOL_UNDO))
#define LIGMA_IS_FOREGROUND_SELECT_TOOL_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_FOREGROUND_SELECT_TOOL_UNDO))
#define LIGMA_FOREGROUND_SELECT_TOOL_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_FOREGROUND_SELECT_TOOL_UNDO, LigmaForegroundSelectToolUndoClass))


typedef struct _LigmaForegroundSelectToolUndo      LigmaForegroundSelectToolUndo;
typedef struct _LigmaForegroundSelectToolUndoClass LigmaForegroundSelectToolUndoClass;

struct _LigmaForegroundSelectToolUndo
{
  LigmaUndo                  parent_instance;

  LigmaForegroundSelectTool *foreground_select_tool;
};

struct _LigmaForegroundSelectToolUndoClass
{
  LigmaUndoClass  parent_class;
};


GType   ligma_foreground_select_tool_undo_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_FOREGROUND_SELECT_TOOL_UNDO_H__ */

#endif
