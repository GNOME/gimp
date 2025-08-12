/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpactionfactory.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#pragma once

#include "core/gimpobject.h"


typedef struct _GimpActionFactoryEntry GimpActionFactoryEntry;

struct _GimpActionFactoryEntry
{
  gchar                     *identifier;
  gchar                     *label;
  gchar                     *icon_name;
  GimpActionGroupSetupFunc   setup_func;
  GimpActionGroupUpdateFunc  update_func;

  GHashTable                *groups;
};


#define GIMP_TYPE_ACTION_FACTORY            (gimp_action_factory_get_type ())
#define GIMP_ACTION_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ACTION_FACTORY, GimpActionFactory))
#define GIMP_ACTION_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ACTION_FACTORY, GimpActionFactoryClass))
#define GIMP_IS_ACTION_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ACTION_FACTORY))
#define GIMP_IS_ACTION_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ACTION_FACTORY))
#define GIMP_ACTION_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ACTION_FACTORY, GimpActionFactoryClass))


typedef struct _GimpActionFactoryClass  GimpActionFactoryClass;

struct _GimpActionFactory
{
  GimpObject  parent_instance;

  Gimp       *gimp;
  GList      *registered_groups;
};

struct _GimpActionFactoryClass
{
  GimpObjectClass  parent_class;
};


GType               gimp_action_factory_get_type       (void) G_GNUC_CONST;

GimpActionFactory * gimp_action_factory_new            (Gimp                      *gimp);

void                gimp_action_factory_group_register (GimpActionFactory         *factory,
                                                        const gchar               *identifier,
                                                        const gchar               *label,
                                                        const gchar               *icon_name,
                                                        GimpActionGroupSetupFunc   setup_func,
                                                        GimpActionGroupUpdateFunc  update_func);

GimpActionGroup *   gimp_action_factory_get_group      (GimpActionFactory         *factory,
                                                        const gchar               *identifier,
                                                        gpointer                   user_data);
void                gimp_action_factory_delete_group   (GimpActionFactory         *factory,
                                                        const gchar               *identifier,
                                                        gpointer                   user_data);
