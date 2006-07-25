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

#include "vectors/gimpvectors.h"

#include "gimpvectorlayer.h"

#include "gimp-intl.h"
/* no doubt more includes will go here */

/* properties enum */

enum
{
  PROP_0,
  PROP_VECTORS,
  PROP_FILL_OPTIONS,
  PROP_STROKE_DESC
};

/* local function declarations */
static void      gimp_vector_layer_finalize       (GObject *object);

static gboolean  gimp_vector_layer_render           (GimpVectorLayer *layer);
static void      gimp_vector_layer_render_vectors   (GimpVectorLayer *layer,
                                                     GimpVectors *vectors);
static void      gimp_vector_layer_refresh_name     (GimpVectorLayer  *layer);
static void      gimp_vector_layer_get_property     (GObject         *object,
                                                     guint            property_id,
                                                     GValue          *value,
                                                     GParamSpec      *pspec);
static void      gimp_vector_layer_set_property     (GObject         *object,
                                                     guint            property_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);
static void      gimp_vector_layer_init_stroke_fill (GimpVectorLayer *layer);

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
  
  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_VECTORS,
                                   "vectors", NULL,
                                   GIMP_TYPE_VECTORS,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_FILL_OPTIONS,
                                   "fill-options", NULL,
                                   GIMP_TYPE_FILL_OPTIONS,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_STROKE_DESC,
                                   "stroke-desc", NULL,
                                   GIMP_TYPE_STROKE_DESC,
                                   GIMP_PARAM_STATIC_STRINGS);
  
  viewable_class->default_stock_id = "gimp-vector-layer";

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
  layer->vectors = NULL;
  layer->fill_options = NULL;
  layer->stroke_desc = NULL;
  layer->parasite = NULL;
}

static void
gimp_vector_layer_finalize (GObject *object)
{
  GimpVectorLayer *layer = GIMP_VECTOR_LAYER(object);
  
  if(layer->vectors)
    {
      g_object_unref (layer->vectors);
      layer->vectors = NULL;
    }
    
  if(layer->fill_options)
    {
      g_object_unref (layer->fill_options);
      layer->fill_options = NULL;
    }
    
  if(layer->vectors)
    {
      g_object_unref (layer->stroke_desc);
      layer->stroke_desc = NULL;
    }
    
    G_OBJECT_CLASS (parent_class)->finalize (object);
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
  
  TileManager *new_tiles = tile_manager_new (width, height,
                                             drawable->bytes);
  
  gimp_drawable_set_tiles (drawable, FALSE, NULL, new_tiles);
  
  tile_manager_unref (new_tiles);
  
  new_tiles = NULL;
  
  /* make the layer background transparent */
  GimpRGB blank;
  gimp_rgba_set(&blank, 1.0, 1.0, 1.0, 0.0);
  
  gimp_drawable_fill (GIMP_DRAWABLE (layer), &blank, NULL);
  
  /* render vectors to the layer */
  gimp_vector_layer_render_vectors (layer, layer->vectors);
  
  g_object_thaw_notify (G_OBJECT (drawable));
  
  return TRUE;
}

/* renders a vectors object onto the layer */
static void
gimp_vector_layer_render_vectors (GimpVectorLayer *layer,
                                  GimpVectors     *vectors)
{
  if (layer->fill_options == NULL || layer->stroke_desc == NULL)
    gimp_vector_layer_init_stroke_fill (layer);
  
  /* fill the vectors object onto the layer */
  gimp_drawable_fill_vectors (GIMP_DRAWABLE (layer),
                              GIMP_FILL_OPTIONS (layer->fill_options),
                              vectors,
                              FALSE);
  
  /* stroke the vectors object onto the layer */
  gimp_item_stroke (GIMP_ITEM (vectors),
                    GIMP_DRAWABLE (layer),
                    gimp_get_user_context (gimp_item_get_image (GIMP_ITEM (layer))->gimp),
                    layer->stroke_desc,
                    FALSE,
                    FALSE);
  
  /*GIMP_ITEM_GET_CLASS (vectors)->stroke(GIMP_ITEM (vectors), GIMP_DRAWABLE (layer), layer->stroke_desc);*/
}

