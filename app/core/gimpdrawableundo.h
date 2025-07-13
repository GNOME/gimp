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

#include "gimpitemundo.h"


#define GIMP_TYPE_DRAWABLE_UNDO            (gimp_drawable_undo_get_type ())
#define GIMP_DRAWABLE_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DRAWABLE_UNDO, GimpDrawableUndo))
#define GIMP_DRAWABLE_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DRAWABLE_UNDO, GimpDrawableUndoClass))
#define GIMP_IS_DRAWABLE_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DRAWABLE_UNDO))
#define GIMP_IS_DRAWABLE_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DRAWABLE_UNDO))
#define GIMP_DRAWABLE_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DRAWABLE_UNDO, GimpDrawableUndoClass))


typedef struct _GimpDrawableUndo      GimpDrawableUndo;
typedef struct _GimpDrawableUndoClass GimpDrawableUndoClass;

struct _GimpDrawableUndo
{
  GimpItemUndo  parent_instance;

  GeglBuffer   *buffer;
  gint          x;
  gint          y;
};

struct _GimpDrawableUndoClass
{
  GimpItemUndoClass  parent_class;
};


GType   gimp_drawable_undo_get_type (void) G_GNUC_CONST;
