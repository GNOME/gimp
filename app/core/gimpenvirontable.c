/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpenvirontable.c
 * (C) 2002 Manish Singh <yosh@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpenvirontable.h"


/* FIXME: check how portable this is */
extern char **environ;


static void gimp_environ_table_class_init     (GimpEnvironTableClass *class);
static void gimp_environ_table_init           (GimpEnvironTable      *environ_table);

static void gimp_environ_table_finalize       (GObject               *object);

static void gimp_environ_table_load_env_file  (GimpDatafileData      *file_data);

static void gimp_environ_table_populate       (GimpEnvironTable      *environ_table);
static void gimp_environ_table_populate_one   (const gchar           *name,
                                               const gchar           *value,
                                               GPtrArray             *env_array);
static void gimp_environ_table_clear_vars     (GimpEnvironTable      *environ_table);
static void gimp_environ_table_clear_internal (GimpEnvironTable      *environ_table);
static void gimp_environ_table_clear_envp     (GimpEnvironTable      *environ_table);


static GObjectClass *parent_class = NULL;


GType
gimp_environ_table_get_type (void)
{
  static GType environ_table_type = 0;

  if (! environ_table_type)
    {
      static const GTypeInfo environ_table_info =
      {
        sizeof (GimpEnvironTableClass),
        NULL,                /* base_init */
        NULL,                /* base_finalize */
        (GClassInitFunc) gimp_environ_table_class_init,
        NULL,                /* class_finalize */
        NULL,                /* class_data */
        sizeof (GimpEnvironTable),
        0,                /* n_preallocs */
        (GInstanceInitFunc) gimp_environ_table_init,
      };

      environ_table_type = g_type_register_static (G_TYPE_OBJECT,
                                                   "GimpEnvironTable",
                                                   &environ_table_info, 0);
    }

  return environ_table_type;
}

static void
gimp_environ_table_class_init (GimpEnvironTableClass *class)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  object_class->finalize = gimp_environ_table_finalize;
}

static void
gimp_environ_table_init (GimpEnvironTable *environ_table)
{
  environ_table->vars = NULL;
  environ_table->internal = NULL;

  environ_table->envp = NULL;
}

static void
gimp_environ_table_finalize (GObject *object)
{
  GimpEnvironTable *environ_table;

  environ_table = GIMP_ENVIRON_TABLE (object);

  gimp_environ_table_clear_all (environ_table);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GimpEnvironTable *
gimp_environ_table_new (void)
{
  return g_object_new (GIMP_TYPE_ENVIRON_TABLE, NULL);
}

void
gimp_environ_table_load (GimpEnvironTable *environ_table,
                         const gchar      *env_path)
{
  g_return_if_fail (GIMP_IS_ENVIRON_TABLE (environ_table));

  gimp_environ_table_clear (environ_table);

  environ_table->vars = g_hash_table_new_full (g_str_hash, g_str_equal,
                                               g_free, g_free);

  gimp_datafiles_read_directories (env_path,
                                   G_FILE_TEST_EXISTS,
                                   gimp_environ_table_load_env_file,
                                   environ_table);
}

void
gimp_environ_table_add (GimpEnvironTable *environ_table,
                        const gchar      *name,
                        const gchar      *value)
{
  g_return_if_fail (GIMP_IS_ENVIRON_TABLE (environ_table));

  gimp_environ_table_clear_envp (environ_table);

  if (! environ_table->internal)
    environ_table->internal = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                     g_free, g_free);

  g_hash_table_insert (environ_table->internal,
                       g_strdup (name),
                       g_strdup (value));
}

void
gimp_environ_table_remove (GimpEnvironTable *environ_table,
                           const gchar      *name)
{
  g_return_if_fail (GIMP_IS_ENVIRON_TABLE (environ_table));

  if (! environ_table->internal)
    return;

  gimp_environ_table_clear_envp (environ_table);

  g_hash_table_remove (environ_table->internal, name);

  if (g_hash_table_size (environ_table->internal) == 0)
    gimp_environ_table_clear_internal (environ_table);
}

void
gimp_environ_table_clear (GimpEnvironTable *environ_table)
{
  g_return_if_fail (GIMP_IS_ENVIRON_TABLE (environ_table));

  gimp_environ_table_clear_envp (environ_table);

  gimp_environ_table_clear_vars (environ_table);
}

