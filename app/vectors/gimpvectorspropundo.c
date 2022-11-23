/* Ligma - The GNU Image Manipulation Program
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

#include "core/ligmaimage.h"

#include "ligmavectors.h"
#include "ligmavectorspropundo.h"


static void   ligma_vectors_prop_undo_constructed (GObject             *object);

static void   ligma_vectors_prop_undo_pop         (LigmaUndo            *undo,
                                                  LigmaUndoMode         undo_mode,
                                                  LigmaUndoAccumulator *accum);


G_DEFINE_TYPE (LigmaVectorsPropUndo, ligma_vectors_prop_undo, LIGMA_TYPE_ITEM_UNDO)

#define parent_class ligma_vectors_prop_undo_parent_class


static void
ligma_vectors_prop_undo_class_init (LigmaVectorsPropUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  LigmaUndoClass *undo_class   = LIGMA_UNDO_CLASS (klass);

  object_class->constructed = ligma_vectors_prop_undo_constructed;

  undo_class->pop           = ligma_vectors_prop_undo_pop;
}

static void
ligma_vectors_prop_undo_init (LigmaVectorsPropUndo *undo)
{
}

static void
ligma_vectors_prop_undo_constructed (GObject *object)
{
  /* LigmaVectors *vectors; */

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_VECTORS (LIGMA_ITEM_UNDO (object)->item));

  /* vectors = LIGMA_VECTORS (LIGMA_ITEM_UNDO (object)->item); */

  switch (LIGMA_UNDO (object)->undo_type)
    {
    default:
      ligma_assert_not_reached ();
    }
}

static void
ligma_vectors_prop_undo_pop (LigmaUndo            *undo,
                            LigmaUndoMode         undo_mode,
                            LigmaUndoAccumulator *accum)
{
#if 0
  LigmaVectorsPropUndo *vectors_prop_undo = LIGMA_VECTORS_PROP_UNDO (undo);
  LigmaVectors         *vectors           = LIGMA_VECTORS (LIGMA_ITEM_UNDO (undo)->item);
#endif

  LIGMA_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    default:
      ligma_assert_not_reached ();
    }
}
