/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpvectors.h
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

#ifndef __GIMP_VECTORS_H__
#define __GIMP_VECTORS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


GIMP_DEPRECATED_FOR(gimp_item_is_valid)
gboolean       gimp_vectors_is_valid        (gint32              vectors_ID);
GIMP_DEPRECATED_FOR(gimp_item_get_image)
gint32         gimp_vectors_get_image       (gint32              vectors_ID);
GIMP_DEPRECATED_FOR(gimp_item_get_name)
gchar        * gimp_vectors_get_name        (gint32              vectors_ID);
GIMP_DEPRECATED_FOR(gimp_item_set_name)
gboolean       gimp_vectors_set_name        (gint32              vectors_ID,
                                             const gchar        *name);
GIMP_DEPRECATED_FOR(gimp_item_get_visible)
gboolean       gimp_vectors_get_visible     (gint32              vectors_ID);
GIMP_DEPRECATED_FOR(gimp_item_get_visible)
gboolean       gimp_vectors_set_visible     (gint32              vectors_ID,
                                             gboolean            visible);
GIMP_DEPRECATED_FOR(gimp_item_get_linked)
gboolean       gimp_vectors_get_linked      (gint32              vectors_ID);
GIMP_DEPRECATED_FOR(gimp_item_set_linked)
gboolean       gimp_vectors_set_linked      (gint32              vectors_ID,
                                             gboolean            linked);
GIMP_DEPRECATED_FOR(gimp_item_get_tattoo)
gint           gimp_vectors_get_tattoo      (gint32              vectors_ID);
GIMP_DEPRECATED_FOR(gimp_item_set_tattoo)
gboolean       gimp_vectors_set_tattoo      (gint32              vectors_ID,
                                             gint                tattoo);
GIMP_DEPRECATED_FOR(gimp_item_get_parasite)
GimpParasite * gimp_vectors_parasite_find   (gint32              vectors_ID,
                                             const gchar        *name);
GIMP_DEPRECATED_FOR(gimp_item_attach_parasite)
gboolean       gimp_vectors_parasite_attach (gint32              vectors_ID,
                                             const GimpParasite *parasite);
GIMP_DEPRECATED_FOR(gimp_item_detach_parasite)
gboolean       gimp_vectors_parasite_detach (gint32              vectors_ID,
                                             const gchar        *name);
GIMP_DEPRECATED_FOR(gimp_item_get_parasite_list)
gboolean       gimp_vectors_parasite_list   (gint32              vectors_ID,
                                             gint               *num_parasites,
                                             gchar            ***parasites);

G_END_DECLS

#endif /* __GIMP_VECTORS_H__ */
