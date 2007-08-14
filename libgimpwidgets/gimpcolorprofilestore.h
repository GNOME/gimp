/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpprofilestore.h
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_COLOR_PROFILE_STORE_H__
#define __GIMP_COLOR_PROFILE_STORE_H__

#include <gtk/gtkliststore.h>

G_BEGIN_DECLS


#define GIMP_TYPE_COLOR_PROFILE_STORE            (gimp_color_profile_store_get_type ())
#define GIMP_COLOR_PROFILE_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_PROFILE_STORE, GimpColorProfileStore))
#define GIMP_COLOR_PROFILE_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_PROFILE_STORE, GimpColorProfileStoreClass))
#define GIMP_IS_COLOR_PROFILE_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_PROFILE_STORE))
#define GIMP_IS_COLOR_PROFILE_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_PROFILE_STORE))
#define GIMP_COLOR_PROFILE_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_PROFILE_STORE, GimpColorProfileStoreClass))


typedef struct _GimpColorProfileStoreClass  GimpColorProfileStoreClass;

struct _GimpColorProfileStore
{
  GtkListStore       parent_instance;

  gchar             *history;
};

struct _GimpColorProfileStoreClass
{
  GtkListStoreClass  parent_class;

  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType          gimp_color_profile_store_get_type (void) G_GNUC_CONST;

GtkListStore * gimp_color_profile_store_new      (const gchar           *history);

void           gimp_color_profile_store_add      (GimpColorProfileStore *store,
                                                  const gchar           *uri,
                                                  const gchar           *label);


#endif  /* __GIMP_COLOR_PROFILE_STORE_H__ */
