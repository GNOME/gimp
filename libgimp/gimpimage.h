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


guchar       * gimp_image_get_colormap            (gint32              image_ID,
                                                   gint               *num_colors);
gboolean       gimp_image_set_colormap            (gint32              image_ID,
                                                   const guchar       *colormap,
                                                   gint                num_colors);

guchar       * gimp_image_get_thumbnail_data      (gint32              image_ID,
                                                   gint               *width,
                                                   gint               *height,
                                                   gint               *bpp);

GimpMetadata * gimp_image_get_metadata            (gint32              image_ID);
gboolean       gimp_image_set_metadata            (gint32              image_ID,
                                                   GimpMetadata       *metadata);

GIMP_DEPRECATED_FOR(gimp_image_get_colormap)
guchar       * gimp_image_get_cmap                (gint32              image_ID,
                                                   gint               *num_colors);
GIMP_DEPRECATED_FOR(gimp_image_set_colormap)
gboolean       gimp_image_set_cmap                (gint32              image_ID,
                                                   const guchar       *cmap,
                                                   gint                num_colors);
GIMP_DEPRECATED_FOR(gimp_image_get_item_position)
gint           gimp_image_get_layer_position      (gint32              image_ID,
                                                   gint32              layer_ID);
GIMP_DEPRECATED_FOR(gimp_image_raise_item)
gboolean       gimp_image_raise_layer             (gint32              image_ID,
                                                   gint32              layer_ID);
GIMP_DEPRECATED_FOR(gimp_image_lower_item)
gboolean       gimp_image_lower_layer             (gint32              image_ID,
                                                   gint32              layer_ID);
GIMP_DEPRECATED_FOR(gimp_image_raise_item_to_top)
gboolean       gimp_image_raise_layer_to_top      (gint32              image_ID,
                                                   gint32              layer_ID);
GIMP_DEPRECATED_FOR(gimp_image_lower_item_to_bottom)
gboolean       gimp_image_lower_layer_to_bottom   (gint32              image_ID,
                                                   gint32              layer_ID);
GIMP_DEPRECATED_FOR(gimp_image_get_item_position)
gint           gimp_image_get_channel_position    (gint32              image_ID,
                                                   gint32              channel_ID);
GIMP_DEPRECATED_FOR(gimp_image_raise_item)
gboolean       gimp_image_raise_channel           (gint32              image_ID,
                                                   gint32              channel_ID);
GIMP_DEPRECATED_FOR(gimp_image_lower_item)
gboolean       gimp_image_lower_channel           (gint32              image_ID,
                                                   gint32              channel_ID);
GIMP_DEPRECATED_FOR(gimp_image_get_item_position)
gint           gimp_image_get_vectors_position    (gint32              image_ID,
                                                   gint32              vectors_ID);
GIMP_DEPRECATED_FOR(gimp_image_raise_item)
gboolean       gimp_image_raise_vectors           (gint32              image_ID,
                                                   gint32              vectors_ID);
GIMP_DEPRECATED_FOR(gimp_image_lower_item)
gboolean       gimp_image_lower_vectors           (gint32              image_ID,
                                                   gint32              vectors_ID);
GIMP_DEPRECATED_FOR(gimp_image_raise_item_to_top)
gboolean       gimp_image_raise_vectors_to_top    (gint32              image_ID,
                                                   gint32              vectors_ID);
GIMP_DEPRECATED_FOR(gimp_image_lower_item_to_bottom)
gboolean       gimp_image_lower_vectors_to_bottom (gint32              image_ID,
                                                   gint32              vectors_ID);
GIMP_DEPRECATED_FOR(gimp_image_get_parasite)
GimpParasite * gimp_image_parasite_find           (gint32              image_ID,
                                                   const gchar        *name);
GIMP_DEPRECATED_FOR(gimp_image_attach_parasite)
gboolean       gimp_image_parasite_attach         (gint32              image_ID,
                                                   const GimpParasite *parasite);
GIMP_DEPRECATED_FOR(gimp_image_detach_parasite)
gboolean       gimp_image_parasite_detach         (gint32              image_ID,
                                                   const gchar        *name);
GIMP_DEPRECATED_FOR(gimp_image_get_parasite_list)
gboolean       gimp_image_parasite_list           (gint32              image_ID,
                                                   gint               *num_parasites,
                                                   gchar            ***parasites);
GIMP_DEPRECATED_FOR(gimp_image_attach_parasite)
gboolean       gimp_image_attach_new_parasite     (gint32              image_ID,
                                                   const gchar        *name,
                                                   gint                flags,
                                                   gint                size,
                                                   gconstpointer       data);

G_END_DECLS

#endif /* __GIMP_IMAGE_H__ */
