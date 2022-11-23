/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaenvirontable.c
 * (C) 2002 Manish Singh <yosh@ligma.org>
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

#include <gio/gio.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "plug-in-types.h"

#include "ligmaenvirontable.h"

#include "ligma-intl.h"


typedef struct _LigmaEnvironValue LigmaEnvironValue;

struct _LigmaEnvironValue
{
  gchar *value;
  gchar *separator;
};


static void     ligma_environ_table_finalize       (GObject               *object);

static void     ligma_environ_table_load_env_file  (LigmaEnvironTable      *environ_table,
                                                   GFile                 *file);
static gboolean ligma_environ_table_legal_name     (gchar                 *name);

static void     ligma_environ_table_populate       (LigmaEnvironTable      *environ_table);
static void     ligma_environ_table_populate_one   (const gchar           *name,
                                                   LigmaEnvironValue      *val,
                                                   GPtrArray             *env_array);
static gboolean ligma_environ_table_pass_through   (LigmaEnvironTable      *environ_table,
                                                   const gchar           *name);

static void     ligma_environ_table_clear_vars     (LigmaEnvironTable      *environ_table);
static void     ligma_environ_table_clear_internal (LigmaEnvironTable      *environ_table);
static void     ligma_environ_table_clear_envp     (LigmaEnvironTable      *environ_table);

static void     ligma_environ_table_free_value     (LigmaEnvironValue      *val);


G_DEFINE_TYPE (LigmaEnvironTable, ligma_environ_table, G_TYPE_OBJECT)

#define parent_class ligma_environ_table_parent_class


static void
ligma_environ_table_class_init (LigmaEnvironTableClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = ligma_environ_table_finalize;
}

static void
ligma_environ_table_init (LigmaEnvironTable *environ_table)
{
}

