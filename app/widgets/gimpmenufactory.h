/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmamenufactory.h
 * Copyright (C) 2003-2004 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_MENU_FACTORY_H__
#define __LIGMA_MENU_FACTORY_H__


#include "core/ligmaobject.h"


typedef struct _LigmaMenuFactoryEntry LigmaMenuFactoryEntry;

struct _LigmaMenuFactoryEntry
{
  gchar *identifier;
  GList *action_groups;
  GList *managed_uis;
};


#define LIGMA_TYPE_MENU_FACTORY            (ligma_menu_factory_get_type ())
#define LIGMA_MENU_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_MENU_FACTORY, LigmaMenuFactory))
#define LIGMA_MENU_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_MENU_FACTORY, LigmaMenuFactoryClass))
#define LIGMA_IS_MENU_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_MENU_FACTORY))
#define LIGMA_IS_MENU_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_MENU_FACTORY))
#define LIGMA_MENU_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_MENU_FACTORY, LigmaMenuFactoryClass))


typedef struct _LigmaMenuFactoryPrivate  LigmaMenuFactoryPrivate;
typedef struct _LigmaMenuFactoryClass    LigmaMenuFactoryClass;

struct _LigmaMenuFactory
{
  LigmaObject              parent_instance;

  LigmaMenuFactoryPrivate *p;
};

struct _LigmaMenuFactoryClass
{
  LigmaObjectClass  parent_class;
};


GType             ligma_menu_factory_get_type             (void) G_GNUC_CONST;
LigmaMenuFactory * ligma_menu_factory_new                  (Ligma              *ligma,
                                                          LigmaActionFactory *action_factory);
void              ligma_menu_factory_manager_register     (LigmaMenuFactory   *factory,
                                                          const gchar       *identifier,
                                                          const gchar       *first_group,
                                                          ...)  G_GNUC_NULL_TERMINATED;
GList           * ligma_menu_factory_get_registered_menus (LigmaMenuFactory   *factory);
LigmaUIManager   * ligma_menu_factory_manager_new          (LigmaMenuFactory   *factory,
                                                          const gchar       *identifier,
                                                          gpointer           callback_data);


#endif  /*  __LIGMA_MENU_FACTORY_H__  */
