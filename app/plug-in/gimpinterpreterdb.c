/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpinterpreterdb.c
 * (C) 2005 Manish Singh <yosh@gimp.org>
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

/*
 * The binfmt_misc bits are derived from linux/fs/binfmt_misc.c
 * Copyright (C) 1997 Richard Günther
 */

/*
 * The sh-bang code is derived from linux/fs/binfmt_script.c
 * Copyright (C) 1996  Martin von Löwis
 * original #!-checking implemented by tytso.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"

#include "plug-in-types.h"

#include "gimpinterpreterdb.h"

#include "gimp-intl.h"


#define BUFSIZE 4096


typedef struct _GimpInterpreterMagic GimpInterpreterMagic;

struct _GimpInterpreterMagic
{
  gulong    offset;
  gchar    *magic;
  gchar    *mask;
  guint     size;
  gchar    *program;
};


static void     gimp_interpreter_db_finalize         (GObject            *object);

static void     gimp_interpreter_db_load_interp_file (GimpInterpreterDB  *db,
                                                      GFile              *file);

static void     gimp_interpreter_db_add_program      (GimpInterpreterDB  *db,
                                                      GFile              *file,
                                                      gchar              *buffer);
static void     gimp_interpreter_db_add_binfmt_misc  (GimpInterpreterDB  *db,
                                                      GFile              *file,
                                                      gchar              *buffer);

static gboolean gimp_interpreter_db_add_extension    (GFile              *file,
                                                      GimpInterpreterDB  *db,
                                                      gchar             **tokens);
static gboolean gimp_interpreter_db_add_magic        (GimpInterpreterDB  *db,
                                                      gchar             **tokens);

static void     gimp_interpreter_db_clear_magics     (GimpInterpreterDB  *db);

static void     gimp_interpreter_db_resolve_programs (GimpInterpreterDB  *db);


G_DEFINE_TYPE (GimpInterpreterDB, gimp_interpreter_db, G_TYPE_OBJECT)

#define parent_class gimp_interpreter_db_parent_class


static void
gimp_interpreter_db_class_init (GimpInterpreterDBClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = gimp_interpreter_db_finalize;
}

static void
gimp_interpreter_db_init (GimpInterpreterDB *db)
{
}

static void
gimp_interpreter_db_finalize (GObject *object)
{
  GimpInterpreterDB *db = GIMP_INTERPRETER_DB (object);

  gimp_interpreter_db_clear (db);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GimpInterpreterDB *
gimp_interpreter_db_new (gboolean verbose)
{
  GimpInterpreterDB *db = g_object_new (GIMP_TYPE_INTERPRETER_DB, NULL);

  db->verbose = verbose;

  return db;
}

void
gimp_interpreter_db_load (GimpInterpreterDB *db,
                          GList             *path)
{
  GList *list;

  g_return_if_fail (GIMP_IS_INTERPRETER_DB (db));

  gimp_interpreter_db_clear (db);

  db->programs = g_hash_table_new_full (g_str_hash, g_str_equal,
                                        g_free, g_free);

  db->extensions = g_hash_table_new_full (g_str_hash, g_str_equal,
                                          g_free, g_free);

  db->magic_names = g_hash_table_new_full (g_str_hash, g_str_equal,
                                           g_free, NULL);

  db->extension_names = g_hash_table_new_full (g_str_hash, g_str_equal,
                                               g_free, NULL);

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

                  gimp_interpreter_db_load_interp_file (db, file);

                  g_object_unref (file);
                }

              g_object_unref (info);
            }

          g_object_unref (enumerator);
        }
    }

  gimp_interpreter_db_resolve_programs (db);
}

void
gimp_interpreter_db_clear (GimpInterpreterDB *db)
{
  g_return_if_fail (GIMP_IS_INTERPRETER_DB (db));

  if (db->magic_names)
    {
      g_hash_table_destroy (db->magic_names);
      db->magic_names = NULL;
    }

  if (db->extension_names)
    {
      g_hash_table_destroy (db->extension_names);
      db->extension_names = NULL;
    }

  if (db->programs)
    {
      g_hash_table_destroy (db->programs);
      db->programs = NULL;
    }

  if (db->extensions)
    {
      g_hash_table_destroy (db->extensions);
      db->extensions = NULL;
    }

  gimp_interpreter_db_clear_magics (db);
}

static void
gimp_interpreter_db_load_interp_file (GimpInterpreterDB *db,
                                      GFile             *file)
{
  GInputStream     *input;
  GDataInputStream *data_input;
  gchar            *buffer;
  gsize             buffer_len;
  GError           *error = NULL;

  if (db->verbose)
    g_print ("Parsing '%s'\n", gimp_file_get_utf8_name (file));

  input = G_INPUT_STREAM (g_file_read (file, NULL, &error));
  if (! input)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_file_get_utf8_name (file),
                 error->message);
      g_clear_error (&error);
      return;
    }

  data_input = g_data_input_stream_new (input);
  g_object_unref (input);

  while ((buffer = g_data_input_stream_read_line (data_input, &buffer_len,
                                                  NULL, &error)))
    {
      /* Skip comments */
      if (buffer[0] == '#')
        {
          g_free (buffer);
          continue;
        }

      if (g_ascii_isalnum (buffer[0]) || (buffer[0] == '/'))
        {
          gimp_interpreter_db_add_program (db, file, buffer);
        }
      else if (! g_ascii_isspace (buffer[0]) && (buffer[0] != '\0'))
        {
          gimp_interpreter_db_add_binfmt_misc (db, file, buffer);
        }

      g_free (buffer);
    }

  if (error)
    {
      g_message (_("Error reading '%s': %s"),
                 gimp_file_get_utf8_name (file),
                 error->message);
      g_clear_error (&error);
    }

  g_object_unref (data_input);
}

