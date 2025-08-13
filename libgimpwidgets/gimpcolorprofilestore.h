/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpprofilestore.h
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
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

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_COLOR_PROFILE_STORE_H__
#define __GIMP_COLOR_PROFILE_STORE_H__

G_BEGIN_DECLS


#define GIMP_TYPE_COLOR_PROFILE_STORE (gimp_color_profile_store_get_type ())
G_DECLARE_FINAL_TYPE (GimpColorProfileStore, gimp_color_profile_store, GIMP, COLOR_PROFILE_STORE, GtkListStore)


GtkListStore * gimp_color_profile_store_new      (GFile                 *history);

void           gimp_color_profile_store_add_file (GimpColorProfileStore *store,
                                                  GFile                 *file,
                                                  const gchar           *label);


G_END_DECLS

#endif  /* __GIMP_COLOR_PROFILE_STORE_H__ */
