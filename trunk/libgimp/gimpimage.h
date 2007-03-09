/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpimage.h
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_IMAGE_H__
#define __GIMP_IMAGE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#ifndef GIMP_DISABLE_DEPRECATED
guchar   * gimp_image_get_cmap            (gint32          image_ID,
                                           gint           *num_colors);
gboolean   gimp_image_set_cmap            (gint32          image_ID,
                                           const guchar   *cmap,
                                           gint            num_colors);
#endif /* GIMP_DISABLE_DEPRECATED */

guchar   * gimp_image_get_colormap        (gint32          image_ID,
                                           gint           *num_colors);
gboolean   gimp_image_set_colormap        (gint32          image_ID,
                                           const guchar   *colormap,
                                           gint            num_colors);

guchar   * gimp_image_get_thumbnail_data  (gint32          image_ID,
                                           gint           *width,
                                           gint           *height,
                                           gint           *bpp);

gboolean   gimp_image_attach_new_parasite (gint32          image_ID,
                                           const gchar    *name,
                                           gint            flags,
                                           gint            size,
                                           gconstpointer   data);


G_END_DECLS

#endif /* __GIMP_IMAGE_H__ */
