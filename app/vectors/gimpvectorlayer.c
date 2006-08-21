/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpVectorLayer
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

#include <stdio.h>

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "vectors-types.h"

#include "base/tile-manager.h"

#include "core/gimp.h"
#include "core/gimpdrawable-stroke.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"
#include "core/gimpstrokedesc.h"
#include "core/gimpfilloptions.h"
#include "core/gimpparasitelist.h"

#include "gimpvectorlayer.h"
#include "gimpvectorlayeroptions.h"
#include "gimpvectors.h"

#include "gimp-intl.h"

/* properties enum */
enum
{
  PROP_0,
  PROP_VECTOR_LAYER_OPTIONS
};

/* local function declarations */
static void      gimp_vector_layer_finalize       (GObject *object);

static gboolean  gimp_vector_layer_render           (GimpVectorLayer *layer);
static void      gimp_vector_layer_render_vectors   (GimpVectorLayer *layer,
                                                     GimpVectors     *vectors);
static void      gimp_vector_layer_refresh_name     (GimpVectorLayer *layer);
static void      gimp_vector_layer_changed_options  (GimpVectorLayer *layer);
static void      gimp_vector_layer_get_property     (GObject         *object,
                                                     guint            property_id,
                                                     GValue          *value,
                                                     GParamSpec      *pspec);
static void      gimp_vector_layer_set_property     (GObject         *object,
                                                     guint            property_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);
static GimpItem *gimp_vector_layer_duplicate        (GimpItem           *item,
                                                     GType               new_type,
                                                     gboolean            add_alpha);

/* class-related stuff */
G_DEFINE_TYPE (GimpVectorLayer, gimp_vector_layer, GIMP_TYPE_LAYER)

#define parent_class gimp_vector_layer_parent_class


/* class initialization */
static void
gimp_vector_layer_class_init (GimpVectorLayerClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class        = GIMP_ITEM_CLASS (klass);
  
  object_class->set_property           = gimp_vector_layer_set_property;
  object_class->get_property           = gimp_vector_layer_get_property;
  object_class->finalize               = gimp_vector_layer_finalize;
  
  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_VECTOR_LAYER_OPTIONS,
                                   "vector-layer-options", NULL,
                                   GIMP_TYPE_VECTOR_LAYER_OPTIONS,
                                   GIMP_PARAM_STATIC_STRINGS);
  
  viewable_class->default_stock_id = "gimp-vector-layer";
  
  item_class->duplicate            = gimp_vector_layer_duplicate;
  item_class->default_name         = _("Vector Layer");
  item_class->rename_desc          = _("Rename Vector Layer");
  item_class->translate_desc       = _("Move Vector Layer");
  item_class->scale_desc           = _("Scale Vector Layer");
  item_class->resize_desc          = _("Resize Vector Layer");
  item_class->flip_desc            = _("Flip Vector Layer");
  item_class->rotate_desc          = _("Rotate Vector Layer");
  item_class->transform_desc       = _("Transform Vector Layer");
}


/* instance initialization */
static void
gimp_vector_layer_init (GimpVectorLayer *layer)
{
  layer->options = NULL;
  layer->parasite = NULL;
}

static void
gimp_vector_layer_finalize (GObject *object)
{
  GimpVectorLayer *layer = GIMP_VECTOR_LAYER(object);
  
  if (layer->options)
    {
      g_object_unref (layer->options);
      layer->options = NULL;
    }
  
  if (layer->parasite)
    {
      gimp_parasite_list_remove (GIMP_ITEM (layer)->parasites,
                                  layer->parasite);
      layer->parasite = NULL;
    }
}

/* local method definitions */

