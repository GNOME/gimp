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

#include <glib-object.h>

#include "libgimpconfig/gimpconfig.h"

#include "libgimpcolor/gimpcolor.h"

#include "vectors-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpstrokedesc.h"
#include "core/gimpfilloptions.h"

#include "gimpvectors.h"
#include "gimpvectorlayer.h"
#include "gimpvectorlayeroptions.h"

#include "gimp-intl.h"

/* properties enum */
enum
{
  PROP_0,
  PROP_VECTORS,
  PROP_VECTORS_TATTOO,
  PROP_FILL_OPTIONS,
  PROP_STROKE_DESC
};

/* local function declarations */
static void     gimp_vector_layer_options_config_iface_init (gpointer    iface,
                                                        gpointer    iface_data);
static void     gimp_vector_layer_options_finalize     (GObject *object);

static void     gimp_vector_layer_options_get_property (GObject         *object,
                                                        guint            property_id,
                                                        GValue          *value,
                                                        GParamSpec      *pspec);
static void     gimp_vector_layer_options_set_property (GObject         *object,
                                                        guint            property_id,
                                                        const GValue    *value,
                                                        GParamSpec      *pspec);
static void     gimp_vector_layer_options_emit_vectors_changed  
                                                       (GimpVectorLayerOptions *options);

/* class-related stuff */
G_DEFINE_TYPE_WITH_CODE (GimpVectorLayerOptions,
                         gimp_vector_layer_options,
                         GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                            gimp_vector_layer_options_config_iface_init))

#define parent_class gimp_vector_layer_options_parent_class

