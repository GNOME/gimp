/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpimage.h"
#include "gimpimage-metadata.h"
#include "gimpimage-private.h"
#include "gimpimage-undo-push.h"


/* public functions */


GimpMetadata *
gimp_image_get_metadata (GimpImage *image)
{
  GimpAttributes *attributes = NULL;
  GimpMetadata   *metadata   = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  
  attributes = gimp_image_get_attributes (image);
  if (attributes)
  {
    metadata = gimp_metadata_new ();
    gimp_attributes_to_metadata (attributes, 
                                 metadata, 
                                 "image/any");
  }
  return metadata;
}

void
gimp_image_set_metadata (GimpImage    *image,
                         GimpMetadata *metadata,
                         gboolean      push_undo)
{
  GimpAttributes *attributes = NULL;

  attributes = gimp_attributes_from_metadata (NULL, metadata);
  
  gimp_image_set_attributes (image, 
                             attributes, 
                             push_undo);
  g_object_unref (attributes);
}

void
gimp_image_set_attributes (GimpImage      *image,
                           GimpAttributes *attributes,
                           gboolean        push_undo)
{
  GimpViewable *viewable;
  gdouble xres, yres;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  if (! attributes)
    return;

  if (push_undo)
    gimp_image_undo_push_image_attributes (image, NULL);

  viewable = GIMP_VIEWABLE (image);

  if (viewable)
    {

      gimp_attributes_set_pixel_size (attributes,
                                      gimp_image_get_width  (image),
                                      gimp_image_get_height (image));

      switch (gimp_image_get_component_type (image))
      {
        case GIMP_COMPONENT_TYPE_U8:
          gimp_attributes_set_bits_per_sample (attributes, 8);
          break;

        case GIMP_COMPONENT_TYPE_U16:
        case GIMP_COMPONENT_TYPE_HALF:
          gimp_attributes_set_bits_per_sample (attributes, 16);
          break;

        case GIMP_COMPONENT_TYPE_U32:
        case GIMP_COMPONENT_TYPE_FLOAT:
          gimp_attributes_set_bits_per_sample (attributes, 32);
          break;

        case GIMP_COMPONENT_TYPE_DOUBLE:
          gimp_attributes_set_bits_per_sample (attributes, 64);
          break;
      }

      gimp_image_get_resolution (image, &xres, &yres);
      gimp_attributes_set_resolution (attributes, xres, yres,
                                      gimp_image_get_unit (image));

      gimp_viewable_set_attributes (viewable, attributes);
    }
}

GimpAttributes *
gimp_image_get_attributes (GimpImage *image)
{
  GimpViewable   *viewable;
  GimpAttributes *attributes;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  viewable = GIMP_VIEWABLE (image);

  if (viewable)
    {
      attributes = gimp_viewable_get_attributes (viewable);
      return attributes;
    }

  return NULL;
}
