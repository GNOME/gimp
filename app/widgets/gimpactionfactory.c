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

#include "gimpaction.h"
#include "gimpactionfactory.h"
#include "gimpactiongroup.h"


static void gimp_action_factory_finalize       (GObject           *object);

static void gimp_action_factory_action_removed (GimpActionGroup   *group,
                                                gchar             *action_name,
                                                GimpActionFactory *factory);


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
      g_hash_table_unref (entry->groups);

      g_slice_free (GimpActionFactoryEntry, entry);
    }

  g_list_free (factory->registered_groups);
  factory->registered_groups = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_action_factory_action_removed (GimpActionGroup   *group,
                                    gchar             *action_name,
                                    GimpActionFactory *factory)
{
  GList *list;

  for (list = factory->registered_groups; list; list = g_list_next (list))
    {
      GimpActionFactoryEntry *entry = list->data;
      GList                  *groups;
      GList                  *iter;

      groups = g_hash_table_get_values (entry->groups);

      for (iter = groups; iter; iter = iter->next)
        {
          if (group != iter->data)
            {
              if (g_action_group_has_action (G_ACTION_GROUP (iter->data), action_name))
                break;
            }
        }

      g_list_free (groups);

      if (iter != NULL)
        break;
    }

  if (list == NULL)
    g_action_map_remove_action (G_ACTION_MAP (factory->gimp->app), action_name);
}


/* Public functions */

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
  entry->groups      = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_object_unref);

  factory->registered_groups = g_list_prepend (factory->registered_groups,
                                               entry);
}

GimpActionGroup *
gimp_action_factory_get_group (GimpActionFactory *factory,
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

          group = g_hash_table_lookup (entry->groups, user_data);

          if (group == NULL)
            {
              group = gimp_action_group_new (factory->gimp,
                                             entry->identifier,
                                             entry->label,
                                             entry->icon_name,
                                             user_data,
                                             entry->update_func);

              if (entry->setup_func)
                entry->setup_func (group);

              g_hash_table_insert (entry->groups, user_data, group);

              g_signal_connect_object (group, "action-removed",
                                       G_CALLBACK (gimp_action_factory_action_removed),
                                       factory, G_CONNECT_AFTER);
            }

          return group;
        }
    }

  g_warning ("%s: no entry registered for \"%s\"",
             G_STRFUNC, identifier);

  return NULL;
}

void
gimp_action_factory_delete_group (GimpActionFactory *factory,
                                  const gchar       *identifier,
                                  gpointer           user_data)
{
  GList *list;

  g_return_if_fail (GIMP_IS_ACTION_FACTORY (factory));
  g_return_if_fail (identifier != NULL);

  for (list = factory->registered_groups; list; list = g_list_next (list))
    {
      GimpActionFactoryEntry *entry = list->data;

      if (g_strcmp0 (entry->identifier, identifier) == 0)
        {
          if (! g_hash_table_remove (entry->groups, user_data))
            g_warning ("%s: no GimpActionGroup for (id \"%s\", data %p)",
                       G_STRFUNC, identifier, user_data);

          return;
        }
    }

  g_warning ("%s: no entry registered for \"%s\"",
             G_STRFUNC, identifier);
}
