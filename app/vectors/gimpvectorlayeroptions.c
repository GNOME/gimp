/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpVectorLayerOptions
 * Copyright (C) 2006 Hendrik Boom
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>
#include <cairo.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "libgimpcolor/gimpcolor.h"

#include "vectors-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpstrokeoptions.h"

#include "gimpvectors.h"
#include "gimpvectorlayer.h"
#include "gimpvectorlayeroptions.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_GIMP,
  PROP_VECTORS,
  PROP_VECTORS_TATTOO,
  PROP_FILL_OPTIONS,
  PROP_STROKE_OPTIONS
};


/* local function declarations */

static GObject *gimp_vector_layer_options_constructor  (GType       type,
                                                        guint       n_params,
                                                        GObjectConstructParam *params);
static void     gimp_vector_layer_options_finalize     (GObject         *object);

static void     gimp_vector_layer_options_get_property (GObject         *object,
                                                        guint            property_id,
                                                        GValue          *value,
                                                        GParamSpec      *pspec);
static void     gimp_vector_layer_options_set_property (GObject         *object,
                                                        guint            property_id,
                                                        const GValue    *value,
                                                        GParamSpec      *pspec);
static void     gimp_vector_layer_options_vectors_changed
                                                       (GimpVectorLayerOptions *options);
static void     gimp_vector_layer_options_vectors_removed
                                                       (GimpVectorLayerOptions *options);


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
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_VECTORS,
                                   g_param_spec_object ("vectors",
                                                        NULL, NULL,
                                                        GIMP_TYPE_VECTORS,
                                                        G_PARAM_CONSTRUCT |
                                                        GIMP_PARAM_READWRITE |
                                                        GIMP_PARAM_STATIC_STRINGS));

#if 0
  g_object_class_install_property (object_class, PROP_VECTORS_TATTOO,
                                   g_param_spec_uint ("vectors-tattoo",
                                                      NULL, NULL,
                                                      0, G_MAXUINT32, 0,
                                                      GIMP_PARAM_READABLE |
                                                      GIMP_CONFIG_PARAM_SERIALIZE |
                                                      GIMP_PARAM_STATIC_STRINGS));
#endif

  GIMP_CONFIG_PROP_UINT   (object_class, PROP_VECTORS_TATTOO,
                           "vectors-tattoo", NULL, NULL,
                           0, G_MAXUINT32, 0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_FILL_OPTIONS,
                           "fill-options", NULL, NULL,
                           GIMP_TYPE_FILL_OPTIONS,
                           GIMP_PARAM_STATIC_STRINGS |
                           GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_STROKE_OPTIONS,
                           "stroke-options", NULL, NULL,
                           GIMP_TYPE_STROKE_OPTIONS,
                           GIMP_PARAM_STATIC_STRINGS |
                           GIMP_CONFIG_PARAM_AGGREGATE);
}

static void
gimp_vector_layer_options_init (GimpVectorLayerOptions *options)
{
  options->vectors        = NULL;
  options->vectors_tattoo = 0;
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

  if (options->vectors)
    {
      g_object_unref (options->vectors);
      options->vectors = NULL;
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
    case PROP_VECTORS:
      g_value_set_object (value, options->vectors);
      break;
    case PROP_VECTORS_TATTOO:
      g_value_set_uint (value, options->vectors_tattoo);
      break;
    case PROP_FILL_OPTIONS:
      g_value_set_object (value, options->fill_options);
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
    case PROP_VECTORS:
      if (options->vectors)
        {
          g_signal_handlers_disconnect_by_func (options->vectors,
                                                G_CALLBACK (gimp_vector_layer_options_vectors_changed),
                                                options);
          g_signal_handlers_disconnect_by_func (options->vectors,
                                                G_CALLBACK (gimp_vector_layer_options_vectors_removed),
                                                options);
          g_object_unref (options->vectors);
        }

      options->vectors = g_value_dup_object (value);

      if (options->vectors)
        {
          g_signal_connect_object (options->vectors, "invalidate-preview",
                                   G_CALLBACK (gimp_vector_layer_options_vectors_changed),
                                   options, G_CONNECT_SWAPPED);
          g_signal_connect_object (options->vectors, "name-changed",
                                   G_CALLBACK (gimp_vector_layer_options_vectors_changed),
                                   options, G_CONNECT_SWAPPED);
          g_signal_connect_object (options->vectors, "removed",
                                   G_CALLBACK (gimp_vector_layer_options_vectors_removed),
                                   options, G_CONNECT_SWAPPED);

          /* update the tattoo */
          options->vectors_tattoo = gimp_item_get_tattoo (GIMP_ITEM (options->vectors));
        }
      break;
    case PROP_VECTORS_TATTOO:
      options->vectors_tattoo = g_value_get_uint (value);

      if (options->vectors &&
          gimp_item_get_tattoo (GIMP_ITEM (options->vectors)) != options->vectors_tattoo)
        {
          GimpImage *image = gimp_item_get_image (GIMP_ITEM (options->vectors));

          g_object_set (options,
                        "vectors", gimp_image_get_vectors_by_tattoo
                        (image, options->vectors_tattoo),
                        NULL);
        }
      break;
    case PROP_FILL_OPTIONS:
      if (g_value_get_object (value))
        {
          gimp_config_sync (g_value_get_object (value),
                            G_OBJECT (options->fill_options), 0);
        }
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
gimp_vector_layer_options_vectors_changed (GimpVectorLayerOptions *options)
{
  g_object_notify (G_OBJECT (options), "vectors");
}

static void
gimp_vector_layer_options_vectors_removed (GimpVectorLayerOptions *options)
{
  g_object_set (options, "vectors", NULL, NULL);
}


/* public functions */

/**
 * gimp_vector_layer_options_new:
 * @image: the #GimpImage the layer belongs to
 * @vectors: the #GimpVectors object for the layer to render
 * @context: the #GimpContext from which to pull context properties
 *
 * Creates a new vector layer options.
 *
 * Return value: a new #GimpVectorLayerOptions or %NULL in case of a problem
 **/
GimpVectorLayerOptions *
gimp_vector_layer_options_new (GimpImage     *image,
                               GimpVectors   *vectors,
                               GimpContext   *context)
{
  GimpVectorLayerOptions *options;
  GimpPattern            *pattern;
  GimpRGB                 stroke_color;
  GimpRGB                 fill_color;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  options = g_object_new (GIMP_TYPE_VECTOR_LAYER_OPTIONS,
                          "gimp", image->gimp,
                          NULL);

  gimp_context_get_foreground (context, &stroke_color);
  gimp_context_get_background (context, &fill_color);
  pattern = gimp_context_get_pattern (context);

  gimp_context_set_foreground (GIMP_CONTEXT (options->fill_options),
                               &fill_color);
  gimp_context_set_pattern (GIMP_CONTEXT (options->fill_options), pattern);

  gimp_context_set_foreground (GIMP_CONTEXT (options->stroke_options),
                               &stroke_color);
  gimp_context_set_pattern (GIMP_CONTEXT (options->stroke_options), pattern);

  g_object_set (options->stroke_options,
                "width", 3.0,
                NULL);

  g_object_set (options,
                "vectors", vectors,
                NULL);

  return options;
}