static void
ligma_environ_table_finalize (GObject *object)
{
  LigmaEnvironTable *environ_table = LIGMA_ENVIRON_TABLE (object);

  ligma_environ_table_clear_all (environ_table);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

LigmaEnvironTable *
ligma_environ_table_new (gboolean verbose)
{
  LigmaEnvironTable *table = g_object_new (LIGMA_TYPE_ENVIRON_TABLE, NULL);

  table->verbose = verbose;

  return table;
}

static guint
ligma_environ_table_str_hash (gconstpointer v)
{
#ifdef G_OS_WIN32
  gchar *p      = g_ascii_strup ((const gchar *) v, -1);
  guint  retval = g_str_hash (p);

  g_free (p);

  return retval;
#else
  return g_str_hash (v);
#endif
}

static gboolean
ligma_environ_table_str_equal (gconstpointer v1,
                              gconstpointer v2)
{
#ifdef G_OS_WIN32
  gchar    *string1 = g_ascii_strup ((const gchar *) v1, -1);
  gchar    *string2 = g_ascii_strup ((const gchar *) v2, -1);
  gboolean  retval  = g_str_equal (string1, string2);

  g_free (string1);
  g_free (string2);

  return retval;
#else
  return g_str_equal (v1, v2);
#endif
}

void
ligma_environ_table_load (LigmaEnvironTable *environ_table,
                         GList            *path)
{
  GList *list;

  g_return_if_fail (LIGMA_IS_ENVIRON_TABLE (environ_table));

  ligma_environ_table_clear (environ_table);

  environ_table->vars =
    g_hash_table_new_full (ligma_environ_table_str_hash,
                           ligma_environ_table_str_equal,
                           g_free,
                           (GDestroyNotify) ligma_environ_table_free_value);

  for (list = path; list; list = g_list_next (list))
    {
      GFile           *dir = list->data;
      GFileEnumerator *enumerator;

      enumerator =
        g_file_enumerate_children (dir,
                                   G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                   G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                   G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                   G_FILE_QUERY_INFO_NONE,
                                   NULL, NULL);

      if (enumerator)
        {
          GFileInfo *info;

          while ((info = g_file_enumerator_next_file (enumerator,
                                                      NULL, NULL)))
            {
              if (! g_file_info_get_is_hidden (info) &&
                  g_file_info_get_file_type (info) == G_FILE_TYPE_REGULAR)
                {
                  GFile *file = g_file_enumerator_get_child (enumerator, info);

                  ligma_environ_table_load_env_file (environ_table, file);

                  g_object_unref (file);
                }

              g_object_unref (info);
            }

          g_object_unref (enumerator);
        }
    }
}

void
ligma_environ_table_add (LigmaEnvironTable *environ_table,
                        const gchar      *name,
                        const gchar      *value,
                        const gchar      *separator)
{
  LigmaEnvironValue *val;

  g_return_if_fail (LIGMA_IS_ENVIRON_TABLE (environ_table));

  ligma_environ_table_clear_envp (environ_table);

  if (! environ_table->internal)
    environ_table->internal =
      g_hash_table_new_full (g_str_hash, g_str_equal,
                             g_free,
                             (GDestroyNotify) ligma_environ_table_free_value);

  val = g_slice_new (LigmaEnvironValue);

  val->value     = g_strdup (value);
  val->separator = g_strdup (separator);

  g_hash_table_insert (environ_table->internal, g_strdup (name), val);
}

void
ligma_environ_table_remove (LigmaEnvironTable *environ_table,
                           const gchar      *name)
{
  g_return_if_fail (LIGMA_IS_ENVIRON_TABLE (environ_table));

  if (! environ_table->internal)
    return;

  ligma_environ_table_clear_envp (environ_table);

  g_hash_table_remove (environ_table->internal, name);

  if (g_hash_table_size (environ_table->internal) == 0)
    ligma_environ_table_clear_internal (environ_table);
}

void
ligma_environ_table_clear (LigmaEnvironTable *environ_table)
{
  g_return_if_fail (LIGMA_IS_ENVIRON_TABLE (environ_table));

  ligma_environ_table_clear_envp (environ_table);

  ligma_environ_table_clear_vars (environ_table);
}

void
ligma_environ_table_clear_all (LigmaEnvironTable *environ_table)
{
  g_return_if_fail (LIGMA_IS_ENVIRON_TABLE (environ_table));

  ligma_environ_table_clear_envp (environ_table);

  ligma_environ_table_clear_vars (environ_table);
  ligma_environ_table_clear_internal (environ_table);
}

gchar **
ligma_environ_table_get_envp (LigmaEnvironTable *environ_table)
{
  g_return_val_if_fail (LIGMA_IS_ENVIRON_TABLE (environ_table), NULL);

  /* Hmm.. should we return a copy here in the future? Not thread safe atm,
   * but the rest of it isn't either.
   */

  if (! environ_table->envp)
    ligma_environ_table_populate (environ_table);

  return environ_table->envp;
}


/* private */

static void
ligma_environ_table_load_env_file (LigmaEnvironTable *environ_table,
                                  GFile            *file)
{
  GInputStream     *input;
  GDataInputStream *data_input;
  gchar            *buffer;
  gsize             buffer_len;
  GError           *error = NULL;

  if (environ_table->verbose)
    g_print ("Parsing '%s'\n", ligma_file_get_utf8_name (file));

  input = G_INPUT_STREAM (g_file_read (file, NULL, &error));
  if (! input)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 ligma_file_get_utf8_name (file),
                 error->message);
      g_clear_error (&error);
      return;
    }

  data_input = g_data_input_stream_new (input);
  g_object_unref (input);

  while ((buffer = g_data_input_stream_read_line (data_input, &buffer_len,
                                                  NULL, &error)))
    {
      gchar *name;
      gchar *value;
      gchar *separator;
      gchar *p;
      gchar *q;

      /* Skip comments */
      if (buffer[0] == '#')
        {
          g_free (buffer);
          continue;
        }

      p = strchr (buffer, '=');
      if (! p)
        {
          g_free (buffer);
          continue;
        }

      *p = '\0';

      name = buffer;
      value = p + 1;

      if (name[0] == '\0')
        {
          g_message (_("Empty variable name in environment file %s"),
                     ligma_file_get_utf8_name (file));
          g_free (buffer);
          continue;
        }

      separator = NULL;

      q = strchr (name, ' ');
      if (q)
        {
          *q = '\0';

          separator = name;
          name = q + 1;
        }

      if (! ligma_environ_table_legal_name (name))
        {
          g_message (_("Illegal variable name in environment file %s: %s"),
                     ligma_file_get_utf8_name (file), name);
          g_free (buffer);
          continue;
        }

      if (! g_hash_table_lookup (environ_table->vars, name))
        {
          LigmaEnvironValue *val = g_slice_new (LigmaEnvironValue);

          val->value     = ligma_config_path_expand (value, FALSE, NULL);
          val->separator = g_strdup (separator);

          g_hash_table_insert (environ_table->vars, g_strdup (name), val);
        }

      g_free (buffer);
    }

  if (error)
    {
      g_message (_("Error reading '%s': %s"),
                 ligma_file_get_utf8_name (file),
                 error->message);
      g_clear_error (&error);
    }

  g_object_unref (data_input);
}

