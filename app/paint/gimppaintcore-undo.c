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

#include "config.h"

#include <glib-object.h>

#include "paint-types.h"

#include "core/gimpimage-undo.h"
#include "core/gimpimage.h"
#include "core/gimpundo.h"

#include "gimppaintcore.h"
#include "gimppaintcore-undo.h"


/****************/
/*  Paint Undo  */
/****************/

typedef struct _PaintUndo PaintUndo;

struct _PaintUndo
{
  GimpPaintCore *core;
  GimpCoords     last_coords;
};

static gboolean undo_pop_paint  (GimpUndo            *undo,
                                 GimpUndoMode         undo_mode,
                                 GimpUndoAccumulator *accum);
static void     undo_free_paint (GimpUndo            *undo,
                                 GimpUndoMode         undo_mode);

gboolean
gimp_paint_core_real_push_undo (GimpPaintCore *core,
                                GimpImage     *image,
                                const gchar   *undo_desc)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_PAINT_CORE (core), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_UNDO,
                                   sizeof (PaintUndo),
                                   sizeof (PaintUndo),
                                   GIMP_UNDO_PAINT, undo_desc,
                                   FALSE,
                                   undo_pop_paint,
                                   undo_free_paint,
                                   NULL)))
    {
      PaintUndo *pu = new->data;

      pu->core        = core;
      pu->last_coords = core->start_coords;

      g_object_add_weak_pointer (G_OBJECT (core), (gpointer) &pu->core);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_paint (GimpUndo            *undo,
                GimpUndoMode         undo_mode,
                GimpUndoAccumulator *accum)
{
  PaintUndo *pu = undo->data;

  /*  only pop if the core still exists  */
  if (pu->core)
    {
      GimpCoords tmp_coords;

      tmp_coords            = pu->core->last_coords;
      pu->core->last_coords = pu->last_coords;
      pu->last_coords       = tmp_coords;
    }

  return TRUE;
}

static void
undo_free_paint (GimpUndo     *undo,
                 GimpUndoMode  undo_mode)
{
  PaintUndo *pu = undo->data;

  if (pu->core)
    g_object_remove_weak_pointer (G_OBJECT (pu->core), (gpointer) &pu->core);

  g_free (pu);
}
