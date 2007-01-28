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

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpimage.h"
#include "gimplayer.h"
#include "gimplayerpropundo.h"


static GObject * gimp_layer_prop_undo_constructor (GType                  type,
                                                   guint                  n_params,
                                                   GObjectConstructParam *params);

static void      gimp_layer_prop_undo_pop         (GimpUndo              *undo,
                                                   GimpUndoMode           undo_mode,
                                                   GimpUndoAccumulator   *accum);


G_DEFINE_TYPE (GimpLayerPropUndo, gimp_layer_prop_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_layer_prop_undo_parent_class


static void
gimp_layer_prop_undo_class_init (GimpLayerPropUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructor = gimp_layer_prop_undo_constructor;

  undo_class->pop           = gimp_layer_prop_undo_pop;
}

static void
gimp_layer_prop_undo_init (GimpLayerPropUndo *undo)
{
}

static GObject *
gimp_layer_prop_undo_constructor (GType                  type,
                                  guint                  n_params,
                                  GObjectConstructParam *params)
{
  GObject           *object;
  GimpLayerPropUndo *layer_prop_undo;
  GimpImage         *image;
  GimpLayer         *layer;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  layer_prop_undo = GIMP_LAYER_PROP_UNDO (object);

  g_assert (GIMP_IS_LAYER (GIMP_ITEM_UNDO (object)->item));

  image = GIMP_UNDO (object)->image;
  layer = GIMP_LAYER (GIMP_ITEM_UNDO (object)->item);

  layer_prop_undo->position   = gimp_image_get_layer_index (image, layer);
  layer_prop_undo->mode       = gimp_layer_get_mode (layer);
  layer_prop_undo->opacity    = gimp_layer_get_opacity (layer);
  layer_prop_undo->lock_alpha = gimp_layer_get_lock_alpha (layer);

  return object;
}

static void
gimp_layer_prop_undo_pop (GimpUndo            *undo,
                          GimpUndoMode         undo_mode,
                          GimpUndoAccumulator *accum)
{
  GimpLayerPropUndo *layer_prop_undo = GIMP_LAYER_PROP_UNDO (undo);
  GimpLayer         *layer           = GIMP_LAYER (GIMP_ITEM_UNDO (undo)->item);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  if (undo->undo_type == GIMP_UNDO_LAYER_REPOSITION)
    {
      gint position;

      position = gimp_image_get_layer_index (undo->image, layer);
      gimp_image_position_layer (undo->image, layer,
                                 layer_prop_undo->position,
                                 FALSE, NULL);
      layer_prop_undo->position = position;
    }
  else if (undo->undo_type == GIMP_UNDO_LAYER_MODE)
    {
      GimpLayerModeEffects mode;

      mode = gimp_layer_get_mode (layer);
      gimp_layer_set_mode (layer, layer_prop_undo->mode, FALSE);
      layer_prop_undo->mode = mode;
    }
  else if (undo->undo_type == GIMP_UNDO_LAYER_OPACITY)
    {
      gdouble opacity;

      opacity = gimp_layer_get_opacity (layer);
      gimp_layer_set_opacity (layer, layer_prop_undo->opacity, FALSE);
      layer_prop_undo->opacity = opacity;
    }
  else if (undo->undo_type == GIMP_UNDO_LAYER_LOCK_ALPHA)
    {
      gboolean lock_alpha;

      lock_alpha = gimp_layer_get_lock_alpha (layer);
      gimp_layer_set_lock_alpha (layer, layer_prop_undo->lock_alpha, FALSE);
      layer_prop_undo->lock_alpha = lock_alpha;
    }
  else
    {
      g_assert_not_reached ();
    }
}
