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

#include "ligma-memsize.h"
#include "ligmaimage.h"
#include "ligmagrouplayer.h"
#include "ligmagrouplayerundo.h"


static void     ligma_group_layer_undo_constructed (GObject             *object);

static gint64   ligma_group_layer_undo_get_memsize (LigmaObject          *object,
                                                   gint64              *gui_size);

static void     ligma_group_layer_undo_pop         (LigmaUndo            *undo,
                                                   LigmaUndoMode         undo_mode,
                                                   LigmaUndoAccumulator *accum);
static void     ligma_group_layer_undo_free        (LigmaUndo            *undo,
                                                   LigmaUndoMode         undo_mode);


G_DEFINE_TYPE (LigmaGroupLayerUndo, ligma_group_layer_undo, LIGMA_TYPE_ITEM_UNDO)

#define parent_class ligma_group_layer_undo_parent_class


static void
ligma_group_layer_undo_class_init (LigmaGroupLayerUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaUndoClass   *undo_class        = LIGMA_UNDO_CLASS (klass);

  object_class->constructed      = ligma_group_layer_undo_constructed;

  ligma_object_class->get_memsize = ligma_group_layer_undo_get_memsize;

  undo_class->pop                = ligma_group_layer_undo_pop;
  undo_class->free               = ligma_group_layer_undo_free;
}

static void
ligma_group_layer_undo_init (LigmaGroupLayerUndo *undo)
{
}

static void
ligma_group_layer_undo_constructed (GObject *object)
{
  LigmaGroupLayerUndo *group_layer_undo = LIGMA_GROUP_LAYER_UNDO (object);
  LigmaGroupLayer     *group;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_return_if_fail (LIGMA_IS_GROUP_LAYER (LIGMA_ITEM_UNDO (object)->item));

  group = LIGMA_GROUP_LAYER (LIGMA_ITEM_UNDO (object)->item);

  switch (LIGMA_UNDO (object)->undo_type)
    {
    case LIGMA_UNDO_GROUP_LAYER_SUSPEND_RESIZE:
    case LIGMA_UNDO_GROUP_LAYER_RESUME_RESIZE:
    case LIGMA_UNDO_GROUP_LAYER_SUSPEND_MASK:
    case LIGMA_UNDO_GROUP_LAYER_START_TRANSFORM:
    case LIGMA_UNDO_GROUP_LAYER_END_TRANSFORM:
      break;

    case LIGMA_UNDO_GROUP_LAYER_RESUME_MASK:
      group_layer_undo->mask_buffer =
        _ligma_group_layer_get_suspended_mask(group,
                                             &group_layer_undo->mask_bounds);

      if (group_layer_undo->mask_buffer)
        g_object_ref (group_layer_undo->mask_buffer);
      break;

    case LIGMA_UNDO_GROUP_LAYER_CONVERT:
      group_layer_undo->prev_type = ligma_drawable_get_base_type (LIGMA_DRAWABLE (group));
      group_layer_undo->prev_precision = ligma_drawable_get_precision (LIGMA_DRAWABLE (group));
      group_layer_undo->prev_has_alpha = ligma_drawable_has_alpha (LIGMA_DRAWABLE (group));
      break;

    default:
      g_return_if_reached ();
    }
}

