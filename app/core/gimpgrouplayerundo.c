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

#include <gegl.h>

#include "core-types.h"

#include "gimpimage.h"
#include "gimpgrouplayer.h"
#include "gimpgrouplayerundo.h"


static void   gimp_group_layer_undo_constructed (GObject             *object);

static void   gimp_group_layer_undo_pop         (GimpUndo            *undo,
                                                 GimpUndoMode         undo_mode,
                                                 GimpUndoAccumulator *accum);


G_DEFINE_TYPE (GimpGroupLayerUndo, gimp_group_layer_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_group_layer_undo_parent_class


static void
gimp_group_layer_undo_class_init (GimpGroupLayerUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructed   = gimp_group_layer_undo_constructed;

  undo_class->pop             = gimp_group_layer_undo_pop;
}

static void
gimp_group_layer_undo_init (GimpGroupLayerUndo *undo)
{
}

static void
gimp_group_layer_undo_constructed (GObject *object)
{
  GimpGroupLayerUndo *group_layer_undo = GIMP_GROUP_LAYER_UNDO (object);
  GimpGroupLayer     *group;

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_GROUP_LAYER (GIMP_ITEM_UNDO (object)->item));

  group = GIMP_GROUP_LAYER (GIMP_ITEM_UNDO (object)->item);

  switch (GIMP_UNDO (object)->undo_type)
    {
    case GIMP_UNDO_GROUP_LAYER_SUSPEND:
    case GIMP_UNDO_GROUP_LAYER_RESUME:
      break;

    case GIMP_UNDO_GROUP_LAYER_CONVERT:
      group_layer_undo->prev_type = GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (GIMP_DRAWABLE (group)));
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
gimp_group_layer_undo_pop (GimpUndo            *undo,
                           GimpUndoMode         undo_mode,
                           GimpUndoAccumulator *accum)
{
  GimpGroupLayerUndo *group_layer_undo = GIMP_GROUP_LAYER_UNDO (undo);
  GimpGroupLayer     *group;

  group = GIMP_GROUP_LAYER (GIMP_ITEM_UNDO (undo)->item);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case GIMP_UNDO_GROUP_LAYER_SUSPEND:
    case GIMP_UNDO_GROUP_LAYER_RESUME:
      if ((undo_mode       == GIMP_UNDO_MODE_UNDO &&
           undo->undo_type == GIMP_UNDO_GROUP_LAYER_SUSPEND) ||
          (undo_mode       == GIMP_UNDO_MODE_REDO &&
           undo->undo_type == GIMP_UNDO_GROUP_LAYER_RESUME))
        {
          /*  resume group layer auto-resizing  */

          gimp_group_layer_resume_resize (group, FALSE);
        }
      else
        {
          /*  suspend group layer auto-resizing  */

          gimp_group_layer_suspend_resize (group, FALSE);
        }
      break;

    case GIMP_UNDO_GROUP_LAYER_CONVERT:
      {
        GimpImageBaseType type;

        type = GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (GIMP_DRAWABLE (group)));
        gimp_drawable_convert_type (GIMP_DRAWABLE (group), NULL,
                                    group_layer_undo->prev_type, FALSE);
        group_layer_undo->prev_type = type;
      }
      break;

    default:
      g_assert_not_reached ();
    }
}
