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
#include "gimpimage-guides.h"
#include "gimpguide.h"
#include "gimpguideundo.h"


static void   gimp_guide_undo_constructed  (GObject            *object);

static void   gimp_guide_undo_pop          (GimpUndo            *undo,
                                            GimpUndoMode         undo_mode,
                                            GimpUndoAccumulator *accum);


G_DEFINE_TYPE (GimpGuideUndo, gimp_guide_undo,
               GIMP_TYPE_AUX_ITEM_UNDO)

#define parent_class gimp_guide_undo_parent_class


static void
gimp_guide_undo_class_init (GimpGuideUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructed = gimp_guide_undo_constructed;

  undo_class->pop           = gimp_guide_undo_pop;
}

static void
gimp_guide_undo_init (GimpGuideUndo *undo)
{
}

static void
gimp_guide_undo_constructed (GObject *object)
{
  GimpGuideUndo *guide_undo = GIMP_GUIDE_UNDO (object);
  GimpGuide     *guide;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  guide = GIMP_GUIDE (GIMP_AUX_ITEM_UNDO (object)->aux_item);

  gimp_assert (GIMP_IS_GUIDE (guide));

  guide_undo->orientation = gimp_guide_get_orientation (guide);
  guide_undo->position    = gimp_guide_get_position (guide);
}

static void
gimp_guide_undo_pop (GimpUndo              *undo,
                     GimpUndoMode           undo_mode,
                     GimpUndoAccumulator   *accum)
{
  GimpGuideUndo       *guide_undo = GIMP_GUIDE_UNDO (undo);
  GimpGuide           *guide;
  GimpOrientationType  orientation;
  gint                 position;
  gboolean             moved = FALSE;

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  guide = GIMP_GUIDE (GIMP_AUX_ITEM_UNDO (undo)->aux_item);

  orientation = gimp_guide_get_orientation (guide);
  position    = gimp_guide_get_position (guide);

  if (position == GIMP_GUIDE_POSITION_UNDEFINED)
    {
      gimp_image_add_guide (undo->image, guide, guide_undo->position);
    }
  else if (guide_undo->position == GIMP_GUIDE_POSITION_UNDEFINED)
    {
      gimp_image_remove_guide (undo->image, guide, FALSE);
    }
  else
    {
      gimp_guide_set_position (guide, guide_undo->position);

      moved = TRUE;
    }

  gimp_guide_set_orientation (guide, guide_undo->orientation);

  if (moved || guide_undo->orientation != orientation)
    gimp_image_guide_moved (undo->image, guide);

  guide_undo->position    = position;
  guide_undo->orientation = orientation;
}