void
gimp_environ_table_clear_all (GimpEnvironTable *environ_table)
{
  g_return_if_fail (GIMP_IS_ENVIRON_TABLE (environ_table));

  gimp_environ_table_clear_envp (environ_table);

  gimp_environ_table_clear_vars (environ_table);
  gimp_environ_table_clear_internal (environ_table);
}

gchar **
gimp_environ_table_get_envp (GimpEnvironTable *environ_table)
{
  g_return_val_if_fail (GIMP_IS_ENVIRON_TABLE (environ_table), NULL);

  /* Hmm.. should we return a copy here in the future? Not thread safe atm,
   * but, the rest of it isn't either.
   */ 

  if (! environ_table->envp)
    gimp_environ_table_populate (environ_table);

  return environ_table->envp;
}


/* private */

static void
gimp_environ_table_load_env_file (GimpDatafileData *file_data)
{
  GimpEnvironTable *environ_table;
  FILE        *env;
  gchar        buffer[4096];
  gsize        len;
  gchar       *name, *value, *p;

  environ_table = GIMP_ENVIRON_TABLE (file_data->user_data);

  env = fopen (file_data->filename, "r");
  if (! env)
    return;

  while (fgets (buffer, sizeof (buffer), env))
    {
      /* Skip comments */
      if (buffer[0] == '#')
        continue;

      len = strlen (buffer) - 1;

      /* Skip too long lines */
      if (buffer[len] != '\n')
        continue;

      buffer[len] = '\0';

      p = strchr (buffer, '=');
      if (! p)
        continue;

      *p = '\0';

      name = buffer;
      value = p + 1;

      if (! g_hash_table_lookup (environ_table->vars, name))
        g_hash_table_insert (environ_table->vars,
                             g_strdup (name),
                             g_strdup (value));
    }

  fclose (env);
}

static void
gimp_environ_table_populate (GimpEnvironTable *environ_table)
{
  gchar     **var = environ;
  gchar      *name, *p;
  GPtrArray  *env_array;

  env_array = g_ptr_array_new ();

  while (*var)
    {
      p = strchr (*var, '=');

      /* shouldn't happen... but just to be safe... */
      if (p)
        {
          name = g_strndup (*var, p - *var);

          if (! g_hash_table_lookup (environ_table->vars, name))
            g_ptr_array_add (env_array, g_strdup (*var));

          g_free (name);
        }

      var++;
    }

  if (environ_table->vars)
    g_hash_table_foreach (environ_table->vars,
                          (GHFunc) gimp_environ_table_populate_one,
                          env_array);

  if (environ_table->internal)
    g_hash_table_foreach (environ_table->internal,
                          (GHFunc) gimp_environ_table_populate_one,
                          env_array);

  g_ptr_array_add (env_array, NULL);

  environ_table->envp = (gchar **) g_ptr_array_free (env_array, FALSE);

#ifdef ENVP_DEBUG
  var = environ_table->envp;

  g_print ("GimpEnvironTable:\n"); 
  while (*var)
    {
      g_print ("%s\n", *var);
      var++;
    }
#endif /* ENVP_DEBUG */
}

static void
gimp_environ_table_populate_one (const gchar *name,
                                 const gchar *value,
                                 GPtrArray   *env_array)
{
  gchar *var;

  var = g_strconcat (name, "=", value, NULL);
  g_ptr_array_add (env_array, var);
}

static void
gimp_environ_table_clear_vars (GimpEnvironTable *environ_table)
{
  if (environ_table->vars)
    {
      g_hash_table_destroy (environ_table->vars);
      environ_table->vars = NULL;
    }
}

static void
gimp_environ_table_clear_internal (GimpEnvironTable *environ_table)
{
  if (environ_table->internal)
    {
      g_hash_table_destroy (environ_table->internal);
      environ_table->internal = NULL;
    }
}

static void
gimp_environ_table_clear_envp (GimpEnvironTable *environ_table)
{
  if (environ_table->envp)
    {
      g_strfreev (environ_table->envp);
      environ_table->envp = NULL;
    }
}
