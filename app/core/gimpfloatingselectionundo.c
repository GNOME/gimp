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

#include "gimpdrawable-floating-selection.h"
#include "gimpfloatingselectionundo.h"
#include "gimpimage.h"
#include "gimplayer.h"
#include "gimplayer-floating-selection.h"


static void   gimp_floating_selection_undo_constructed (GObject             *object);

static void   gimp_floating_selection_undo_pop         (GimpUndo            *undo,
                                                        GimpUndoMode         undo_mode,
                                                        GimpUndoAccumulator *accum);


G_DEFINE_TYPE (GimpFloatingSelectionUndo, gimp_floating_selection_undo,
               GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_floating_selection_undo_parent_class


static void
gimp_floating_selection_undo_class_init (GimpFloatingSelectionUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructed = gimp_floating_selection_undo_constructed;

  undo_class->pop           = gimp_floating_selection_undo_pop;
}

static void
gimp_floating_selection_undo_init (GimpFloatingSelectionUndo *undo)
{
}

static void
gimp_floating_selection_undo_constructed (GObject *object)
{
  GimpFloatingSelectionUndo *floating_sel_undo;
  GimpLayer                 *layer;

  floating_sel_undo = GIMP_FLOATING_SELECTION_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_LAYER (GIMP_ITEM_UNDO (object)->item));

  layer = GIMP_LAYER (GIMP_ITEM_UNDO (object)->item);

  switch (GIMP_UNDO (object)->undo_type)
    {
    case GIMP_UNDO_FS_TO_LAYER:
      floating_sel_undo->drawable = gimp_layer_get_floating_sel_drawable (layer);
      break;

    default:
      g_return_if_reached ();
    }
}

static void
gimp_floating_selection_undo_pop (GimpUndo            *undo,
                                  GimpUndoMode         undo_mode,
                                  GimpUndoAccumulator *accum)
{
  GimpFloatingSelectionUndo *floating_sel_undo;
  GimpLayer                 *floating_layer;

  floating_sel_undo = GIMP_FLOATING_SELECTION_UNDO (undo);
  floating_layer    = GIMP_LAYER (GIMP_ITEM_UNDO (undo)->item);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case GIMP_UNDO_FS_TO_LAYER:
      if (undo_mode == GIMP_UNDO_MODE_UNDO)
        {
          GList *layers;

          /*  Update the preview for the floating selection  */
          gimp_viewable_invalidate_preview (GIMP_VIEWABLE (floating_layer));

          gimp_layer_set_floating_sel_drawable (floating_layer,
                                                floating_sel_undo->drawable);
          layers = g_list_prepend (NULL, floating_layer);
          gimp_image_set_selected_layers (undo->image, layers);
          g_list_free (layers);

          gimp_drawable_attach_floating_sel (gimp_layer_get_floating_sel_drawable (floating_layer),
                                             floating_layer);
        }
      else
        {
          gimp_drawable_detach_floating_sel (gimp_layer_get_floating_sel_drawable (floating_layer));
          gimp_layer_set_floating_sel_drawable (floating_layer, NULL);
        }

      /* When the floating selection is converted to/from a normal
       * layer it does something resembling a name change, so emit the
       * "name-changed" signal
       */
      gimp_object_name_changed (GIMP_OBJECT (floating_layer));

      gimp_drawable_update (GIMP_DRAWABLE (floating_layer),
                            0, 0,
                            gimp_item_get_width  (GIMP_ITEM (floating_layer)),
                            gimp_item_get_height (GIMP_ITEM (floating_layer)));
      break;

    default:
      g_return_if_reached ();
    }
}
