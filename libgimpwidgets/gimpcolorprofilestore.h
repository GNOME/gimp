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
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_COLOR_PROFILE_STORE_H__
#define __GIMP_COLOR_PROFILE_STORE_H__

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
  GtkListStore  parent_instance;
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

GIMP_DEPRECATED_FOR(gimp_color_profile_store_add_file)
void           gimp_color_profile_store_add      (GimpColorProfileStore *store,
                                                  const gchar           *filename,
                                                  const gchar           *label);

void           gimp_color_profile_store_add_file (GimpColorProfileStore *store,
                                                  GFile                 *file,
                                                  const gchar           *label);


G_END_DECLS

#endif  /* __GIMP_COLOR_PROFILE_STORE_H__ */
