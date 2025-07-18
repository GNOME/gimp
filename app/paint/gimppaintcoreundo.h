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


#define GIMP_TYPE_PAINT_CORE_UNDO            (gimp_paint_core_undo_get_type ())
#define GIMP_PAINT_CORE_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PAINT_CORE_UNDO, GimpPaintCoreUndo))
#define GIMP_PAINT_CORE_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PAINT_CORE_UNDO, GimpPaintCoreUndoClass))
#define GIMP_IS_PAINT_CORE_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PAINT_CORE_UNDO))
#define GIMP_IS_PAINT_CORE_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PAINT_CORE_UNDO))
#define GIMP_PAINT_CORE_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PAINT_CORE_UNDO, GimpPaintCoreUndoClass))


typedef struct _GimpPaintCoreUndo      GimpPaintCoreUndo;
typedef struct _GimpPaintCoreUndoClass GimpPaintCoreUndoClass;

struct _GimpPaintCoreUndo
{
  GimpUndo       parent_instance;

  GimpPaintCore *paint_core;
  GimpCoords     last_coords;
};

struct _GimpPaintCoreUndoClass
{
  GimpUndoClass  parent_class;
};


GType   gimp_paint_core_undo_get_type (void) G_GNUC_CONST;
