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

#ifndef __LIGMA_IMAGE_METADATA_H__
#define __LIGMA_IMAGE_METADATA_H__


LigmaMetadata * ligma_image_get_metadata                    (LigmaImage    *image);
void           ligma_image_set_metadata                    (LigmaImage    *image,
                                                           LigmaMetadata *metadata,
                                                           gboolean      push_undo);

void           ligma_image_metadata_update_pixel_size      (LigmaImage    *image);
void           ligma_image_metadata_update_bits_per_sample (LigmaImage    *image);
void           ligma_image_metadata_update_resolution      (LigmaImage    *image);
void           ligma_image_metadata_update_colorspace      (LigmaImage    *image);


#endif /* __LIGMA_IMAGE_METADATA_H__ */
