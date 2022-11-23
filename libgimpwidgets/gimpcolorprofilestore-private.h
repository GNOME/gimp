/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaprofilestore-private.h
 * Copyright (C) 2007  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_COLOR_PROFILE_STORE_PRIVATE_H__
#define __LIGMA_COLOR_PROFILE_STORE_PRIVATE_H__


typedef enum
{
  LIGMA_COLOR_PROFILE_STORE_ITEM_FILE,
  LIGMA_COLOR_PROFILE_STORE_ITEM_SEPARATOR_TOP,
  LIGMA_COLOR_PROFILE_STORE_ITEM_SEPARATOR_BOTTOM,
  LIGMA_COLOR_PROFILE_STORE_ITEM_DIALOG
} LigmaColorProfileStoreItemType;

typedef enum
{
  LIGMA_COLOR_PROFILE_STORE_ITEM_TYPE,
  LIGMA_COLOR_PROFILE_STORE_LABEL,
  LIGMA_COLOR_PROFILE_STORE_FILE,
  LIGMA_COLOR_PROFILE_STORE_INDEX
} LigmaColorProfileStoreColumns;


G_GNUC_INTERNAL gboolean  _ligma_color_profile_store_history_add     (LigmaColorProfileStore *store,
                                                                     GFile                 *file,
                                                                     const gchar           *label,
                                                                     GtkTreeIter           *iter);

G_GNUC_INTERNAL gboolean  _ligma_color_profile_store_history_find_profile
                                                                    (LigmaColorProfileStore *store,
                                                                     LigmaColorProfile      *profile,
                                                                     GtkTreeIter           *iter);

G_GNUC_INTERNAL void      _ligma_color_profile_store_history_reorder (LigmaColorProfileStore *store,
                                                                     GtkTreeIter           *iter);


#endif  /* __LIGMA_COLOR_PROFILE_STORE_PRIVATE_H__ */