static gboolean
ligma_environ_table_legal_name (gchar *name)
{
  gchar *s;

  if (! g_ascii_isalpha (*name) && (*name != '_'))
    return FALSE;

  for (s = name + 1; *s; s++)
    if (! g_ascii_isalnum (*s) && (*s != '_'))
      return FALSE;

  return TRUE;
}

static void
ligma_environ_table_populate (LigmaEnvironTable *environ_table)
{
  gchar     **env = g_listenv ();
  gchar     **var;
  GPtrArray  *env_array;

  var = env;
  env_array = g_ptr_array_new ();

  while (*var)
    {
      /* g_listenv() only returns the names of environment variables
       * that are correctly specified (name=value) in the environ
       * array (Unix) or the process environment string table (Win32).
       */

      if (ligma_environ_table_pass_through (environ_table, *var))
        g_ptr_array_add (env_array, g_strconcat (*var, "=", g_getenv (*var), NULL));

      var++;
    }

  g_strfreev (env);

  if (environ_table->vars)
    g_hash_table_foreach (environ_table->vars,
                          (GHFunc) ligma_environ_table_populate_one,
                          env_array);

  if (environ_table->internal)
    g_hash_table_foreach (environ_table->internal,
                          (GHFunc) ligma_environ_table_populate_one,
                          env_array);

  g_ptr_array_add (env_array, NULL);

  environ_table->envp = (gchar **) g_ptr_array_free (env_array, FALSE);

#ifdef ENVP_DEBUG
  var = environ_table->envp;

  g_print ("LigmaEnvironTable:\n");
  while (*var)
    {
      g_print ("%s\n", *var);
      var++;
    }
#endif /* ENVP_DEBUG */
}

static void
ligma_environ_table_populate_one (const gchar      *name,
                                 LigmaEnvironValue *val,
                                 GPtrArray        *env_array)
{
  const gchar *old;
  gchar *var = NULL;

  if (val->separator)
    {
      old = g_getenv (name);

      if (old)
        var = g_strconcat (name, "=", val->value, val->separator, old, NULL);
    }

  if (! var)
    var = g_strconcat (name, "=", val->value, NULL);

  g_ptr_array_add (env_array, var);
}

static gboolean
ligma_environ_table_pass_through (LigmaEnvironTable *environ_table,
                                 const gchar      *name)
{
  gboolean vars, internal;

  vars = environ_table->vars &&
         g_hash_table_lookup (environ_table->vars, name);

  internal = environ_table->internal &&
             g_hash_table_lookup (environ_table->internal, name);

  return (!vars && !internal);
}

static void
ligma_environ_table_clear_vars (LigmaEnvironTable *environ_table)
{
  if (environ_table->vars)
    {
      g_hash_table_destroy (environ_table->vars);
      environ_table->vars = NULL;
    }
}

static void
ligma_environ_table_clear_internal (LigmaEnvironTable *environ_table)
{
  if (environ_table->internal)
    {
      g_hash_table_destroy (environ_table->internal);
      environ_table->internal = NULL;
    }
}

static void
ligma_environ_table_clear_envp (LigmaEnvironTable *environ_table)
{
  if (environ_table->envp)
    {
      g_strfreev (environ_table->envp);
      environ_table->envp = NULL;
    }
}

static void
ligma_environ_table_free_value (LigmaEnvironValue *val)
{
  g_free (val->value);
  g_free (val->separator);

  g_slice_free (LigmaEnvironValue, val);
}
