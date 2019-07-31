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
#include "gimplayermask.h"
#include "gimplayermaskundo.h"


enum
{
  PROP_0,
  PROP_LAYER_MASK
};


static void     gimp_layer_mask_undo_constructed  (GObject             *object);
static void     gimp_layer_mask_undo_set_property (GObject             *object,
                                                   guint                property_id,
                                                   const GValue        *value,
                                                   GParamSpec          *pspec);
static void     gimp_layer_mask_undo_get_property (GObject             *object,
                                                   guint                property_id,
                                                   GValue              *value,
                                                   GParamSpec          *pspec);

static gint64   gimp_layer_mask_undo_get_memsize  (GimpObject          *object,
                                                   gint64              *gui_size);

static void     gimp_layer_mask_undo_pop          (GimpUndo            *undo,
                                                   GimpUndoMode         undo_mode,
                                                   GimpUndoAccumulator *accum);
static void     gimp_layer_mask_undo_free         (GimpUndo            *undo,
                                                   GimpUndoMode         undo_mode);


G_DEFINE_TYPE (GimpLayerMaskUndo, gimp_layer_mask_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_layer_mask_undo_parent_class


static void
gimp_layer_mask_undo_class_init (GimpLayerMaskUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpUndoClass   *undo_class        = GIMP_UNDO_CLASS (klass);

  object_class->constructed      = gimp_layer_mask_undo_constructed;
  object_class->set_property     = gimp_layer_mask_undo_set_property;
  object_class->get_property     = gimp_layer_mask_undo_get_property;

  gimp_object_class->get_memsize = gimp_layer_mask_undo_get_memsize;

  undo_class->pop                = gimp_layer_mask_undo_pop;
  undo_class->free               = gimp_layer_mask_undo_free;

  g_object_class_install_property (object_class, PROP_LAYER_MASK,
                                   g_param_spec_object ("layer-mask", NULL, NULL,
                                                        GIMP_TYPE_LAYER_MASK,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_layer_mask_undo_init (GimpLayerMaskUndo *undo)
{
}

static void
gimp_layer_mask_undo_constructed (GObject *object)
{
  GimpLayerMaskUndo *layer_mask_undo = GIMP_LAYER_MASK_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_LAYER (GIMP_ITEM_UNDO (object)->item));
  gimp_assert (GIMP_IS_LAYER_MASK (layer_mask_undo->layer_mask));
}

static void
gimp_layer_mask_undo_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpLayerMaskUndo *layer_mask_undo = GIMP_LAYER_MASK_UNDO (object);

  switch (property_id)
    {
    case PROP_LAYER_MASK:
      layer_mask_undo->layer_mask = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_layer_mask_undo_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpLayerMaskUndo *layer_mask_undo = GIMP_LAYER_MASK_UNDO (object);

  switch (property_id)
    {
    case PROP_LAYER_MASK:
      g_value_set_object (value, layer_mask_undo->layer_mask);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_layer_mask_undo_get_memsize (GimpObject *object,
                                  gint64     *gui_size)
{
  GimpLayerMaskUndo *layer_mask_undo = GIMP_LAYER_MASK_UNDO (object);
  GimpLayer         *layer           = GIMP_LAYER (GIMP_ITEM_UNDO (object)->item);
  gint64             memsize         = 0;

  /* don't use !gimp_item_is_attached() here */
  if (gimp_layer_get_mask (layer) != layer_mask_undo->layer_mask)
    memsize += gimp_object_get_memsize (GIMP_OBJECT (layer_mask_undo->layer_mask),
                                        gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_layer_mask_undo_pop (GimpUndo            *undo,
                          GimpUndoMode         undo_mode,
                          GimpUndoAccumulator *accum)
{
  GimpLayerMaskUndo *layer_mask_undo = GIMP_LAYER_MASK_UNDO (undo);
  GimpLayer         *layer           = GIMP_LAYER (GIMP_ITEM_UNDO (undo)->item);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  if ((undo_mode       == GIMP_UNDO_MODE_UNDO &&
       undo->undo_type == GIMP_UNDO_LAYER_MASK_ADD) ||
      (undo_mode       == GIMP_UNDO_MODE_REDO &&
       undo->undo_type == GIMP_UNDO_LAYER_MASK_REMOVE))
    {
      /*  remove layer mask  */

      gimp_layer_apply_mask (layer, GIMP_MASK_DISCARD, FALSE);
    }
  else
    {
      /*  restore layer mask  */

      gimp_layer_add_mask (layer, layer_mask_undo->layer_mask, FALSE, NULL);
    }
}

static void
gimp_layer_mask_undo_free (GimpUndo     *undo,
                           GimpUndoMode  undo_mode)
{
  GimpLayerMaskUndo *layer_mask_undo = GIMP_LAYER_MASK_UNDO (undo);

  g_clear_object (&layer_mask_undo->layer_mask);

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
