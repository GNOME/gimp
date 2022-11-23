/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligma-debug.c
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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

#include <glib-object.h>

#include "core/core-types.h"

#include "core/ligmaobject.h"

#include "ligma-debug.h"


static GHashTable *class_hash = NULL;


void
ligma_debug_enable_instances (void)
{
  g_return_if_fail (class_hash == NULL);

  class_hash = g_hash_table_new_full (g_str_hash,
                                      g_str_equal,
                                      NULL,
                                      (GDestroyNotify ) g_hash_table_unref);
}

void
ligma_debug_add_instance (GObject      *instance,
                         GObjectClass *klass)
{
  if (class_hash)
    {
      GHashTable  *instance_hash;
      const gchar *type_name;

      type_name = g_type_name (G_TYPE_FROM_CLASS (klass));

      instance_hash = g_hash_table_lookup (class_hash, type_name);

      if (! instance_hash)
        {
          instance_hash = g_hash_table_new (g_direct_hash,
                                            g_direct_equal);
          g_hash_table_insert (class_hash, (gchar *) type_name, instance_hash);
        }

      g_hash_table_insert (instance_hash, instance, instance);
    }
}

void
ligma_debug_remove_instance (GObject *instance)
{
  if (class_hash)
    {
      GHashTable  *instance_hash;
      const gchar *type_name;

      type_name = g_type_name (G_OBJECT_TYPE (instance));

      instance_hash = g_hash_table_lookup (class_hash, type_name);

      if (instance_hash)
        {
          g_hash_table_remove (instance_hash, instance);

          if (g_hash_table_size (instance_hash) == 0)
            g_hash_table_remove (class_hash, type_name);
        }
    }
}

static void
ligma_debug_instance_foreach (GObject *instance)
{
  g_printerr ("  \'%s\': ref_count = %d\n",
              LIGMA_IS_OBJECT (instance) ?
              ligma_object_get_name (instance) : "GObject",
              instance->ref_count);
}

static void
ligma_debug_class_foreach (const gchar *type_name,
                          GHashTable  *instance_hash)
{
  g_printerr ("Leaked %s instances: %d\n",
              type_name, g_hash_table_size (instance_hash));

  g_hash_table_foreach (instance_hash,
                        (GHFunc) ligma_debug_instance_foreach,
                        NULL);
}

void
ligma_debug_instances (void)
{
  if (class_hash)
    {
      g_hash_table_foreach (class_hash,
                            (GHFunc) ligma_debug_class_foreach,
                            NULL);
    }
}
