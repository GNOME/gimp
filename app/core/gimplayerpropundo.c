/* Gimp - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpimage.h"
#include "gimplayer.h"
#include "gimplayerpropundo.h"


static void   gimp_layer_prop_undo_constructed (GObject             *object);

static void   gimp_layer_prop_undo_pop         (GimpUndo            *undo,
                                                GimpUndoMode         undo_mode,
                                                GimpUndoAccumulator *accum);


G_DEFINE_TYPE (GimpLayerPropUndo, gimp_layer_prop_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_layer_prop_undo_parent_class


static void
gimp_layer_prop_undo_class_init (GimpLayerPropUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructed = gimp_layer_prop_undo_constructed;

  undo_class->pop           = gimp_layer_prop_undo_pop;
}

static void
gimp_layer_prop_undo_init (GimpLayerPropUndo *undo)
{
}

static void
gimp_layer_prop_undo_constructed (GObject *object)
{
  GimpLayerPropUndo *layer_prop_undo = GIMP_LAYER_PROP_UNDO (object);
  GimpLayer         *layer;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_LAYER (GIMP_ITEM_UNDO (object)->item));

  layer = GIMP_LAYER (GIMP_ITEM_UNDO (object)->item);

  switch (GIMP_UNDO (object)->undo_type)
    {
    case GIMP_UNDO_LAYER_MODE:
      layer_prop_undo->mode            = gimp_layer_get_mode (layer);
      layer_prop_undo->blend_space     = gimp_layer_get_blend_space (layer);
      layer_prop_undo->composite_space = gimp_layer_get_composite_space (layer);
      layer_prop_undo->composite_mode  = gimp_layer_get_composite_mode (layer);
      break;

    case GIMP_UNDO_LAYER_OPACITY:
      layer_prop_undo->opacity = gimp_layer_get_opacity (layer);
      break;

    case GIMP_UNDO_LAYER_LOCK_ALPHA:
      layer_prop_undo->lock_alpha = gimp_layer_get_lock_alpha (layer);
      break;

    default:
      g_return_if_reached ();
    }
}

static void
gimp_layer_prop_undo_pop (GimpUndo            *undo,
                          GimpUndoMode         undo_mode,
                          GimpUndoAccumulator *accum)
{
  GimpLayerPropUndo *layer_prop_undo = GIMP_LAYER_PROP_UNDO (undo);
  GimpLayer         *layer           = GIMP_LAYER (GIMP_ITEM_UNDO (undo)->item);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case GIMP_UNDO_LAYER_MODE:
      {
        GimpLayerMode          mode;
        GimpLayerColorSpace    blend_space;
        GimpLayerColorSpace    composite_space;
        GimpLayerCompositeMode composite_mode;

        mode            = gimp_layer_get_mode (layer);
        blend_space     = gimp_layer_get_blend_space (layer);
        composite_space = gimp_layer_get_composite_space (layer);
        composite_mode  = gimp_layer_get_composite_mode (layer);

        gimp_layer_set_mode            (layer, layer_prop_undo->mode,
                                        FALSE);
        gimp_layer_set_blend_space     (layer, layer_prop_undo->blend_space,
                                        FALSE);
        gimp_layer_set_composite_space (layer, layer_prop_undo->composite_space,
                                        FALSE);
        gimp_layer_set_composite_mode  (layer, layer_prop_undo->composite_mode,
                                        FALSE);

        layer_prop_undo->mode            = mode;
        layer_prop_undo->blend_space     = blend_space;
        layer_prop_undo->composite_space = composite_space;
        layer_prop_undo->composite_mode  = composite_mode;
      }
      break;

    case GIMP_UNDO_LAYER_OPACITY:
      {
        gdouble opacity;

        opacity = gimp_layer_get_opacity (layer);
        gimp_layer_set_opacity (layer, layer_prop_undo->opacity, FALSE);
        layer_prop_undo->opacity = opacity;
      }
      break;

    case GIMP_UNDO_LAYER_LOCK_ALPHA:
      {
        gboolean lock_alpha;

        lock_alpha = gimp_layer_get_lock_alpha (layer);
        gimp_layer_set_lock_alpha (layer, layer_prop_undo->lock_alpha, FALSE);
        layer_prop_undo->lock_alpha = lock_alpha;
      }
      break;

    default:
      g_return_if_reached ();
    }
}
