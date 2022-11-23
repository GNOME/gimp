/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaactionfactory.c
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

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"

#include "ligmaactionfactory.h"
#include "ligmaactiongroup.h"


static void   ligma_action_factory_finalize (GObject *object);


G_DEFINE_TYPE (LigmaActionFactory, ligma_action_factory, LIGMA_TYPE_OBJECT)

#define parent_class ligma_action_factory_parent_class


static void
ligma_action_factory_class_init (LigmaActionFactoryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ligma_action_factory_finalize;
}

static void
ligma_action_factory_init (LigmaActionFactory *factory)
{
  factory->ligma              = NULL;
  factory->registered_groups = NULL;
}

static void
ligma_action_factory_finalize (GObject *object)
{
  LigmaActionFactory *factory = LIGMA_ACTION_FACTORY (object);
  GList             *list;

  for (list = factory->registered_groups; list; list = g_list_next (list))
    {
      LigmaActionFactoryEntry *entry = list->data;

      g_free (entry->identifier);
      g_free (entry->label);
      g_free (entry->icon_name);

      g_slice_free (LigmaActionFactoryEntry, entry);
    }

  g_list_free (factory->registered_groups);
  factory->registered_groups = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

LigmaActionFactory *
ligma_action_factory_new (Ligma *ligma)
{
  LigmaActionFactory *factory;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  factory = g_object_new (LIGMA_TYPE_ACTION_FACTORY, NULL);

  factory->ligma = ligma;

  return factory;
}

void
ligma_action_factory_group_register (LigmaActionFactory         *factory,
                                    const gchar               *identifier,
                                    const gchar               *label,
                                    const gchar               *icon_name,
                                    LigmaActionGroupSetupFunc   setup_func,
                                    LigmaActionGroupUpdateFunc  update_func)
{
  LigmaActionFactoryEntry *entry;

  g_return_if_fail (LIGMA_IS_ACTION_FACTORY (factory));
  g_return_if_fail (identifier != NULL);
  g_return_if_fail (label != NULL);
  g_return_if_fail (setup_func != NULL);
  g_return_if_fail (update_func != NULL);

  entry = g_slice_new0 (LigmaActionFactoryEntry);

  entry->identifier  = g_strdup (identifier);
  entry->label       = g_strdup (label);
  entry->icon_name   = g_strdup (icon_name);
  entry->setup_func  = setup_func;
  entry->update_func = update_func;

  factory->registered_groups = g_list_prepend (factory->registered_groups,
                                               entry);
}

LigmaActionGroup *
ligma_action_factory_group_new (LigmaActionFactory *factory,
                               const gchar       *identifier,
                               gpointer           user_data)
{
  GList *list;

  g_return_val_if_fail (LIGMA_IS_ACTION_FACTORY (factory), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);

  for (list = factory->registered_groups; list; list = g_list_next (list))
    {
      LigmaActionFactoryEntry *entry = list->data;

      if (! strcmp (entry->identifier, identifier))
        {
          LigmaActionGroup *group;

          group = ligma_action_group_new (factory->ligma,
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
