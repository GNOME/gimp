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

#include "core-types.h"

#include "gimpimage.h"
#include "gimpimage-sample-points.h"
#include "gimpsamplepoint.h"
#include "gimpsamplepointundo.h"


static void   gimp_sample_point_undo_constructed  (GObject             *object);

static void   gimp_sample_point_undo_pop          (GimpUndo            *undo,
                                                   GimpUndoMode         undo_mode,
                                                   GimpUndoAccumulator *accum);


G_DEFINE_TYPE (GimpSamplePointUndo, gimp_sample_point_undo,
               GIMP_TYPE_AUX_ITEM_UNDO)

#define parent_class gimp_sample_point_undo_parent_class


static void
gimp_sample_point_undo_class_init (GimpSamplePointUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructed = gimp_sample_point_undo_constructed;

  undo_class->pop           = gimp_sample_point_undo_pop;
}

static void
gimp_sample_point_undo_init (GimpSamplePointUndo *undo)
{
}

static void
gimp_sample_point_undo_constructed (GObject *object)
{
  GimpSamplePointUndo *sample_point_undo = GIMP_SAMPLE_POINT_UNDO (object);
  GimpSamplePoint     *sample_point;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  sample_point = GIMP_SAMPLE_POINT (GIMP_AUX_ITEM_UNDO (object)->aux_item);

  gimp_assert (GIMP_IS_SAMPLE_POINT (sample_point));

  gimp_sample_point_get_position (sample_point,
                                  &sample_point_undo->x,
                                  &sample_point_undo->y);
  sample_point_undo->pick_mode = gimp_sample_point_get_pick_mode (sample_point);
}

static void
gimp_sample_point_undo_pop (GimpUndo              *undo,
                            GimpUndoMode           undo_mode,
                            GimpUndoAccumulator   *accum)
{
  GimpSamplePointUndo *sample_point_undo = GIMP_SAMPLE_POINT_UNDO (undo);
  GimpSamplePoint     *sample_point;
  gint                 x;
  gint                 y;
  GimpColorPickMode    pick_mode;

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  sample_point = GIMP_SAMPLE_POINT (GIMP_AUX_ITEM_UNDO (undo)->aux_item);

  gimp_sample_point_get_position (sample_point, &x, &y);
  pick_mode = gimp_sample_point_get_pick_mode (sample_point);

  if (x == GIMP_SAMPLE_POINT_POSITION_UNDEFINED)
    {
      gimp_image_add_sample_point (undo->image,
                                   sample_point,
                                   sample_point_undo->x,
                                   sample_point_undo->y);
    }
  else if (sample_point_undo->x == GIMP_SAMPLE_POINT_POSITION_UNDEFINED)
    {
      gimp_image_remove_sample_point (undo->image, sample_point, FALSE);
    }
  else
    {
      gimp_sample_point_set_position (sample_point,
                                      sample_point_undo->x,
                                      sample_point_undo->y);
      gimp_sample_point_set_pick_mode (sample_point,
                                       sample_point_undo->pick_mode);

      gimp_image_sample_point_moved (undo->image, sample_point);
    }

  sample_point_undo->x         = x;
  sample_point_undo->y         = y;
  sample_point_undo->pick_mode = pick_mode;
}
