/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpenumstore.h
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_ENUM_STORE_H__
#define __GIMP_ENUM_STORE_H__

#include <gtk/gtkliststore.h>


typedef enum
{
  GIMP_ENUM_STORE_VALUE,
  GIMP_ENUM_STORE_LABEL,
  GIMP_ENUM_STORE_ICON,
  GIMP_ENUM_STORE_USER_DATA,
  GIMP_ENUM_STORE_NUM_COLUMNS
} GimpEnumStoreColumns;


#define GIMP_TYPE_ENUM_STORE            (gimp_enum_store_get_type ())
#define GIMP_ENUM_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ENUM_STORE, GimpEnumStore))
#define GIMP_ENUM_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ENUM_STORE, GimpEnumStoreClass))
#define GIMP_IS_ENUM_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ENUM_STORE))
#define GIMP_IS_ENUM_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ENUM_STORE))
#define GIMP_ENUM_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ENUM_STORE, GimpEnumStoreClass))


typedef struct _GimpEnumStoreClass  GimpEnumStoreClass;

struct _GimpEnumStoreClass
{
  GtkListStoreClass  parent_instance;
};

struct _GimpEnumStore
{
  GtkListStore   parent_instance;

  GEnumClass    *enum_class;
};


GType          gimp_enum_store_get_type (void) G_GNUC_CONST;

GtkListStore * gimp_enum_store_new                    (GType    enum_type);;
GtkListStore * gimp_enum_store_new_with_range         (GType    enum_type,
                                                       gint     minimum,
                                                       gint     maximum);
GtkListStore * gimp_enum_store_new_with_values        (GType    enum_type,
                                                       gint     n_values,
                                                       ...);
GtkListStore * gimp_enum_store_new_with_values_valist (GType    enum_type,
                                                       gint     n_values,
                                                       va_list  args);

void           gimp_enum_store_set_icons        (GimpEnumStore *store,
                                                 GtkWidget     *widget,
                                                 const gchar   *stock_prefix,
                                                 GtkIconSize    size);

#endif  /* __GIMP_ENUM_STORE_H__ */
