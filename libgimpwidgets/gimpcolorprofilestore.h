/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaprofilestore.h
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

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_COLOR_PROFILE_STORE_H__
#define __LIGMA_COLOR_PROFILE_STORE_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_COLOR_PROFILE_STORE            (ligma_color_profile_store_get_type ())
#define LIGMA_COLOR_PROFILE_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_PROFILE_STORE, LigmaColorProfileStore))
#define LIGMA_COLOR_PROFILE_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_PROFILE_STORE, LigmaColorProfileStoreClass))
#define LIGMA_IS_COLOR_PROFILE_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_PROFILE_STORE))
#define LIGMA_IS_COLOR_PROFILE_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_PROFILE_STORE))
#define LIGMA_COLOR_PROFILE_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_PROFILE_STORE, LigmaColorProfileStoreClass))


typedef struct _LigmaColorProfileStorePrivate LigmaColorProfileStorePrivate;
typedef struct _LigmaColorProfileStoreClass   LigmaColorProfileStoreClass;

struct _LigmaColorProfileStore
{
  GtkListStore                  parent_instance;

  LigmaColorProfileStorePrivate *priv;
};

struct _LigmaColorProfileStoreClass
{
  GtkListStoreClass  parent_class;

  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType          ligma_color_profile_store_get_type (void) G_GNUC_CONST;

GtkListStore * ligma_color_profile_store_new      (GFile                 *history);

void           ligma_color_profile_store_add_file (LigmaColorProfileStore *store,
                                                  GFile                 *file,
                                                  const gchar           *label);


G_END_DECLS

#endif  /* __LIGMA_COLOR_PROFILE_STORE_H__ */
