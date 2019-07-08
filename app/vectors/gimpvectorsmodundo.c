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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "vectors-types.h"

#include "gimpvectors.h"
#include "gimpvectorsmodundo.h"


static void     gimp_vectors_mod_undo_constructed (GObject             *object);

static gint64   gimp_vectors_mod_undo_get_memsize (GimpObject          *object,
                                                   gint64              *gui_size);

static void     gimp_vectors_mod_undo_pop         (GimpUndo            *undo,
                                                   GimpUndoMode         undo_mode,
                                                   GimpUndoAccumulator *accum);
static void     gimp_vectors_mod_undo_free        (GimpUndo            *undo,
                                                   GimpUndoMode         undo_mode);


G_DEFINE_TYPE (GimpVectorsModUndo, gimp_vectors_mod_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_vectors_mod_undo_parent_class


static void
gimp_vectors_mod_undo_class_init (GimpVectorsModUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpUndoClass   *undo_class        = GIMP_UNDO_CLASS (klass);

  object_class->constructed      = gimp_vectors_mod_undo_constructed;

  gimp_object_class->get_memsize = gimp_vectors_mod_undo_get_memsize;

  undo_class->pop                = gimp_vectors_mod_undo_pop;
  undo_class->free               = gimp_vectors_mod_undo_free;
}

static void
gimp_vectors_mod_undo_init (GimpVectorsModUndo *undo)
{
}

static void
gimp_vectors_mod_undo_constructed (GObject *object)
{
  GimpVectorsModUndo *vectors_mod_undo = GIMP_VECTORS_MOD_UNDO (object);
  GimpVectors        *vectors;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_VECTORS (GIMP_ITEM_UNDO (object)->item));

  vectors = GIMP_VECTORS (GIMP_ITEM_UNDO (object)->item);

  vectors_mod_undo->vectors =
    GIMP_VECTORS (gimp_item_duplicate (GIMP_ITEM (vectors),
                                       G_TYPE_FROM_INSTANCE (vectors)));
}

static gint64
gimp_vectors_mod_undo_get_memsize (GimpObject *object,
                                   gint64     *gui_size)
{
  GimpVectorsModUndo *vectors_mod_undo = GIMP_VECTORS_MOD_UNDO (object);
  gint64              memsize          = 0;

  memsize += gimp_object_get_memsize (GIMP_OBJECT (vectors_mod_undo->vectors),
                                      gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_vectors_mod_undo_pop (GimpUndo            *undo,
                           GimpUndoMode         undo_mode,
                           GimpUndoAccumulator *accum)
{
  GimpVectorsModUndo *vectors_mod_undo = GIMP_VECTORS_MOD_UNDO (undo);
  GimpVectors        *vectors          = GIMP_VECTORS (GIMP_ITEM_UNDO (undo)->item);
  GimpVectors        *temp;
  gint                offset_x;
  gint                offset_y;

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  temp = vectors_mod_undo->vectors;

  vectors_mod_undo->vectors =
    GIMP_VECTORS (gimp_item_duplicate (GIMP_ITEM (vectors),
                                       G_TYPE_FROM_INSTANCE (vectors)));

  gimp_vectors_freeze (vectors);

  gimp_vectors_copy_strokes (temp, vectors);

  gimp_item_get_offset (GIMP_ITEM (temp), &offset_x, &offset_y);
  gimp_item_set_offset (GIMP_ITEM (vectors), offset_x, offset_y);

  gimp_item_set_size (GIMP_ITEM (vectors),
                      gimp_item_get_width  (GIMP_ITEM (temp)),
                      gimp_item_get_height (GIMP_ITEM (temp)));

  g_object_unref (temp);

  gimp_vectors_thaw (vectors);
}

static void
gimp_vectors_mod_undo_free (GimpUndo     *undo,
                            GimpUndoMode  undo_mode)
{
  GimpVectorsModUndo *vectors_mod_undo = GIMP_VECTORS_MOD_UNDO (undo);

  g_clear_object (&vectors_mod_undo->vectors);

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