static void
gimp_interpreter_db_add_program (GimpInterpreterDB  *db,
                                 GFile              *file,
                                 gchar              *buffer)
{
  gchar *name;
  gchar *program;
  gchar *p;

  p = strchr (buffer, '=');
  if (! p)
    return;

  *p = '\0';

  name = buffer;
  program = p + 1;

  if (! g_file_test (program, G_FILE_TEST_IS_EXECUTABLE))
    {
      gchar *prog;

      prog = g_find_program_in_path (program);
      if (! prog || ! g_file_test (prog, G_FILE_TEST_IS_EXECUTABLE))
        {
          g_message (_("Bad interpreter referenced in interpreter file %s: %s"),
                     gimp_file_get_utf8_name (file),
                     gimp_filename_to_utf8 (program));
          if (prog)
            g_free (prog);
          return;
        }
      program = prog;
    }
  else
    program = g_strdup (program);

  if (! g_hash_table_lookup (db->programs, name))
    g_hash_table_insert (db->programs, g_strdup (name), program);
}

static void
gimp_interpreter_db_add_binfmt_misc (GimpInterpreterDB *db,
                                     GFile             *file,
                                     gchar             *buffer)
{
  gchar **tokens = NULL;
  gchar  *name, *type, *program;
  gsize   count;
  gchar   del[2];

  count = strlen (buffer);

  if ((count < 10) || (count > 255))
    goto bail;

  buffer = g_strndup (buffer, count + 9);

  del[0] = *buffer;
  del[1] = '\0';

  memset (buffer + count, del[0], 8);

  tokens = g_strsplit (buffer + 1, del, -1);

  g_free (buffer);

  name    = tokens[0];
  type    = tokens[1];
  program = tokens[5];

  if ((name[0] == '\0') || (program[0] == '\0') ||
      (type[0] == '\0') || (type[1] != '\0'))
    goto bail;

  switch (type[0])
    {
    case 'E':
      if (! gimp_interpreter_db_add_extension (file, db, tokens))
        goto bail;
      break;

    case 'M':
      if (! gimp_interpreter_db_add_magic (db, tokens))
        goto bail;
      break;

    default:
      goto bail;
    }

  goto out;

bail:
  g_message (_("Bad binary format string in interpreter file %s"),
             gimp_file_get_utf8_name (file));

out:
  g_strfreev (tokens);
}

