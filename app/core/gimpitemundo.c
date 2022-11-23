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
#include "ligmaitem.h"
#include "ligmaitemundo.h"


enum
{
  PROP_0,
  PROP_ITEM
};


static void   ligma_item_undo_constructed  (GObject      *object);
static void   ligma_item_undo_set_property (GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec);
static void   ligma_item_undo_get_property (GObject      *object,
                                           guint         property_id,
                                           GValue       *value,
                                           GParamSpec   *pspec);

static void   ligma_item_undo_free         (LigmaUndo     *undo,
                                           LigmaUndoMode  undo_mode);


G_DEFINE_TYPE (LigmaItemUndo, ligma_item_undo, LIGMA_TYPE_UNDO)

#define parent_class ligma_item_undo_parent_class


static void
ligma_item_undo_class_init (LigmaItemUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  LigmaUndoClass *undo_class   = LIGMA_UNDO_CLASS (klass);

  object_class->constructed  = ligma_item_undo_constructed;
  object_class->set_property = ligma_item_undo_set_property;
  object_class->get_property = ligma_item_undo_get_property;

  undo_class->free           = ligma_item_undo_free;

  g_object_class_install_property (object_class, PROP_ITEM,
                                   g_param_spec_object ("item", NULL, NULL,
                                                        LIGMA_TYPE_ITEM,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_item_undo_init (LigmaItemUndo *undo)
{
}

static void
ligma_item_undo_constructed (GObject *object)
{
  LigmaItemUndo *item_undo = LIGMA_ITEM_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_ITEM (item_undo->item));
}

static void
ligma_item_undo_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  LigmaItemUndo *item_undo = LIGMA_ITEM_UNDO (object);

  switch (property_id)
    {
    case PROP_ITEM:
      item_undo->item = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_item_undo_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  LigmaItemUndo *item_undo = LIGMA_ITEM_UNDO (object);

  switch (property_id)
    {
    case PROP_ITEM:
      g_value_set_object (value, item_undo->item);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_item_undo_free (LigmaUndo     *undo,
                     LigmaUndoMode  undo_mode)
{
  LigmaItemUndo *item_undo = LIGMA_ITEM_UNDO (undo);

  g_clear_object (&item_undo->item);

  LIGMA_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
