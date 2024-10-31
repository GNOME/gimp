/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpintstore.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_INT_STORE_H__
#define __GIMP_INT_STORE_H__

G_BEGIN_DECLS


/**
 * GimpIntStoreColumns:
 * @GIMP_INT_STORE_VALUE:       the integer value
 * @GIMP_INT_STORE_LABEL:       a human-readable label
 * @GIMP_INT_STORE_ABBREV:      an abbreviated label
 * @GIMP_INT_STORE_ICON_NAME:   an icon name
 * @GIMP_INT_STORE_PIXBUF:      a #GdkPixbuf
 * @GIMP_INT_STORE_USER_DATA:   arbitrary user data
 * @GIMP_INT_STORE_NUM_COLUMNS: the number of columns
 *
 * The column types of #GimpIntStore.
 **/
typedef enum
{
  GIMP_INT_STORE_VALUE,
  GIMP_INT_STORE_LABEL,
  GIMP_INT_STORE_ABBREV,
  GIMP_INT_STORE_ICON_NAME,
  GIMP_INT_STORE_PIXBUF,
  GIMP_INT_STORE_USER_DATA,
  GIMP_INT_STORE_NUM_COLUMNS
} GimpIntStoreColumns;


#define GIMP_TYPE_INT_STORE (gimp_int_store_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpIntStore, gimp_int_store, GIMP, INT_STORE, GtkListStore)

struct _GimpIntStoreClass
{
  GtkListStoreClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved0) (void);
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
  void (* _gimp_reserved9) (void);
};


GtkListStore * gimp_int_store_new             (const gchar   *first_label,
                                               gint           first_value,
                                               ...) G_GNUC_NULL_TERMINATED;
GtkListStore * gimp_int_store_new_valist      (const gchar   *first_label,
                                               gint           first_value,
                                               va_list        values);
GtkListStore * gimp_int_store_new_array       (gint           n_values,
                                               const gchar   *labels[]);

gboolean       gimp_int_store_lookup_by_value (GtkTreeModel  *model,
                                               gint           value,
                                               GtkTreeIter   *iter);
gboolean   gimp_int_store_lookup_by_user_data (GtkTreeModel  *model,
                                               gpointer       user_data,
                                               GtkTreeIter   *iter);


G_END_DECLS

#endif  /* __GIMP_INT_STORE_H__ */
