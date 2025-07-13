/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   gimpvectorlayers.c
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpcolor/gimpcolor.h"

#include "path-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpstrokeoptions.h"

#include "gimppath.h"
#include "gimpvectorlayer.h"
#include "gimpvectorlayeroptions.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_GIMP,
  PROP_PATH,
  PROP_PATH_TATTOO,
  PROP_ENABLE_FILL,
  PROP_FILL_OPTIONS,
  PROP_ENABLE_STROKE,
  PROP_STROKE_OPTIONS
};


/* local function declarations */

static GObject *gimp_vector_layer_options_constructor  (GType                   type,
                                                        guint                   n_params,
                                                        GObjectConstructParam  *params);
static void     gimp_vector_layer_options_finalize     (GObject                *object);

static void     gimp_vector_layer_options_get_property (GObject                *object,
                                                        guint                   property_id,
                                                        GValue                 *value,
                                                        GParamSpec             *pspec);
static void     gimp_vector_layer_options_set_property (GObject                *object,
                                                        guint                   property_id,
                                                        const GValue           *value,
                                                        GParamSpec             *pspec);
static void     gimp_vector_layer_options_path_changed (GimpVectorLayerOptions *options);


G_DEFINE_TYPE_WITH_CODE (GimpVectorLayerOptions,
                         gimp_vector_layer_options,
                         GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG, NULL))

#define parent_class gimp_vector_layer_options_parent_class


static void
gimp_vector_layer_options_class_init (GimpVectorLayerOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor  = gimp_vector_layer_options_constructor;
  object_class->finalize     = gimp_vector_layer_options_finalize;
  object_class->set_property = gimp_vector_layer_options_set_property;
  object_class->get_property = gimp_vector_layer_options_get_property;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_PATH,
                           "path", NULL, NULL,
                           GIMP_TYPE_PATH,
                           GIMP_PARAM_READWRITE |
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_UINT   (object_class, PROP_PATH_TATTOO,
                           "path-tattoo", NULL, NULL,
                           0, G_MAXUINT32, 0,
                           G_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_ENABLE_FILL,
                            "enable-fill",
                            _("Fill Style"), NULL,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_FILL_OPTIONS,
                           "fill-options", NULL, NULL,
                           GIMP_TYPE_FILL_OPTIONS,
                           G_PARAM_STATIC_STRINGS |
                           GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_ENABLE_STROKE,
                            "enable-stroke",
                            _("Stroke Style"), NULL,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_STROKE_OPTIONS,
                           "stroke-options", NULL, NULL,
                           GIMP_TYPE_STROKE_OPTIONS,
                           G_PARAM_STATIC_STRINGS |
                           GIMP_CONFIG_PARAM_AGGREGATE);
}

static void
gimp_vector_layer_options_init (GimpVectorLayerOptions *options)
{
  options->path        = NULL;
  options->path_tattoo = 0;
}

static GObject *
gimp_vector_layer_options_constructor (GType                  type,
                                       guint                  n_params,
                                       GObjectConstructParam *params)
{
  GObject                *object;
  GimpVectorLayerOptions *options;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  options = GIMP_VECTOR_LAYER_OPTIONS (object);
  g_assert (GIMP_IS_GIMP (options->gimp));

  options->fill_options   = gimp_fill_options_new   (options->gimp, NULL, FALSE);
  options->stroke_options = gimp_stroke_options_new (options->gimp, NULL, FALSE);

  return object;
}

