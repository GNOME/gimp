/* Gimp - The GNU Image Manipulation Program
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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "path-types.h"

#include "core/gimpimage.h"

#include "gimppath.h"
#include "gimppathpropundo.h"


static void   gimp_path_prop_undo_constructed (GObject             *object);

static void   gimp_path_prop_undo_pop         (GimpUndo            *undo,
                                               GimpUndoMode         undo_mode,
                                               GimpUndoAccumulator *accum);


G_DEFINE_TYPE (GimpPathPropUndo, gimp_path_prop_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_path_prop_undo_parent_class


static void
gimp_path_prop_undo_class_init (GimpPathPropUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructed = gimp_path_prop_undo_constructed;

  undo_class->pop           = gimp_path_prop_undo_pop;
}

static void
gimp_path_prop_undo_init (GimpPathPropUndo *undo)
{
}

static void
gimp_path_prop_undo_constructed (GObject *object)
{
  /* GimpPath *path; */

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_PATH (GIMP_ITEM_UNDO (object)->item));

  /* path = GIMP_PATH (GIMP_ITEM_UNDO (object)->item); */

  switch (GIMP_UNDO (object)->undo_type)
    {
    default:
      gimp_assert_not_reached ();
    }
}

static void
gimp_path_prop_undo_pop (GimpUndo            *undo,
                         GimpUndoMode         undo_mode,
                         GimpUndoAccumulator *accum)
{
#if 0
  GimpPathPropUndo *path_prop_undo = GIMP_PATH_PROP_UNDO (undo);
  GimpPath         *path           = GIMP_PATH (GIMP_ITEM_UNDO (undo)->item);
#endif

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    default:
      gimp_assert_not_reached ();
    }
}
