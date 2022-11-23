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

#include "ligmaauxitem.h"
#include "ligmaauxitemundo.h"


enum
{
  PROP_0,
  PROP_AUX_ITEM
};


static void   ligma_aux_item_undo_constructed  (GObject      *object);
static void   ligma_aux_item_undo_set_property (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec);
static void   ligma_aux_item_undo_get_property (GObject      *object,
                                               guint         property_id,
                                               GValue       *value,
                                               GParamSpec   *pspec);

static void   ligma_aux_item_undo_free         (LigmaUndo     *undo,
                                               LigmaUndoMode  undo_mode);


G_DEFINE_ABSTRACT_TYPE (LigmaAuxItemUndo, ligma_aux_item_undo, LIGMA_TYPE_UNDO)

#define parent_class ligma_aux_item_undo_parent_class


static void
ligma_aux_item_undo_class_init (LigmaAuxItemUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  LigmaUndoClass *undo_class   = LIGMA_UNDO_CLASS (klass);

  object_class->constructed  = ligma_aux_item_undo_constructed;
  object_class->set_property = ligma_aux_item_undo_set_property;
  object_class->get_property = ligma_aux_item_undo_get_property;

  undo_class->free           = ligma_aux_item_undo_free;

  g_object_class_install_property (object_class, PROP_AUX_ITEM,
                                   g_param_spec_object ("aux-item", NULL, NULL,
                                                        LIGMA_TYPE_AUX_ITEM,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_aux_item_undo_init (LigmaAuxItemUndo *undo)
{
}

static void
ligma_aux_item_undo_constructed (GObject *object)
{
  LigmaAuxItemUndo *aux_item_undo = LIGMA_AUX_ITEM_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_AUX_ITEM (aux_item_undo->aux_item));
}

static void
ligma_aux_item_undo_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  LigmaAuxItemUndo *aux_item_undo = LIGMA_AUX_ITEM_UNDO (object);

  switch (property_id)
    {
    case PROP_AUX_ITEM:
      aux_item_undo->aux_item = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_aux_item_undo_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  LigmaAuxItemUndo *aux_item_undo = LIGMA_AUX_ITEM_UNDO (object);

  switch (property_id)
    {
    case PROP_AUX_ITEM:
      g_value_set_object (value, aux_item_undo->aux_item);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_aux_item_undo_free (LigmaUndo     *undo,
                         LigmaUndoMode  undo_mode)
{
  LigmaAuxItemUndo *aux_item_undo = LIGMA_AUX_ITEM_UNDO (undo);

  g_clear_object (&aux_item_undo->aux_item);

  LIGMA_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
