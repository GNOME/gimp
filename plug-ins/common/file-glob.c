/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/* The idea is taken from a plug-in written by George Hartz; the code isn't.
 */

#include "config.h"

#include <string.h>

#include "libgimp/gimp.h"


#define PLUG_IN_PROC "file-glob"


typedef struct _Glob      Glob;
typedef struct _GlobClass GlobClass;

struct _Glob
{
  GimpPlugIn parent_instance;
};

struct _GlobClass
{
  GimpPlugInClass parent_class;
};


#define GLOB_TYPE (glob_get_type ())
#define GLOB(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GLOB_TYPE, Glob))

GType                   glob_get_type         (void) G_GNUC_CONST;

static GList          * glob_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * glob_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray * glob_run              (GimpProcedure        *procedure,
                                               GimpProcedureConfig  *config,
                                               gpointer              run_data);

static gboolean         glob_match            (const gchar          *pattern,
                                               gboolean              filename_encoding,
                                               gchar              ***matches);
static gboolean         glob_fnmatch          (const gchar          *pattern,
                                               const gchar          *string);


G_DEFINE_TYPE (Glob, glob, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (GLOB_TYPE)


static void
glob_class_init (GlobClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = glob_query_procedures;
  plug_in_class->create_procedure = glob_create_procedure;
  plug_in_class->set_i18n         = NULL;
}

static void
glob_init (Glob *glob)
{
}

static GList *
glob_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
glob_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_procedure_new (plug_in, name,
                                      GIMP_PDB_PROC_TYPE_PLUGIN,
                                      glob_run, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Returns a list of matching filenames",
                                        "This can be useful in scripts and "
                                        "other plug-ins (e.g., "
                                        "batch-conversion). See the glob(7) "
                                        "manpage for more info. Note however "
                                        "that this isn't a full-featured glob "
                                        "implementation. It only handles "
                                        "simple patterns like "
                                        "\"/home/foo/bar/*.jpg\".",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Sven Neumann",
                                      "Sven Neumann",
                                      "2004");

      gimp_procedure_add_string_argument (procedure, "pattern",
                                          "Pattern",
                                          "The glob pattern (in UTF-8 encoding)",
                                          NULL,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "filename-encoding",
                                           "Filename encoding",
                                           "FALSE to return UTF-8 strings, TRUE to return "
                                           "strings in filename encoding",
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_string_array_return_value (procedure, "files",
                                                    "Files",
                                                    "The list of matching filenames",
                                                    G_PARAM_READWRITE |
                                                    GIMP_PARAM_NO_VALIDATE);
    }

  return procedure;
}

static GimpValueArray *
glob_run (GimpProcedure        *procedure,
          GimpProcedureConfig  *config,
          gpointer              run_data)
{
  GimpValueArray *return_vals;
  gchar          *pattern;
  gboolean        filename_encoding;
  gchar         **matches;

  g_object_get (config,
                "pattern",           &pattern,
                "filename-encoding", &filename_encoding,
                NULL);

  if (! glob_match (pattern, filename_encoding, &matches))
    {
      return_vals = gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               NULL);
    }
  else
    {
      return_vals = gimp_procedure_new_return_values (procedure,
                                                      GIMP_PDB_SUCCESS,
                                                      NULL);
      GIMP_VALUES_TAKE_STRV (return_vals, 1, matches);
    }
  g_free (pattern);

  return return_vals;
}

static gboolean
glob_match (const gchar   *pattern,
            gboolean       filename_encoding,
            gchar       ***matches)
{
  GDir        *dir;
  GPtrArray   *array;
  const gchar *filename;
  gchar       *dirname;
  gchar       *tmp;

  g_return_val_if_fail (pattern != NULL, FALSE);
  g_return_val_if_fail (matches != NULL, FALSE);

  *matches = NULL;

  /*  This is not a complete glob() implementation but rather a very
   *  simplistic approach. However it works for the most common use
   *  case and is better than nothing.
   */

  tmp = g_filename_from_utf8 (pattern, -1, NULL, NULL, NULL);
  if (! tmp)
    return FALSE;

  dirname = g_path_get_dirname (tmp);

  dir = g_dir_open (dirname, 0, NULL);
  g_free (tmp);

  if (! dir)
    {
      g_free (dirname);
      return TRUE;
    }

  /*  check if the pattern has a directory part at all  */
  tmp = g_path_get_basename (pattern);
  if (strcmp (pattern, tmp) == 0)
    {
      g_free (dirname);
      dirname = NULL;
    }
  g_free (tmp);

  array = g_ptr_array_new ();

  for (filename = g_dir_read_name (dir);
       filename;
       filename = g_dir_read_name (dir))
    {
      gchar *path;
      gchar *name;

      if (dirname)
        path = g_build_filename (dirname, filename, NULL);
      else
        path = g_strdup (filename);

      name = g_filename_to_utf8 (path, -1, NULL, NULL, NULL);

      if (name && glob_fnmatch (pattern, name))
        {
          if (filename_encoding)
            {
              g_ptr_array_add (array, path);
              path = NULL;
            }
          else
            {
              g_ptr_array_add (array, name);
              name = NULL;
            }
        }

      g_free (path);
      g_free (name);
    }

  g_dir_close (dir);
  g_free (dirname);

  /* NULL-terminator */
  g_ptr_array_add (array, NULL);

  *matches = (gchar **) g_ptr_array_free (array, FALSE);

  return TRUE;
}