/* renders the layer */
static gboolean
gimp_vector_layer_render (GimpVectorLayer *layer)
{
  GimpDrawable   *drawable = GIMP_DRAWABLE (layer);
  GimpItem       *item     = GIMP_ITEM (layer);
  GimpImage      *image    = gimp_item_get_image (item);
  gint            width    = gimp_image_get_width (image);
  gint            height   = gimp_image_get_height (image);
    
  g_return_val_if_fail(gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  
  g_object_freeze_notify (G_OBJECT (drawable));
  
  gimp_drawable_configure (GIMP_DRAWABLE (layer),
                           image,
                           0, 0, width, height, /* x and y offsets, x and y dimensions */
                           gimp_image_base_type_with_alpha (image),
                           NULL);
  
  /* make the layer background transparent */
  GimpRGB blank;
  gimp_rgba_set(&blank, 1.0, 1.0, 1.0, 0.0);
  
  gimp_drawable_fill (GIMP_DRAWABLE (layer), &blank, NULL);
  
  /* render vectors to the layer */
  gimp_vector_layer_render_vectors (layer, layer->options->vectors);
  
  g_object_thaw_notify (G_OBJECT (drawable));
  
  return TRUE;
}

/* renders a vectors object onto the layer */
static void
gimp_vector_layer_render_vectors (GimpVectorLayer *layer,
                                  GimpVectors     *vectors)
{
  /* fill the vectors object onto the layer */
  gimp_drawable_fill_vectors (GIMP_DRAWABLE (layer),
                              GIMP_FILL_OPTIONS (layer->options->fill_options),
                              vectors,
                              FALSE);
  
  /* stroke the vectors object onto the layer */
  gimp_item_stroke (GIMP_ITEM (vectors),
                    GIMP_DRAWABLE (layer),
                    gimp_get_user_context (gimp_item_get_image (GIMP_ITEM (layer))->gimp),
                    layer->options->stroke_desc,
                    FALSE,
                    FALSE);
  
}

/* sets the layer's name to be the same as the vector's name */
static void
gimp_vector_layer_refresh_name (GimpVectorLayer  *layer)
{
  gimp_object_set_name_safe (GIMP_OBJECT (layer),
                             gimp_object_get_name (GIMP_OBJECT(layer->options->vectors)));
}

static void
gimp_vector_layer_changed_options (GimpVectorLayer *layer)
{
  GimpItem *item = GIMP_ITEM (layer);
  
  if (layer->parasite)
    {
      /* parasite is out of date, discard it */
      gimp_parasite_list_remove (GIMP_ITEM (layer)->parasites,
                                 layer->parasite);
      layer->parasite = NULL;
    }
  
  if (gimp_item_is_attached (item))
    gimp_vector_layer_refresh (layer);
}

static void
gimp_vector_layer_get_property (GObject      *object,
                                guint         property_id,
                                GValue       *value,
                                GParamSpec   *pspec)
{
  GimpVectorLayer *vector_layer = GIMP_VECTOR_LAYER (object);

  switch (property_id)
    {
    case PROP_VECTOR_LAYER_OPTIONS:
      g_value_set_object (value, vector_layer->options);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_vector_layer_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpVectorLayer *vector_layer = GIMP_VECTOR_LAYER (object);

  switch (property_id)
    {
    case PROP_VECTOR_LAYER_OPTIONS:
      if (vector_layer->options)
        {
          g_object_disconnect (vector_layer->options, "notify",
                               G_CALLBACK (gimp_vector_layer_changed_options),
                               vector_layer, NULL);
          g_object_unref (vector_layer->options);
        }
      vector_layer->options = (GimpVectorLayerOptions *) g_value_dup_object (value);
      gimp_vector_layer_changed_options (vector_layer);
      
      g_signal_connect_object (vector_layer->options, "notify",
                           G_CALLBACK (gimp_vector_layer_changed_options),
                           vector_layer, G_CONNECT_SWAPPED);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GimpItem *
gimp_vector_layer_duplicate        (GimpItem *item,
                                    GType     new_type,
                                    gboolean  add_alpha)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type,
                                                        add_alpha);
  
  if (GIMP_IS_VECTOR_LAYER (new_item))
    {
      GimpVectorLayer *vector_layer     = GIMP_VECTOR_LAYER (item);
      GimpVectorLayer *new_vector_layer = GIMP_VECTOR_LAYER (new_item);
      
      g_object_set (new_vector_layer,
                    "vector-layer-options", gimp_config_duplicate (GIMP_CONFIG (vector_layer->options)),
                    NULL);
    }
  
  return new_item;
}

/* public method definitions */

/**
 * gimp_vector_layer_new:
 * @image: the #GimpImage the layer should belong to
 * @vectors: the #GimpVectors object the layer should render
 * @context: the #GimpContext from which to pull context properties
 *
 * Creates a new vector layer.
 *
 * Return value: a new #GimpVectorLayer or %NULL in case of a problem
 **/
GimpVectorLayer *
gimp_vector_layer_new (GimpImage     *image,
                       GimpVectors   *vectors,
                       GimpContext   *context)
{
  GimpVectorLayer        *layer;
  GimpVectorLayerOptions *options;
  
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);
  
  options = gimp_vector_layer_options_new (image, vectors, context);
  
  layer = g_object_new (GIMP_TYPE_VECTOR_LAYER,
                        "vector-layer-options", options,
                        NULL);
  
  gimp_drawable_configure (GIMP_DRAWABLE (layer),
                           image,
                           0, 0, 1, 1, /* x and y offsets, x and y dimensions */
                           gimp_image_base_type_with_alpha (image),
                           NULL);
  return layer;
}

void
gimp_vector_layer_refresh (GimpVectorLayer  *layer)
{
  gimp_vector_layer_refresh_name (layer);
  gimp_vector_layer_render (layer);
}

gboolean
gimp_drawable_is_vector_layer (GimpDrawable  *drawable)
{
  return (GIMP_IS_VECTOR_LAYER (drawable) &&
          GIMP_VECTOR_LAYER (drawable)->options);
}
