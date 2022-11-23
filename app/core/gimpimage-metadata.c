/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"

#include "core-types.h"

#include "ligmaimage.h"
#include "ligmaimage-color-profile.h"
#include "ligmaimage-metadata.h"
#include "ligmaimage-private.h"
#include "ligmaimage-undo-push.h"


/* public functions */


LigmaMetadata *
ligma_image_get_metadata (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  return private->metadata;
}

void
ligma_image_set_metadata (LigmaImage    *image,
                         LigmaMetadata *metadata,
                         gboolean      push_undo)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (metadata != private->metadata)
    {
      if (push_undo)
        ligma_image_undo_push_image_metadata (image, NULL);

      g_set_object (&private->metadata, metadata);

      if (private->metadata)
        {
          ligma_image_metadata_update_pixel_size      (image);
          ligma_image_metadata_update_bits_per_sample (image);
          ligma_image_metadata_update_resolution      (image);
          ligma_image_metadata_update_colorspace      (image);
        }

      g_object_notify (G_OBJECT (image), "metadata");
    }
}

void
ligma_image_metadata_update_pixel_size (LigmaImage *image)
{
  LigmaMetadata *metadata;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  metadata = ligma_image_get_metadata (image);

  if (metadata)
    {
      ligma_metadata_set_pixel_size (metadata,
                                    ligma_image_get_width  (image),
                                    ligma_image_get_height (image));
    }
}

void
ligma_image_metadata_update_bits_per_sample (LigmaImage *image)
{
  LigmaMetadata *metadata;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  metadata = ligma_image_get_metadata (image);

  if (metadata)
    {
      switch (ligma_image_get_component_type (image))
        {
        case LIGMA_COMPONENT_TYPE_U8:
          ligma_metadata_set_bits_per_sample (metadata, 8);
          break;

        case LIGMA_COMPONENT_TYPE_U16:
        case LIGMA_COMPONENT_TYPE_HALF:
          ligma_metadata_set_bits_per_sample (metadata, 16);
          break;

        case LIGMA_COMPONENT_TYPE_U32:
        case LIGMA_COMPONENT_TYPE_FLOAT:
          ligma_metadata_set_bits_per_sample (metadata, 32);
          break;

        case LIGMA_COMPONENT_TYPE_DOUBLE:
          ligma_metadata_set_bits_per_sample (metadata, 64);
          break;
        }
    }
}

void
ligma_image_metadata_update_resolution (LigmaImage *image)
{
  LigmaMetadata *metadata;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  metadata = ligma_image_get_metadata (image);

  if (metadata)
    {
      gdouble xres, yres;

      ligma_image_get_resolution (image, &xres, &yres);
      ligma_metadata_set_resolution (metadata, xres, yres,
                                    ligma_image_get_unit (image));
    }
}

void
ligma_image_metadata_update_colorspace (LigmaImage *image)
{
  LigmaMetadata *metadata;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  metadata = ligma_image_get_metadata (image);

  if (metadata)
    {
      /*  See the discussions in issue #3532 and issue #301  */

      LigmaColorProfile       *profile = ligma_image_get_color_profile (image);
      LigmaMetadataColorspace  space   = LIGMA_METADATA_COLORSPACE_UNSPECIFIED;

      if (profile)
        {
          static LigmaColorProfile *adobe = NULL;

          if (! adobe)
            adobe = ligma_color_profile_new_rgb_adobe ();

          if (ligma_color_profile_is_equal (profile, adobe))
            space = LIGMA_METADATA_COLORSPACE_ADOBERGB;
        }
      else
        {
          space = LIGMA_METADATA_COLORSPACE_SRGB;
        }

      ligma_metadata_set_colorspace (metadata, space);
    }
}
