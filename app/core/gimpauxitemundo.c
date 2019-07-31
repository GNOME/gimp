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

#include "gimpauxitem.h"
#include "gimpauxitemundo.h"


enum
{
  PROP_0,
  PROP_AUX_ITEM
};


static void   gimp_aux_item_undo_constructed  (GObject      *object);
static void   gimp_aux_item_undo_set_property (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec);
static void   gimp_aux_item_undo_get_property (GObject      *object,
                                               guint         property_id,
                                               GValue       *value,
                                               GParamSpec   *pspec);

static void   gimp_aux_item_undo_free         (GimpUndo     *undo,
                                               GimpUndoMode  undo_mode);


G_DEFINE_ABSTRACT_TYPE (GimpAuxItemUndo, gimp_aux_item_undo, GIMP_TYPE_UNDO)

#define parent_class gimp_aux_item_undo_parent_class


static void
gimp_aux_item_undo_class_init (GimpAuxItemUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructed  = gimp_aux_item_undo_constructed;
  object_class->set_property = gimp_aux_item_undo_set_property;
  object_class->get_property = gimp_aux_item_undo_get_property;

  undo_class->free           = gimp_aux_item_undo_free;

  g_object_class_install_property (object_class, PROP_AUX_ITEM,
                                   g_param_spec_object ("aux-item", NULL, NULL,
                                                        GIMP_TYPE_AUX_ITEM,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_aux_item_undo_init (GimpAuxItemUndo *undo)
{
}

static void
gimp_aux_item_undo_constructed (GObject *object)
{
  GimpAuxItemUndo *aux_item_undo = GIMP_AUX_ITEM_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_AUX_ITEM (aux_item_undo->aux_item));
}

static void
gimp_aux_item_undo_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpAuxItemUndo *aux_item_undo = GIMP_AUX_ITEM_UNDO (object);

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
gimp_aux_item_undo_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpAuxItemUndo *aux_item_undo = GIMP_AUX_ITEM_UNDO (object);

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
gimp_aux_item_undo_free (GimpUndo     *undo,
                         GimpUndoMode  undo_mode)
{
  GimpAuxItemUndo *aux_item_undo = GIMP_AUX_ITEM_UNDO (undo);

  g_clear_object (&aux_item_undo->aux_item);

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
