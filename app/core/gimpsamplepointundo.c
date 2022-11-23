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
#include "ligmaimage-sample-points.h"
#include "ligmasamplepoint.h"
#include "ligmasamplepointundo.h"


static void   ligma_sample_point_undo_constructed  (GObject             *object);

static void   ligma_sample_point_undo_pop          (LigmaUndo            *undo,
                                                   LigmaUndoMode         undo_mode,
                                                   LigmaUndoAccumulator *accum);


G_DEFINE_TYPE (LigmaSamplePointUndo, ligma_sample_point_undo,
               LIGMA_TYPE_AUX_ITEM_UNDO)

#define parent_class ligma_sample_point_undo_parent_class


static void
ligma_sample_point_undo_class_init (LigmaSamplePointUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  LigmaUndoClass *undo_class   = LIGMA_UNDO_CLASS (klass);

  object_class->constructed = ligma_sample_point_undo_constructed;

  undo_class->pop           = ligma_sample_point_undo_pop;
}

static void
ligma_sample_point_undo_init (LigmaSamplePointUndo *undo)
{
}

static void
ligma_sample_point_undo_constructed (GObject *object)
{
  LigmaSamplePointUndo *sample_point_undo = LIGMA_SAMPLE_POINT_UNDO (object);
  LigmaSamplePoint     *sample_point;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  sample_point = LIGMA_SAMPLE_POINT (LIGMA_AUX_ITEM_UNDO (object)->aux_item);

  ligma_assert (LIGMA_IS_SAMPLE_POINT (sample_point));

  ligma_sample_point_get_position (sample_point,
                                  &sample_point_undo->x,
                                  &sample_point_undo->y);
  sample_point_undo->pick_mode = ligma_sample_point_get_pick_mode (sample_point);
}

static void
ligma_sample_point_undo_pop (LigmaUndo              *undo,
                            LigmaUndoMode           undo_mode,
                            LigmaUndoAccumulator   *accum)
{
  LigmaSamplePointUndo *sample_point_undo = LIGMA_SAMPLE_POINT_UNDO (undo);
  LigmaSamplePoint     *sample_point;
  gint                 x;
  gint                 y;
  LigmaColorPickMode    pick_mode;

  LIGMA_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  sample_point = LIGMA_SAMPLE_POINT (LIGMA_AUX_ITEM_UNDO (undo)->aux_item);

  ligma_sample_point_get_position (sample_point, &x, &y);
  pick_mode = ligma_sample_point_get_pick_mode (sample_point);

  if (x == LIGMA_SAMPLE_POINT_POSITION_UNDEFINED)
    {
      ligma_image_add_sample_point (undo->image,
                                   sample_point,
                                   sample_point_undo->x,
                                   sample_point_undo->y);
    }
  else if (sample_point_undo->x == LIGMA_SAMPLE_POINT_POSITION_UNDEFINED)
    {
      ligma_image_remove_sample_point (undo->image, sample_point, FALSE);
    }
  else
    {
      ligma_sample_point_set_position (sample_point,
                                      sample_point_undo->x,
                                      sample_point_undo->y);
      ligma_sample_point_set_pick_mode (sample_point,
                                       sample_point_undo->pick_mode);

      ligma_image_sample_point_moved (undo->image, sample_point);
    }

  sample_point_undo->x         = x;
  sample_point_undo->y         = y;
  sample_point_undo->pick_mode = pick_mode;
}