static gboolean
gimp_interpreter_db_add_extension (GFile              *file,
                                   GimpInterpreterDB  *db,
                                   gchar             **tokens)
{
  const gchar *name      = tokens[0];
  const gchar *extension = tokens[3];
  const gchar *program   = tokens[5];

  if (! g_hash_table_lookup (db->extension_names, name))
    {
      gchar *prog;

      if (extension[0] == '\0' || extension[0] == '/')
        return FALSE;

      if (! g_file_test (program, G_FILE_TEST_IS_EXECUTABLE))
        {
          prog = g_find_program_in_path (program);
          if (! prog || ! g_file_test (prog, G_FILE_TEST_IS_EXECUTABLE))
            {
              g_message (_("Bad interpreter referenced in interpreter file %s: %s"),
                         gimp_file_get_utf8_name (file),
                         gimp_filename_to_utf8 (program));
              if (prog)
                g_free (prog);
              return FALSE;
            }
        }
      else
        prog = g_strdup (program);

      g_hash_table_insert (db->extensions, g_strdup (extension), prog);
      g_hash_table_insert (db->extension_names, g_strdup (name), prog);
    }

  return TRUE;
}

static gboolean
scanarg (const gchar *s)
{
  gchar c;

  while ((c = *s++) != '\0')
    {
      if (c == '\\' && *s == 'x')
        {
          s++;

          if (! g_ascii_isxdigit (*s++))
            return FALSE;

          if (! g_ascii_isxdigit (*s++))
            return FALSE;
        }
   }

  return TRUE;
}

static guint
unquote (gchar *from)
{
  gchar *s = from;
  gchar *p = from;
  gchar  c;

  while ((c = *s++) != '\0')
    {
      if (c == '\\' && *s == 'x')
        {
          s++;
          *p = g_ascii_xdigit_value (*s++) << 4;
          *p++ |= g_ascii_xdigit_value (*s++);
          continue;
        }

      *p++ = c;
    }

  return p - from;
}

static gboolean
gimp_interpreter_db_add_magic (GimpInterpreterDB  *db,
                               gchar             **tokens)
{
  GimpInterpreterMagic *interp_magic;
  gchar                *name, *num, *magic, *mask, *program;
  gulong                offset;
  guint                 size;

  name    = tokens[0];
  num     = tokens[2];
  magic   = tokens[3];
  mask    = tokens[4];
  program = tokens[5];

  if (! g_hash_table_lookup (db->magic_names, name))
    {
      if (num[0] != '\0')
        {
          offset = strtoul (num, &num, 10);

          if (num[0] != '\0')
            return FALSE;

          if (offset > (BUFSIZE / 4))
            return FALSE;
        }
      else
        {
          offset = 0;
        }

      if (! scanarg (magic))
        return FALSE;

      if (! scanarg (mask))
        return FALSE;

      size = unquote (magic);

      if ((size + offset) > (BUFSIZE / 2))
        return FALSE;

      if (mask[0] == '\0')
        mask = NULL;
      else if (unquote (mask) != size)
        return FALSE;

      interp_magic = g_slice_new (GimpInterpreterMagic);

      interp_magic->offset  = offset;
      interp_magic->magic   = g_memdup (magic, size);
      interp_magic->mask    = g_memdup (mask, size);
      interp_magic->size    = size;
      interp_magic->program = g_strdup (program);

      db->magics = g_slist_append (db->magics, interp_magic);

      g_hash_table_insert (db->magic_names, g_strdup (name), interp_magic);
    }

  return TRUE;
}

