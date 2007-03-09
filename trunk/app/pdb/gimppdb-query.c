/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
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

#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_GLIBC_REGEX
#include <regex.h>
#else
#include "regexrepl/regex.h"
#endif

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "pdb-types.h"

#include "core/gimpparamspecs-desc.h"

#include "gimppdb.h"
#include "gimppdb-query.h"
#include "gimp-pdb-compat.h"
#include "gimpprocedure.h"

#include "gimp-intl.h"


#define PDB_REGCOMP_FLAGS  REG_ICASE

#define COMPAT_BLURB       "This procedure is deprecated! Use '%s' instead."


typedef struct _PDBDump PDBDump;

struct _PDBDump
{
  GimpPDB  *pdb;
  FILE     *file;

  gboolean  dumping_compat;
};

typedef struct _PDBQuery PDBQuery;

struct _PDBQuery
{
  GimpPDB  *pdb;

  regex_t   name_regex;
  regex_t   blurb_regex;
  regex_t   help_regex;
  regex_t   author_regex;
  regex_t   copyright_regex;
  regex_t   date_regex;
  regex_t   proc_type_regex;

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
gimp_pdb_dump (GimpPDB     *pdb,
               const gchar *filename)
{
  PDBDump pdb_dump;

  g_return_val_if_fail (GIMP_IS_PDB (pdb), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);

  pdb_dump.pdb  = pdb;
  pdb_dump.file = g_fopen (filename, "w");

  if (! pdb_dump.file)
    return FALSE;

  pdb_dump.dumping_compat = FALSE;

  g_hash_table_foreach (pdb->procedures,
                        gimp_pdb_print_entry,
                        &pdb_dump);

  pdb_dump.dumping_compat = TRUE;

  g_hash_table_foreach (pdb->compat_proc_names,
                        gimp_pdb_print_entry,
                        &pdb_dump);

  fclose (pdb_dump.file);

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
                gchar       ***procs)
{
  PDBQuery pdb_query;
  gboolean success = FALSE;

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

  *num_procs = 0;
  *procs     = NULL;

  if (regcomp (&pdb_query.name_regex, name, PDB_REGCOMP_FLAGS))
    goto free_name;
  if (regcomp (&pdb_query.blurb_regex, blurb, PDB_REGCOMP_FLAGS))
    goto free_blurb;
  if (regcomp (&pdb_query.help_regex, help, PDB_REGCOMP_FLAGS))
    goto free_help;
  if (regcomp (&pdb_query.author_regex, author, PDB_REGCOMP_FLAGS))
    goto free_author;
  if (regcomp (&pdb_query.copyright_regex, copyright, PDB_REGCOMP_FLAGS))
    goto free_copyright;
  if (regcomp (&pdb_query.date_regex, date, PDB_REGCOMP_FLAGS))
    goto free_date;
  if (regcomp (&pdb_query.proc_type_regex, proc_type, PDB_REGCOMP_FLAGS))
    goto free_proc_type;

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

 free_proc_type:
  regfree (&pdb_query.proc_type_regex);
 free_date:
  regfree (&pdb_query.date_regex);
 free_copyright:
  regfree (&pdb_query.copyright_regex);
 free_author:
  regfree (&pdb_query.author_regex);
 free_help:
  regfree (&pdb_query.help_regex);
 free_blurb:
  regfree (&pdb_query.blurb_regex);
 free_name:
  regfree (&pdb_query.name_regex);

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
                    gint             *num_values)
{
  GimpProcedure *procedure;
  PDBStrings     strings;

  g_return_val_if_fail (GIMP_IS_PDB (pdb), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

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

  return FALSE;
}


/*  private functions  */

static int
match_strings (regex_t     *preg,
               const gchar *a)
{
  if (!a)
    a = "";

  return regexec (preg, a, 0, NULL, 0);
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

  if (! match_strings (&pdb_query->name_regex,      proc_name)         &&
      ! match_strings (&pdb_query->blurb_regex,     strings.blurb)     &&
      ! match_strings (&pdb_query->help_regex,      strings.help)      &&
      ! match_strings (&pdb_query->author_regex,    strings.author)    &&
      ! match_strings (&pdb_query->copyright_regex, strings.copyright) &&
      ! match_strings (&pdb_query->date_regex,      strings.date)      &&
      ! match_strings (&pdb_query->proc_type_regex, type_desc->value_desc))
    {
      pdb_query->num_procs++;
      pdb_query->list_of_procs = g_renew (gchar *, pdb_query->list_of_procs,
                                          pdb_query->num_procs);
      pdb_query->list_of_procs[pdb_query->num_procs - 1] = g_strdup (proc_name);
    }

  gimp_pdb_free_strings (&strings);
}

/* #define DEBUG_OUTPUT 1 */

static gboolean
output_string (FILE        *file,
               const gchar *string)
{
#ifndef DEBUG_OUTPUT
  if (fprintf (file, "\"") < 0)
    return FALSE;
#endif

  if (string)
    while (*string)
      {
        switch (*string)
          {
          case '\\' : if (fprintf (file, "\\\\") < 0) return FALSE; break;
          case '\"' : if (fprintf (file, "\\\"") < 0) return FALSE; break;
          case '{'  : if (fprintf (file, "@{")   < 0) return FALSE; break;
          case '@'  : if (fprintf (file, "@@")   < 0) return FALSE; break;
          case '}'  : if (fprintf (file, "@}")   < 0) return FALSE; break;

          default:
            if (fprintf (file, "%c", *string) < 0)
              return FALSE;
          }
        string++;
      }

#ifndef DEBUG_OUTPUT
  if (fprintf (file, "\"\n") < 0)
    return FALSE;
#endif

  return TRUE;
}

static void
gimp_pdb_print_entry (gpointer key,
                      gpointer value,
                      gpointer user_data)
{
  PDBDump     *pdb_dump = user_data;
  FILE        *file     = pdb_dump->file;
  const gchar *proc_name;
  GList       *list;
  GEnumClass  *arg_class;
  GEnumClass  *proc_class;
  GString     *buf;
  gint         num = 0;

  proc_name = key;

  if (pdb_dump->dumping_compat)
    list = g_hash_table_lookup (pdb_dump->pdb->procedures, value);
  else
    list = value;

  arg_class  = g_type_class_ref (GIMP_TYPE_PDB_ARG_TYPE);
  proc_class = g_type_class_ref (GIMP_TYPE_PDB_PROC_TYPE);

  buf = g_string_new ("");

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
      fprintf (file, "(");
#else
      fprintf (file, "(register-procedure ");
#endif

      if (num != 1)
        {
          g_string_printf (buf, "%s <%d>", proc_name, num);
          output_string (file, buf->str);
        }
      else
        {
          output_string (file, proc_name);
        }

      type_desc = gimp_enum_get_desc (proc_class, procedure->proc_type);

#ifdef DEBUG_OUTPUT

      fprintf (file, " (");

      for (i = 0; i < procedure->num_args; i++)
        {
          GParamSpec     *pspec = procedure->args[i];
          GimpPDBArgType  arg_type;

          arg_type = gimp_pdb_compat_arg_type_from_gtype (pspec->value_type);

          arg_value = g_enum_get_value (arg_class, arg_type);

          if (i > 0)
            fprintf (file, " ");

          output_string (file, arg_value->value_name);
        }

      fprintf (file, ") (");

      for (i = 0; i < procedure->num_values; i++)
        {
          GParamSpec     *pspec = procedure->values[i];
          GimpPDBArgType  arg_type;

          arg_type = gimp_pdb_compat_arg_type_from_gtype (pspec->value_type);

          arg_value = g_enum_get_value (arg_class, arg_type);

          if (i > 0)
            fprintf (file, " ");

          output_string (file, arg_value->value_name);
        }
      fprintf (file, "))\n");

#else /* ! DEBUG_OUTPUT */

      fprintf (file, "  ");
      output_string (file, strings.blurb);
      fprintf (file, "  ");
      output_string (file, strings.help);
      fprintf (file, "  ");
      output_string (file, strings.author);
      fprintf (file, "  ");
      output_string (file, strings.copyright);
      fprintf (file, "  ");
      output_string (file, strings.date);
      fprintf (file, "  ");
      output_string (file, type_desc->value_desc);

      fprintf (file, "  (");
      for (i = 0; i < procedure->num_args; i++)
        {
          GParamSpec     *pspec = procedure->args[i];
          GimpPDBArgType  arg_type;
          gchar          *desc  = gimp_param_spec_get_desc (pspec);

          fprintf (file, "\n    (\n");

          arg_type = gimp_pdb_compat_arg_type_from_gtype (pspec->value_type);

          arg_value = g_enum_get_value (arg_class, arg_type);

          fprintf (file, "      ");
          output_string (file, g_param_spec_get_name (pspec));
          fprintf (file, "      ");
          output_string (file, arg_value->value_name);
          fprintf (file, "      ");
          output_string (file, desc);
          g_free (desc);

          fprintf (file, "    )");
        }
      fprintf (file, "\n  )\n");

      fprintf (file, "  (");
      for (i = 0; i < procedure->num_values; i++)
        {
          GParamSpec     *pspec = procedure->values[i];
          GimpPDBArgType  arg_type;
          gchar          *desc  = gimp_param_spec_get_desc (pspec);

          fprintf (file, "\n    (\n");

          arg_type = gimp_pdb_compat_arg_type_from_gtype (pspec->value_type);

          arg_value = g_enum_get_value (arg_class, arg_type);

          fprintf (file, "      ");
          output_string (file, g_param_spec_get_name (pspec));
          fprintf (file, "      ");
          output_string (file, arg_value->value_name);
          fprintf (file, "      ");
          output_string (file, desc);
          g_free (desc);

          fprintf (file, "    )");
        }
      fprintf (file, "\n  )");
      fprintf (file, "\n)\n");

#endif /* DEBUG_OUTPUT */

      gimp_pdb_free_strings (&strings);
    }

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
                                            GIMP_OBJECT (procedure)->name);
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