static gint64
ligma_group_layer_undo_get_memsize (LigmaObject *object,
                                   gint64     *gui_size)
{
  LigmaGroupLayerUndo *group_layer_undo = LIGMA_GROUP_LAYER_UNDO (object);
  gint64              memsize       = 0;

  memsize += ligma_gegl_buffer_get_memsize (group_layer_undo->mask_buffer);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
ligma_group_layer_undo_pop (LigmaUndo            *undo,
                           LigmaUndoMode         undo_mode,
                           LigmaUndoAccumulator *accum)
{
  LigmaGroupLayerUndo *group_layer_undo = LIGMA_GROUP_LAYER_UNDO (undo);
  LigmaGroupLayer     *group;

  group = LIGMA_GROUP_LAYER (LIGMA_ITEM_UNDO (undo)->item);

  LIGMA_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case LIGMA_UNDO_GROUP_LAYER_SUSPEND_RESIZE:
    case LIGMA_UNDO_GROUP_LAYER_RESUME_RESIZE:
      if ((undo_mode       == LIGMA_UNDO_MODE_UNDO &&
           undo->undo_type == LIGMA_UNDO_GROUP_LAYER_SUSPEND_RESIZE) ||
          (undo_mode       == LIGMA_UNDO_MODE_REDO &&
           undo->undo_type == LIGMA_UNDO_GROUP_LAYER_RESUME_RESIZE))
        {
          /*  resume group layer auto-resizing  */

          ligma_group_layer_resume_resize (group, FALSE);
        }
      else
        {
          /*  suspend group layer auto-resizing  */

          ligma_group_layer_suspend_resize (group, FALSE);

          if (undo->undo_type == LIGMA_UNDO_GROUP_LAYER_RESUME_RESIZE &&
              group_layer_undo->mask_buffer)
            {
              LigmaLayerMask *mask = ligma_layer_get_mask (LIGMA_LAYER (group));

              ligma_drawable_set_buffer_full (LIGMA_DRAWABLE (mask),
                                             FALSE, NULL,
                                             group_layer_undo->mask_buffer,
                                             &group_layer_undo->mask_bounds,
                                             TRUE);
            }
        }
      break;

    case LIGMA_UNDO_GROUP_LAYER_SUSPEND_MASK:
    case LIGMA_UNDO_GROUP_LAYER_RESUME_MASK:
      if ((undo_mode       == LIGMA_UNDO_MODE_UNDO &&
           undo->undo_type == LIGMA_UNDO_GROUP_LAYER_SUSPEND_MASK) ||
          (undo_mode       == LIGMA_UNDO_MODE_REDO &&
           undo->undo_type == LIGMA_UNDO_GROUP_LAYER_RESUME_MASK))
        {
          /*  resume group layer mask auto-resizing  */

          ligma_group_layer_resume_mask (group, FALSE);
        }
      else
        {
          /*  suspend group layer mask auto-resizing  */

          ligma_group_layer_suspend_mask (group, FALSE);

          if (undo->undo_type == LIGMA_UNDO_GROUP_LAYER_RESUME_MASK &&
              group_layer_undo->mask_buffer)
            {
              _ligma_group_layer_set_suspended_mask (
                group,
                group_layer_undo->mask_buffer,
                &group_layer_undo->mask_bounds);
            }
        }
      break;

    case LIGMA_UNDO_GROUP_LAYER_START_TRANSFORM:
    case LIGMA_UNDO_GROUP_LAYER_END_TRANSFORM:
      if ((undo_mode       == LIGMA_UNDO_MODE_UNDO &&
           undo->undo_type == LIGMA_UNDO_GROUP_LAYER_START_TRANSFORM) ||
          (undo_mode       == LIGMA_UNDO_MODE_REDO &&
           undo->undo_type == LIGMA_UNDO_GROUP_LAYER_END_TRANSFORM))
        {
          /*  end group layer transform operation  */

          _ligma_group_layer_end_transform (group, FALSE);
        }
      else
        {
          /*  start group layer transform operation  */

          _ligma_group_layer_start_transform (group, FALSE);
        }
      break;

    case LIGMA_UNDO_GROUP_LAYER_CONVERT:
      {
        LigmaImageBaseType type;
        LigmaPrecision     precision;
        gboolean          has_alpha;

        type      = ligma_drawable_get_base_type (LIGMA_DRAWABLE (group));
        precision = ligma_drawable_get_precision (LIGMA_DRAWABLE (group));
        has_alpha = ligma_drawable_has_alpha (LIGMA_DRAWABLE (group));

        ligma_drawable_convert_type (LIGMA_DRAWABLE (group),
                                    ligma_item_get_image (LIGMA_ITEM (group)),
                                    group_layer_undo->prev_type,
                                    group_layer_undo->prev_precision,
                                    group_layer_undo->prev_has_alpha,
                                    NULL, NULL,
                                    0, 0,
                                    FALSE, NULL);

        group_layer_undo->prev_type      = type;
        group_layer_undo->prev_precision = precision;
        group_layer_undo->prev_has_alpha = has_alpha;
      }
      break;

    default:
      g_return_if_reached ();
    }
}

static void
ligma_group_layer_undo_free (LigmaUndo     *undo,
                            LigmaUndoMode  undo_mode)
{
  LigmaGroupLayerUndo *group_layer_undo = LIGMA_GROUP_LAYER_UNDO (undo);

  g_clear_object (&group_layer_undo->mask_buffer);

  LIGMA_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