/* class initialization */
static void
gimp_vector_layer_options_class_init (GimpVectorLayerOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  object_class->set_property = gimp_vector_layer_options_set_property;
  object_class->get_property = gimp_vector_layer_options_get_property;
  object_class->finalize     = gimp_vector_layer_options_finalize;
  
  g_object_class_install_property (object_class, PROP_VECTORS,
                                   g_param_spec_object ("vectors",
                                                        NULL, NULL,
                                                        GIMP_TYPE_VECTORS,
                                                        G_PARAM_CONSTRUCT |
                                                        GIMP_PARAM_READWRITE |
                                                        GIMP_PARAM_STATIC_STRINGS));
  /*g_object_class_install_property (object_class, PROP_VECTORS_TATTOO,
                                   g_param_spec_uint ("vectors-tattoo",
                                                      NULL, NULL,
                                                      0, G_MAXUINT32, 0,
                                                      GIMP_PARAM_READABLE | 
                                                      GIMP_CONFIG_PARAM_SERIALIZE | 
                                                      GIMP_PARAM_STATIC_STRINGS));*/
  GIMP_CONFIG_INSTALL_PROP_UINT   (object_class, PROP_VECTORS_TATTOO,
                                   "vectors-tattoo", NULL, 0, G_MAXUINT32, 0,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_FILL_OPTIONS,
                                   "fill-options", NULL,
                                   GIMP_TYPE_FILL_OPTIONS,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_STROKE_DESC,
                                   "stroke-desc", NULL,
                                   GIMP_TYPE_STROKE_DESC,
                                   GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_vector_layer_options_config_iface_init (gpointer  iface,
                                             gpointer  iface_data)
{
  /* Do Nothing */
}

/* instance initialization */
static void
gimp_vector_layer_options_init (GimpVectorLayerOptions *options)
{
  options->vectors = NULL;
  options->vectors_tattoo = 0;
  options->fill_options = NULL;
  options->stroke_desc = NULL;
}

static void
gimp_vector_layer_options_finalize (GObject *object)
{
  GimpVectorLayerOptions *options = GIMP_VECTOR_LAYER_OPTIONS(object);
  
  if(options->vectors)
    {
      g_object_unref (options->vectors);
      options->vectors = NULL;
    }
    
  if(options->fill_options)
    {
      g_object_unref (options->fill_options);
      options->fill_options = NULL;
    }
    
  if(options->stroke_desc)
    {
      g_object_unref (options->stroke_desc);
      options->stroke_desc = NULL;
    }
    
    G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* local definitions */

static void
gimp_vector_layer_options_get_property (GObject      *object,
                                        guint         property_id,
                                        GValue       *value,
                                        GParamSpec   *pspec)
{
  GimpVectorLayerOptions *options = GIMP_VECTOR_LAYER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_VECTORS:
      g_value_set_object (value, options->vectors);
      break;
    case PROP_VECTORS_TATTOO:
      g_value_set_uint (value, options->vectors_tattoo);
      break;
    case PROP_FILL_OPTIONS:
      g_value_set_object (value, options->fill_options);
      break;
    case PROP_STROKE_DESC:
      g_value_set_object (value, options->stroke_desc);
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
    case PROP_VECTORS:
      if (options->vectors)
        g_object_unref (options->vectors);
      options->vectors = (GimpVectors *) g_value_dup_object (value);
      
      g_signal_connect_object (options->vectors, "invalidate-preview",
                           G_CALLBACK (gimp_vector_layer_options_emit_vectors_changed),
                           options, G_CONNECT_SWAPPED);
      g_signal_connect_object (options->vectors, "name-changed",
                           G_CALLBACK (gimp_vector_layer_options_emit_vectors_changed),
                           options, G_CONNECT_SWAPPED);
      
      /* update the tattoo */
      options->vectors_tattoo = gimp_item_get_tattoo(GIMP_ITEM (options->vectors));
      break;
    case PROP_VECTORS_TATTOO:
      options->vectors_tattoo = g_value_get_uint (value);
      
      if (options->vectors &&
          gimp_item_get_tattoo (GIMP_ITEM (options->vectors)) != options->vectors_tattoo)
        {
          GimpImage *image = gimp_item_get_image (GIMP_ITEM (options->vectors));
          
          g_object_set (options,
                        "vectors", gimp_image_get_vectors_by_tattoo (
                          image, options->vectors_tattoo),
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
    case PROP_STROKE_DESC:
      if (g_value_get_object (value))
        /*gimp_config_sync (g_value_get_object (value),
                          G_OBJECT (vector_layer->stroke_desc), 0);*/
        {
          if (options->stroke_desc)
            g_object_unref (options->stroke_desc);
          options->stroke_desc = gimp_config_duplicate (g_value_get_object (value));
        }    
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_vector_layer_options_emit_vectors_changed (GimpVectorLayerOptions *options)
{
  g_object_notify (G_OBJECT (options), "vectors");
}

/* public functions */

/**
 * gimp_vector_layer_options_new:
 * @image: the #GimpImage the layer belongs to
 * @vectors: the #GimpVectors object for the layer to render
 *
 * Creates a new vector layer options.
 *
 * Return value: a new #GimpVectorLayerOptions or %NULL in case of a problem
 **/
GimpVectorLayerOptions *
gimp_vector_layer_options_new (GimpImage     *image,
                       				 GimpVectors   *vectors)
{
  GimpVectorLayerOptions *options;
  GimpContext            *user_context = gimp_get_user_context (image->gimp);
  GimpPattern            *pattern      = gimp_context_get_pattern (user_context);
  GimpRGB                 black;
  GimpRGB                 blue;
  
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);
  
  options = g_object_new (GIMP_TYPE_VECTOR_LAYER_OPTIONS,
                          "vectors", vectors,
                          NULL);
  
  gimp_rgba_set(&black, 0.0, 0.0, 0.0, 1.0);
  gimp_rgba_set(&blue, 0.0, 0.0, 1.0, 1.0);
  
  options->fill_options = g_object_new (GIMP_TYPE_FILL_OPTIONS,
                                        "gimp", image->gimp,
                                        "foreground",   &blue,
                                        NULL);
  gimp_context_set_pattern (GIMP_CONTEXT (options->fill_options), pattern);
  
  options->stroke_desc = gimp_stroke_desc_new (image->gimp, NULL);
  g_object_set (options->stroke_desc->stroke_options,
                "foreground",   &black,
                "width",        2.0,
                NULL); 
  gimp_context_set_pattern (GIMP_CONTEXT (options->stroke_desc->stroke_options), pattern);
  
  return options;
}




