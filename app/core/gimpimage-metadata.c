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


//void
//gimp_image_set_metadata (GimpImage    *image,
//                         GimpMetadata *metadata,
//                         gboolean      push_undo)
//{
//  GimpImagePrivate *private;
//
//  g_return_if_fail (GIMP_IS_IMAGE (image));
//
//  private = GIMP_IMAGE_GET_PRIVATE (image);
//
//  if (metadata != private->metadata)
//    {
//      if (push_undo)
//        gimp_image_undo_push_image_metadata (image, NULL);
//
//      if (private->metadata)
//        g_object_unref (private->metadata);
//
//      private->metadata = metadata;
//
//      if (private->metadata)
//        {
//          gdouble xres, yres;
//
//          g_object_ref (private->metadata);
//
//          gimp_metadata_set_pixel_size (metadata,
//                                        gimp_image_get_width  (image),
//                                        gimp_image_get_height (image));
//
//          switch (gimp_image_get_component_type (image))
//            {
//            case GIMP_COMPONENT_TYPE_U8:
//              gimp_metadata_set_bits_per_sample (metadata, 8);
//              break;
//
//            case GIMP_COMPONENT_TYPE_U16:
//            case GIMP_COMPONENT_TYPE_HALF:
//              gimp_metadata_set_bits_per_sample (metadata, 16);
//              break;
//
//            case GIMP_COMPONENT_TYPE_U32:
//            case GIMP_COMPONENT_TYPE_FLOAT:
//              gimp_metadata_set_bits_per_sample (metadata, 32);
//              break;
//
//            case GIMP_COMPONENT_TYPE_DOUBLE:
//              gimp_metadata_set_bits_per_sample (metadata, 64);
//              break;
//            }
//
//          gimp_image_get_resolution (image, &xres, &yres);
//          gimp_metadata_set_resolution (metadata, xres, yres,
//                                        gimp_image_get_unit (image));
//        }
//
//      g_object_notify (G_OBJECT (image), "metadata");
//    }
//}

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
