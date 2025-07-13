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

#include "core/gimpundo.h"


#define GIMP_TYPE_TRANSFORM_GRID_TOOL_UNDO            (gimp_transform_grid_tool_undo_get_type ())
#define GIMP_TRANSFORM_GRID_TOOL_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TRANSFORM_GRID_TOOL_UNDO, GimpTransformGridToolUndo))
#define GIMP_TRANSFORM_GRID_TOOL_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TRANSFORM_GRID_TOOL_UNDO, GimpTransformGridToolUndoClass))
#define GIMP_IS_TRANSFORM_GRID_TOOL_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TRANSFORM_GRID_TOOL_UNDO))
#define GIMP_IS_TRANSFORM_GRID_TOOL_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TRANSFORM_GRID_TOOL_UNDO))
#define GIMP_TRANSFORM_GRID_TOOL_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TRANSFORM_GRID_TOOL_UNDO, GimpTransformGridToolUndoClass))


typedef struct _GimpTransformGridToolUndo      GimpTransformGridToolUndo;
typedef struct _GimpTransformGridToolUndoClass GimpTransformGridToolUndoClass;

struct _GimpTransformGridToolUndo
{
  GimpUndo               parent_instance;

  GimpTransformGridTool *tg_tool;
  TransInfo              trans_infos[2];
#if 0
  TileManager           *original;
#endif
};

struct _GimpTransformGridToolUndoClass
{
  GimpUndoClass  parent_class;
};


GType   gimp_transform_grid_tool_undo_get_type (void) G_GNUC_CONST;
