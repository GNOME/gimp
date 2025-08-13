/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2019 Jehan
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
#include "gimplink.h"
#include "gimplinklayer.h"
#include "gimplinklayerundo.h"


enum
{
  PROP_0,
  PROP_PREV_LINK
};


static void     gimp_link_layer_undo_constructed  (GObject             *object);
static void     gimp_link_layer_undo_finalize     (GObject             *object);
static void     gimp_link_layer_undo_set_property (GObject             *object,
                                                   guint                property_id,
                                                   const GValue        *value,
                                                   GParamSpec          *pspec);
static void     gimp_link_layer_undo_get_property (GObject             *object,
                                                   guint                property_id,
                                                   GValue              *value,
                                                   GParamSpec          *pspec);

static gint64   gimp_link_layer_undo_get_memsize  (GimpObject          *object,
                                                   gint64              *gui_size);

static void     gimp_link_layer_undo_pop          (GimpUndo            *undo,
                                                   GimpUndoMode         undo_mode,
                                                   GimpUndoAccumulator *accum);


G_DEFINE_TYPE (GimpLinkLayerUndo, gimp_link_layer_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_link_layer_undo_parent_class


static void
gimp_link_layer_undo_class_init (GimpLinkLayerUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpUndoClass   *undo_class        = GIMP_UNDO_CLASS (klass);

  object_class->constructed      = gimp_link_layer_undo_constructed;
  object_class->finalize         = gimp_link_layer_undo_finalize;
  object_class->set_property     = gimp_link_layer_undo_set_property;
  object_class->get_property     = gimp_link_layer_undo_get_property;

  gimp_object_class->get_memsize = gimp_link_layer_undo_get_memsize;

  undo_class->pop                = gimp_link_layer_undo_pop;

  g_object_class_install_property (object_class, PROP_PREV_LINK,
                                   g_param_spec_object ("prev-link",
                                                        NULL, NULL,
                                                        GIMP_TYPE_LINK,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_link_layer_undo_init (GimpLinkLayerUndo *undo)
{
}

static void
gimp_link_layer_undo_constructed (GObject *object)
{
  GimpLinkLayerUndo *undo = GIMP_LINK_LAYER_UNDO (object);
  GimpLinkLayer     *layer;
  GimpLink          *link;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_LINK_LAYER (GIMP_ITEM_UNDO (object)->item));

  layer = GIMP_LINK_LAYER (GIMP_ITEM_UNDO (undo)->item);

  link       = gimp_link_layer_get_link (layer);
  undo->link = link ? gimp_link_duplicate (link) : NULL;
  gimp_link_layer_get_transform (layer,
                                 &undo->matrix,
                                 &undo->offset_x,
                                 &undo->offset_y,
                                 &undo->interpolation);
}

static void
gimp_link_layer_undo_finalize (GObject *object)
{
  GimpLinkLayerUndo *undo = GIMP_LINK_LAYER_UNDO (object);

  g_clear_object (&undo->link);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_link_layer_undo_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpLinkLayerUndo *undo = GIMP_LINK_LAYER_UNDO (object);

  switch (property_id)
    {
    case PROP_PREV_LINK:
      g_clear_object (&undo->link);
      undo->link = g_value_get_object (value) ? gimp_link_duplicate (g_value_get_object (value)) : NULL;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_link_layer_undo_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpLinkLayerUndo *undo = GIMP_LINK_LAYER_UNDO (object);

  switch (property_id)
    {
    case PROP_PREV_LINK:
      g_value_set_object (value, undo->link);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_link_layer_undo_get_memsize (GimpObject *object,
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
gimp_link_layer_undo_pop (GimpUndo            *undo,
                          GimpUndoMode         undo_mode,
                          GimpUndoAccumulator *accum)
{
  GimpLinkLayerUndo     *layer_undo = GIMP_LINK_LAYER_UNDO (undo);
  GimpLinkLayer         *layer      = GIMP_LINK_LAYER (GIMP_ITEM_UNDO (undo)->item);
  GimpLink              *link;
  GimpMatrix3            matrix;
  gint                   offset_x;
  gint                   offset_y;
  GimpInterpolationType  interpolation;

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  link = gimp_link_layer_get_link (layer);
  link = link ? g_object_ref (link) : NULL;

  gimp_link_layer_get_transform (layer, &matrix, &offset_x, &offset_y, &interpolation);
  gimp_link_layer_set_link_with_matrix (layer, layer_undo->link,
                                        &layer_undo->matrix,
                                        layer_undo->interpolation,
                                        layer_undo->offset_x,
                                        layer_undo->offset_y,
                                        FALSE);


  layer_undo->matrix        = matrix;
  layer_undo->interpolation = interpolation;
  layer_undo->offset_x      = offset_x;
  layer_undo->offset_y      = offset_y;

  g_clear_object (&layer_undo->link);
  layer_undo->link = link;
}
