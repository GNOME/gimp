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
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_ENUM_STORE_H__
#define __GIMP_ENUM_STORE_H__

#include <libgimpwidgets/gimpintstore.h>


G_BEGIN_DECLS

#define GIMP_TYPE_ENUM_STORE            (gimp_enum_store_get_type ())
#define GIMP_ENUM_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ENUM_STORE, GimpEnumStore))
#define GIMP_ENUM_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ENUM_STORE, GimpEnumStoreClass))
#define GIMP_IS_ENUM_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ENUM_STORE))
#define GIMP_IS_ENUM_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ENUM_STORE))
#define GIMP_ENUM_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ENUM_STORE, GimpEnumStoreClass))


typedef struct _GimpEnumStorePrivate GimpEnumStorePrivate;
typedef struct _GimpEnumStoreClass   GimpEnumStoreClass;

struct _GimpEnumStore
{
  GimpIntStore          parent_instance;

  GimpEnumStorePrivate *priv;
};

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


GType          gimp_enum_store_get_type               (void) G_GNUC_CONST;

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
