/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimplayer.h
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

#ifndef __GIMP_LAYER_H__
#define __GIMP_LAYER_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


gint32   gimp_layer_new                (gint32           image_ID,
                                        const gchar     *name,
                                        gint             width,
                                        gint             height,
                                        GimpImageType    type,
                                        gdouble          opacity,
                                        GimpLayerMode    mode);
gint32   gimp_layer_copy               (gint32           layer_ID);

gint32   gimp_layer_new_from_pixbuf    (gint32           image_ID,
                                        const gchar     *name,
                                        GdkPixbuf       *pixbuf,
                                        gdouble          opacity,
                                        GimpLayerMode    mode,
                                        gdouble          progress_start,
                                        gdouble          progress_end);
gint32   gimp_layer_new_from_surface   (gint32           image_ID,
                                        const gchar     *name,
                                        cairo_surface_t *surface,
                                        gdouble          progress_start,
                                        gdouble          progress_end);

GIMP_DEPRECATED_FOR(gimp_layer_get_lock_alpha)
gboolean gimp_layer_get_preserve_trans (gint32           layer_ID);
GIMP_DEPRECATED_FOR(gimp_layer_set_lock_alpha)
gboolean gimp_layer_set_preserve_trans (gint32           layer_ID,
                                        gboolean         preserve_trans);

G_END_DECLS

#endif /* __GIMP_LAYER_H__ */
