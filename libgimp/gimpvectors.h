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
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_VECTORS_H__
#define __GIMP_VECTORS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#ifndef GIMP_DISABLE_DEPRECATED
gboolean       gimp_vectors_is_valid        (gint32              vectors_ID);
gint32         gimp_vectors_get_image       (gint32              vectors_ID);
gchar        * gimp_vectors_get_name        (gint32              vectors_ID);
gboolean       gimp_vectors_set_name        (gint32              vectors_ID,
                                             const gchar        *name);
gboolean       gimp_vectors_get_visible     (gint32              vectors_ID);
gboolean       gimp_vectors_set_visible     (gint32              vectors_ID,
                                             gboolean            visible);
gboolean       gimp_vectors_get_linked      (gint32              vectors_ID);
gboolean       gimp_vectors_set_linked      (gint32              vectors_ID,
                                             gboolean            linked);
gint           gimp_vectors_get_tattoo      (gint32              vectors_ID);
gboolean       gimp_vectors_set_tattoo      (gint32              vectors_ID,
                                             gint                tattoo);
GimpParasite * gimp_vectors_parasite_find   (gint32              vectors_ID,
                                             const gchar        *name);
gboolean       gimp_vectors_parasite_attach (gint32              vectors_ID,
                                             const GimpParasite *parasite);
gboolean       gimp_vectors_parasite_detach (gint32              vectors_ID,
                                             const gchar        *name);
gboolean       gimp_vectors_parasite_list   (gint32              vectors_ID,
                                             gint               *num_parasites,
                                             gchar            ***parasites);
#endif /* GIMP_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* __GIMP_VECTORS_H__ */
