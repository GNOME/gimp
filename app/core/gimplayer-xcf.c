/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   gimplayer-xcf.c
 *
 *   Copyright 2003 Sven Neumann <sven@gimp.org>
 *   Copyright 2025 Jehan
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

#include "core-types.h"

#include "gimpimage.h"
#include "gimplayer.h"
#include "gimplayer-xcf.h"

/**
 * gimp_layer_from_layer:
 * @layer: a #GimpLayer object
 * @options: a #GimpVectorLayerOptions object
 *
 * Converts a standard #GimpLayer into a more specific type of
 * #GimpLayer. The new layer takes ownership of properties of @layer.
 * The @layer object is rendered unusable by this function. Don't even
 * try to use it afterwards!
 *
 * This is a gross hack that is needed in order to load text or vector
 * layers from XCF files in a backwards-compatible way, or as a
 * secondary step (after more data has been loaded). Please don't use it
 * for anything else!
 *
 * The variable list of arguments will be the properties which will be
 * used to create the new layer of type @new_layer_type. Note that the
 * "image" property needs to be set at the minimum.
 *
 * Return value: a newly allocated object of a subtype of #GimpLayer.
 **/
GimpLayer *
gimp_layer_from_layer (GimpLayer *layer,
                       GType      new_layer_type,
                       ...)
{
  GimpLayer    *new_layer;
  GimpDrawable *drawable;
  GimpImage    *image;
  gboolean      attached;
  GimpLayer    *parent   = NULL;
  gint          position = 0;
  va_list       args;
  const gchar  *first_prop;

  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (g_type_is_a (new_layer_type, GIMP_TYPE_LAYER), NULL);

  image = gimp_item_get_image (GIMP_ITEM (layer));

  if ((attached = gimp_item_is_attached (GIMP_ITEM (layer))))
    {
      parent   = gimp_layer_get_parent (layer);
      position = gimp_item_get_index (GIMP_ITEM (layer));

      g_object_ref (layer);
      gimp_image_remove_layer (image, layer, FALSE, NULL);
    }

  va_start (args, new_layer_type);
  first_prop = va_arg (args, gchar *);
  new_layer = GIMP_LAYER (g_object_new_valist (new_layer_type, first_prop, args));
  va_end (args);

  gimp_item_replace_item (GIMP_ITEM (new_layer), GIMP_ITEM (layer));

  drawable = GIMP_DRAWABLE (new_layer);
  gimp_drawable_steal_buffer (drawable, GIMP_DRAWABLE (layer));

  gimp_layer_set_opacity         (GIMP_LAYER (new_layer),
                                  gimp_layer_get_opacity (layer), FALSE);
  gimp_layer_set_mode            (GIMP_LAYER (new_layer),
                                  gimp_layer_get_mode (layer), FALSE);
  gimp_layer_set_blend_space     (GIMP_LAYER (new_layer),
                                  gimp_layer_get_blend_space (layer), FALSE);
  gimp_layer_set_composite_space (GIMP_LAYER (new_layer),
                                  gimp_layer_get_composite_space (layer), FALSE);
  gimp_layer_set_composite_mode  (GIMP_LAYER (new_layer),
                                  gimp_layer_get_composite_mode (layer), FALSE);
  gimp_layer_set_lock_alpha      (GIMP_LAYER (new_layer),
                                  gimp_layer_get_lock_alpha (layer), FALSE);

  g_object_unref (layer);

  if (attached)
    gimp_image_add_layer (image, new_layer, parent, position, FALSE);

  return new_layer;
}
