/* Ligma - The GNU Image Manipulation Program
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

#include "ligmalayer.h"
#include "ligmalayermaskpropundo.h"


static void   ligma_layer_mask_prop_undo_constructed (GObject             *object);

static void   ligma_layer_mask_prop_undo_pop         (LigmaUndo            *undo,
                                                     LigmaUndoMode         undo_mode,
                                                     LigmaUndoAccumulator *accum);


G_DEFINE_TYPE (LigmaLayerMaskPropUndo, ligma_layer_mask_prop_undo,
               LIGMA_TYPE_ITEM_UNDO)

#define parent_class ligma_layer_mask_prop_undo_parent_class


static void
ligma_layer_mask_prop_undo_class_init (LigmaLayerMaskPropUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  LigmaUndoClass *undo_class   = LIGMA_UNDO_CLASS (klass);

  object_class->constructed = ligma_layer_mask_prop_undo_constructed;

  undo_class->pop           = ligma_layer_mask_prop_undo_pop;
}

static void
ligma_layer_mask_prop_undo_init (LigmaLayerMaskPropUndo *undo)
{
}

static void
ligma_layer_mask_prop_undo_constructed (GObject *object)
{
  LigmaLayerMaskPropUndo *layer_mask_prop_undo;
  LigmaLayer             *layer;

  layer_mask_prop_undo = LIGMA_LAYER_MASK_PROP_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_LAYER (LIGMA_ITEM_UNDO (object)->item));

  layer = LIGMA_LAYER (LIGMA_ITEM_UNDO (object)->item);

  switch (LIGMA_UNDO (object)->undo_type)
    {
    case LIGMA_UNDO_LAYER_MASK_APPLY:
      layer_mask_prop_undo->apply = ligma_layer_get_apply_mask (layer);
      break;

    case LIGMA_UNDO_LAYER_MASK_SHOW:
      layer_mask_prop_undo->show = ligma_layer_get_show_mask (layer);
      break;

    default:
      g_return_if_reached ();
    }
}

static void
ligma_layer_mask_prop_undo_pop (LigmaUndo            *undo,
                               LigmaUndoMode         undo_mode,
                               LigmaUndoAccumulator *accum)
{
  LigmaLayerMaskPropUndo *layer_mask_prop_undo = LIGMA_LAYER_MASK_PROP_UNDO (undo);
  LigmaLayer             *layer                = LIGMA_LAYER (LIGMA_ITEM_UNDO (undo)->item);

  LIGMA_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case LIGMA_UNDO_LAYER_MASK_APPLY:
      {
        gboolean apply;

        apply = ligma_layer_get_apply_mask (layer);
        ligma_layer_set_apply_mask (layer, layer_mask_prop_undo->apply, FALSE);
        layer_mask_prop_undo->apply = apply;
      }
      break;

    case LIGMA_UNDO_LAYER_MASK_SHOW:
      {
        gboolean show;

        show = ligma_layer_get_show_mask (layer);
        ligma_layer_set_show_mask (layer, layer_mask_prop_undo->show, FALSE);
        layer_mask_prop_undo->show = show;
      }
      break;

    default:
      g_return_if_reached ();
    }
}
