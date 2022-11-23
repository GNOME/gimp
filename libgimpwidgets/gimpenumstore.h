/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaenumstore.h
 * Copyright (C) 2004  Sven Neumann <sven@ligma.org>
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

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_ENUM_STORE_H__
#define __LIGMA_ENUM_STORE_H__

#include <libligmawidgets/ligmaintstore.h>


G_BEGIN_DECLS

#define LIGMA_TYPE_ENUM_STORE            (ligma_enum_store_get_type ())
#define LIGMA_ENUM_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ENUM_STORE, LigmaEnumStore))
#define LIGMA_ENUM_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ENUM_STORE, LigmaEnumStoreClass))
#define LIGMA_IS_ENUM_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ENUM_STORE))
#define LIGMA_IS_ENUM_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ENUM_STORE))
#define LIGMA_ENUM_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ENUM_STORE, LigmaEnumStoreClass))


typedef struct _LigmaEnumStorePrivate LigmaEnumStorePrivate;
typedef struct _LigmaEnumStoreClass   LigmaEnumStoreClass;

struct _LigmaEnumStore
{
  LigmaIntStore          parent_instance;

  LigmaEnumStorePrivate *priv;
};

struct _LigmaEnumStoreClass
{
  LigmaIntStoreClass  parent_class;

  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType          ligma_enum_store_get_type               (void) G_GNUC_CONST;

GtkListStore * ligma_enum_store_new                    (GType    enum_type);
GtkListStore * ligma_enum_store_new_with_range         (GType    enum_type,
                                                       gint     minimum,
                                                       gint     maximum);
GtkListStore * ligma_enum_store_new_with_values        (GType    enum_type,
                                                       gint     n_values,
                                                       ...);
GtkListStore * ligma_enum_store_new_with_values_valist (GType    enum_type,
                                                       gint     n_values,
                                                       va_list  args);

void           ligma_enum_store_set_icon_prefix  (LigmaEnumStore *store,
                                                 const gchar   *icon_prefix);


G_END_DECLS

#endif  /* __LIGMA_ENUM_STORE_H__ */
