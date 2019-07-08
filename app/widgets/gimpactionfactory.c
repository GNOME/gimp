/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpactionfactory.c
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

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"

#include "gimpactionfactory.h"
#include "gimpactiongroup.h"


static void   gimp_action_factory_finalize (GObject *object);


G_DEFINE_TYPE (GimpActionFactory, gimp_action_factory, GIMP_TYPE_OBJECT)

#define parent_class gimp_action_factory_parent_class


static void
gimp_action_factory_class_init (GimpActionFactoryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_action_factory_finalize;
}

static void
gimp_action_factory_init (GimpActionFactory *factory)
{
  factory->gimp              = NULL;
  factory->registered_groups = NULL;
}

static void
gimp_action_factory_finalize (GObject *object)
{
  GimpActionFactory *factory = GIMP_ACTION_FACTORY (object);
  GList             *list;

  for (list = factory->registered_groups; list; list = g_list_next (list))
    {
      GimpActionFactoryEntry *entry = list->data;

      g_free (entry->identifier);
      g_free (entry->label);
      g_free (entry->icon_name);

      g_slice_free (GimpActionFactoryEntry, entry);
    }

  g_list_free (factory->registered_groups);
  factory->registered_groups = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GimpActionFactory *
gimp_action_factory_new (Gimp *gimp)
{
  GimpActionFactory *factory;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  factory = g_object_new (GIMP_TYPE_ACTION_FACTORY, NULL);

  factory->gimp = gimp;

  return factory;
}

void
gimp_action_factory_group_register (GimpActionFactory         *factory,
                                    const gchar               *identifier,
                                    const gchar               *label,
                                    const gchar               *icon_name,
                                    GimpActionGroupSetupFunc   setup_func,
                                    GimpActionGroupUpdateFunc  update_func)
{
  GimpActionFactoryEntry *entry;

  g_return_if_fail (GIMP_IS_ACTION_FACTORY (factory));
  g_return_if_fail (identifier != NULL);
  g_return_if_fail (label != NULL);
  g_return_if_fail (setup_func != NULL);
  g_return_if_fail (update_func != NULL);

  entry = g_slice_new0 (GimpActionFactoryEntry);

  entry->identifier  = g_strdup (identifier);
  entry->label       = g_strdup (label);
  entry->icon_name   = g_strdup (icon_name);
  entry->setup_func  = setup_func;
  entry->update_func = update_func;

  factory->registered_groups = g_list_prepend (factory->registered_groups,
                                               entry);
}

GimpActionGroup *
gimp_action_factory_group_new (GimpActionFactory *factory,
                               const gchar       *identifier,
                               gpointer           user_data)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_ACTION_FACTORY (factory), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);

  for (list = factory->registered_groups; list; list = g_list_next (list))
    {
      GimpActionFactoryEntry *entry = list->data;

      if (! strcmp (entry->identifier, identifier))
        {
          GimpActionGroup *group;

          group = gimp_action_group_new (factory->gimp,
                                         entry->identifier,
                                         entry->label,
                                         entry->icon_name,
                                         user_data,
                                         entry->update_func);

          if (entry->setup_func)
            entry->setup_func (group);

          return group;
        }
    }

  g_warning ("%s: no entry registered for \"%s\"",
             G_STRFUNC, identifier);

  return NULL;
}
