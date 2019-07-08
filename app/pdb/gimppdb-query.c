/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
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

#include <stdlib.h>
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "pdb-types.h"

#include "core/gimpparamspecs-desc.h"

#include "gimppdb.h"
#include "gimppdb-query.h"
#include "gimppdberror.h"
#include "gimp-pdb-compat.h"
#include "gimpprocedure.h"

#include "gimp-intl.h"


#define PDB_REGEX_FLAGS    (G_REGEX_CASELESS | G_REGEX_OPTIMIZE)

#define COMPAT_BLURB       "This procedure is deprecated! Use '%s' instead."


typedef struct _PDBDump PDBDump;

struct _PDBDump
{
  GimpPDB       *pdb;
  GOutputStream *output;
  GError        *error;

  gboolean       dumping_compat;
};

typedef struct _PDBQuery PDBQuery;

struct _PDBQuery
{
  GimpPDB  *pdb;

  GRegex   *name_regex;
  GRegex   *blurb_regex;
  GRegex   *help_regex;
  GRegex   *author_regex;
  GRegex   *copyright_regex;
  GRegex   *date_regex;
  GRegex   *proc_type_regex;

  gchar   **list_of_procs;
  gint      num_procs;
  gboolean  querying_compat;
};

typedef struct _PDBStrings PDBStrings;

struct _PDBStrings
{
  gboolean  compat;

  gchar    *blurb;
  gchar    *help;
  gchar    *author;
  gchar    *copyright;
  gchar    *date;
};


/*  local function prototypes  */

static void   gimp_pdb_query_entry  (gpointer       key,
                                     gpointer       value,
                                     gpointer       user_data);
static void   gimp_pdb_print_entry  (gpointer       key,
                                     gpointer       value,
                                     gpointer       user_data);
static void   gimp_pdb_get_strings  (PDBStrings    *strings,
                                     GimpProcedure *procedure,
                                     gboolean       compat);
static void   gimp_pdb_free_strings (PDBStrings    *strings);


/*  public functions  */

