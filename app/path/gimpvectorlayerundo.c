/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   gimpvectorlayerundo.h
 *
 *   Copyright 2006 Hendrik Boom
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>
#include <gegl.h>

#include "libgimpconfig/gimpconfig.h"

#include "path-types.h"

#include "core/gimp-memsize.h"
#include "core/gimp-utils.h"
#include "core/gimpitem.h"
#include "core/gimpitemundo.h"

#include "gimpvectorlayer.h"
#include "gimpvectorlayeroptions.h"
#include "gimpvectorlayerundo.h"

enum
{
  PROP_0,
  PROP_PARAM
};


static void     gimp_vector_layer_undo_constructed  (GObject             *object);
static void     gimp_vector_layer_undo_set_property (GObject             *object,
                                                     guint                property_id,
                                                     const GValue        *value,
                                                     GParamSpec          *pspec);
static void     gimp_vector_layer_undo_get_property (GObject             *object,
                                                     guint                property_id,
                                                     GValue              *value,
                                                     GParamSpec          *pspec);
static gint64   gimp_vector_layer_undo_get_memsize  (GimpObject          *object,
                                                     gint64              *gui_size);
static void     gimp_vector_layer_undo_pop          (GimpUndo            *undo,
                                                     GimpUndoMode         undo_mode,
                                                     GimpUndoAccumulator *accum);
static void     gimp_vector_layer_undo_free         (GimpUndo            *undo,
                                                     GimpUndoMode         undo_mode);



G_DEFINE_TYPE (GimpVectorLayerUndo, gimp_vector_layer_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_vector_layer_undo_parent_class


static void
gimp_vector_layer_undo_class_init (GimpVectorLayerUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpUndoClass   *undo_class        = GIMP_UNDO_CLASS (klass);

  object_class->constructed          = gimp_vector_layer_undo_constructed;
  object_class->set_property         = gimp_vector_layer_undo_set_property;
  object_class->get_property         = gimp_vector_layer_undo_get_property;

  gimp_object_class->get_memsize = gimp_vector_layer_undo_get_memsize;

  undo_class->pop                    = gimp_vector_layer_undo_pop;
  undo_class->free                   = gimp_vector_layer_undo_free;

  g_object_class_install_property (object_class, PROP_PARAM,
                                   g_param_spec_param ("param", NULL, NULL,
                                                       G_TYPE_PARAM,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_vector_layer_undo_init (GimpVectorLayerUndo *undo)
{
}

static void
gimp_vector_layer_undo_constructed (GObject *object)
{
  GimpVectorLayerUndo *vector_undo = GIMP_VECTOR_LAYER_UNDO (object);
  GimpVectorLayer     *vector_layer;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_VECTOR_LAYER (GIMP_ITEM_UNDO (vector_undo)->item));

  vector_layer = GIMP_VECTOR_LAYER (GIMP_ITEM_UNDO (vector_undo)->item);

  switch (GIMP_UNDO (object)->undo_type)
    {
    case GIMP_UNDO_VECTOR_LAYER:
      if (vector_undo->pspec)
        {
          g_assert (vector_undo->pspec->owner_type == GIMP_TYPE_VECTOR_LAYER_OPTIONS);

          vector_undo->value = g_slice_new0 (GValue);

          g_value_init (vector_undo->value, vector_undo->pspec->value_type);
          g_object_get_property (G_OBJECT (vector_layer->options),
                                 vector_undo->pspec->name, vector_undo->value);
        }
      else if (vector_layer->options)
        {
          vector_undo->vector_layer_options = gimp_config_duplicate (GIMP_CONFIG (vector_layer->options));
        }
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
gimp_vector_layer_undo_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpVectorLayerUndo *vector_undo = GIMP_VECTOR_LAYER_UNDO (object);

  switch (property_id)
    {
    case PROP_PARAM:
      vector_undo->pspec = g_value_get_param (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_vector_layer_undo_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GimpVectorLayerUndo *vector_undo = GIMP_VECTOR_LAYER_UNDO (object);

  switch (property_id)
    {
    case PROP_PARAM:
      g_value_set_param (value, (GParamSpec *) vector_undo->pspec);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_vector_layer_undo_get_memsize (GimpObject *object,
                                    gint64     *gui_size)
{
  GimpVectorLayerUndo *undo    = GIMP_VECTOR_LAYER_UNDO (object);
  gint64               memsize = 0;

  memsize += gimp_g_value_get_memsize (undo->value);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (undo->vector_layer_options), NULL);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_vector_layer_undo_pop (GimpUndo            *undo,
                            GimpUndoMode         undo_mode,
                            GimpUndoAccumulator *accum)
{
  GimpVectorLayerUndo *vector_undo  = GIMP_VECTOR_LAYER_UNDO (undo);
  GimpVectorLayer     *vector_layer = GIMP_VECTOR_LAYER (GIMP_ITEM_UNDO (undo)->item);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case GIMP_UNDO_VECTOR_LAYER:
      if (vector_undo->pspec)
        {
          GValue *value;

          g_return_if_fail (vector_layer->options != NULL);

          value = g_slice_new0 (GValue);
          g_value_init (value, vector_undo->pspec->value_type);

          g_object_get_property (G_OBJECT (vector_layer->options),
                                 vector_undo->pspec->name, value);

          g_object_set_property (G_OBJECT (vector_layer->options),
                                 vector_undo->pspec->name, vector_undo->value);

          g_value_unset (vector_undo->value);
          g_slice_free (GValue, vector_undo->value);

          vector_undo->value = value;
        }
      else
        {
          GimpVectorLayerOptions *vector_layer_options;

          vector_layer_options = (vector_layer->options ?
                                  gimp_config_duplicate (GIMP_CONFIG (vector_layer->options)) : NULL);

          if (vector_layer->options && vector_undo->vector_layer_options)
            gimp_config_sync (G_OBJECT (vector_undo->vector_layer_options),
                              G_OBJECT (vector_layer->options), 0);
          else
            g_object_set (vector_layer,
                          "vector-layer-options", vector_undo->vector_layer_options,
                          NULL);

          if (vector_undo->vector_layer_options)
            g_object_unref (vector_undo->vector_layer_options);

          vector_undo->vector_layer_options = vector_layer_options;
        }
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
gimp_vector_layer_undo_free (GimpUndo     *undo,
                             GimpUndoMode  undo_mode)
{
  GimpVectorLayerUndo *vector_undo = GIMP_VECTOR_LAYER_UNDO (undo);

  if (vector_undo->vector_layer_options)
    {
      g_object_unref (vector_undo->vector_layer_options);
      vector_undo->vector_layer_options = NULL;
    }

  if (vector_undo->pspec)
    {
      g_value_unset (vector_undo->value);
      g_slice_free (GValue, vector_undo->value);

      vector_undo->value = NULL;
      vector_undo->pspec = NULL;
    }

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
