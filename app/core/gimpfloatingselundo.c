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

#include <gegl.h>

#include "core-types.h"

#include "base/tile-manager.h"

#include "gimpfloatingselundo.h"
#include "gimpimage.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"


static GObject * gimp_floating_sel_undo_constructor (GType                  type,
                                                     guint                  n_params,
                                                     GObjectConstructParam *params);

static void      gimp_floating_sel_undo_pop         (GimpUndo              *undo,
                                                     GimpUndoMode           undo_mode,
                                                     GimpUndoAccumulator   *accum);


G_DEFINE_TYPE (GimpFloatingSelUndo, gimp_floating_sel_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_floating_sel_undo_parent_class


static void
gimp_floating_sel_undo_class_init (GimpFloatingSelUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructor = gimp_floating_sel_undo_constructor;

  undo_class->pop           = gimp_floating_sel_undo_pop;
}

static void
gimp_floating_sel_undo_init (GimpFloatingSelUndo *undo)
{
}

static GObject *
gimp_floating_sel_undo_constructor (GType                  type,
                                    guint                  n_params,
                                    GObjectConstructParam *params)
{
  GObject             *object;
  GimpFloatingSelUndo *floating_sel_undo;
  GimpLayer           *layer;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  floating_sel_undo = GIMP_FLOATING_SEL_UNDO (object);

  g_assert (GIMP_IS_LAYER (GIMP_ITEM_UNDO (object)->item));

  layer = GIMP_LAYER (GIMP_ITEM_UNDO (object)->item);

  switch (GIMP_UNDO (object)->undo_type)
    {
    case GIMP_UNDO_FS_TO_LAYER:
      floating_sel_undo->drawable = layer->fs.drawable;
      break;

    default:
      g_assert_not_reached ();
    }

  return object;
}

static void
gimp_floating_sel_undo_pop (GimpUndo            *undo,
                            GimpUndoMode         undo_mode,
                            GimpUndoAccumulator *accum)
{
  GimpFloatingSelUndo *floating_sel_undo = GIMP_FLOATING_SEL_UNDO (undo);
  GimpLayer           *floating_layer    = GIMP_LAYER (GIMP_ITEM_UNDO (undo)->item);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case GIMP_UNDO_FS_TO_LAYER:
      if (undo_mode == GIMP_UNDO_MODE_UNDO)
        {
          /*  Update the preview for the floating sel  */
          gimp_viewable_invalidate_preview (GIMP_VIEWABLE (floating_layer));

          floating_layer->fs.drawable = floating_sel_undo->drawable;
          gimp_image_set_active_layer (undo->image, floating_layer);
          gimp_image_set_floating_selection (undo->image, floating_layer);

          /*  clear the selection  */
          gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (floating_layer));
        }
      else
        {
          /*  Update the preview for the underlying drawable  */
          gimp_viewable_invalidate_preview (GIMP_VIEWABLE (floating_layer));

          /*  clear the selection  */
          gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (floating_layer));

          /*  update the pointers  */
          floating_layer->fs.drawable = NULL;
          gimp_image_set_floating_selection (undo->image, NULL);
        }

      gimp_object_name_changed (GIMP_OBJECT (floating_layer));

      gimp_drawable_update (GIMP_DRAWABLE (floating_layer),
                            0, 0,
                            gimp_item_get_width  (GIMP_ITEM (floating_layer)),
                            gimp_item_get_height (GIMP_ITEM (floating_layer)));
      break;

    default:
      g_assert_not_reached ();
    }
}
