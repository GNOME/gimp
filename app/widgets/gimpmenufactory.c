/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmenufactory.c
 * Copyright (C) 2001-2004 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"

#include "gimpactionfactory.h"
#include "gimpmenufactory.h"
#include "gimpuimanager.h"


struct _GimpMenuFactoryPrivate
{
  Gimp              *gimp;
  GimpActionFactory *action_factory;
  GList             *registered_menus;
};


static void   gimp_menu_factory_finalize (GObject *object);


G_DEFINE_TYPE (GimpMenuFactory, gimp_menu_factory, GIMP_TYPE_OBJECT)

#define parent_class gimp_menu_factory_parent_class


static void
gimp_menu_factory_class_init (GimpMenuFactoryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_menu_factory_finalize;

  g_type_class_add_private (klass, sizeof (GimpMenuFactoryPrivate));
}

static void
gimp_menu_factory_init (GimpMenuFactory *factory)
{
  factory->p = G_TYPE_INSTANCE_GET_PRIVATE (factory,
                                            GIMP_TYPE_MENU_FACTORY,
                                            GimpMenuFactoryPrivate);
}

static void
gimp_menu_factory_finalize (GObject *object)
{
  GimpMenuFactory *factory = GIMP_MENU_FACTORY (object);
  GList           *list;

  for (list = factory->p->registered_menus; list; list = g_list_next (list))
    {
      GimpMenuFactoryEntry *entry = list->data;
      GList                *uis;

      g_free (entry->identifier);

      g_list_free_full (entry->action_groups, (GDestroyNotify) g_free);

      for (uis = entry->managed_uis; uis; uis = g_list_next (uis))
        {
          GimpUIManagerUIEntry *ui_entry = uis->data;

          g_free (ui_entry->ui_path);
          g_free (ui_entry->basename);

          g_slice_free (GimpUIManagerUIEntry, ui_entry);
        }

      g_list_free (entry->managed_uis);

      g_slice_free (GimpMenuFactoryEntry, entry);
    }

  g_list_free (factory->p->registered_menus);
  factory->p->registered_menus = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GimpMenuFactory *
gimp_menu_factory_new (Gimp              *gimp,
                       GimpActionFactory *action_factory)
{
  GimpMenuFactory *factory;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_ACTION_FACTORY (action_factory), NULL);

  factory = g_object_new (GIMP_TYPE_MENU_FACTORY, NULL);

  factory->p->gimp           = gimp;
  factory->p->action_factory = action_factory;

  return factory;
}

void
gimp_menu_factory_manager_register (GimpMenuFactory *factory,
                                    const gchar     *identifier,
                                    const gchar     *first_group,
                                    ...)
{
  GimpMenuFactoryEntry *entry;
  const gchar          *group;
  const gchar          *ui_path;
  va_list               args;

  g_return_if_fail (GIMP_IS_MENU_FACTORY (factory));
  g_return_if_fail (identifier != NULL);
  g_return_if_fail (first_group != NULL);

  entry = g_slice_new0 (GimpMenuFactoryEntry);

  entry->identifier = g_strdup (identifier);

  factory->p->registered_menus = g_list_prepend (factory->p->registered_menus, entry);

  va_start (args, first_group);

  for (group = first_group;
       group;
       group = va_arg (args, const gchar *))
    {
      entry->action_groups = g_list_prepend (entry->action_groups,
                                             g_strdup (group));
    }

  entry->action_groups = g_list_reverse (entry->action_groups);

  ui_path = va_arg (args, const gchar *);

  while (ui_path)
    {
      const gchar            *ui_basename;
      GimpUIManagerSetupFunc  setup_func;
      GimpUIManagerUIEntry   *ui_entry;

      ui_basename = va_arg (args, const gchar *);
      setup_func  = va_arg (args, GimpUIManagerSetupFunc);

      ui_entry = g_slice_new0 (GimpUIManagerUIEntry);

      ui_entry->ui_path    = g_strdup (ui_path);
      ui_entry->basename   = g_strdup (ui_basename);
      ui_entry->setup_func = setup_func;

      entry->managed_uis = g_list_prepend (entry->managed_uis, ui_entry);

      ui_path = va_arg (args, const gchar *);
    }

  entry->managed_uis = g_list_reverse (entry->managed_uis);

  va_end (args);
}

GList *
gimp_menu_factory_get_registered_menus (GimpMenuFactory *factory)
{
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (factory), NULL);

  return factory->p->registered_menus;
}

static void
gimp_menu_factory_manager_action_added (GimpActionGroup *group,
                                        GtkAction       *action,
                                        GtkAccelGroup   *accel_group)
{
  gtk_action_set_accel_group (action, accel_group);
  gtk_action_connect_accelerator (action);
}

GimpUIManager *
gimp_menu_factory_manager_new (GimpMenuFactory *factory,
                               const gchar     *identifier,
                               gpointer         callback_data)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (factory), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);

  for (list = factory->p->registered_menus; list; list = g_list_next (list))
    {
      GimpMenuFactoryEntry *entry = list->data;

      if (! strcmp (entry->identifier, identifier))
        {
          GimpUIManager *manager;
          GtkAccelGroup *accel_group;
          GList         *list;

          manager = gimp_ui_manager_new (factory->p->gimp, entry->identifier);
          accel_group = gtk_ui_manager_get_accel_group (GTK_UI_MANAGER (manager));

          for (list = entry->action_groups; list; list = g_list_next (list))
            {
              GimpActionGroup *group;
              GList           *actions;
              GList           *list2;

              group = gimp_action_factory_group_new (factory->p->action_factory,
                                                     (const gchar *) list->data,
                                                     callback_data);

              actions = gtk_action_group_list_actions (GTK_ACTION_GROUP (group));

              for (list2 = actions; list2; list2 = g_list_next (list2))
                {
                  GtkAction *action = list2->data;

                  gtk_action_set_accel_group (action, accel_group);
                  gtk_action_connect_accelerator (action);
                }

              g_list_free (actions);

              g_signal_connect_object (group, "action-added",
                                       G_CALLBACK (gimp_menu_factory_manager_action_added),
                                       accel_group, 0);

              gtk_ui_manager_insert_action_group (GTK_UI_MANAGER (manager),
                                                  GTK_ACTION_GROUP (group),
                                                  -1);

              g_object_unref (group);
            }

          for (list = entry->managed_uis; list; list = g_list_next (list))
            {
              GimpUIManagerUIEntry *ui_entry = list->data;

              gimp_ui_manager_ui_register (manager,
                                           ui_entry->ui_path,
                                           ui_entry->basename,
                                           ui_entry->setup_func);
            }

          return manager;
        }
    }

  g_warning ("%s: no entry registered for \"%s\"",
             G_STRFUNC, identifier);

  return NULL;
}
