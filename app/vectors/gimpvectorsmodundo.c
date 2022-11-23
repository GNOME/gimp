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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "vectors-types.h"

#include "ligmavectors.h"
#include "ligmavectorsmodundo.h"


static void     ligma_vectors_mod_undo_constructed (GObject             *object);

static gint64   ligma_vectors_mod_undo_get_memsize (LigmaObject          *object,
                                                   gint64              *gui_size);

static void     ligma_vectors_mod_undo_pop         (LigmaUndo            *undo,
                                                   LigmaUndoMode         undo_mode,
                                                   LigmaUndoAccumulator *accum);
static void     ligma_vectors_mod_undo_free        (LigmaUndo            *undo,
                                                   LigmaUndoMode         undo_mode);


G_DEFINE_TYPE (LigmaVectorsModUndo, ligma_vectors_mod_undo, LIGMA_TYPE_ITEM_UNDO)

#define parent_class ligma_vectors_mod_undo_parent_class


static void
ligma_vectors_mod_undo_class_init (LigmaVectorsModUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaUndoClass   *undo_class        = LIGMA_UNDO_CLASS (klass);

  object_class->constructed      = ligma_vectors_mod_undo_constructed;

  ligma_object_class->get_memsize = ligma_vectors_mod_undo_get_memsize;

  undo_class->pop                = ligma_vectors_mod_undo_pop;
  undo_class->free               = ligma_vectors_mod_undo_free;
}

static void
ligma_vectors_mod_undo_init (LigmaVectorsModUndo *undo)
{
}

static void
ligma_vectors_mod_undo_constructed (GObject *object)
{
  LigmaVectorsModUndo *vectors_mod_undo = LIGMA_VECTORS_MOD_UNDO (object);
  LigmaVectors        *vectors;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_VECTORS (LIGMA_ITEM_UNDO (object)->item));

  vectors = LIGMA_VECTORS (LIGMA_ITEM_UNDO (object)->item);

  vectors_mod_undo->vectors =
    LIGMA_VECTORS (ligma_item_duplicate (LIGMA_ITEM (vectors),
                                       G_TYPE_FROM_INSTANCE (vectors)));
}

static gint64
ligma_vectors_mod_undo_get_memsize (LigmaObject *object,
                                   gint64     *gui_size)
{
  LigmaVectorsModUndo *vectors_mod_undo = LIGMA_VECTORS_MOD_UNDO (object);
  gint64              memsize          = 0;

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (vectors_mod_undo->vectors),
                                      gui_size);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
ligma_vectors_mod_undo_pop (LigmaUndo            *undo,
                           LigmaUndoMode         undo_mode,
                           LigmaUndoAccumulator *accum)
{
  LigmaVectorsModUndo *vectors_mod_undo = LIGMA_VECTORS_MOD_UNDO (undo);
  LigmaVectors        *vectors          = LIGMA_VECTORS (LIGMA_ITEM_UNDO (undo)->item);
  LigmaVectors        *temp;
  gint                offset_x;
  gint                offset_y;

  LIGMA_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  temp = vectors_mod_undo->vectors;

  vectors_mod_undo->vectors =
    LIGMA_VECTORS (ligma_item_duplicate (LIGMA_ITEM (vectors),
                                       G_TYPE_FROM_INSTANCE (vectors)));

  ligma_vectors_freeze (vectors);

  ligma_vectors_copy_strokes (temp, vectors);

  ligma_item_get_offset (LIGMA_ITEM (temp), &offset_x, &offset_y);
  ligma_item_set_offset (LIGMA_ITEM (vectors), offset_x, offset_y);

  ligma_item_set_size (LIGMA_ITEM (vectors),
                      ligma_item_get_width  (LIGMA_ITEM (temp)),
                      ligma_item_get_height (LIGMA_ITEM (temp)));

  g_object_unref (temp);

  ligma_vectors_thaw (vectors);
}

static void
ligma_vectors_mod_undo_free (LigmaUndo     *undo,
                            LigmaUndoMode  undo_mode)
{
  LigmaVectorsModUndo *vectors_mod_undo = LIGMA_VECTORS_MOD_UNDO (undo);

  g_clear_object (&vectors_mod_undo->vectors);

  LIGMA_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
