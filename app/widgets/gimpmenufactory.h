/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmenufactory.h
 * Copyright (C) 2003-2004 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_MENU_FACTORY_H__
#define __GIMP_MENU_FACTORY_H__


#include "core/gimpobject.h"


typedef struct _GimpMenuFactoryEntry GimpMenuFactoryEntry;

struct _GimpMenuFactoryEntry
{
  gchar        *identifier;
  GList        *action_groups;
  GList        *managed_uis;

  GHashTable   *managers;
};


#define GIMP_TYPE_MENU_FACTORY            (gimp_menu_factory_get_type ())
#define GIMP_MENU_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MENU_FACTORY, GimpMenuFactory))
#define GIMP_MENU_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MENU_FACTORY, GimpMenuFactoryClass))
#define GIMP_IS_MENU_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MENU_FACTORY))
#define GIMP_IS_MENU_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MENU_FACTORY))
#define GIMP_MENU_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MENU_FACTORY, GimpMenuFactoryClass))


typedef struct _GimpMenuFactoryPrivate  GimpMenuFactoryPrivate;
typedef struct _GimpMenuFactoryClass    GimpMenuFactoryClass;

struct _GimpMenuFactory
{
  GimpObject              parent_instance;

  GimpMenuFactoryPrivate *p;
};

struct _GimpMenuFactoryClass
{
  GimpObjectClass  parent_class;
};


GType             gimp_menu_factory_get_type             (void) G_GNUC_CONST;
GimpMenuFactory * gimp_menu_factory_new                  (Gimp              *gimp,
                                                          GimpActionFactory *action_factory);
void              gimp_menu_factory_manager_register     (GimpMenuFactory   *factory,
                                                          const gchar       *identifier,
                                                          const gchar       *first_group,
                                                          ...)  G_GNUC_NULL_TERMINATED;
GList           * gimp_menu_factory_get_registered_menus (GimpMenuFactory   *factory);
GimpUIManager   * gimp_menu_factory_get_manager          (GimpMenuFactory   *factory,
                                                          const gchar       *identifier,
                                                          gpointer           callback_data);
void              gimp_menu_factory_delete_manager       (GimpMenuFactory   *factory,
                                                          const gchar       *identifier,
                                                          gpointer           callback_data);


#endif  /*  __GIMP_MENU_FACTORY_H__  */
