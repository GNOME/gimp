/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpprofilestore-private.h
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

#ifndef __GIMP_COLOR_PROFILE_STORE_PRIVATE_H__
#define __GIMP_COLOR_PROFILE_STORE_PRIVATE_H__


typedef enum
{
  GIMP_COLOR_PROFILE_STORE_ITEM_FILE,
  GIMP_COLOR_PROFILE_STORE_ITEM_SEPARATOR_TOP,
  GIMP_COLOR_PROFILE_STORE_ITEM_SEPARATOR_BOTTOM,
  GIMP_COLOR_PROFILE_STORE_ITEM_DIALOG
} GimpColorProfileStoreItemType;

typedef enum
{
  GIMP_COLOR_PROFILE_STORE_ITEM_TYPE,
  GIMP_COLOR_PROFILE_STORE_LABEL,
  GIMP_COLOR_PROFILE_STORE_FILE,
  GIMP_COLOR_PROFILE_STORE_INDEX
} GimpColorProfileStoreColumns;


G_GNUC_INTERNAL gboolean  _gimp_color_profile_store_history_add     (GimpColorProfileStore *store,
                                                                     GFile                 *file,
                                                                     const gchar           *label,
                                                                     GtkTreeIter           *iter);

G_GNUC_INTERNAL gboolean  _gimp_color_profile_store_history_find_profile
                                                                    (GimpColorProfileStore *store,
                                                                     GimpColorProfile      *profile,
                                                                     GtkTreeIter           *iter);

G_GNUC_INTERNAL void      _gimp_color_profile_store_history_reorder (GimpColorProfileStore *store,
                                                                     GtkTreeIter           *iter);


#endif  /* __GIMP_COLOR_PROFILE_STORE_PRIVATE_H__ */
