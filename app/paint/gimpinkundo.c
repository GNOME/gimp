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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "paint-types.h"

#include "gimpink.h"
#include "gimpink-blob.h"
#include "gimpinkundo.h"


static void   gimp_ink_undo_constructed (GObject             *object);

static void   gimp_ink_undo_pop         (GimpUndo            *undo,
                                         GimpUndoMode         undo_mode,
                                         GimpUndoAccumulator *accum);
static void   gimp_ink_undo_free        (GimpUndo            *undo,
                                         GimpUndoMode         undo_mode);


G_DEFINE_TYPE (GimpInkUndo, gimp_ink_undo, GIMP_TYPE_PAINT_CORE_UNDO)

#define parent_class gimp_ink_undo_parent_class


static void
gimp_ink_undo_class_init (GimpInkUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructed = gimp_ink_undo_constructed;

  undo_class->pop           = gimp_ink_undo_pop;
  undo_class->free          = gimp_ink_undo_free;
}

static void
gimp_ink_undo_init (GimpInkUndo *undo)
{
}

static void
gimp_ink_undo_constructed (GObject *object)
{
  GimpInkUndo *ink_undo = GIMP_INK_UNDO (object);
  GimpInk     *ink;

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_INK (GIMP_PAINT_CORE_UNDO (ink_undo)->paint_core));

  ink = GIMP_INK (GIMP_PAINT_CORE_UNDO (ink_undo)->paint_core);

  if (ink->start_blob)
    ink_undo->last_blob = gimp_blob_duplicate (ink->start_blob);
}

static void
gimp_ink_undo_pop (GimpUndo              *undo,
                   GimpUndoMode           undo_mode,
                   GimpUndoAccumulator   *accum)
{
  GimpInkUndo *ink_undo = GIMP_INK_UNDO (undo);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  if (GIMP_PAINT_CORE_UNDO (ink_undo)->paint_core)
    {
      GimpInk  *ink = GIMP_INK (GIMP_PAINT_CORE_UNDO (ink_undo)->paint_core);
      GimpBlob *tmp_blob;

      tmp_blob = ink->last_blob;
      ink->last_blob = ink_undo->last_blob;
      ink_undo->last_blob = tmp_blob;

    }
}

static void
gimp_ink_undo_free (GimpUndo     *undo,
                    GimpUndoMode  undo_mode)
{
  GimpInkUndo *ink_undo = GIMP_INK_UNDO (undo);

  if (ink_undo->last_blob)
    {
      g_free (ink_undo->last_blob);
      ink_undo->last_blob = NULL;
    }

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
