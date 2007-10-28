/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <stdarg.h>
#include <string.h>
#include <sys/types.h>

#include <glib-object.h>
#include <gobject/gvaluecollector.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "pdb-types.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"
#include "core/gimpprogress.h"

#include "gimppdb.h"
#include "gimpprocedure.h"

#include "gimp-intl.h"


enum
{
  REGISTER_PROCEDURE,
  UNREGISTER_PROCEDURE,
  LAST_SIGNAL
};


static void     gimp_pdb_finalize                  (GObject       *object);
static gint64   gimp_pdb_get_memsize               (GimpObject    *object,
                                                    gint64        *gui_size);
static void     gimp_pdb_real_register_procedure   (GimpPDB       *pdb,
                                                    GimpProcedure *procedure);
static void     gimp_pdb_real_unregister_procedure (GimpPDB       *pdb,
                                                    GimpProcedure *procedure);
static void     gimp_pdb_entry_free                (gpointer       key,
                                                    gpointer       value,
                                                    gpointer       user_data);
static gint64   gimp_pdb_entry_get_memsize         (GList         *procedures,
                                                    gint64        *gui_size);


G_DEFINE_TYPE (GimpPDB, gimp_pdb, GIMP_TYPE_OBJECT)

#define parent_class gimp_pdb_parent_class

static guint gimp_pdb_signals[LAST_SIGNAL] = { 0 };


static void
gimp_pdb_class_init (GimpPDBClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  gimp_pdb_signals[REGISTER_PROCEDURE] =
    g_signal_new ("register-procedure",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpPDBClass, register_procedure),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_PROCEDURE);

  gimp_pdb_signals[UNREGISTER_PROCEDURE] =
    g_signal_new ("unregister-procedure",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpPDBClass, unregister_procedure),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_PROCEDURE);

  object_class->finalize         = gimp_pdb_finalize;

  gimp_object_class->get_memsize = gimp_pdb_get_memsize;

  klass->register_procedure      = gimp_pdb_real_register_procedure;
  klass->unregister_procedure    = gimp_pdb_real_unregister_procedure;
}