static void
gimp_interpreter_db_clear_magics (GimpInterpreterDB *db)
{
  GimpInterpreterMagic *magic;
  GSList               *list, *last;

  list = db->magics;
  db->magics = NULL;

  while (list)
    {
      magic = list->data;

      g_free (magic->magic);
      g_free (magic->mask);
      g_free (magic->program);

      g_slice_free (GimpInterpreterMagic, magic);

      last = list;
      list = list->next;

      g_slist_free_1 (last);
    }
}

#ifdef INTERP_DEBUG
static void
print_kv (gpointer key,
          gpointer value,
          gpointer user_data)
{
  g_print ("%s: %s\n", (gchar *) key, (gchar *) value);
}

static gchar *
quote (gchar *s,
       guint  size)
{
  GString *d;
  guint    i;

  if (s == NULL)
    return "(null)";

  d = g_string_sized_new (size * 4);

  for (i = 0; i < size; i++)
    g_string_append_printf (d, "\\x%02x", ((guint) s[i]) & 0xff);

  return g_string_free (d, FALSE);
}
#endif

static gboolean
resolve_program (gpointer key,
                 gpointer value,
                 gpointer user_data)
{
  GimpInterpreterDB *db = user_data;
  gchar             *program;

  program = g_hash_table_lookup (db->programs, value);

  if (program != NULL)
    {
      g_free (value);
      value = g_strdup (program);
    }

  g_hash_table_insert (db->extensions, key, value);

  return TRUE;
}

static void
gimp_interpreter_db_resolve_programs (GimpInterpreterDB *db)
{
  GSList     *list;
  GHashTable *extensions;

  for (list = db->magics; list; list = list->next)
    {
      GimpInterpreterMagic *magic = list->data;
      const gchar          *program;

      program = g_hash_table_lookup (db->programs, magic->program);

      if (program != NULL)
        {
          g_free (magic->program);
          magic->program = g_strdup (program);
        }
    }

  extensions = db->extensions;
  db->extensions = g_hash_table_new_full (g_str_hash, g_str_equal,
                                          g_free, g_free);

  g_hash_table_foreach_steal (extensions, resolve_program, db);

  g_hash_table_destroy (extensions);

#ifdef INTERP_DEBUG
  g_print ("Programs:\n");
  g_hash_table_foreach (db->programs, print_kv, NULL);

  g_print ("\nExtensions:\n");
  g_hash_table_foreach (db->extensions, print_kv, NULL);

  g_print ("\nMagics:\n");

  list = db->magics;

  while (list)
    {
      GimpInterpreterMagic *magic;

      magic = list->data;
      g_print ("program: %s, offset: %lu, magic: %s, mask: %s\n",
               magic->program, magic->offset,
               quote (magic->magic, magic->size),
               quote (magic->mask, magic->size));

      list = list->next;
    }

  g_print ("\n");
#endif
}

static gchar *
resolve_extension (GimpInterpreterDB *db,
                   const gchar       *program_path)
{
  gchar       *filename;
  gchar       *p;
  const gchar *program;

  filename = g_path_get_basename (program_path);

  p = strrchr (filename, '.');
  if (! p)
    {
      g_free (filename);
      return NULL;
    }

  program = g_hash_table_lookup (db->extensions, p + 1);

  g_free (filename);

  return g_strdup (program);
}

static gchar *
resolve_sh_bang (GimpInterpreterDB  *db,
                 const gchar        *program_path,
                 gchar              *buffer,
                 gssize              len,
                 gchar             **interp_arg)
{
  gchar *cp;
  gchar *name;
  gchar *program;

  cp = strchr (buffer, '\n');
  if (! cp)
    cp = buffer + len - 1;

  *cp = '\0';

  while (cp > buffer)
    {
      cp--;
      if ((*cp == ' ') || (*cp == '\t') || (*cp == '\r'))
        *cp = '\0';
      else
        break;
    }

  for (cp = buffer + 2; (*cp == ' ') || (*cp == '\t'); cp++);

  if (*cp == '\0')
    return NULL;

  name = cp;

  for ( ; *cp && (*cp != ' ') && (*cp != '\t'); cp++)
    /* nothing */ ;

  while ((*cp == ' ') || (*cp == '\t'))
    *cp++ = '\0';

  if (*cp)
    {
      if (strcmp ("/usr/bin/env", name) == 0)
        {
          program = g_hash_table_lookup (db->programs, cp);

          if (program)
            {
              /* Shift program name and arguments to the right, if and
               * only if we recorded a specific interpreter for such
               * script. Otherwise let `env` tool do its job.
               */
              name = cp;

              for ( ; *cp && (*cp != ' ') && (*cp != '\t'); cp++)
                ;

              while ((*cp == ' ') || (*cp == '\t'))
                *cp++ = '\0';
            }
        }

      if (*cp)
        *interp_arg = g_strdup (cp);
    }

  program = g_hash_table_lookup (db->programs, name);
  if (! program)
    program = name;

  return g_strdup (program);
}

