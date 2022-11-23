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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligmaimage.h"
#include "ligmalayer.h"
#include "ligmalayerpropundo.h"


static void   ligma_layer_prop_undo_constructed (GObject             *object);

static void   ligma_layer_prop_undo_pop         (LigmaUndo            *undo,
                                                LigmaUndoMode         undo_mode,
                                                LigmaUndoAccumulator *accum);


G_DEFINE_TYPE (LigmaLayerPropUndo, ligma_layer_prop_undo, LIGMA_TYPE_ITEM_UNDO)

#define parent_class ligma_layer_prop_undo_parent_class


static void
ligma_layer_prop_undo_class_init (LigmaLayerPropUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  LigmaUndoClass *undo_class   = LIGMA_UNDO_CLASS (klass);

  object_class->constructed = ligma_layer_prop_undo_constructed;

  undo_class->pop           = ligma_layer_prop_undo_pop;
}

static void
ligma_layer_prop_undo_init (LigmaLayerPropUndo *undo)
{
}

static void
ligma_layer_prop_undo_constructed (GObject *object)
{
  LigmaLayerPropUndo *layer_prop_undo = LIGMA_LAYER_PROP_UNDO (object);
  LigmaLayer         *layer;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_LAYER (LIGMA_ITEM_UNDO (object)->item));

  layer = LIGMA_LAYER (LIGMA_ITEM_UNDO (object)->item);

  switch (LIGMA_UNDO (object)->undo_type)
    {
    case LIGMA_UNDO_LAYER_MODE:
      layer_prop_undo->mode            = ligma_layer_get_mode (layer);
      layer_prop_undo->blend_space     = ligma_layer_get_blend_space (layer);
      layer_prop_undo->composite_space = ligma_layer_get_composite_space (layer);
      layer_prop_undo->composite_mode  = ligma_layer_get_composite_mode (layer);
      break;

    case LIGMA_UNDO_LAYER_OPACITY:
      layer_prop_undo->opacity = ligma_layer_get_opacity (layer);
      break;

    case LIGMA_UNDO_LAYER_LOCK_ALPHA:
      layer_prop_undo->lock_alpha = ligma_layer_get_lock_alpha (layer);
      break;

    default:
      g_return_if_reached ();
    }
}

static void
ligma_layer_prop_undo_pop (LigmaUndo            *undo,
                          LigmaUndoMode         undo_mode,
                          LigmaUndoAccumulator *accum)
{
  LigmaLayerPropUndo *layer_prop_undo = LIGMA_LAYER_PROP_UNDO (undo);
  LigmaLayer         *layer           = LIGMA_LAYER (LIGMA_ITEM_UNDO (undo)->item);

  LIGMA_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case LIGMA_UNDO_LAYER_MODE:
      {
        LigmaLayerMode          mode;
        LigmaLayerColorSpace    blend_space;
        LigmaLayerColorSpace    composite_space;
        LigmaLayerCompositeMode composite_mode;

        mode            = ligma_layer_get_mode (layer);
        blend_space     = ligma_layer_get_blend_space (layer);
        composite_space = ligma_layer_get_composite_space (layer);
        composite_mode  = ligma_layer_get_composite_mode (layer);

        ligma_layer_set_mode            (layer, layer_prop_undo->mode,
                                        FALSE);
        ligma_layer_set_blend_space     (layer, layer_prop_undo->blend_space,
                                        FALSE);
        ligma_layer_set_composite_space (layer, layer_prop_undo->composite_space,
                                        FALSE);
        ligma_layer_set_composite_mode  (layer, layer_prop_undo->composite_mode,
                                        FALSE);

        layer_prop_undo->mode            = mode;
        layer_prop_undo->blend_space     = blend_space;
        layer_prop_undo->composite_space = composite_space;
        layer_prop_undo->composite_mode  = composite_mode;
      }
      break;

    case LIGMA_UNDO_LAYER_OPACITY:
      {
        gdouble opacity;

        opacity = ligma_layer_get_opacity (layer);
        ligma_layer_set_opacity (layer, layer_prop_undo->opacity, FALSE);
        layer_prop_undo->opacity = opacity;
      }
      break;

    case LIGMA_UNDO_LAYER_LOCK_ALPHA:
      {
        gboolean lock_alpha;

        lock_alpha = ligma_layer_get_lock_alpha (layer);
        ligma_layer_set_lock_alpha (layer, layer_prop_undo->lock_alpha, FALSE);
        layer_prop_undo->lock_alpha = lock_alpha;
      }
      break;

    default:
      g_return_if_reached ();
    }
}
