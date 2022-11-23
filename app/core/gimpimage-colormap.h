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

#ifndef __LIGMA_IMAGE_COLORMAP_H__
#define __LIGMA_IMAGE_COLORMAP_H__


#define LIGMA_IMAGE_COLORMAP_SIZE 768


void           ligma_image_colormap_init            (LigmaImage     *image);
void           ligma_image_colormap_dispose         (LigmaImage     *image);
void           ligma_image_colormap_free            (LigmaImage     *image);

void           ligma_image_colormap_update_formats  (LigmaImage     *image);

const Babl   * ligma_image_colormap_get_rgb_format  (LigmaImage     *image);
const Babl   * ligma_image_colormap_get_rgba_format (LigmaImage     *image);

LigmaPalette  * ligma_image_get_colormap_palette     (LigmaImage     *image);
void           ligma_image_set_colormap_palette     (LigmaImage     *image,
                                                    LigmaPalette   *palette,
                                                    gboolean       push_undo);

guchar *       ligma_image_get_colormap             (LigmaImage     *image);
gint           ligma_image_get_colormap_size        (LigmaImage     *image);
void           ligma_image_set_colormap             (LigmaImage     *image,
                                                    const guchar  *colormap,
                                                    gint           n_colors,
                                                    gboolean       push_undo);
void           ligma_image_unset_colormap           (LigmaImage     *image,
                                                    gboolean       push_undo);

void           ligma_image_get_colormap_entry       (LigmaImage     *image,
                                                    gint           color_index,
                                                    LigmaRGB       *color);
void           ligma_image_set_colormap_entry       (LigmaImage     *image,
                                                    gint           color_index,
                                                    const LigmaRGB *color,
                                                    gboolean       push_undo);

void           ligma_image_add_colormap_entry       (LigmaImage     *image,
                                                    const LigmaRGB *color);


#endif /* __LIGMA_IMAGE_COLORMAP_H__ */
