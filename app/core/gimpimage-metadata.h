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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_IMAGE_METADATA_H__
#define __GIMP_IMAGE_METADATA_H__


GimpMetadata * gimp_image_get_metadata                    (GimpImage    *image);
void           gimp_image_set_metadata                    (GimpImage    *image,
                                                           GimpMetadata *metadata,
                                                           gboolean      push_undo);

void           gimp_image_metadata_update_pixel_size      (GimpImage    *image);
void           gimp_image_metadata_update_bits_per_sample (GimpImage    *image);
void           gimp_image_metadata_update_resolution      (GimpImage    *image);
void           gimp_image_metadata_update_colorspace      (GimpImage    *image);

GimpImage    * gimp_image_metadata_load_thumbnail         (Gimp         *gimp,
                                                           GFile        *file,
                                                           gint         *full_image_width,
                                                           gint         *full_image_height,
                                                           const Babl  **format,
                                                           GError      **error);


#endif /* __GIMP_IMAGE_METADATA_H__ */
