/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpimage.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_IMAGE_H__
#define __GIMP_IMAGE_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define GIMP_TYPE_IMAGE (gimp_image_get_type ())
G_DECLARE_FINAL_TYPE (GimpImage, gimp_image, GIMP, IMAGE, GObject)


gint32         gimp_image_get_id                  (GimpImage    *image);
GimpImage    * gimp_image_get_by_id               (gint32        image_id);

gboolean       gimp_image_is_valid                (GimpImage    *image);

GList        * gimp_list_images                   (void);

GList        * gimp_image_list_layers             (GimpImage    *image);
GList        * gimp_image_list_channels           (GimpImage    *image);
GList        * gimp_image_list_paths              (GimpImage    *image);

GList        * gimp_image_list_selected_layers    (GimpImage    *image);
gboolean       gimp_image_take_selected_layers    (GimpImage    *image,
                                                   GList        *layers);
GList        * gimp_image_list_selected_channels  (GimpImage    *image);
gboolean       gimp_image_take_selected_channels  (GimpImage    *image,
                                                   GList        *channels);
GList        * gimp_image_list_selected_paths     (GimpImage    *image);
gboolean       gimp_image_take_selected_paths     (GimpImage    *image,
                                                   GList        *paths);

GList        * gimp_image_list_selected_drawables (GimpImage    *image);

guchar       * gimp_image_get_colormap            (GimpImage    *image,
                                                   gint         *colormap_len,
                                                   gint         *num_colors);
gboolean       gimp_image_set_colormap            (GimpImage    *image,
                                                   const guchar *colormap,
                                                   gint          num_colors);

GBytes       * gimp_image_get_thumbnail_data      (GimpImage    *image,
                                                   gint         *width,
                                                   gint         *height,
                                                   gint         *bpp);
GdkPixbuf    * gimp_image_get_thumbnail           (GimpImage    *image,
                                                   gint          width,
                                                   gint          height,
                                                   GimpPixbufTransparency  alpha);

GimpMetadata * gimp_image_get_metadata            (GimpImage    *image);
gboolean       gimp_image_set_metadata            (GimpImage    *image,
                                                   GimpMetadata *metadata);


G_END_DECLS

#endif /* __GIMP_IMAGE_H__ */
