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

#include "ligmadrawable-floating-selection.h"
#include "ligmafloatingselectionundo.h"
#include "ligmaimage.h"
#include "ligmalayer.h"
#include "ligmalayer-floating-selection.h"


static void   ligma_floating_selection_undo_constructed (GObject             *object);

static void   ligma_floating_selection_undo_pop         (LigmaUndo            *undo,
                                                        LigmaUndoMode         undo_mode,
                                                        LigmaUndoAccumulator *accum);


G_DEFINE_TYPE (LigmaFloatingSelectionUndo, ligma_floating_selection_undo,
               LIGMA_TYPE_ITEM_UNDO)

#define parent_class ligma_floating_selection_undo_parent_class


static void
ligma_floating_selection_undo_class_init (LigmaFloatingSelectionUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  LigmaUndoClass *undo_class   = LIGMA_UNDO_CLASS (klass);

  object_class->constructed = ligma_floating_selection_undo_constructed;

  undo_class->pop           = ligma_floating_selection_undo_pop;
}

static void
ligma_floating_selection_undo_init (LigmaFloatingSelectionUndo *undo)
{
}

static void
ligma_floating_selection_undo_constructed (GObject *object)
{
  LigmaFloatingSelectionUndo *floating_sel_undo;
  LigmaLayer                 *layer;

  floating_sel_undo = LIGMA_FLOATING_SELECTION_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_LAYER (LIGMA_ITEM_UNDO (object)->item));

  layer = LIGMA_LAYER (LIGMA_ITEM_UNDO (object)->item);

  switch (LIGMA_UNDO (object)->undo_type)
    {
    case LIGMA_UNDO_FS_TO_LAYER:
      floating_sel_undo->drawable = ligma_layer_get_floating_sel_drawable (layer);
      break;

    default:
      g_return_if_reached ();
    }
}

static void
ligma_floating_selection_undo_pop (LigmaUndo            *undo,
                                  LigmaUndoMode         undo_mode,
                                  LigmaUndoAccumulator *accum)
{
  LigmaFloatingSelectionUndo *floating_sel_undo;
  LigmaLayer                 *floating_layer;

  floating_sel_undo = LIGMA_FLOATING_SELECTION_UNDO (undo);
  floating_layer    = LIGMA_LAYER (LIGMA_ITEM_UNDO (undo)->item);

  LIGMA_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case LIGMA_UNDO_FS_TO_LAYER:
      if (undo_mode == LIGMA_UNDO_MODE_UNDO)
        {
          GList *layers;

          /*  Update the preview for the floating selection  */
          ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (floating_layer));

          ligma_layer_set_floating_sel_drawable (floating_layer,
                                                floating_sel_undo->drawable);
          layers = g_list_prepend (NULL, floating_layer);
          ligma_image_set_selected_layers (undo->image, layers);
          g_list_free (layers);

          ligma_drawable_attach_floating_sel (ligma_layer_get_floating_sel_drawable (floating_layer),
                                             floating_layer);
        }
      else
        {
          ligma_drawable_detach_floating_sel (ligma_layer_get_floating_sel_drawable (floating_layer));
          ligma_layer_set_floating_sel_drawable (floating_layer, NULL);
        }

      /* When the floating selection is converted to/from a normal
       * layer it does something resembling a name change, so emit the
       * "name-changed" signal
       */
      ligma_object_name_changed (LIGMA_OBJECT (floating_layer));

      ligma_drawable_update (LIGMA_DRAWABLE (floating_layer),
                            0, 0,
                            ligma_item_get_width  (LIGMA_ITEM (floating_layer)),
                            ligma_item_get_height (LIGMA_ITEM (floating_layer)));
      break;

    default:
      g_return_if_reached ();
    }
}
