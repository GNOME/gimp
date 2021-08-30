/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpVectorLayer-xcf
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
 * Copyright (C) 2006  Hendrik Boom
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
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "vectors-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpparasitelist.h"

#include "gimpvectorlayeroptions.h"
#include "gimpvectorlayeroptions-parasite.h"
#include "gimpvectorlayer.h"
#include "gimpvectorlayer-xcf.h"

#include "gimp-intl.h"

/* FIXME Maybe TextLayer and VectorLayers should have a common ancestor?
 * There is a lot of duplicate code between the two - Wormnest.
 */

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
                    "vectors", gimp_image_get_vectors_by_tattoo (
                                 gimp_item_get_image (GIMP_ITEM (*layer)),
                                 options->vectors_tattoo),
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

      /*  let the vector layer know what parasite was used to create it  */
      GIMP_VECTOR_LAYER (*layer)->parasite = name;
    }

  return (options != NULL);
}

void
gimp_vector_layer_xcf_save_prepare (GimpVectorLayer *layer)
{
  GimpVectorLayerOptions *options;

  g_return_if_fail (GIMP_IS_VECTOR_LAYER (layer));

  /*  If the layer has a vector parasite already, it wasn't changed and we
   *  can simply save the original parasite back which is still attached.
   */
  if (layer->parasite)
    return;

  g_object_get (layer, "vector-layer-options", &options, NULL);
  if (options)
    {
      GimpParasite *parasite = gimp_vector_layer_options_to_parasite (options);

      //gimp_parasite_list_add (gimp_item_get_parasites (GIMP_ITEM (layer)), parasite);
      // Wormnest: changed to the way TextLayer does it, not sure of implications yet!
      /*  Don't push an undo because the parasite only exists temporarily
       *  while the text layer is saved to XCF.
       */
      gimp_item_parasite_attach (GIMP_ITEM (layer), parasite, FALSE);

      gimp_parasite_free (parasite);
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
 * by this function. Don't even try to use if afterwards!
 *
 * This is a gross hack that is needed in order to load vector layers
 * from XCF files in a backwards-compatible way. Please don't use it
 * for anything else!
 *
 * Returns: a newly allocated #GimpVectorLayer object
 **/
static GimpLayer *
gimp_vector_layer_from_layer (GimpLayer              *layer,
                              GimpVectorLayerOptions *options)
{
  GimpVectorLayer *vector_layer;
//  GimpItem        *item;
  GimpDrawable    *drawable;
//  gint             offset_x, offset_y;

  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (GIMP_IS_VECTOR_LAYER_OPTIONS (options), NULL);

//  vector_layer = g_object_new (GIMP_TYPE_VECTOR_LAYER,
//                               "vector-layer-options", options,
//                               NULL);
// Wormnest: text layer uses the below form instead, lets follow that for now:
  vector_layer = g_object_new (GIMP_TYPE_VECTOR_LAYER,
                               "image", gimp_item_get_image (GIMP_ITEM (layer)),
                               NULL);

  gimp_item_replace_item (GIMP_ITEM (vector_layer), GIMP_ITEM (layer));

//  item     = GIMP_ITEM (vector_layer);
  drawable = GIMP_DRAWABLE (vector_layer);
  gimp_drawable_steal_buffer (drawable, GIMP_DRAWABLE (layer));

/* Wormnest: replacing this with what text layer does...
  gimp_object_set_name (GIMP_OBJECT (vector_layer),
                        gimp_object_get_name (GIMP_OBJECT (layer)));

  //item->ID        = gimp_item_get_ID (GIMP_ITEM (layer));
  //item->tattoo    = gimp_item_get_tattoo (GIMP_ITEM (layer));
  gimp_item_set_tattoo (GIMP_ITEM (item),
                        gimp_item_get_tattoo (GIMP_ITEM (layer)));
  //  item->image     = gimp_item_get_image (GIMP_ITEM (layer));
  gimp_item_set_image (GIMP_ITEM (item),
                       gimp_item_get_image (GIMP_ITEM (layer)));

  gimp_item_set_image (GIMP_ITEM (layer), NULL);

  // Wormnest: is this still necessary or does GimpItem handle this internally?
  g_hash_table_replace (item->image->gimp->item_table,
                        GINT_TO_POINTER (item->ID),
                        item);

  //item->parasites = GIMP_ITEM (layer)->parasites;
  //GIMP_ITEM (layer)->parasites = NULL;
  gimp_item_set_parasites (GIMP_ITEM (item),
                           gimp_item_get_parasites (GIMP_ITEM (layer)));
  gimp_item_set_parasites (GIMP_ITEM (layer), NULL);

  //item->width     = gimp_item_get_width (GIMP_ITEM (layer));
  //item->height    = gimp_item_get_height (GIMP_ITEM (layer));
  gimp_item_set_size (GIMP_ITEM (item),
                      gimp_item_get_width (GIMP_ITEM (layer)),
                      gimp_item_get_height (GIMP_ITEM (layer)));

  gimp_item_get_offset (GIMP_ITEM (layer), &offset_x, &offset_y);
  gimp_item_set_offset (GIMP_ITEM (item), offset_x, offset_y);

  //item->visible   = gimp_item_get_visible (GIMP_ITEM (layer));
  //item->linked    = gimp_item_get_linked (GIMP_ITEM (layer));
  gimp_item_set_visible (GIMP_ITEM (item),
                         gimp_item_get_visible (GIMP_ITEM (layer)),
                         FALSE);
  gimp_item_set_linked (GIMP_ITEM (item),
                        gimp_item_get_linked (GIMP_ITEM (layer)),
                         FALSE);

  gimp_drawable_set_tiles (drawable,
                           FALSE,
                           NULL,
                           gimp_drawable_get_tiles (GIMP_DRAWABLE (layer)),
                           gimp_drawable_type (GIMP_DRAWABLE (layer)));
  gimp_drawable_set_tiles (GIMP_DRAWABLE (layer),
                           FALSE,
                           NULL,
                           NULL,
                           gimp_drawable_type (GIMP_DRAWABLE (layer)));

  drawable->bytes     = gimp_drawable_bytes (GIMP_DRAWABLE (layer));
  drawable->type      = gimp_drawable_type (GIMP_DRAWABLE (layer));
  drawable->has_alpha = gimp_drawable_has_alpha (GIMP_DRAWABLE (layer));

  GIMP_LAYER (vector_layer)->opacity    = gimp_layer_get_opacity (layer);
  GIMP_LAYER (vector_layer)->mode       = gimp_layer_get_mode (layer);
  GIMP_LAYER (vector_layer)->lock_alpha = gimp_layer_get_lock_alpha (layer);
*/

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

  /* Wormnest: FIXME below add version for vector layer
   * Needs get/set functions in gimpvectorlayer like for gimptextlayer */
  //gimp_text_layer_set_text (text_layer, text);

  g_object_unref (options);
  g_object_unref (layer);

  return GIMP_LAYER (vector_layer);
}
