/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_IMAGE_COLORMAP_H__
#define __GIMP_IMAGE_COLORMAP_H__


#define GIMP_IMAGE_COLORMAP_SIZE 768


void           gimp_image_colormap_init            (GimpImage     *image);
void           gimp_image_colormap_dispose         (GimpImage     *image);
void           gimp_image_colormap_free            (GimpImage     *image);

void           gimp_image_colormap_update_formats  (GimpImage     *image);

const Babl   * gimp_image_colormap_get_rgb_format  (GimpImage     *image);
const Babl   * gimp_image_colormap_get_rgba_format (GimpImage     *image);

GimpPalette  * gimp_image_get_colormap_palette     (GimpImage     *image);

const guchar * gimp_image_get_colormap             (GimpImage     *image);
gint           gimp_image_get_colormap_size        (GimpImage     *image);
void           gimp_image_set_colormap             (GimpImage     *image,
                                                    const guchar  *colormap,
                                                    gint           n_colors,
                                                    gboolean       push_undo);
void           gimp_image_unset_colormap           (GimpImage     *image,
                                                    gboolean       push_undo);

void           gimp_image_get_colormap_entry       (GimpImage     *image,
                                                    gint           color_index,
                                                    GimpRGB       *color);
void           gimp_image_set_colormap_entry       (GimpImage     *image,
                                                    gint           color_index,
                                                    const GimpRGB *color,
                                                    gboolean       push_undo);

void           gimp_image_add_colormap_entry       (GimpImage     *image,
                                                    const GimpRGB *color);


#endif /* __GIMP_IMAGE_COLORMAP_H__ */
