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

#include "vectors-types.h"

#include "core/gimpimage.h"

#include "gimpvectors.h"
#include "gimpvectorspropundo.h"


static void   gimp_vectors_prop_undo_constructed (GObject             *object);

static void   gimp_vectors_prop_undo_pop         (GimpUndo            *undo,
                                                  GimpUndoMode         undo_mode,
                                                  GimpUndoAccumulator *accum);


G_DEFINE_TYPE (GimpVectorsPropUndo, gimp_vectors_prop_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_vectors_prop_undo_parent_class


static void
gimp_vectors_prop_undo_class_init (GimpVectorsPropUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructed = gimp_vectors_prop_undo_constructed;

  undo_class->pop           = gimp_vectors_prop_undo_pop;
}

static void
gimp_vectors_prop_undo_init (GimpVectorsPropUndo *undo)
{
}

static void
gimp_vectors_prop_undo_constructed (GObject *object)
{
  /* GimpVectors *vectors; */

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_VECTORS (GIMP_ITEM_UNDO (object)->item));

  /* vectors = GIMP_VECTORS (GIMP_ITEM_UNDO (object)->item); */

  switch (GIMP_UNDO (object)->undo_type)
    {
    default:
      gimp_assert_not_reached ();
    }
}

static void
gimp_vectors_prop_undo_pop (GimpUndo            *undo,
                            GimpUndoMode         undo_mode,
                            GimpUndoAccumulator *accum)
{
#if 0
  GimpVectorsPropUndo *vectors_prop_undo = GIMP_VECTORS_PROP_UNDO (undo);
  GimpVectors         *vectors           = GIMP_VECTORS (GIMP_ITEM_UNDO (undo)->item);
#endif

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    default:
      gimp_assert_not_reached ();
    }
}
