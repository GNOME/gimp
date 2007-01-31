/* Gimp - The GNU Image Manipulation Program
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

#include <glib-object.h>

#include "core-types.h"

#include "gimplayermask.h"
#include "gimplayermaskpropundo.h"


static GObject * gimp_layer_mask_prop_undo_constructor (GType                  type,
                                                        guint                  n_params,
                                                        GObjectConstructParam *params);

static void      gimp_layer_mask_prop_undo_pop         (GimpUndo              *undo,
                                                        GimpUndoMode           undo_mode,
                                                        GimpUndoAccumulator   *accum);


G_DEFINE_TYPE (GimpLayerMaskPropUndo, gimp_layer_mask_prop_undo,
               GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_layer_mask_prop_undo_parent_class


static void
gimp_layer_mask_prop_undo_class_init (GimpLayerMaskPropUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructor = gimp_layer_mask_prop_undo_constructor;

  undo_class->pop           = gimp_layer_mask_prop_undo_pop;
}

static void
gimp_layer_mask_prop_undo_init (GimpLayerMaskPropUndo *undo)
{
}

static GObject *
gimp_layer_mask_prop_undo_constructor (GType                  type,
                                       guint                  n_params,
                                       GObjectConstructParam *params)
{
  GObject               *object;
  GimpLayerMaskPropUndo *layer_mask_prop_undo;
  GimpLayerMask         *layer_mask;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  layer_mask_prop_undo = GIMP_LAYER_MASK_PROP_UNDO (object);

  g_assert (GIMP_IS_LAYER_MASK (GIMP_ITEM_UNDO (object)->item));

  layer_mask = GIMP_LAYER_MASK (GIMP_ITEM_UNDO (object)->item);

  switch (GIMP_UNDO (object)->undo_type)
    {
    case GIMP_UNDO_LAYER_MASK_APPLY:
      layer_mask_prop_undo->apply = gimp_layer_mask_get_apply (layer_mask);
      break;

    case GIMP_UNDO_LAYER_MASK_SHOW:
      layer_mask_prop_undo->show = gimp_layer_mask_get_show (layer_mask);
      break;

    default:
      g_assert_not_reached ();
    }

  return object;
}

static void
gimp_layer_mask_prop_undo_pop (GimpUndo            *undo,
                               GimpUndoMode         undo_mode,
                               GimpUndoAccumulator *accum)
{
  GimpLayerMaskPropUndo *layer_mask_prop_undo = GIMP_LAYER_MASK_PROP_UNDO (undo);
  GimpLayerMask         *layer_mask           = GIMP_LAYER_MASK (GIMP_ITEM_UNDO (undo)->item);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case GIMP_UNDO_LAYER_MASK_APPLY:
      {
        gboolean apply;

        apply = gimp_layer_mask_get_apply (layer_mask);
        gimp_layer_mask_set_apply (layer_mask, layer_mask_prop_undo->apply, FALSE);
        layer_mask_prop_undo->apply = apply;
      }
      break;

    case GIMP_UNDO_LAYER_MASK_SHOW:
      {
        gboolean show;

        show = gimp_layer_mask_get_show (layer_mask);
        gimp_layer_mask_set_show (layer_mask, layer_mask_prop_undo->show, FALSE);
        layer_mask_prop_undo->show = show;
      }
      break;

    default:
      g_assert_not_reached ();
    }
}