gboolean
gimp_pdb_dump (GimpPDB  *pdb,
               GFile    *file,
               GError  **error)
{
  PDBDump pdb_dump = { 0, };

  g_return_val_if_fail (GIMP_IS_PDB (pdb), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  pdb_dump.pdb    = pdb;
  pdb_dump.output = G_OUTPUT_STREAM (g_file_replace (file,
                                                     NULL, FALSE,
                                                     G_FILE_CREATE_NONE,
                                                     NULL, error));
  if (! pdb_dump.output)
    return FALSE;

  pdb_dump.dumping_compat = FALSE;

  g_hash_table_foreach (pdb->procedures,
                        gimp_pdb_print_entry,
                        &pdb_dump);

  pdb_dump.dumping_compat = TRUE;

  g_hash_table_foreach (pdb->compat_proc_names,
                        gimp_pdb_print_entry,
                        &pdb_dump);

  if (pdb_dump.error)
    {
      GCancellable *cancellable = g_cancellable_new ();

      g_set_error (error, pdb_dump.error->domain, pdb_dump.error->code,
                   _("Writing PDB file '%s' failed: %s"),
                   gimp_file_get_utf8_name (file), pdb_dump.error->message);
      g_clear_error (&pdb_dump.error);

      /* Cancel the overwrite initiated by g_file_replace(). */
      g_cancellable_cancel (cancellable);
      g_output_stream_close (pdb_dump.output, cancellable, NULL);
      g_object_unref (cancellable);
      g_object_unref (pdb_dump.output);

      return FALSE;
    }

  g_object_unref (pdb_dump.output);

  return TRUE;
}

gboolean
gimp_pdb_query (GimpPDB       *pdb,
                const gchar   *name,
                const gchar   *blurb,
                const gchar   *help,
                const gchar   *author,
                const gchar   *copyright,
                const gchar   *date,
                const gchar   *proc_type,
                gint          *num_procs,
                gchar       ***procs,
                GError       **error)
{
  PDBQuery pdb_query = { 0, };
  gboolean success   = FALSE;

  g_return_val_if_fail (GIMP_IS_PDB (pdb), FALSE);
  g_return_val_if_fail (name != NULL, FALSE);
  g_return_val_if_fail (blurb != NULL, FALSE);
  g_return_val_if_fail (help != NULL, FALSE);
  g_return_val_if_fail (author != NULL, FALSE);
  g_return_val_if_fail (copyright != NULL, FALSE);
  g_return_val_if_fail (date != NULL, FALSE);
  g_return_val_if_fail (proc_type != NULL, FALSE);
  g_return_val_if_fail (num_procs != NULL, FALSE);
  g_return_val_if_fail (procs != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  *num_procs = 0;
  *procs     = NULL;

  pdb_query.name_regex = g_regex_new (name, PDB_REGEX_FLAGS, 0, error);
  if (! pdb_query.name_regex)
    goto cleanup;

  pdb_query.blurb_regex = g_regex_new (blurb, PDB_REGEX_FLAGS, 0, error);
  if (! pdb_query.blurb_regex)
    goto cleanup;

  pdb_query.help_regex = g_regex_new (help, PDB_REGEX_FLAGS, 0, error);
  if (! pdb_query.help_regex)
    goto cleanup;

  pdb_query.author_regex = g_regex_new (author, PDB_REGEX_FLAGS, 0, error);
  if (! pdb_query.author_regex)
    goto cleanup;

  pdb_query.copyright_regex = g_regex_new (copyright, PDB_REGEX_FLAGS, 0, error);
  if (! pdb_query.copyright_regex)
    goto cleanup;

  pdb_query.date_regex = g_regex_new (date, PDB_REGEX_FLAGS, 0, error);
  if (! pdb_query.date_regex)
    goto cleanup;

  pdb_query.proc_type_regex = g_regex_new (proc_type, PDB_REGEX_FLAGS, 0, error);
  if (! pdb_query.proc_type_regex)
    goto cleanup;

  success = TRUE;

  pdb_query.pdb             = pdb;
  pdb_query.list_of_procs   = NULL;
  pdb_query.num_procs       = 0;
  pdb_query.querying_compat = FALSE;

  g_hash_table_foreach (pdb->procedures,
                        gimp_pdb_query_entry, &pdb_query);

  pdb_query.querying_compat = TRUE;

  g_hash_table_foreach (pdb->compat_proc_names,
                        gimp_pdb_query_entry, &pdb_query);

 cleanup:

  if (pdb_query.proc_type_regex)
    g_regex_unref (pdb_query.proc_type_regex);

  if (pdb_query.date_regex)
    g_regex_unref (pdb_query.date_regex);

  if (pdb_query.copyright_regex)
    g_regex_unref (pdb_query.copyright_regex);

  if (pdb_query.author_regex)
    g_regex_unref (pdb_query.author_regex);

  if (pdb_query.help_regex)
    g_regex_unref (pdb_query.help_regex);

  if (pdb_query.blurb_regex)
    g_regex_unref (pdb_query.blurb_regex);

  if (pdb_query.name_regex)
    g_regex_unref (pdb_query.name_regex);

  if (success)
    {
      *num_procs = pdb_query.num_procs;
      *procs     = pdb_query.list_of_procs;
    }

  return success;
}

gboolean
gimp_pdb_proc_info (GimpPDB          *pdb,
                    const gchar      *proc_name,
                    gchar           **blurb,
                    gchar           **help,
                    gchar           **author,
                    gchar           **copyright,
                    gchar           **date,
                    GimpPDBProcType  *proc_type,
                    gint             *num_args,
                    gint             *num_values,
                    GError          **error)
{
  GimpProcedure *procedure;
  PDBStrings     strings;

  g_return_val_if_fail (GIMP_IS_PDB (pdb), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  procedure = gimp_pdb_lookup_procedure (pdb, proc_name);

  if (procedure)
    {
      gimp_pdb_get_strings (&strings, procedure, FALSE);
    }
  else
    {
      const gchar *compat_name;

      compat_name = gimp_pdb_lookup_compat_proc_name (pdb, proc_name);

      if (compat_name)
        {
          procedure = gimp_pdb_lookup_procedure (pdb, compat_name);

          if (procedure)
            gimp_pdb_get_strings (&strings, procedure, TRUE);
        }
    }

  if (procedure)
    {
      *blurb      = strings.compat ? strings.blurb : g_strdup (strings.blurb);
      *help       = strings.compat ? strings.help : g_strdup (strings.help);
      *author     = strings.compat ? strings.author : g_strdup (strings.author);
      *copyright  = strings.compat ? strings.copyright : g_strdup (strings.copyright);
      *date       = strings.compat ? strings.date : g_strdup (strings.date);
      *proc_type  = procedure->proc_type;
      *num_args   = procedure->num_args;
      *num_values = procedure->num_values;

      return TRUE;
    }

  g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
               _("Procedure '%s' not found"), proc_name);

  return FALSE;
}


/*  private functions  */

static gboolean
match_string (GRegex      *regex,
              const gchar *string)
{
  if (! string)
    string = "";

  return g_regex_match (regex, string, 0, NULL);
}

static void
gimp_pdb_query_entry (gpointer key,
                      gpointer value,
                      gpointer user_data)
{
  PDBQuery      *pdb_query = user_data;
  GList         *list;
  GimpProcedure *procedure;
  const gchar   *proc_name;
  PDBStrings     strings;
  GEnumClass    *enum_class;
  GimpEnumDesc  *type_desc;

  proc_name = key;

  if (pdb_query->querying_compat)
    list = g_hash_table_lookup (pdb_query->pdb->procedures, value);
  else
    list = value;

  if (! list)
    return;

  procedure = list->data;

  gimp_pdb_get_strings (&strings, procedure, pdb_query->querying_compat);

  enum_class = g_type_class_ref (GIMP_TYPE_PDB_PROC_TYPE);
  type_desc = gimp_enum_get_desc (enum_class, procedure->proc_type);
  g_type_class_unref  (enum_class);

  if (match_string (pdb_query->name_regex,      proc_name)         &&
      match_string (pdb_query->blurb_regex,     strings.blurb)     &&
      match_string (pdb_query->help_regex,      strings.help)      &&
      match_string (pdb_query->author_regex,    strings.author)    &&
      match_string (pdb_query->copyright_regex, strings.copyright) &&
      match_string (pdb_query->date_regex,      strings.date)      &&
      match_string (pdb_query->proc_type_regex, type_desc->value_desc))
    {
      pdb_query->num_procs++;
      pdb_query->list_of_procs = g_renew (gchar *, pdb_query->list_of_procs,
                                          pdb_query->num_procs);
      pdb_query->list_of_procs[pdb_query->num_procs - 1] = g_strdup (proc_name);
    }

  gimp_pdb_free_strings (&strings);
}

/* #define DEBUG_OUTPUT 1 */

static void
output_string (GString     *dest,
               const gchar *string)
{
#ifndef DEBUG_OUTPUT
  g_string_append_printf (dest, "\"");
#endif

  if (string)
    while (*string)
      {
        switch (*string)
          {
          case '\\' : g_string_append_printf (dest, "\\\\"); break;
          case '\"' : g_string_append_printf (dest, "\\\""); break;
          case '{'  : g_string_append_printf (dest, "@{");   break;
          case '@'  : g_string_append_printf (dest, "@@");   break;
          case '}'  : g_string_append_printf (dest, "@}");   break;

          default:
            g_string_append_printf (dest, "%c", *string);
          }
        string++;
      }

#ifndef DEBUG_OUTPUT
  g_string_append_printf (dest, "\"\n");
#endif
}

static void
gimp_pdb_print_entry (gpointer key,
                      gpointer value,
                      gpointer user_data)
{
  PDBDump       *pdb_dump = user_data;
  GOutputStream *output   = pdb_dump->output;
  const gchar   *proc_name;
  GList         *list;
  GEnumClass    *arg_class;
  GEnumClass    *proc_class;
  GString       *buf;
  GString       *string;
  gint           num = 0;

  if (pdb_dump->error)
    return;

  proc_name = key;

  if (pdb_dump->dumping_compat)
    list = g_hash_table_lookup (pdb_dump->pdb->procedures, value);
  else
    list = value;

  arg_class  = g_type_class_ref (GIMP_TYPE_PDB_ARG_TYPE);
  proc_class = g_type_class_ref (GIMP_TYPE_PDB_PROC_TYPE);

  buf    = g_string_new (NULL);
  string = g_string_new (NULL);

  for (; list; list = list->next)
    {
      GimpProcedure *procedure = list->data;
      PDBStrings     strings;
      GEnumValue    *arg_value;
      GimpEnumDesc  *type_desc;
      gint           i;

      num++;

      gimp_pdb_get_strings (&strings, procedure, pdb_dump->dumping_compat);

#ifdef DEBUG_OUTPUT
      g_string_append_printf (string, "(");
#else
      g_string_append_printf (string, "(register-procedure ");
#endif

      if (num != 1)
        {
          g_string_printf (buf, "%s <%d>", proc_name, num);
          output_string (string, buf->str);
        }
      else
        {
          output_string (string, proc_name);
        }

      type_desc = gimp_enum_get_desc (proc_class, procedure->proc_type);

#ifdef DEBUG_OUTPUT

      g_string_append_printf (string, " (");

      for (i = 0; i < procedure->num_args; i++)
        {
          GParamSpec     *pspec = procedure->args[i];
          GimpPDBArgType  arg_type;

          arg_type = gimp_pdb_compat_arg_type_from_gtype (pspec->value_type);

          arg_value = g_enum_get_value (arg_class, arg_type);

          if (i > 0)
            g_string_append_printf (string, " ");

          output_string (string, arg_value->value_name);
        }

      g_string_append_printf (string, ") (");

      for (i = 0; i < procedure->num_values; i++)
        {
          GParamSpec     *pspec = procedure->values[i];
          GimpPDBArgType  arg_type;

          arg_type = gimp_pdb_compat_arg_type_from_gtype (pspec->value_type);

          arg_value = g_enum_get_value (arg_class, arg_type);

          if (i > 0)
            g_string_append_printf (string, " ");

          output_string (string, arg_value->value_name);
        }

      g_string_append_printf (string, "))\n");

#else /* ! DEBUG_OUTPUT */

      g_string_append_printf (string, "  ");
      output_string (string, strings.blurb);

      g_string_append_printf (string, "  ");
      output_string (string, strings.help);

      g_string_append_printf (string, "  ");
      output_string (string, strings.author);

      g_string_append_printf (string, "  ");
      output_string (string, strings.copyright);

      g_string_append_printf (string, "  ");
      output_string (string, strings.date);

      g_string_append_printf (string, "  ");
      output_string (string, type_desc->value_desc);

      g_string_append_printf (string, "  (");

      for (i = 0; i < procedure->num_args; i++)
        {
          GParamSpec     *pspec = procedure->args[i];
          GimpPDBArgType  arg_type;
          gchar          *desc  = gimp_param_spec_get_desc (pspec);

          g_string_append_printf (string, "\n    (\n");

          arg_type = gimp_pdb_compat_arg_type_from_gtype (pspec->value_type);

          arg_value = g_enum_get_value (arg_class, arg_type);

          g_string_append_printf (string, "      ");
          output_string (string, g_param_spec_get_name (pspec));

          g_string_append_printf (string, "      ");
          output_string (string, arg_value->value_name);

          g_string_append_printf (string, "      ");
          output_string (string, desc);

          g_free (desc);

          g_string_append_printf (string, "    )");
        }

      g_string_append_printf (string, "\n  )\n");

      g_string_append_printf (string, "  (");

      for (i = 0; i < procedure->num_values; i++)
        {
          GParamSpec     *pspec = procedure->values[i];
          GimpPDBArgType  arg_type;
          gchar          *desc  = gimp_param_spec_get_desc (pspec);

          g_string_append_printf (string, "\n    (\n");

          arg_type = gimp_pdb_compat_arg_type_from_gtype (pspec->value_type);

          arg_value = g_enum_get_value (arg_class, arg_type);

          g_string_append_printf (string, "      ");
          output_string (string, g_param_spec_get_name (pspec));

          g_string_append_printf (string, "      ");
          output_string (string, arg_value->value_name);

          g_string_append_printf (string, "      ");
          output_string (string, desc);

          g_free (desc);

          g_string_append_printf (string, "    )");
        }

      g_string_append_printf (string, "\n  )");
      g_string_append_printf (string, "\n)\n");

#endif /* DEBUG_OUTPUT */

      gimp_pdb_free_strings (&strings);
    }

  g_output_stream_write_all (output, string->str, string->len,
                             NULL, NULL, &pdb_dump->error);

  g_string_free (string, TRUE);
  g_string_free (buf, TRUE);

  g_type_class_unref (arg_class);
  g_type_class_unref (proc_class);
}

static void
gimp_pdb_get_strings (PDBStrings    *strings,
                      GimpProcedure *procedure,
                      gboolean       compat)
{
  strings->compat = compat;

  if (compat)
    {
      strings->blurb     = g_strdup_printf (COMPAT_BLURB,
                                            gimp_object_get_name (procedure));
      strings->help      = g_strdup (strings->blurb);
      strings->author    = NULL;
      strings->copyright = NULL;
      strings->date      = NULL;
    }
  else
    {
      strings->blurb     = procedure->blurb;
      strings->help      = procedure->help;
      strings->author    = procedure->author;
      strings->copyright = procedure->copyright;
      strings->date      = procedure->date;
    }
}

static void
gimp_pdb_free_strings (PDBStrings *strings)
{
  if (strings->compat)
    {
      g_free (strings->blurb);
      g_free (strings->help);
    }
}
