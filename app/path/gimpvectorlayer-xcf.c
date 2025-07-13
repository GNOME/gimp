/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   gimpvectorlayer-xcf.c
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

#include "libgimpbase/gimpbase.h"

#include "path-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpparasitelist.h"

#include "gimpvectorlayeroptions.h"
#include "gimpvectorlayeroptions-parasite.h"
#include "gimpvectorlayer.h"
#include "gimpvectorlayer-xcf.h"

#include "gimp-intl.h"


static GimpLayer * gimp_vector_layer_from_layer (GimpLayer              *layer,
                                                 GimpVectorLayerOptions *options);


gboolean
gimp_vector_layer_xcf_load_hack (GimpLayer **layer)
{
  const gchar            *name;
  GimpVectorLayerOptions *options = NULL;
  const GimpParasite     *parasite;

  g_return_val_if_fail (layer != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (*layer), FALSE);

  name = gimp_vector_layer_options_parasite_name ();
  parasite = gimp_item_parasite_find (GIMP_ITEM (*layer), name);

  if (parasite)
    {
      GError *error = NULL;

      options = gimp_vector_layer_options_from_parasite (
        parasite, &error, gimp_item_get_image (GIMP_ITEM (*layer))->gimp);

      g_object_set (G_OBJECT (options),
                    "path", gimp_image_get_path_by_tattoo (
                            gimp_item_get_image (GIMP_ITEM (*layer)),
                            options->path_tattoo),
                    NULL);

      if (error)
        {
          g_message (_("Problems parsing the vector layer parasite for layer '%s':\n"
                       "%s\n\n"
                       "Some vector layer properties may be wrong. "),
                     gimp_object_get_name (GIMP_OBJECT (*layer)),
                     error->message);

          g_error_free (error);
        }
    }

  if (options)
    {
      *layer = gimp_vector_layer_from_layer (*layer, options);

      /*  let the text layer know what parasite was used to create it  */
      GIMP_VECTOR_LAYER (*layer)->parasite = name;
    }

  return (options != NULL);
}

void
gimp_vector_layer_xcf_save_prepare (GimpVectorLayer *layer)
{
  GimpVectorLayerOptions *options;

  g_return_if_fail (GIMP_IS_VECTOR_LAYER (layer));

  /*  If the layer has a text parasite already, it wasn't changed and we
   *  can simply save the original parasite back which is still attached.
   */
  if (layer->parasite)
    return;

  g_object_get (layer, "vector-layer-options", &options, NULL);
  if (options)
    {
      GimpParasite *parasite = gimp_vector_layer_options_to_parasite (options);

      gimp_parasite_list_add (gimp_item_get_parasites (GIMP_ITEM (layer)), parasite);
    }
}

/**
 * gimp_vector_layer_from_layer:
 * @layer: a #GimpLayer object
 * @options: a #GimpVectorLayerOptions object
 *
 * Converts a standard #GimpLayer and a #GimpVectorLayerOptions object
 * into a #GimpVectorLayer. The new vector layer takes ownership of the
 * @options and @layer objects.  The @layer object is rendered unusable
 * by this function. Don't even try to use it afterwards!
 *
 * This is a gross hack that is needed in order to load vector layers
 * from XCF files in a backwards-compatible way. Please don't use it
 * for anything else!
 *
 * Return value: a newly allocated #GimpVectorLayer object
 **/
static GimpLayer *
gimp_vector_layer_from_layer (GimpLayer              *layer,
                              GimpVectorLayerOptions *options)
{
  GimpVectorLayer *vector_layer;
  GimpDrawable    *drawable;

  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (GIMP_IS_VECTOR_LAYER_OPTIONS (options), NULL);

  vector_layer = g_object_new (GIMP_TYPE_VECTOR_LAYER,
                               "image", gimp_item_get_image (GIMP_ITEM (layer)),
                               "vector-layer-options", options,
                               NULL);

  gimp_item_replace_item (GIMP_ITEM (vector_layer), GIMP_ITEM (layer));

  drawable = GIMP_DRAWABLE (vector_layer);
  gimp_drawable_steal_buffer (drawable, GIMP_DRAWABLE (layer));

  gimp_layer_set_opacity         (GIMP_LAYER (vector_layer),
                                  gimp_layer_get_opacity (layer), FALSE);
  gimp_layer_set_mode            (GIMP_LAYER (vector_layer),
                                  gimp_layer_get_mode (layer), FALSE);
  gimp_layer_set_blend_space     (GIMP_LAYER (vector_layer),
                                  gimp_layer_get_blend_space (layer), FALSE);
  gimp_layer_set_composite_space (GIMP_LAYER (vector_layer),
                                  gimp_layer_get_composite_space (layer), FALSE);
  gimp_layer_set_composite_mode  (GIMP_LAYER (vector_layer),
                                  gimp_layer_get_composite_mode (layer), FALSE);
  gimp_layer_set_lock_alpha      (GIMP_LAYER (vector_layer),
                                  gimp_layer_get_lock_alpha (layer), FALSE);

  g_object_unref (options);
  g_object_unref (layer);

  return GIMP_LAYER (vector_layer);
}
