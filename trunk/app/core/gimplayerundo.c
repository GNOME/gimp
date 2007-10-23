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

#include <glib-object.h>

#include "core-types.h"

#include "gimpcontainer.h"
#include "gimpimage.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimplayerundo.h"


enum
{
  PROP_0,
  PROP_PREV_POSITION,
  PROP_PREV_LAYER
};


static GObject * gimp_layer_undo_constructor  (GType                  type,
                                               guint                  n_params,
                                               GObjectConstructParam *params);
static void      gimp_layer_undo_set_property (GObject               *object,
                                               guint                  property_id,
                                               const GValue          *value,
                                               GParamSpec            *pspec);
static void      gimp_layer_undo_get_property (GObject               *object,
                                               guint                  property_id,
                                               GValue                *value,
                                               GParamSpec            *pspec);

static gint64    gimp_layer_undo_get_memsize  (GimpObject            *object,
                                               gint64                *gui_size);

static void      gimp_layer_undo_pop          (GimpUndo              *undo,
                                               GimpUndoMode           undo_mode,
                                               GimpUndoAccumulator   *accum);


G_DEFINE_TYPE (GimpLayerUndo, gimp_layer_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_layer_undo_parent_class


static void
gimp_layer_undo_class_init (GimpLayerUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpUndoClass   *undo_class        = GIMP_UNDO_CLASS (klass);

  object_class->constructor      = gimp_layer_undo_constructor;
  object_class->set_property     = gimp_layer_undo_set_property;
  object_class->get_property     = gimp_layer_undo_get_property;

  gimp_object_class->get_memsize = gimp_layer_undo_get_memsize;

  undo_class->pop                = gimp_layer_undo_pop;

  g_object_class_install_property (object_class, PROP_PREV_POSITION,
                                   g_param_spec_int ("prev-position", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PREV_LAYER,
                                   g_param_spec_object ("prev-layer", NULL, NULL,
                                                        GIMP_TYPE_LAYER,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_layer_undo_init (GimpLayerUndo *undo)
{
}

static GObject *
gimp_layer_undo_constructor (GType                  type,
                             guint                  n_params,
                             GObjectConstructParam *params)
{
  GObject       *object;
  GimpLayerUndo *layer_undo;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  layer_undo = GIMP_LAYER_UNDO (object);

  g_assert (GIMP_IS_LAYER (GIMP_ITEM_UNDO (object)->item));

  return object;
}

static void
gimp_layer_undo_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpLayerUndo *layer_undo = GIMP_LAYER_UNDO (object);

  switch (property_id)
    {
    case PROP_PREV_POSITION:
      layer_undo->prev_position = g_value_get_int (value);
      break;
    case PROP_PREV_LAYER:
      layer_undo->prev_layer = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_layer_undo_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpLayerUndo *layer_undo = GIMP_LAYER_UNDO (object);

  switch (property_id)
    {
    case PROP_PREV_POSITION:
      g_value_set_int (value, layer_undo->prev_position);
      break;
    case PROP_PREV_LAYER:
      g_value_set_object (value, layer_undo->prev_layer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_layer_undo_get_memsize (GimpObject *object,
                             gint64     *gui_size)
{
  GimpItemUndo *item_undo = GIMP_ITEM_UNDO (object);
  gint64        memsize   = 0;

  if (! gimp_item_is_attached (item_undo->item))
    memsize += gimp_object_get_memsize (GIMP_OBJECT (item_undo->item),
                                        gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_layer_undo_pop (GimpUndo            *undo,
                     GimpUndoMode         undo_mode,
                     GimpUndoAccumulator *accum)
{
  GimpLayerUndo *layer_undo = GIMP_LAYER_UNDO (undo);
  GimpLayer     *layer      = GIMP_LAYER (GIMP_ITEM_UNDO (undo)->item);
  gboolean       old_has_alpha;

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  old_has_alpha = gimp_image_has_alpha (undo->image);

  if ((undo_mode       == GIMP_UNDO_MODE_UNDO &&
       undo->undo_type == GIMP_UNDO_LAYER_ADD) ||
      (undo_mode       == GIMP_UNDO_MODE_REDO &&
       undo->undo_type == GIMP_UNDO_LAYER_REMOVE))
    {
      /*  remove layer  */

      /*  record the current position  */
      layer_undo->prev_position = gimp_image_get_layer_index (undo->image,
                                                              layer);

      gimp_container_remove (undo->image->layers, GIMP_OBJECT (layer));
      undo->image->layer_stack = g_slist_remove (undo->image->layer_stack,
                                                 layer);

      if (gimp_layer_is_floating_sel (layer))
        {
          /*  invalidate the boundary *before* setting the
           *  floating_sel pointer to NULL because the selection's
           *  outline is affected by the floating_sel and won't be
           *  completely cleared otherwise (bug #160247).
           */
          gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (layer));

          undo->image->floating_sel = NULL;

          /*  activate the underlying drawable  */
          floating_sel_activate_drawable (layer);

          gimp_image_floating_selection_changed (undo->image);
        }
      else if (layer == gimp_image_get_active_layer (undo->image))
        {
          if (layer_undo->prev_layer)
            {
              gimp_image_set_active_layer (undo->image, layer_undo->prev_layer);
            }
          else if (undo->image->layer_stack)
            {
              gimp_image_set_active_layer (undo->image,
                                           undo->image->layer_stack->data);
            }
          else
            {
              gimp_image_set_active_layer (undo->image, NULL);
            }
        }

      gimp_item_removed (GIMP_ITEM (layer));
    }
  else
    {
      /*  restore layer  */

      /*  record the active layer  */
      layer_undo->prev_layer = gimp_image_get_active_layer (undo->image);

      /*  if this is a floating selection, set the fs pointer  */
      if (gimp_layer_is_floating_sel (layer))
        undo->image->floating_sel = layer;

      gimp_container_insert (undo->image->layers,
                             GIMP_OBJECT (layer), layer_undo->prev_position);
      gimp_image_set_active_layer (undo->image, layer);

      if (gimp_layer_is_floating_sel (layer))
        gimp_image_floating_selection_changed (undo->image);

      GIMP_ITEM (layer)->removed = FALSE;

      if (layer->mask)
        GIMP_ITEM (layer->mask)->removed = FALSE;
    }

  if (old_has_alpha != gimp_image_has_alpha (undo->image))
    accum->alpha_changed = TRUE;
}
