/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_TRANSFORM_TOOL_UNDO_H__
#define __GIMP_TRANSFORM_TOOL_UNDO_H__


#include "core/gimpundo.h"


#define GIMP_TYPE_TRANSFORM_TOOL_UNDO            (gimp_transform_tool_undo_get_type ())
#define GIMP_TRANSFORM_TOOL_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TRANSFORM_TOOL_UNDO, GimpTransformToolUndo))
#define GIMP_TRANSFORM_TOOL_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TRANSFORM_TOOL_UNDO, GimpTransformToolUndoClass))
#define GIMP_IS_TRANSFORM_TOOL_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TRANSFORM_TOOL_UNDO))
#define GIMP_IS_TRANSFORM_TOOL_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TRANSFORM_TOOL_UNDO))
#define GIMP_TRANSFORM_TOOL_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TRANSFORM_TOOL_UNDO, GimpTransformToolUndoClass))


typedef struct _GimpTransformToolUndoClass GimpTransformToolUndoClass;

struct _GimpTransformToolUndo
{
  GimpUndo           parent_instance;

  GimpTransformTool *transform_tool;
  TransInfo          trans_info;
  TileManager       *original;
};

struct _GimpTransformToolUndoClass
{
  GimpUndoClass  parent_class;
};


GType   gimp_transform_tool_undo_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_TRANSFORM_TOOL_UNDO_H__ */