static gchar *
resolve_magic (GimpInterpreterDB *db,
               const gchar       *program_path,
               gchar             *buffer)
{
  GSList                *list;
  GimpInterpreterMagic  *magic;
  gchar                 *s;
  guint                  i;

  list = db->magics;

  while (list)
    {
      magic = list->data;

      s = buffer + magic->offset;

      if (magic->mask)
        {
          for (i = 0; i < magic->size; i++)
            if ((*s++ ^ magic->magic[i]) & magic->mask[i])
              break;
        }
      else
        {
          for (i = 0; i < magic->size; i++)
            if ((*s++ ^ magic->magic[i]))
              break;
        }

      if (i == magic->size)
        return g_strdup (magic->program);

      list = list->next;
    }

  return NULL;
}

gchar *
gimp_interpreter_db_resolve (GimpInterpreterDB  *db,
                             const gchar        *program_path,
                             gchar             **interp_arg)
{
  GFile        *file;
  GInputStream *input;
  gchar        *program = NULL;

  g_return_val_if_fail (GIMP_IS_INTERPRETER_DB (db), NULL);
  g_return_val_if_fail (program_path != NULL, NULL);
  g_return_val_if_fail (interp_arg != NULL, NULL);

  *interp_arg = NULL;

  file = g_file_new_for_path (program_path);
  input = G_INPUT_STREAM (g_file_read (file, NULL, NULL));
  g_object_unref (file);

  if (input)
    {
      gsize bytes_read;
      gchar buffer[BUFSIZE];

      memset (buffer, 0, sizeof (buffer));
      g_input_stream_read_all (input, buffer,
                               sizeof (buffer) - 1, /* leave one nul at the end */
                               &bytes_read, NULL, NULL);
      g_object_unref (input);

      if (bytes_read)
        {
          if (bytes_read > 3 && buffer[0] == '#' && buffer[1] == '!')
            program = resolve_sh_bang (db, program_path, buffer, bytes_read, interp_arg);

          if (! program)
            program = resolve_magic (db, program_path, buffer);
        }
    }

  if (! program)
    program = resolve_extension (db, program_path);

  return program;
}

static void
collect_extensions (const gchar *ext,
                    const gchar *program G_GNUC_UNUSED,
                    GString     *str)
{
  if (str->len)
    g_string_append_c (str, G_SEARCHPATH_SEPARATOR);

  g_string_append_c (str, '.');
  g_string_append (str, ext);
}

/**
 * gimp_interpreter_db_get_extensions:
 * @db:
 *
 * Return value: a newly allocated string with all registered file
 *               extensions separated by %G_SEARCHPATH_SEPARATOR;
 *               or %NULL if no extensions are registered
 **/
gchar *
gimp_interpreter_db_get_extensions (GimpInterpreterDB *db)
{
  GString *str;

  g_return_val_if_fail (GIMP_IS_INTERPRETER_DB (db), NULL);

  if (g_hash_table_size (db->extensions) == 0)
    return NULL;

  str = g_string_new (NULL);

  g_hash_table_foreach (db->extensions, (GHFunc) collect_extensions, str);

  return g_string_free (str, FALSE);
}