static void
gimp_vector_layer_options_finalize (GObject *object)
{
  GimpVectorLayerOptions *options = GIMP_VECTOR_LAYER_OPTIONS (object);

  if (options->path)
    {
      g_object_unref (options->path);
      options->path = NULL;
    }

  if (options->fill_options)
    {
      g_object_unref (options->fill_options);
      options->fill_options = NULL;
    }

  if (options->stroke_options)
    {
      g_object_unref (options->stroke_options);
      options->stroke_options = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_vector_layer_options_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GimpVectorLayerOptions *options = GIMP_VECTOR_LAYER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, options->gimp);
      break;
    case PROP_PATH:
      g_value_set_object (value, options->path);
      break;
    case PROP_PATH_TATTOO:
      g_value_set_uint (value, options->path_tattoo);
      break;
    case PROP_ENABLE_FILL:
      g_value_set_boolean (value, options->enable_fill);
      break;
    case PROP_FILL_OPTIONS:
      g_value_set_object (value, options->fill_options);
      break;
    case PROP_ENABLE_STROKE:
      g_value_set_boolean (value, options->enable_stroke);
      break;
    case PROP_STROKE_OPTIONS:
      g_value_set_object (value, options->stroke_options);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_vector_layer_options_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GimpVectorLayerOptions *options = GIMP_VECTOR_LAYER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_GIMP:
      options->gimp = g_value_get_object (value);
      break;

    case PROP_PATH:
      if (options->path)
        {
          g_signal_handlers_disconnect_by_func (options->path,
                                                G_CALLBACK (gimp_vector_layer_options_path_changed),
                                                options);
          g_object_unref (options->path);
        }

      options->path = g_value_dup_object (value);

      if (options->path)
        {
          g_signal_connect_object (options->path, "invalidate-preview",
                                   G_CALLBACK (gimp_vector_layer_options_path_changed),
                                   options, G_CONNECT_SWAPPED);
          g_signal_connect_object (options->path, "name-changed",
                                   G_CALLBACK (gimp_vector_layer_options_path_changed),
                                   options, G_CONNECT_SWAPPED);

          /* update the tattoo */
          options->path_tattoo = gimp_item_get_tattoo (GIMP_ITEM (options->path));
        }
      break;

    case PROP_PATH_TATTOO:
      options->path_tattoo = g_value_get_uint (value);

      if (options->path &&
          gimp_item_get_tattoo (GIMP_ITEM (options->path)) != options->path_tattoo)
        {
          GimpImage *image = gimp_item_get_image (GIMP_ITEM (options->path));

          g_object_set (options,
                        "path", gimp_image_get_path_by_tattoo
                        (image, options->path_tattoo),
                        NULL);
        }
      break;

    case PROP_ENABLE_FILL:
      options->enable_fill = g_value_get_boolean (value);
      break;

    case PROP_FILL_OPTIONS:
      if (g_value_get_object (value))
          gimp_config_sync (g_value_get_object (value),
                            G_OBJECT (options->fill_options), 0);
      break;

    case PROP_ENABLE_STROKE:
      options->enable_stroke = g_value_get_boolean (value);
      break;

    case PROP_STROKE_OPTIONS:
      if (g_value_get_object (value))
        {
          if (options->stroke_options)
            g_object_unref (options->stroke_options);
          options->stroke_options = gimp_config_duplicate (g_value_get_object (value));
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_vector_layer_options_path_changed (GimpVectorLayerOptions *options)
{
  g_object_notify (G_OBJECT (options), "path");
}


/* public functions */

/**
 * gimp_vector_layer_options_new:
 * @image:   the #GimpImage the layer belongs to
 * @path:    the #GimpPath object for the layer to render
 * @context: the #GimpContext from which to pull context properties
 *
 * Creates a new vector layer options.
 *
 * Return value: a new #GimpVectorLayerOptions or %NULL in case of a problem
 **/
GimpVectorLayerOptions *
gimp_vector_layer_options_new (GimpImage     *image,
                               GimpPath      *path,
                               GimpContext   *context)
{
  GimpVectorLayerOptions *options;
  GimpPattern            *pattern;
  GeglColor              *stroke_color;
  GeglColor              *fill_color;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_PATH (path), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  options = g_object_new (GIMP_TYPE_VECTOR_LAYER_OPTIONS,
                          "gimp", image->gimp,
                          NULL);

  stroke_color = gimp_context_get_foreground (context);
  fill_color   = gimp_context_get_background (context);
  pattern      = gimp_context_get_pattern (context);

  gimp_context_set_foreground (GIMP_CONTEXT (options->fill_options),
                               fill_color);
  gimp_context_set_pattern (GIMP_CONTEXT (options->fill_options), pattern);

  gimp_context_set_foreground (GIMP_CONTEXT (options->stroke_options),
                               stroke_color);
  gimp_context_set_pattern (GIMP_CONTEXT (options->stroke_options), pattern);

  g_object_set (options,
                "path", path,
                NULL);

  return options;
}
