/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpidtable.c
 * Copyright (C) 2011 Martin Nordholts <martinn@src.gnome.org>
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

#include <gio/gio.h>
#include <gegl.h>

#include "core-types.h"

#include "gimp-memsize.h"
#include "gimpidtable.h"


#define GIMP_ID_TABLE_START_ID 1
#define GIMP_ID_TABLE_END_ID   G_MAXINT


struct _GimpIdTablePrivate
{
  GHashTable *id_table;
  gint        next_id;
};


static void    gimp_id_table_finalize    (GObject    *object);
static gint64  gimp_id_table_get_memsize (GimpObject *object,
                                          gint64     *gui_size);


G_DEFINE_TYPE_WITH_PRIVATE (GimpIdTable, gimp_id_table, GIMP_TYPE_OBJECT)

#define parent_class gimp_id_table_parent_class


static void
gimp_id_table_class_init (GimpIdTableClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  object_class->finalize         = gimp_id_table_finalize;

  gimp_object_class->get_memsize = gimp_id_table_get_memsize;
}

static void
gimp_id_table_init (GimpIdTable *id_table)
{
  id_table->priv = gimp_id_table_get_instance_private (id_table);

  id_table->priv->id_table = g_hash_table_new (g_direct_hash, NULL);
  id_table->priv->next_id  = GIMP_ID_TABLE_START_ID;
}

static void
gimp_id_table_finalize (GObject *object)
{
  GimpIdTable *id_table = GIMP_ID_TABLE (object);

  g_clear_pointer (&id_table->priv->id_table, g_hash_table_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_id_table_get_memsize (GimpObject *object,
                           gint64     *gui_size)
{
  GimpIdTable *id_table = GIMP_ID_TABLE (object);
  gint64       memsize  = 0;

  memsize += gimp_g_hash_table_get_memsize (id_table->priv->id_table, 0);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

/**
 * gimp_id_table_new:
 *
 * Returns: A new #GimpIdTable.
 **/
GimpIdTable *
gimp_id_table_new (void)
{
  return g_object_new (GIMP_TYPE_ID_TABLE, NULL);
}

/**
 * gimp_id_table_insert:
 * @id_table: A #GimpIdTable
 * @data: Data to insert and assign an id to
 *
 * Insert data in the id table. The data will get an, in this table,
 * unused ID assigned to it that can be used to later lookup the data.
 *
 * Returns: The assigned ID.
 **/
gint
gimp_id_table_insert (GimpIdTable *id_table, gpointer data)
{
  gint new_id;
  gint start_id;

  g_return_val_if_fail (GIMP_IS_ID_TABLE (id_table), 0);

  start_id = id_table->priv->next_id;

  do
    {
      new_id = id_table->priv->next_id++;

      if (id_table->priv->next_id == GIMP_ID_TABLE_END_ID)
        id_table->priv->next_id = GIMP_ID_TABLE_START_ID;

      if (start_id == id_table->priv->next_id)
        {
          /* We looped once over all used ids. Very unlikely to happen.
             And if it does, there is probably not much to be done.
             It is just good design not to allow a theoretical infinite loop. */
          g_error ("%s: out of ids!", G_STRFUNC);
          break;
        }
    }
  while (gimp_id_table_lookup (id_table, new_id));

  return gimp_id_table_insert_with_id (id_table, new_id, data);
}

/**
 * gimp_id_table_insert_with_id:
 * @id_table: An #GimpIdTable
 * @id: The ID to use. Must be greater than 0.
 * @data: The data to associate with the id
 *
 * Insert data in the id table with a specific ID. If data already
 * exists with the given ID, this function fails.
 *
 * Returns: The used ID if successful, -1 if it was already in use.
 **/
gint
gimp_id_table_insert_with_id (GimpIdTable *id_table, gint id, gpointer data)
{
  g_return_val_if_fail (GIMP_IS_ID_TABLE (id_table), 0);
  g_return_val_if_fail (id > 0, 0);

  if (gimp_id_table_lookup (id_table, id))
    return -1;

  g_hash_table_insert (id_table->priv->id_table, GINT_TO_POINTER (id), data);

  return id;
}

/**
 * gimp_id_table_replace:
 * @id_table: An #GimpIdTable
 * @id: The ID to use. Must be greater than 0.
 * @data: The data to insert/replace
 *
 * Replaces (if an item with the given ID exists) or inserts a new
 * entry in the id table.
 **/
void
gimp_id_table_replace (GimpIdTable *id_table, gint id, gpointer data)
{
  g_return_if_fail (GIMP_IS_ID_TABLE (id_table));
  g_return_if_fail (id > 0);

  g_hash_table_replace (id_table->priv->id_table, GINT_TO_POINTER (id), data);
}

/**
 * gimp_id_table_lookup:
 * @id_table: An #GimpIdTable
 * @id: The ID of the data to lookup
 *
 * Lookup data based on ID.
 *
 * Returns: (nullable) (transfer none): The data,
 *          or %NULL if no data with the given ID was found.
 **/
gpointer
gimp_id_table_lookup (GimpIdTable *id_table, gint id)
{
  g_return_val_if_fail (GIMP_IS_ID_TABLE (id_table), NULL);

  return g_hash_table_lookup (id_table->priv->id_table, GINT_TO_POINTER (id));
}


/**
 * gimp_id_table_remove:
 * @id_table: An #GimpIdTable
 * @id: The ID of the data to remove.
 *
 * Remove the data from the table with the given ID.
 *
 * Returns: %TRUE if data with the ID existed and was successfully
 *          removed, %FALSE otherwise.
 **/
gboolean
gimp_id_table_remove (GimpIdTable *id_table, gint id)
{
  g_return_val_if_fail (GIMP_IS_ID_TABLE (id_table), FALSE);

  g_return_val_if_fail (id_table != NULL, FALSE);

  return g_hash_table_remove (id_table->priv->id_table, GINT_TO_POINTER (id));
}
