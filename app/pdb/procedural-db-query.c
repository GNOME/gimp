/* The GIMP -- an image manipulation program
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

#include "core/gimp.h"

#include "procedural_db.h"
#include "procedural-db-query.h"

#include "gimp-intl.h"


#define PDB_REGCOMP_FLAGS  REG_ICASE

#define COMPAT_BLURB       "This procedure is deprecated! Use '%s' instead."


typedef struct _PDBQuery PDBQuery;

struct _PDBQuery
{
  Gimp     *gimp;

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

static void   procedural_db_query_entry (gpointer    key,
                                         gpointer    value,
                                         gpointer    user_data);
static void   procedural_db_print_entry (gpointer    key,
                                         gpointer    value,
                                         gpointer    user_data);
static void   procedural_db_get_strings (PDBStrings *strings,
                                         ProcRecord *proc,
                                         gboolean    compat);


/*  public functions  */

gboolean
procedural_db_dump (Gimp        *gimp,
                    const gchar *filename)
{
  FILE *file;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);

  file = g_fopen (filename, "w");

  if (! file)
    return FALSE;

  g_hash_table_foreach (gimp->procedural_ht,
                        procedural_db_print_entry,
                        file);
  fclose (file);

  return TRUE;
}

gboolean
procedural_db_query (Gimp          *gimp,
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

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
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

  pdb_query.gimp            = gimp;
  pdb_query.list_of_procs   = NULL;
  pdb_query.num_procs       = 0;
  pdb_query.querying_compat = FALSE;

  g_hash_table_foreach (gimp->procedural_ht,
                        procedural_db_query_entry, &pdb_query);

  pdb_query.querying_compat = TRUE;

  g_hash_table_foreach (gimp->procedural_compat_ht,
                        procedural_db_query_entry, &pdb_query);

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
procedural_db_proc_info (Gimp             *gimp,
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
  ProcRecord *proc;
  PDBStrings  strings;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = procedural_db_lookup (gimp, proc_name);

  if (proc)
    {
      procedural_db_get_strings (&strings, proc, FALSE);
    }
  else
    {
      const gchar *compat_name;

      compat_name = g_hash_table_lookup (gimp->procedural_compat_ht, proc_name);

      if (compat_name)
        {
          proc = procedural_db_lookup (gimp, compat_name);

          if (proc)
            procedural_db_get_strings (&strings, proc, TRUE);
        }
    }

  if (proc)
    {
      *blurb      = strings.compat ? strings.blurb : g_strdup (strings.blurb);
      *help       = strings.compat ? strings.help : g_strdup (strings.help);
      *author     = strings.compat ? strings.author : g_strdup (strings.author);
      *copyright  = strings.compat ? strings.copyright : g_strdup (strings.copyright);
      *date       = strings.compat ? strings.date : g_strdup (strings.date);
      *proc_type  = proc->proc_type;
      *num_args   = proc->num_args;
      *num_values = proc->num_values;

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
procedural_db_query_entry (gpointer key,
                           gpointer value,
                           gpointer user_data)
{
  PDBQuery     *pdb_query = user_data;
  GList        *list;
  ProcRecord   *proc;
  const gchar  *proc_name;
  PDBStrings    strings;
  GEnumClass   *enum_class;
  GimpEnumDesc *type_desc;

  proc_name = key;

  if (pdb_query->querying_compat)
    list = g_hash_table_lookup (pdb_query->gimp->procedural_ht, value);
  else
    list = value;

  if (! list)
    return;

  proc = (ProcRecord *) list->data;

  procedural_db_get_strings (&strings, proc, pdb_query->querying_compat);

  enum_class = g_type_class_ref (GIMP_TYPE_PDB_PROC_TYPE);
  type_desc = gimp_enum_get_desc (enum_class, proc->proc_type);
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

  if (strings.compat)
    {
      g_free (strings.blurb);
      g_free (strings.help);
    }
}

static gboolean
output_string (FILE        *file,
               const gchar *string)
{
  if (fprintf (file, "\"") < 0)
    return FALSE;

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

  if (fprintf (file, "\"\n") < 0)
    return FALSE;

  return TRUE;
}

static void
procedural_db_print_entry (gpointer key,
                           gpointer value,
                           gpointer user_data)
{
  FILE       *file = user_data;
  GEnumClass *arg_class;
  GEnumClass *proc_class;
  GList      *list;
  GString    *buf;
  gint        num = 0;

  arg_class  = g_type_class_ref (GIMP_TYPE_PDB_ARG_TYPE);
  proc_class = g_type_class_ref (GIMP_TYPE_PDB_PROC_TYPE);

  buf = g_string_new ("");

  for (list = (GList *) value; list; list = list->next)
    {
      ProcRecord   *procedure = list->data;
      GEnumValue   *arg_value;
      GimpEnumDesc *type_desc;
      gint          i;

      num++;

      fprintf (file, "\n(register-procedure ");

      if (list || num != 1)
        {
          g_string_printf (buf, "%s <%d>", procedure->name, num);
          output_string (file, buf->str);
        }
      else
        output_string (file, procedure->name);

      type_desc = gimp_enum_get_desc (proc_class, procedure->proc_type);

      output_string (file, procedure->blurb);
      output_string (file, procedure->help);
      output_string (file, procedure->author);
      output_string (file, procedure->copyright);
      output_string (file, procedure->date);
      output_string (file, type_desc->value_desc);

      fprintf (file, "( ");
      for (i = 0; i < procedure->num_args; i++)
        {
          fprintf (file, "( ");

          arg_value = g_enum_get_value (arg_class,
                                        procedure->args[i].arg_type);

          output_string (file, procedure->args[i].name);
          output_string (file, arg_value->value_name);
          output_string (file, procedure->args[i].description);

          fprintf (file, " ) ");
        }
      fprintf (file, " ) ");

      fprintf (file, "( ");
      for (i = 0; i < procedure->num_values; i++)
        {
          fprintf (file, "( ");

          arg_value = g_enum_get_value (arg_class,
                                        procedure->values[i].arg_type);

          output_string (file, procedure->values[i].name);
          output_string (file, arg_value->value_name);
          output_string (file, procedure->values[i].description);

          fprintf (file, " ) ");
        }
      fprintf (file, " ) ");
      fprintf (file, " ) ");
    }

  g_string_free (buf, TRUE);

  g_type_class_unref (arg_class);
  g_type_class_unref (proc_class);
}

static void
procedural_db_get_strings (PDBStrings *strings,
                           ProcRecord *proc,
                           gboolean    compat)
{
  strings->compat = compat;

  if (compat)
    {
      strings->blurb     = g_strdup_printf (COMPAT_BLURB, proc->name);
      strings->help      = g_strdup (strings->blurb);
      strings->author    = NULL;
      strings->copyright = NULL;
      strings->date      = NULL;
    }
  else
    {
      strings->blurb     = proc->blurb;
      strings->help      = proc->help;
      strings->author    = proc->author;
      strings->copyright = proc->copyright;
      strings->date      = proc->date;
    }
}