/* sets the layer's name to be the same as the vector's name */
static void
gimp_vector_layer_refresh_name (GimpVectorLayer  *layer)
{
  gimp_object_set_name_safe (GIMP_OBJECT (layer),
                             gimp_object_get_name (GIMP_OBJECT(layer->vectors)));
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
    case PROP_VECTORS:
      g_value_set_object (value, vector_layer->vectors);
      break;
    case PROP_FILL_OPTIONS:
      g_value_set_object (value, vector_layer->fill_options);
      break;
    case PROP_STROKE_DESC:
      g_value_set_object (value, vector_layer->stroke_desc);
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
    case PROP_VECTORS:
      if (vector_layer->vectors)
        g_object_unref (vector_layer->vectors);
      vector_layer->vectors = (GimpVectors *) g_value_dup_object (value);
      
      g_signal_connect_object (vector_layer->vectors, "invalidate-preview",
                           G_CALLBACK (gimp_vector_layer_refresh),
                           vector_layer, G_CONNECT_SWAPPED);
      g_signal_connect_object (vector_layer->vectors, "name-changed",
                           G_CALLBACK (gimp_vector_layer_refresh_name),
                           vector_layer, G_CONNECT_SWAPPED);
      
      /* The parasite is no longer up to date, so remove it */
      if (vector_layer->parasite)
        {
          gimp_parasite_list_remove (GIMP_ITEM (vector_layer)->parasites,
                                     vector_layer->parasite);
          vector_layer->parasite = NULL;
        }
      break;
    case PROP_FILL_OPTIONS: /*TODO: will wierd things happen if this is called before init_stroke_fill? */
      if (g_value_get_object (value))
        {
          gimp_config_sync (g_value_get_object (value),
                            G_OBJECT (vector_layer->fill_options), 0);
          
          if (vector_layer->parasite)
            {
              gimp_parasite_list_remove (GIMP_ITEM (vector_layer)->parasites,
                                         vector_layer->parasite);
              vector_layer->parasite = NULL;
            }
        }
      break;
    case PROP_STROKE_DESC:
      if (g_value_get_object (value))
        /*gimp_config_sync (g_value_get_object (value),
                          G_OBJECT (vector_layer->stroke_desc), 0);*/
        {
          if (vector_layer->stroke_desc)
            g_object_unref (vector_layer->stroke_desc);
          vector_layer->stroke_desc = gimp_config_duplicate (g_value_get_object (value));
          
          if (vector_layer->parasite)
            {
              gimp_parasite_list_remove (GIMP_ITEM (vector_layer)->parasites,
                                         vector_layer->parasite);
              vector_layer->parasite = NULL;
            }
        }    
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_vector_layer_init_stroke_fill (GimpVectorLayer *layer)
{
  GimpImage   *image        = gimp_item_get_image (GIMP_ITEM (layer));
  GimpContext *user_context = gimp_get_user_context (image->gimp);
  GimpPattern *pattern      = gimp_context_get_pattern (user_context);
  GimpRGB      black;
  GimpRGB      blue;
  
  gimp_rgba_set(&black, 0.0, 0.0, 0.0, 1.0);
  gimp_rgba_set(&blue, 0.0, 0.0, 1.0, 1.0);
  
  layer->fill_options = g_object_new (GIMP_TYPE_FILL_OPTIONS,
                                      "gimp", image->gimp,
                                      "foreground",   &blue,
                                      NULL);
  gimp_context_set_pattern (GIMP_CONTEXT (layer->fill_options), pattern);
  
  layer->stroke_desc = gimp_stroke_desc_new (image->gimp, NULL);
  g_object_set (layer->stroke_desc->stroke_options,
                "foreground",   &black,
                "width",        2.0,
                NULL); 
  gimp_context_set_pattern (GIMP_CONTEXT (layer->stroke_desc->stroke_options), pattern);
}

/* public method definitions */

/**
 * gimp_vector_layer_new:
 * @image: the #GimpImage the layer should belong to
 * @vectors: the #GimpVectors object the layer should render
 *
 * Creates a new vector layer.
 *
 * Return value: a new #GimpVectorLayer or %NULL in case of a problem
 **/
GimpVectorLayer *
gimp_vector_layer_new (GimpImage     *image,
                       GimpVectors   *vectors)
{
  GimpVectorLayer *layer;
  
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_VECTORS(vectors), NULL);
  
  layer = g_object_new (GIMP_TYPE_VECTOR_LAYER, NULL);
  
  g_object_set (G_OBJECT (layer),
                "vectors", vectors,
                NULL);
  
  gimp_drawable_configure (GIMP_DRAWABLE (layer),
                           image,
                           0, 0, 1, 1,          /* x and y offsets, x and y dimensions */
                           gimp_image_base_type_with_alpha (image),
                           NULL);
  
  return layer;
}

void
gimp_vector_layer_refresh (GimpVectorLayer  *layer)
{
  GimpImage      *image    = gimp_item_get_image (GIMP_ITEM (layer));
  gint            width    = gimp_image_get_width (image);
  gint            height   = gimp_image_get_height (image);
  
  gimp_vector_layer_refresh_name (layer);
  gimp_vector_layer_render (layer);
}

gboolean
gimp_drawable_is_vector_layer (GimpDrawable  *drawable)
{
  return GIMP_IS_VECTOR_LAYER (drawable);
}






































