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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "paint-types.h"

#include "ligmaink.h"
#include "ligmaink-blob.h"
#include "ligmainkundo.h"


static void   ligma_ink_undo_constructed (GObject             *object);

static void   ligma_ink_undo_pop         (LigmaUndo            *undo,
                                         LigmaUndoMode         undo_mode,
                                         LigmaUndoAccumulator *accum);
static void   ligma_ink_undo_free        (LigmaUndo            *undo,
                                         LigmaUndoMode         undo_mode);


G_DEFINE_TYPE (LigmaInkUndo, ligma_ink_undo, LIGMA_TYPE_PAINT_CORE_UNDO)

#define parent_class ligma_ink_undo_parent_class


static void
ligma_ink_undo_class_init (LigmaInkUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  LigmaUndoClass *undo_class   = LIGMA_UNDO_CLASS (klass);

  object_class->constructed = ligma_ink_undo_constructed;

  undo_class->pop           = ligma_ink_undo_pop;
  undo_class->free          = ligma_ink_undo_free;
}

static void
ligma_ink_undo_init (LigmaInkUndo *undo)
{
  undo->last_blobs = NULL;
}

static void
ligma_ink_undo_constructed (GObject *object)
{
  LigmaInkUndo *ink_undo = LIGMA_INK_UNDO (object);
  LigmaInk     *ink;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_INK (LIGMA_PAINT_CORE_UNDO (ink_undo)->paint_core));

  ink = LIGMA_INK (LIGMA_PAINT_CORE_UNDO (ink_undo)->paint_core);

  if (ink->start_blobs)
    {
      gint      i;
      LigmaBlob *blob;

      for (i = 0; i < g_list_length (ink->start_blobs); i++)
        {
          blob = g_list_nth_data (ink->start_blobs, i);

          ink_undo->last_blobs = g_list_prepend (ink_undo->last_blobs,
                                                 ligma_blob_duplicate (blob));
        }
      ink_undo->last_blobs = g_list_reverse (ink_undo->last_blobs);
    }
}

static void
ligma_ink_undo_pop (LigmaUndo              *undo,
                   LigmaUndoMode           undo_mode,
                   LigmaUndoAccumulator   *accum)
{
  LigmaInkUndo *ink_undo = LIGMA_INK_UNDO (undo);

  LIGMA_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  if (LIGMA_PAINT_CORE_UNDO (ink_undo)->paint_core)
    {
      LigmaInk  *ink = LIGMA_INK (LIGMA_PAINT_CORE_UNDO (ink_undo)->paint_core);
      GList    *tmp_blobs;

      tmp_blobs = ink->last_blobs;
      ink->last_blobs = ink_undo->last_blobs;
      ink_undo->last_blobs = tmp_blobs;
    }
}

static void
ligma_ink_undo_free (LigmaUndo     *undo,
                    LigmaUndoMode  undo_mode)
{
  LigmaInkUndo *ink_undo = LIGMA_INK_UNDO (undo);

  if (ink_undo->last_blobs)
    {
      g_list_free_full (ink_undo->last_blobs, g_free);
      ink_undo->last_blobs = NULL;
    }

  LIGMA_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
