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

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpimage.h"
#include "gimplayer.h"
#include "gimplayerundo.h"


enum
{
  PROP_0,
  PROP_PREV_PARENT,
  PROP_PREV_POSITION,
  PROP_PREV_LAYERS
};


static void     gimp_layer_undo_constructed  (GObject             *object);
static void     gimp_layer_undo_finalize     (GObject             *object);
static void     gimp_layer_undo_set_property (GObject             *object,
                                              guint                property_id,
                                              const GValue        *value,
                                              GParamSpec          *pspec);
static void     gimp_layer_undo_get_property (GObject             *object,
                                              guint                property_id,
                                              GValue              *value,
                                              GParamSpec          *pspec);

static gint64   gimp_layer_undo_get_memsize  (GimpObject          *object,
                                              gint64              *gui_size);

static void     gimp_layer_undo_pop          (GimpUndo            *undo,
                                              GimpUndoMode         undo_mode,
                                              GimpUndoAccumulator *accum);


G_DEFINE_TYPE (GimpLayerUndo, gimp_layer_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_layer_undo_parent_class


static void
gimp_layer_undo_class_init (GimpLayerUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpUndoClass   *undo_class        = GIMP_UNDO_CLASS (klass);

  object_class->constructed      = gimp_layer_undo_constructed;
  object_class->finalize         = gimp_layer_undo_finalize;
  object_class->set_property     = gimp_layer_undo_set_property;
  object_class->get_property     = gimp_layer_undo_get_property;

  gimp_object_class->get_memsize = gimp_layer_undo_get_memsize;

  undo_class->pop                = gimp_layer_undo_pop;

  g_object_class_install_property (object_class, PROP_PREV_PARENT,
                                   g_param_spec_object ("prev-parent",
                                                        NULL, NULL,
                                                        GIMP_TYPE_LAYER,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PREV_POSITION,
                                   g_param_spec_int ("prev-position", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PREV_LAYERS,
                                   g_param_spec_pointer ("prev-layers", NULL, NULL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_layer_undo_init (GimpLayerUndo *undo)
{
  undo->prev_layers = NULL;
}

static void
gimp_layer_undo_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_LAYER (GIMP_ITEM_UNDO (object)->item));
}

static void
gimp_layer_undo_finalize (GObject *object)
{
  GimpLayerUndo *undo = GIMP_LAYER_UNDO (object);

  g_clear_pointer (&undo->prev_layers, g_list_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
gimp_layer_undo_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpLayerUndo *layer_undo = GIMP_LAYER_UNDO (object);

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

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  if ((undo_mode       == GIMP_UNDO_MODE_UNDO &&
       undo->undo_type == GIMP_UNDO_LAYER_ADD) ||
      (undo_mode       == GIMP_UNDO_MODE_REDO &&
       undo->undo_type == GIMP_UNDO_LAYER_REMOVE))
    {
      /*  remove layer  */

      /*  record the current parent and position  */
      layer_undo->prev_parent   = gimp_layer_get_parent (layer);
      layer_undo->prev_position = gimp_item_get_index (GIMP_ITEM (layer));

      gimp_image_remove_layer (undo->image, layer, FALSE,
                               layer_undo->prev_layers);
    }
  else
    {
      /*  restore layer  */

      /*  record the active layer  */
      g_clear_pointer (&layer_undo->prev_layers, g_list_free);
      layer_undo->prev_layers = g_list_copy (gimp_image_get_selected_layers (undo->image));

      gimp_image_add_layer (undo->image, layer,
                            layer_undo->prev_parent,
                            layer_undo->prev_position, FALSE);
    }
}
