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

#include "core-types.h"

#include "ligmaimage.h"
#include "ligmaimage-guides.h"
#include "ligmaguide.h"
#include "ligmaguideundo.h"


static void   ligma_guide_undo_constructed  (GObject            *object);

static void   ligma_guide_undo_pop          (LigmaUndo            *undo,
                                            LigmaUndoMode         undo_mode,
                                            LigmaUndoAccumulator *accum);


G_DEFINE_TYPE (LigmaGuideUndo, ligma_guide_undo,
               LIGMA_TYPE_AUX_ITEM_UNDO)

#define parent_class ligma_guide_undo_parent_class


static void
ligma_guide_undo_class_init (LigmaGuideUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  LigmaUndoClass *undo_class   = LIGMA_UNDO_CLASS (klass);

  object_class->constructed = ligma_guide_undo_constructed;

  undo_class->pop           = ligma_guide_undo_pop;
}

static void
ligma_guide_undo_init (LigmaGuideUndo *undo)
{
}

static void
ligma_guide_undo_constructed (GObject *object)
{
  LigmaGuideUndo *guide_undo = LIGMA_GUIDE_UNDO (object);
  LigmaGuide     *guide;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  guide = LIGMA_GUIDE (LIGMA_AUX_ITEM_UNDO (object)->aux_item);

  ligma_assert (LIGMA_IS_GUIDE (guide));

  guide_undo->orientation = ligma_guide_get_orientation (guide);
  guide_undo->position    = ligma_guide_get_position (guide);
}

static void
ligma_guide_undo_pop (LigmaUndo              *undo,
                     LigmaUndoMode           undo_mode,
                     LigmaUndoAccumulator   *accum)
{
  LigmaGuideUndo       *guide_undo = LIGMA_GUIDE_UNDO (undo);
  LigmaGuide           *guide;
  LigmaOrientationType  orientation;
  gint                 position;
  gboolean             moved = FALSE;

  LIGMA_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  guide = LIGMA_GUIDE (LIGMA_AUX_ITEM_UNDO (undo)->aux_item);

  orientation = ligma_guide_get_orientation (guide);
  position    = ligma_guide_get_position (guide);

  if (position == LIGMA_GUIDE_POSITION_UNDEFINED)
    {
      ligma_image_add_guide (undo->image, guide, guide_undo->position);
    }
  else if (guide_undo->position == LIGMA_GUIDE_POSITION_UNDEFINED)
    {
      ligma_image_remove_guide (undo->image, guide, FALSE);
    }
  else
    {
      ligma_guide_set_position (guide, guide_undo->position);

      moved = TRUE;
    }

  ligma_guide_set_orientation (guide, guide_undo->orientation);

  if (moved || guide_undo->orientation != orientation)
    ligma_image_guide_moved (undo->image, guide);

  guide_undo->position    = position;
  guide_undo->orientation = orientation;
}