/*
 * The following code is borrowed from GTK+.
 *
 * GTK+ used to use a old version of GNU fnmatch() that was buggy
 * in various ways and didn't handle UTF-8. The following is
 * converted to UTF-8. To simplify the process of making it
 * correct, this is special-cased to the combinations of flags
 * that gtkfilesel.c uses.
 *
 *   FNM_FILE_NAME   - always set
 *   FNM_LEADING_DIR - never set
 *   FNM_NOESCAPE    - set only on windows
 *   FNM_CASEFOLD    - set only on windows
 */

/* We need to make sure that all constants are defined
 * to properly compile this file
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

static gunichar
get_char (const char **str)
{
  gunichar c = g_utf8_get_char (*str);
  *str = g_utf8_next_char (*str);

#ifdef G_PLATFORM_WIN32
  c = g_unichar_tolower (c);
#endif

  return c;
}

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)
#define DO_ESCAPE 0
#else
#define DO_ESCAPE 1
#endif

static gunichar
get_unescaped_char (const char **str,
                    gboolean    *was_escaped)
{
  gunichar c = get_char (str);

  *was_escaped = DO_ESCAPE && c == '\\';
  if (*was_escaped)
    c = get_char (str);

  return c;
}

/* Match STRING against the filename pattern PATTERN,
 * returning TRUE if it matches, FALSE otherwise.
 */
static gboolean
fnmatch_intern (const gchar *pattern,
                const gchar *string,
                gboolean     component_start,
                gboolean     no_leading_period)
{
  const char *p = pattern, *n = string;

  while (*p)
    {
      const char *last_n = n;

      gunichar c = get_char (&p);
      gunichar nc = get_char (&n);

      switch (c)
        {
        case '?':
          if (nc == '\0')
            return FALSE;
          else if (nc == G_DIR_SEPARATOR)
            return FALSE;
          else if (nc == '.' && component_start && no_leading_period)
            return FALSE;
          break;
        case '\\':
          if (DO_ESCAPE)
            c = get_char (&p);
          if (nc != c)
            return FALSE;
          break;
        case '*':
          if (nc == '.' && component_start && no_leading_period)
            return FALSE;

          {
            const char *last_p = p;

            for (last_p = p, c = get_char (&p);
                 c == '?' || c == '*';
                 last_p = p, c = get_char (&p))
              {
                if (c == '?')
                  {
                    if (nc == '\0')
                      return FALSE;
                    else if (nc == G_DIR_SEPARATOR)
                      return FALSE;
                    else
                      {
                        last_n = n; nc = get_char (&n);
                      }
                  }
              }

            /* If the pattern ends with wildcards, we have a
             * guaranteed match unless there is a dir separator
             * in the remainder of the string.
             */
            if (c == '\0')
              {
                if (strchr (last_n, G_DIR_SEPARATOR) != NULL)
                  return FALSE;
                else
                  return TRUE;
              }

            if (DO_ESCAPE && c == '\\')
              c = get_char (&p);

            for (p = last_p; nc != '\0';)
              {
                if ((c == '[' || nc == c) &&
                    fnmatch_intern (p, last_n,
                                    component_start, no_leading_period))
                  return TRUE;

                component_start = (nc == G_DIR_SEPARATOR);
                last_n = n;
                nc = get_char (&n);
              }

            return FALSE;
          }

        case '[':
          {
            /* Nonzero if the sense of the character class is inverted.  */
            gboolean not;
            gboolean was_escaped;

            if (nc == '\0' || nc == G_DIR_SEPARATOR)
              return FALSE;

            if (nc == '.' && component_start && no_leading_period)
              return FALSE;

            not = (*p == '!' || *p == '^');
            if (not)
              ++p;

            c = get_unescaped_char (&p, &was_escaped);
            for (;;)
              {
                register gunichar cstart = c, cend = c;
                if (c == '\0')
                  /* [ (unterminated) loses.  */
                  return FALSE;

                c = get_unescaped_char (&p, &was_escaped);

                if (!was_escaped && c == '-' && *p != ']')
                  {
                    cend = get_unescaped_char (&p, &was_escaped);
                    if (cend == '\0')
                      return FALSE;

                    c = get_char (&p);
                  }

                if (nc >= cstart && nc <= cend)
                  goto matched;

                if (!was_escaped && c == ']')
                  break;
              }
            if (!not)
              return FALSE;
            break;

          matched:;
            /* Skip the rest of the [...] that already matched.  */
            /* XXX 1003.2d11 is unclear if was_escaped is right.  */
            while (was_escaped || c != ']')
              {
                if (c == '\0')
                  /* [... (unterminated) loses.  */
                  return FALSE;

                c = get_unescaped_char (&p, &was_escaped);
              }
            if (not)
              return FALSE;
          }
          break;

        default:
          if (c != nc)
            return FALSE;
        }

      component_start = (nc == G_DIR_SEPARATOR);
    }

  if (*n == '\0')
    return TRUE;

  return FALSE;
}

static gboolean
glob_fnmatch (const gchar *pattern,
              const gchar *string)
{
  return fnmatch_intern (pattern, string, TRUE, TRUE);
}