static void
gimp_pdb_init (GimpPDB *pdb)
{
  pdb->procedures        = g_hash_table_new (g_str_hash, g_str_equal);
  pdb->compat_proc_names = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
gimp_pdb_finalize (GObject *object)
{
  GimpPDB *pdb = GIMP_PDB (object);

  if (pdb->procedures)
    {
      g_hash_table_foreach (pdb->procedures, gimp_pdb_entry_free, NULL);
      g_hash_table_destroy (pdb->procedures);
      pdb->procedures = NULL;
    }

  if (pdb->compat_proc_names)
    {
      g_hash_table_destroy (pdb->compat_proc_names);
      pdb->compat_proc_names = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_pdb_get_memsize (GimpObject *object,
                      gint64     *gui_size)
{
  GimpPDB *pdb     = GIMP_PDB (object);
  gint64   memsize = 0;

  memsize += gimp_g_hash_table_get_memsize_foreach (pdb->procedures,
                                                    (GimpMemsizeFunc)
                                                    gimp_pdb_entry_get_memsize,
                                                    gui_size);
  memsize += gimp_g_hash_table_get_memsize (pdb->compat_proc_names, 0);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_pdb_real_register_procedure (GimpPDB       *pdb,
                                  GimpProcedure *procedure)
{
  const gchar *name;
  GList       *list;

  name = gimp_object_get_name (GIMP_OBJECT (procedure));

  list = g_hash_table_lookup (pdb->procedures, name);

  g_hash_table_replace (pdb->procedures, (gpointer) name,
                        g_list_prepend (list, g_object_ref (procedure)));
}

static void
gimp_pdb_real_unregister_procedure (GimpPDB       *pdb,
                                    GimpProcedure *procedure)
{
  const gchar *name;
  GList       *list;

  name = gimp_object_get_name (GIMP_OBJECT (procedure));

  list = g_hash_table_lookup (pdb->procedures, name);

  if (list)
    {
      list = g_list_remove (list, procedure);

      if (list)
        {
          name = gimp_object_get_name (GIMP_OBJECT (list->data));
          g_hash_table_replace (pdb->procedures, (gpointer) name, list);
        }
      else
        {
          g_hash_table_remove (pdb->procedures, name);
        }

      g_object_unref (procedure);
    }
}


/*  public functions  */

GimpPDB *
gimp_pdb_new (Gimp *gimp)
{
  GimpPDB *pdb;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  pdb = g_object_new (GIMP_TYPE_PDB,
                      "name", "pdb",
                      NULL);

  pdb->gimp = gimp;

  return pdb;
}

void
gimp_pdb_register_procedure (GimpPDB       *pdb,
                             GimpProcedure *procedure)
{
  g_return_if_fail (GIMP_IS_PDB (pdb));
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  if (! procedure->deprecated ||
      pdb->gimp->pdb_compat_mode != GIMP_PDB_COMPAT_OFF)
    {
      g_signal_emit (pdb, gimp_pdb_signals[REGISTER_PROCEDURE], 0,
                     procedure);
    }
}

void
gimp_pdb_unregister_procedure (GimpPDB       *pdb,
                               GimpProcedure *procedure)
{
  g_return_if_fail (GIMP_IS_PDB (pdb));
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  g_signal_emit (pdb, gimp_pdb_signals[UNREGISTER_PROCEDURE], 0,
                 procedure);
}

GimpProcedure *
gimp_pdb_lookup_procedure (GimpPDB     *pdb,
                           const gchar *name)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_PDB (pdb), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  list = g_hash_table_lookup (pdb->procedures, name);

  if (list)
    return list->data;

  return NULL;
}

void
gimp_pdb_register_compat_proc_name (GimpPDB     *pdb,
                                    const gchar *old_name,
                                    const gchar *new_name)
{
  g_return_if_fail (GIMP_IS_PDB (pdb));
  g_return_if_fail (old_name != NULL);
  g_return_if_fail (new_name != NULL);

  g_hash_table_insert (pdb->compat_proc_names,
                       (gpointer) old_name,
                       (gpointer) new_name);
}

const gchar *
gimp_pdb_lookup_compat_proc_name (GimpPDB     *pdb,
                                  const gchar *old_name)
{
  g_return_val_if_fail (GIMP_IS_PDB (pdb), NULL);
  g_return_val_if_fail (old_name != NULL, NULL);

  return g_hash_table_lookup (pdb->compat_proc_names, old_name);
}

GValueArray *
gimp_pdb_execute_procedure_by_name_args (GimpPDB      *pdb,
                                         GimpContext  *context,
                                         GimpProgress *progress,
                                         const gchar  *name,
                                         GValueArray  *args)
{
  GValueArray *return_vals = NULL;
  GList       *list;

  g_return_val_if_fail (GIMP_IS_PDB (pdb), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  list = g_hash_table_lookup (pdb->procedures, name);

  if (list == NULL)
    {
      gimp_message (pdb->gimp, G_OBJECT (progress), GIMP_MESSAGE_ERROR,
                    _("PDB calling error:\n"
                      "Procedure '%s' not found"), name);

      return_vals = gimp_procedure_get_return_values (NULL, FALSE);
      g_value_set_enum (return_vals->values, GIMP_PDB_CALLING_ERROR);

      return return_vals;
    }

  g_return_val_if_fail (args != NULL, NULL);

  for (; list; list = g_list_next (list))
    {
      GimpProcedure *procedure = list->data;

      g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

      return_vals = gimp_procedure_execute (procedure,
                                            pdb->gimp, context, progress,
                                            args);

      if (g_value_get_enum (&return_vals->values[0]) == GIMP_PDB_PASS_THROUGH)
        {
          /*  If the return value is GIMP_PDB_PASS_THROUGH and there is
           *  a next procedure in the list, destroy the return values
           *  and run the next procedure.
           */
          if (g_list_next (list))
            g_value_array_free (return_vals);
        }
      else
        {
          /*  No GIMP_PDB_PASS_THROUGH, break out of the list of
           *  procedures and return the current return values.
           */
          break;
        }
    }

  return return_vals;
}

GValueArray *
gimp_pdb_execute_procedure_by_name (GimpPDB      *pdb,
                                    GimpContext  *context,
                                    GimpProgress *progress,
                                    const gchar  *name,
                                    ...)
{
  GimpProcedure *procedure;
  GValueArray   *args;
  GValueArray   *return_vals;
  va_list        va_args;
  gint           i;

  g_return_val_if_fail (GIMP_IS_PDB (pdb), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  procedure = gimp_pdb_lookup_procedure (pdb, name);

  if (procedure == NULL)
    {
      gimp_message (pdb->gimp, G_OBJECT (progress), GIMP_MESSAGE_ERROR,
                    _("PDB calling error:\n"
                      "Procedure '%s' not found"), name);

      return_vals = gimp_procedure_get_return_values (NULL, FALSE);
      g_value_set_enum (return_vals->values, GIMP_PDB_CALLING_ERROR);

      return return_vals;
    }

  args = gimp_procedure_get_arguments (procedure);

  va_start (va_args, name);

  for (i = 0; i < procedure->num_args; i++)
    {
      GValue *value;
      GType   arg_type;
      gchar  *error = NULL;

      arg_type = va_arg (va_args, GType);

      if (arg_type == G_TYPE_NONE)
        break;

      value = &args->values[i];

      if (arg_type != G_VALUE_TYPE (value))
        {
          const gchar *expected = g_type_name (G_VALUE_TYPE (value));
          const gchar *got      = g_type_name (arg_type);

          g_value_array_free (args);

          gimp_message (pdb->gimp, G_OBJECT (progress), GIMP_MESSAGE_ERROR,
                        _("PDB calling error for procedure '%s':\n"
                          "Argument #%d type mismatch (expected %s, got %s)"),
                        gimp_object_get_name (GIMP_OBJECT (procedure)),
                        i + 1, expected, got);

          return_vals = gimp_procedure_get_return_values (procedure, FALSE);
          g_value_set_enum (return_vals->values, GIMP_PDB_CALLING_ERROR);

          va_end (va_args);

          return return_vals;
        }

      G_VALUE_COLLECT (value, va_args, G_VALUE_NOCOPY_CONTENTS, &error);

      if (error)
        {
          g_warning ("%s: %s", G_STRFUNC, error);
          g_free (error);

          g_value_array_free (args);

          return_vals = gimp_procedure_get_return_values (procedure, FALSE);
          g_value_set_enum (return_vals->values, GIMP_PDB_CALLING_ERROR);

          va_end (va_args);

          return return_vals;
        }
    }

  va_end (va_args);

  return_vals = gimp_pdb_execute_procedure_by_name_args (pdb, context,
                                                         progress,
                                                         name, args);

  g_value_array_free (args);

  return return_vals;
}


/*  private functions  */

static void
gimp_pdb_entry_free (gpointer key,
                     gpointer value,
                     gpointer user_data)
{
  if (value)
    {
      g_list_foreach (value, (GFunc) g_object_unref, NULL);
      g_list_free (value);
    }
}

static gint64
gimp_pdb_entry_get_memsize (GList  *procedures,
                            gint64 *gui_size)
{
  return gimp_g_list_get_memsize_foreach (procedures,
                                          (GimpMemsizeFunc)
                                          gimp_object_get_memsize,
                                          gui_size);
}
