/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpenumstore.h
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

#ifndef __GIMP_ENUM_STORE_H__
#define __GIMP_ENUM_STORE_H__

#include <libgimpwidgets/gimpintstore.h>


G_BEGIN_DECLS

#define GIMP_TYPE_ENUM_STORE (gimp_enum_store_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpEnumStore, gimp_enum_store, GIMP, ENUM_STORE, GimpIntStore)

struct _GimpEnumStoreClass
{
  GimpIntStoreClass  parent_class;

  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};


GtkListStore * gimp_enum_store_new                    (GType    enum_type);
GtkListStore * gimp_enum_store_new_with_range         (GType    enum_type,
                                                       gint     minimum,
                                                       gint     maximum);
GtkListStore * gimp_enum_store_new_with_values        (GType    enum_type,
                                                       gint     n_values,
                                                       ...);
GtkListStore * gimp_enum_store_new_with_values_valist (GType    enum_type,
                                                       gint     n_values,
                                                       va_list  args);

void           gimp_enum_store_set_icon_prefix  (GimpEnumStore *store,
                                                 const gchar   *icon_prefix);


G_END_DECLS

#endif  /* __GIMP_ENUM_STORE_H__ */
