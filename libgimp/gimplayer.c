/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimplayer.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "gimp.h"
#undef GIMP_DISABLE_DEPRECATED
#undef __GIMP_LAYER_H__
#include "gimplayer.h"

/**
 * gimp_layer_new:
 * @image_ID: The image to which to add the layer.
 * @name: The layer name.
 * @width: The layer width.
 * @height: The layer height.
 * @type: The layer type.
 * @opacity: The layer opacity.
 * @mode: The layer combination mode.
 *
 * Create a new layer.
 *
 * This procedure creates a new layer with the specified width, height,
 * and type. Name, opacity, and mode are also supplied parameters. The
 * new layer still needs to be added to the image, as this is not
 * automatic. Add the new layer with the 'gimp_image_add_layer'
 * command. Other attributes such as layer mask modes, and offsets
 * should be set with explicit procedure calls.
 *
 * Returns: The newly created layer.
 */
gint32
gimp_layer_new (gint32                image_ID,
                const gchar          *name,
                gint                  width,
                gint                  height,
                GimpImageType         type,
                gdouble               opacity,
                GimpLayerModeEffects  mode)
{
  return _gimp_layer_new (image_ID,
                          width,
                          height,
                          type,
                          name,
                          opacity,
                          mode);
}

/**
 * gimp_layer_copy:
 * @layer_ID: The layer to copy.
 *
 * Copy a layer.
 *
 * This procedure copies the specified layer and returns the copy. The
 * newly copied layer is for use within the original layer's image. It
 * should not be subsequently added to any other image.
 *
 * Returns: The newly copied layer.
 */
gint32
gimp_layer_copy (gint32  layer_ID)
{
  return _gimp_layer_copy (layer_ID, FALSE);
}

/**
 * gimp_layer_get_preserve_trans:
 * @layer_ID: The layer.
 *
 * This procedure is deprecated! Use gimp_layer_get_lock_alpha() instead.
 *
 * Returns: The layer's preserve transperancy setting.
 */
gboolean
gimp_layer_get_preserve_trans (gint32 layer_ID)
{
  return gimp_layer_get_lock_alpha (layer_ID);
}

/**
 * gimp_layer_set_preserve_trans:
 * @layer_ID: The layer.
 * @preserve_trans: The new layer's preserve transperancy setting.
 *
 * This procedure is deprecated! Use gimp_layer_set_lock_alpha() instead.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_layer_set_preserve_trans (gint32   layer_ID,
                               gboolean preserve_trans)
{
  return gimp_layer_set_lock_alpha (layer_ID, preserve_trans);
}
