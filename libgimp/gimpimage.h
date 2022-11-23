/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * ligmaimage.h
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

#if !defined (__LIGMA_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligma.h> can be included directly."
#endif

#ifndef __LIGMA_IMAGE_H__
#define __LIGMA_IMAGE_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_IMAGE (ligma_image_get_type ())
G_DECLARE_FINAL_TYPE (LigmaImage, ligma_image, LIGMA, IMAGE, GObject)


gint32         ligma_image_get_id             (LigmaImage    *image);
LigmaImage    * ligma_image_get_by_id          (gint32        image_id);

gboolean       ligma_image_is_valid           (LigmaImage    *image);

GList        * ligma_list_images              (void);

GList        * ligma_image_list_layers        (LigmaImage    *image);
GList        * ligma_image_list_channels      (LigmaImage    *image);
GList        * ligma_image_list_vectors       (LigmaImage    *image);

GList    * ligma_image_list_selected_layers   (LigmaImage    *image);
gboolean   ligma_image_take_selected_layers   (LigmaImage    *image,
                                              GList        *layers);
GList    * ligma_image_list_selected_channels (LigmaImage    *image);
gboolean   ligma_image_take_selected_channels (LigmaImage    *image,
                                              GList        *channels);
GList    * ligma_image_list_selected_vectors  (LigmaImage    *image);
gboolean   ligma_image_take_selected_vectors  (LigmaImage    *image,
                                              GList        *vectors);

GList    * ligma_image_list_selected_drawables(LigmaImage    *image);

guchar       * ligma_image_get_colormap       (LigmaImage    *image,
                                              gint         *num_colors);
gboolean       ligma_image_set_colormap       (LigmaImage    *image,
                                              const guchar *colormap,
                                              gint          num_colors);

guchar       * ligma_image_get_thumbnail_data (LigmaImage    *image,
                                              gint         *width,
                                              gint         *height,
                                              gint         *bpp);
GdkPixbuf    * ligma_image_get_thumbnail      (LigmaImage    *image,
                                              gint          width,
                                              gint          height,
                                              LigmaPixbufTransparency  alpha);

LigmaMetadata * ligma_image_get_metadata       (LigmaImage    *image);
gboolean       ligma_image_set_metadata       (LigmaImage    *image,
                                              LigmaMetadata *metadata);


G_END_DECLS

#endif /* __LIGMA_IMAGE_H__ */
