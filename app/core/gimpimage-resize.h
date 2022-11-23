/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattisbvf
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

#ifndef __LIGMA_IMAGE_RESIZE_H__
#define __LIGMA_IMAGE_RESIZE_H__


void   ligma_image_resize              (LigmaImage    *image,
                                       LigmaContext  *context,
                                       gint          new_width,
                                       gint          new_height,
                                       gint          offset_x,
                                       gint          offset_y,
                                       LigmaProgress *progress);

void   ligma_image_resize_with_layers  (LigmaImage    *image,
                                       LigmaContext  *context,
                                       LigmaFillType  fill_type,
                                       gint          new_width,
                                       gint          new_height,
                                       gint          offset_x,
                                       gint          offset_y,
                                       LigmaItemSet   layer_set,
                                       gboolean      resize_text_layers,
                                       LigmaProgress *progress);

void   ligma_image_resize_to_layers    (LigmaImage    *image,
                                       LigmaContext  *context,
                                       gint         *offset_x,
                                       gint         *offset_y,
                                       gint         *new_width,
                                       gint         *new_height,
                                       LigmaProgress *progress);
void   ligma_image_resize_to_selection (LigmaImage    *image,
                                       LigmaContext  *context,
                                       LigmaProgress *progress);


#endif /* __LIGMA_IMAGE_RESIZE_H__ */
