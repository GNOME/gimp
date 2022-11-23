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
#include "ligmalayermask.h"
#include "ligmalayermaskundo.h"


enum
{
  PROP_0,
  PROP_LAYER_MASK
};


static void     ligma_layer_mask_undo_constructed  (GObject             *object);
static void     ligma_layer_mask_undo_set_property (GObject             *object,
                                                   guint                property_id,
                                                   const GValue        *value,
                                                   GParamSpec          *pspec);
static void     ligma_layer_mask_undo_get_property (GObject             *object,
                                                   guint                property_id,
                                                   GValue              *value,
                                                   GParamSpec          *pspec);

static gint64   ligma_layer_mask_undo_get_memsize  (LigmaObject          *object,
                                                   gint64              *gui_size);

static void     ligma_layer_mask_undo_pop          (LigmaUndo            *undo,
                                                   LigmaUndoMode         undo_mode,
                                                   LigmaUndoAccumulator *accum);
static void     ligma_layer_mask_undo_free         (LigmaUndo            *undo,
                                                   LigmaUndoMode         undo_mode);


G_DEFINE_TYPE (LigmaLayerMaskUndo, ligma_layer_mask_undo, LIGMA_TYPE_ITEM_UNDO)

#define parent_class ligma_layer_mask_undo_parent_class


static void
ligma_layer_mask_undo_class_init (LigmaLayerMaskUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaUndoClass   *undo_class        = LIGMA_UNDO_CLASS (klass);

  object_class->constructed      = ligma_layer_mask_undo_constructed;
  object_class->set_property     = ligma_layer_mask_undo_set_property;
  object_class->get_property     = ligma_layer_mask_undo_get_property;

  ligma_object_class->get_memsize = ligma_layer_mask_undo_get_memsize;

  undo_class->pop                = ligma_layer_mask_undo_pop;
  undo_class->free               = ligma_layer_mask_undo_free;

  g_object_class_install_property (object_class, PROP_LAYER_MASK,
                                   g_param_spec_object ("layer-mask", NULL, NULL,
                                                        LIGMA_TYPE_LAYER_MASK,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_layer_mask_undo_init (LigmaLayerMaskUndo *undo)
{
}

static void
ligma_layer_mask_undo_constructed (GObject *object)
{
  LigmaLayerMaskUndo *layer_mask_undo = LIGMA_LAYER_MASK_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_LAYER (LIGMA_ITEM_UNDO (object)->item));
  ligma_assert (LIGMA_IS_LAYER_MASK (layer_mask_undo->layer_mask));
}

static void
ligma_layer_mask_undo_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  LigmaLayerMaskUndo *layer_mask_undo = LIGMA_LAYER_MASK_UNDO (object);

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
ligma_layer_mask_undo_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  LigmaLayerMaskUndo *layer_mask_undo = LIGMA_LAYER_MASK_UNDO (object);

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
ligma_layer_mask_undo_get_memsize (LigmaObject *object,
                                  gint64     *gui_size)
{
  LigmaLayerMaskUndo *layer_mask_undo = LIGMA_LAYER_MASK_UNDO (object);
  LigmaLayer         *layer           = LIGMA_LAYER (LIGMA_ITEM_UNDO (object)->item);
  gint64             memsize         = 0;

  /* don't use !ligma_item_is_attached() here */
  if (ligma_layer_get_mask (layer) != layer_mask_undo->layer_mask)
    memsize += ligma_object_get_memsize (LIGMA_OBJECT (layer_mask_undo->layer_mask),
                                        gui_size);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
ligma_layer_mask_undo_pop (LigmaUndo            *undo,
                          LigmaUndoMode         undo_mode,
                          LigmaUndoAccumulator *accum)
{
  LigmaLayerMaskUndo *layer_mask_undo = LIGMA_LAYER_MASK_UNDO (undo);
  LigmaLayer         *layer           = LIGMA_LAYER (LIGMA_ITEM_UNDO (undo)->item);

  LIGMA_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  if ((undo_mode       == LIGMA_UNDO_MODE_UNDO &&
       undo->undo_type == LIGMA_UNDO_LAYER_MASK_ADD) ||
      (undo_mode       == LIGMA_UNDO_MODE_REDO &&
       undo->undo_type == LIGMA_UNDO_LAYER_MASK_REMOVE))
    {
      /*  remove layer mask  */

      ligma_layer_apply_mask (layer, LIGMA_MASK_DISCARD, FALSE);
    }
  else
    {
      /*  restore layer mask  */

      ligma_layer_add_mask (layer, layer_mask_undo->layer_mask, FALSE, NULL);
    }
}

static void
ligma_layer_mask_undo_free (LigmaUndo     *undo,
                           LigmaUndoMode  undo_mode)
{
  LigmaLayerMaskUndo *layer_mask_undo = LIGMA_LAYER_MASK_UNDO (undo);

  g_clear_object (&layer_mask_undo->layer_mask);

  LIGMA_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
