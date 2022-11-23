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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligmaimage.h"
#include "ligmalayer.h"
#include "ligmalayerundo.h"


enum
{
  PROP_0,
  PROP_PREV_PARENT,
  PROP_PREV_POSITION,
  PROP_PREV_LAYERS
};


static void     ligma_layer_undo_constructed  (GObject             *object);
static void     ligma_layer_undo_finalize     (GObject             *object);
static void     ligma_layer_undo_set_property (GObject             *object,
                                              guint                property_id,
                                              const GValue        *value,
                                              GParamSpec          *pspec);
static void     ligma_layer_undo_get_property (GObject             *object,
                                              guint                property_id,
                                              GValue              *value,
                                              GParamSpec          *pspec);

static gint64   ligma_layer_undo_get_memsize  (LigmaObject          *object,
                                              gint64              *gui_size);

static void     ligma_layer_undo_pop          (LigmaUndo            *undo,
                                              LigmaUndoMode         undo_mode,
                                              LigmaUndoAccumulator *accum);


G_DEFINE_TYPE (LigmaLayerUndo, ligma_layer_undo, LIGMA_TYPE_ITEM_UNDO)

#define parent_class ligma_layer_undo_parent_class


static void
ligma_layer_undo_class_init (LigmaLayerUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaUndoClass   *undo_class        = LIGMA_UNDO_CLASS (klass);

  object_class->constructed      = ligma_layer_undo_constructed;
  object_class->finalize         = ligma_layer_undo_finalize;
  object_class->set_property     = ligma_layer_undo_set_property;
  object_class->get_property     = ligma_layer_undo_get_property;

  ligma_object_class->get_memsize = ligma_layer_undo_get_memsize;

  undo_class->pop                = ligma_layer_undo_pop;

  g_object_class_install_property (object_class, PROP_PREV_PARENT,
                                   g_param_spec_object ("prev-parent",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_LAYER,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PREV_POSITION,
                                   g_param_spec_int ("prev-position", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PREV_LAYERS,
                                   g_param_spec_pointer ("prev-layers", NULL, NULL,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_layer_undo_init (LigmaLayerUndo *undo)
{
  undo->prev_layers = NULL;
}

static void
ligma_layer_undo_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_LAYER (LIGMA_ITEM_UNDO (object)->item));
}

static void
ligma_layer_undo_finalize (GObject *object)
{
  LigmaLayerUndo *undo = LIGMA_LAYER_UNDO (object);

  g_clear_pointer (&undo->prev_layers, g_list_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}
static void
ligma_layer_undo_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  LigmaLayerUndo *layer_undo = LIGMA_LAYER_UNDO (object);

  switch (property_id)
    {
    case PROP_PREV_PARENT:
      layer_undo->prev_parent = g_value_get_object (value);
      break;
    case PROP_PREV_POSITION:
      layer_undo->prev_position = g_value_get_int (value);
      break;
    case PROP_PREV_LAYERS:
      layer_undo->prev_layers = g_list_copy (g_value_get_pointer (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_layer_undo_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  LigmaLayerUndo *layer_undo = LIGMA_LAYER_UNDO (object);

  switch (property_id)
    {
    case PROP_PREV_PARENT:
      g_value_set_object (value, layer_undo->prev_parent);
      break;
    case PROP_PREV_POSITION:
      g_value_set_int (value, layer_undo->prev_position);
      break;
    case PROP_PREV_LAYERS:
      g_value_set_pointer (value, layer_undo->prev_layers);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
ligma_layer_undo_get_memsize (LigmaObject *object,
                             gint64     *gui_size)
{
  LigmaItemUndo *item_undo = LIGMA_ITEM_UNDO (object);
  gint64        memsize   = 0;

  if (! ligma_item_is_attached (item_undo->item))
    memsize += ligma_object_get_memsize (LIGMA_OBJECT (item_undo->item),
                                        gui_size);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
ligma_layer_undo_pop (LigmaUndo            *undo,
                     LigmaUndoMode         undo_mode,
                     LigmaUndoAccumulator *accum)
{
  LigmaLayerUndo *layer_undo = LIGMA_LAYER_UNDO (undo);
  LigmaLayer     *layer      = LIGMA_LAYER (LIGMA_ITEM_UNDO (undo)->item);

  LIGMA_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  if ((undo_mode       == LIGMA_UNDO_MODE_UNDO &&
       undo->undo_type == LIGMA_UNDO_LAYER_ADD) ||
      (undo_mode       == LIGMA_UNDO_MODE_REDO &&
       undo->undo_type == LIGMA_UNDO_LAYER_REMOVE))
    {
      /*  remove layer  */

      /*  record the current parent and position  */
      layer_undo->prev_parent   = ligma_layer_get_parent (layer);
      layer_undo->prev_position = ligma_item_get_index (LIGMA_ITEM (layer));

      ligma_image_remove_layer (undo->image, layer, FALSE,
                               layer_undo->prev_layers);
    }
  else
    {
      /*  restore layer  */

      /*  record the active layer  */
      g_clear_pointer (&layer_undo->prev_layers, g_list_free);
      layer_undo->prev_layers = g_list_copy (ligma_image_get_selected_layers (undo->image));

      ligma_image_add_layer (undo->image, layer,
                            layer_undo->prev_parent,
                            layer_undo->prev_position, FALSE);
    }
}
