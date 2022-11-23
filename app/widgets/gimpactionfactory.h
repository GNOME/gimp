/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaactionfactory.h
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_ACTION_FACTORY_H__
#define __LIGMA_ACTION_FACTORY_H__


#include "core/ligmaobject.h"


typedef struct _LigmaActionFactoryEntry LigmaActionFactoryEntry;

struct _LigmaActionFactoryEntry
{
  gchar                     *identifier;
  gchar                     *label;
  gchar                     *icon_name;
  LigmaActionGroupSetupFunc   setup_func;
  LigmaActionGroupUpdateFunc  update_func;
};


#define LIGMA_TYPE_ACTION_FACTORY            (ligma_action_factory_get_type ())
#define LIGMA_ACTION_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ACTION_FACTORY, LigmaActionFactory))
#define LIGMA_ACTION_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ACTION_FACTORY, LigmaActionFactoryClass))
#define LIGMA_IS_ACTION_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ACTION_FACTORY))
#define LIGMA_IS_ACTION_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ACTION_FACTORY))
#define LIGMA_ACTION_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ACTION_FACTORY, LigmaActionFactoryClass))


typedef struct _LigmaActionFactoryClass  LigmaActionFactoryClass;

struct _LigmaActionFactory
{
  LigmaObject  parent_instance;

  Ligma       *ligma;
  GList      *registered_groups;
};

struct _LigmaActionFactoryClass
{
  LigmaObjectClass  parent_class;
};


GType               ligma_action_factory_get_type (void) G_GNUC_CONST;

LigmaActionFactory * ligma_action_factory_new      (Ligma              *ligma);

void          ligma_action_factory_group_register (LigmaActionFactory *factory,
                                                  const gchar       *identifier,
                                                  const gchar       *label,
                                                  const gchar       *icon_name,
                                                  LigmaActionGroupSetupFunc  setup_func,
                                                  LigmaActionGroupUpdateFunc update_func);

LigmaActionGroup * ligma_action_factory_group_new  (LigmaActionFactory *factory,
                                                  const gchar       *identifier,
                                                  gpointer           user_data);


#endif  /*  __LIGMA_ACTION_FACTORY_H__  */
