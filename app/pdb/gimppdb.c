/* LIGMA - The GNU Image Manipulation Program
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

#include "config.h"

#include <stdarg.h>
#include <string.h>
#include <sys/types.h>

#include <gobject/gvaluecollector.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "pdb-types.h"

#include "core/ligma.h"
#include "core/ligmaitem.h"
#include "core/ligma-memsize.h"
#include "core/ligmacontext.h"
#include "core/ligmaprogress.h"

#include "ligmapdb.h"
#include "ligmapdberror.h"
#include "ligmaprocedure.h"

#include "ligma-intl.h"


enum
{
  REGISTER_PROCEDURE,
  UNREGISTER_PROCEDURE,
  LAST_SIGNAL
};


static void     ligma_pdb_finalize                  (GObject       *object);
static gint64   ligma_pdb_get_memsize               (LigmaObject    *object,
                                                    gint64        *gui_size);
static void     ligma_pdb_real_register_procedure   (LigmaPDB       *pdb,
                                                    LigmaProcedure *procedure);
static void     ligma_pdb_real_unregister_procedure (LigmaPDB       *pdb,
                                                    LigmaProcedure *procedure);
static void     ligma_pdb_entry_free                (gpointer       key,
                                                    gpointer       value,
                                                    gpointer       user_data);
static gint64   ligma_pdb_entry_get_memsize         (GList         *procedures,
                                                    gint64        *gui_size);


G_DEFINE_TYPE (LigmaPDB, ligma_pdb, LIGMA_TYPE_OBJECT)

#define parent_class ligma_pdb_parent_class

static guint ligma_pdb_signals[LAST_SIGNAL] = { 0 };


static void
ligma_pdb_class_init (LigmaPDBClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);

  ligma_pdb_signals[REGISTER_PROCEDURE] =
    g_signal_new ("register-procedure",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaPDBClass, register_procedure),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_PROCEDURE);

  ligma_pdb_signals[UNREGISTER_PROCEDURE] =
    g_signal_new ("unregister-procedure",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaPDBClass, unregister_procedure),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_PROCEDURE);

  object_class->finalize         = ligma_pdb_finalize;

  ligma_object_class->get_memsize = ligma_pdb_get_memsize;

  klass->register_procedure      = ligma_pdb_real_register_procedure;
  klass->unregister_procedure    = ligma_pdb_real_unregister_procedure;
}

static void
ligma_pdb_init (LigmaPDB *pdb)
{
  pdb->procedures        = g_hash_table_new (g_str_hash, g_str_equal);
  pdb->compat_proc_names = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
ligma_pdb_finalize (GObject *object)
{
  LigmaPDB *pdb = LIGMA_PDB (object);

  if (pdb->procedures)
    {
      g_hash_table_foreach (pdb->procedures, ligma_pdb_entry_free, NULL);
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
ligma_pdb_get_memsize (LigmaObject *object,
                      gint64     *gui_size)
{
  LigmaPDB *pdb     = LIGMA_PDB (object);
  gint64   memsize = 0;

  memsize += ligma_g_hash_table_get_memsize_foreach (pdb->procedures,
                                                    (LigmaMemsizeFunc)
                                                    ligma_pdb_entry_get_memsize,
                                                    gui_size);
  memsize += ligma_g_hash_table_get_memsize (pdb->compat_proc_names, 0);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
ligma_pdb_real_register_procedure (LigmaPDB       *pdb,
                                  LigmaProcedure *procedure)
{
  const gchar *name;
  GList       *list;

  name = ligma_object_get_name (procedure);

  list = g_hash_table_lookup (pdb->procedures, name);

  g_hash_table_replace (pdb->procedures, (gpointer) name,
                        g_list_prepend (list, g_object_ref (procedure)));
}

static void
ligma_pdb_real_unregister_procedure (LigmaPDB       *pdb,
                                    LigmaProcedure *procedure)
{
  const gchar *name;
  GList       *list;

  name = ligma_object_get_name (procedure);

  list = g_hash_table_lookup (pdb->procedures, name);

  if (list)
    {
      list = g_list_remove (list, procedure);

      if (list)
        {
          name = ligma_object_get_name (list->data);
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

LigmaPDB *
ligma_pdb_new (Ligma *ligma)
{
  LigmaPDB *pdb;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  pdb = g_object_new (LIGMA_TYPE_PDB,
                      "name", "pdb",
                      NULL);

  pdb->ligma = ligma;

  return pdb;
}

void
ligma_pdb_register_procedure (LigmaPDB       *pdb,
                             LigmaProcedure *procedure)
{
  g_return_if_fail (LIGMA_IS_PDB (pdb));
  g_return_if_fail (LIGMA_IS_PROCEDURE (procedure));

  if (! procedure->deprecated ||
      pdb->ligma->pdb_compat_mode != LIGMA_PDB_COMPAT_OFF)
    {
      g_signal_emit (pdb, ligma_pdb_signals[REGISTER_PROCEDURE], 0,
                     procedure);
    }
}

void
ligma_pdb_unregister_procedure (LigmaPDB       *pdb,
                               LigmaProcedure *procedure)
{
  g_return_if_fail (LIGMA_IS_PDB (pdb));
  g_return_if_fail (LIGMA_IS_PROCEDURE (procedure));

  g_signal_emit (pdb, ligma_pdb_signals[UNREGISTER_PROCEDURE], 0,
                 procedure);
}

LigmaProcedure *
ligma_pdb_lookup_procedure (LigmaPDB     *pdb,
                           const gchar *name)
{
  GList *list;

  g_return_val_if_fail (LIGMA_IS_PDB (pdb), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  list = g_hash_table_lookup (pdb->procedures, name);

  if (list)
    return list->data;

  return NULL;
}

void
ligma_pdb_register_compat_proc_name (LigmaPDB     *pdb,
                                    const gchar *old_name,
                                    const gchar *new_name)
{
  g_return_if_fail (LIGMA_IS_PDB (pdb));
  g_return_if_fail (old_name != NULL);
  g_return_if_fail (new_name != NULL);

  g_hash_table_insert (pdb->compat_proc_names,
                       (gpointer) old_name,
                       (gpointer) new_name);
}

const gchar *
ligma_pdb_lookup_compat_proc_name (LigmaPDB     *pdb,
                                  const gchar *old_name)
{
  g_return_val_if_fail (LIGMA_IS_PDB (pdb), NULL);
  g_return_val_if_fail (old_name != NULL, NULL);

  return g_hash_table_lookup (pdb->compat_proc_names, old_name);
}

LigmaValueArray *
ligma_pdb_execute_procedure_by_name_args (LigmaPDB         *pdb,
                                         LigmaContext     *context,
                                         LigmaProgress    *progress,
                                         GError         **error,
                                         const gchar     *name,
                                         LigmaValueArray  *args)
{
  LigmaValueArray *return_vals = NULL;
  GList          *list;

  g_return_val_if_fail (LIGMA_IS_PDB (pdb), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);

  list = g_hash_table_lookup (pdb->procedures, name);

  if (list == NULL)
    {
      GError *pdb_error = g_error_new (LIGMA_PDB_ERROR,
                                       LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND,
                                       _("Procedure '%s' not found"), name);

      return_vals = ligma_procedure_get_return_values (NULL, FALSE, pdb_error);
      g_propagate_error (error, pdb_error);

      return return_vals;
    }

  g_return_val_if_fail (args != NULL, NULL);

  for (; list; list = g_list_next (list))
    {
      LigmaProcedure *procedure = list->data;

      g_return_val_if_fail (LIGMA_IS_PROCEDURE (procedure), NULL);

      return_vals = ligma_procedure_execute (procedure,
                                            pdb->ligma, context, progress,
                                            args, error);

      if (g_value_get_enum (ligma_value_array_index (return_vals, 0)) ==
          LIGMA_PDB_PASS_THROUGH)
        {
          /*  If the return value is LIGMA_PDB_PASS_THROUGH and there is
           *  a next procedure in the list, destroy the return values
           *  and run the next procedure.
           */
          if (g_list_next (list))
            {
              ligma_value_array_unref (return_vals);
              g_clear_error (error);
            }
        }
      else
        {
          /*  No LIGMA_PDB_PASS_THROUGH, break out of the list of
           *  procedures and return the current return values.
           */
          break;
        }
    }

  return return_vals;
}

LigmaValueArray *
ligma_pdb_execute_procedure_by_name (LigmaPDB       *pdb,
                                    LigmaContext   *context,
                                    LigmaProgress  *progress,
                                    GError       **error,
                                    const gchar   *name,
                                    ...)
{
  LigmaProcedure  *procedure;
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  va_list         va_args;
  GType           prev_value_type = G_TYPE_NONE;
  gint            prev_int_value  = 0;
  gint            i;

  g_return_val_if_fail (LIGMA_IS_PDB (pdb), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);

  procedure = ligma_pdb_lookup_procedure (pdb, name);

  if (! procedure)
    {
      GError *pdb_error = g_error_new (LIGMA_PDB_ERROR,
                                       LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND,
                                       _("Procedure '%s' not found"), name);

      return_vals = ligma_procedure_get_return_values (NULL, FALSE, pdb_error);
      g_propagate_error (error, pdb_error);

      return return_vals;
    }

  args = ligma_procedure_get_arguments (procedure);

  va_start (va_args, name);

  for (i = 0; i < procedure->num_args; i++)
    {
      GValue *value;
      GType   arg_type;
      GType   value_type;
      gchar  *error_msg = NULL;

      arg_type = va_arg (va_args, GType);

      if (arg_type == G_TYPE_NONE)
        break;

      value = ligma_value_array_index (args, i);

      value_type = G_VALUE_TYPE (value);

      /* G_TYPE_INT is widely abused for enums and booleans in
       * old plug-ins, silently copy stuff into integers when enums
       * and booleans are passed
       */
      if (arg_type != value_type

          &&

          value_type == G_TYPE_INT

          &&

          (arg_type == G_TYPE_BOOLEAN  ||
           g_type_is_a (arg_type, G_TYPE_ENUM)))
        {
          arg_type = value_type;
        }

      if (arg_type != value_type)
        {
          GError      *pdb_error;
          const gchar *expected = g_type_name (value_type);
          const gchar *got      = g_type_name (arg_type);

          ligma_value_array_unref (args);

          pdb_error = g_error_new (LIGMA_PDB_ERROR,
                                   LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                                   _("Procedure '%s' has been called with a "
                                     "wrong type for argument #%d. "
                                     "Expected %s, got %s."),
                                   ligma_object_get_name (procedure),
                                   i + 1, expected, got);

          return_vals = ligma_procedure_get_return_values (procedure,
                                                          FALSE, pdb_error);
          g_propagate_error (error, pdb_error);

          va_end (va_args);

          return return_vals;
        }

      if (LIGMA_VALUE_HOLDS_INT32_ARRAY (value)  ||
          LIGMA_VALUE_HOLDS_UINT8_ARRAY (value)  ||
          LIGMA_VALUE_HOLDS_FLOAT_ARRAY (value)  ||
          LIGMA_VALUE_HOLDS_RGB_ARRAY (value)    ||
          LIGMA_VALUE_HOLDS_OBJECT_ARRAY (value))
        {
          /* Array arguments don't have their size information when they
           * are set by core code, in C array form.
           * By convention, the previous argument has to be the array
           * size argument.
           */
          g_return_val_if_fail (prev_value_type == G_TYPE_INT && prev_int_value >= 0, NULL);

          if (LIGMA_VALUE_HOLDS_INT32_ARRAY (value))
            ligma_value_set_int32_array (value,
                                        (const gint32 *) va_arg (va_args, gpointer),
                                        prev_int_value);
          else if (LIGMA_VALUE_HOLDS_UINT8_ARRAY (value))
            ligma_value_set_uint8_array (value,
                                        (const guint8 *) va_arg (va_args, gpointer),
                                        prev_int_value);
          else if (LIGMA_VALUE_HOLDS_FLOAT_ARRAY (value))
            ligma_value_set_float_array (value,
                                        (const gdouble *) va_arg (va_args, gpointer),
                                        prev_int_value);
          else if (LIGMA_VALUE_HOLDS_RGB_ARRAY (value))
            ligma_value_set_rgb_array (value,
                                      (const LigmaRGB *) va_arg (va_args, gpointer),
                                      prev_int_value);
          else if (LIGMA_VALUE_HOLDS_OBJECT_ARRAY (value))
            ligma_value_set_object_array (value, LIGMA_TYPE_ITEM,
                                         va_arg (va_args, gpointer),
                                         prev_int_value);
        }
      else
        {
          G_VALUE_COLLECT (value, va_args, G_VALUE_NOCOPY_CONTENTS, &error_msg);
        }

      if (error_msg)
        {
          GError *pdb_error = g_error_new_literal (LIGMA_PDB_ERROR,
                                                   LIGMA_PDB_ERROR_INTERNAL_ERROR,
                                                   error_msg);
          g_warning ("%s: %s", G_STRFUNC, error_msg);
          g_free (error_msg);

          ligma_value_array_unref (args);

          return_vals = ligma_procedure_get_return_values (procedure,
                                                          FALSE, pdb_error);
          g_propagate_error (error, pdb_error);

          va_end (va_args);

          return return_vals;
        }

      prev_value_type = value_type;
      if (prev_value_type == G_TYPE_INT)
        prev_int_value = g_value_get_int (value);
    }

  va_end (va_args);

  return_vals = ligma_pdb_execute_procedure_by_name_args (pdb, context,
                                                         progress, error,
                                                         name, args);

  ligma_value_array_unref (args);

  return return_vals;
}

/**
 * ligma_pdb_get_deprecated_procedures:
 * @pdb:
 *
 * Returns: A new #GList with the deprecated procedures. Free with
 *          g_list_free().
 **/
GList *
ligma_pdb_get_deprecated_procedures (LigmaPDB *pdb)
{
  GList *result = NULL;
  GList *procs;
  GList *iter;

  g_return_val_if_fail (LIGMA_IS_PDB (pdb), NULL);

  procs = g_hash_table_get_values (pdb->procedures);

  for (iter = procs;
       iter;
       iter = g_list_next (iter))
    {
      GList *proc_list = iter->data;

      /* Only care about the first procedure in the list */
      LigmaProcedure *procedure = LIGMA_PROCEDURE (proc_list->data);

      if (procedure->deprecated)
        result = g_list_prepend (result, procedure);
    }

  result = g_list_sort (result, (GCompareFunc) ligma_procedure_name_compare);

  g_list_free (procs);

  return result;
}


/*  private functions  */

static void
ligma_pdb_entry_free (gpointer key,
                     gpointer value,
                     gpointer user_data)
{
  if (value)
    g_list_free_full (value, (GDestroyNotify) g_object_unref);
}

static gint64
ligma_pdb_entry_get_memsize (GList  *procedures,
                            gint64 *gui_size)
{
  return ligma_g_list_get_memsize_foreach (procedures,
                                          (LigmaMemsizeFunc)
                                          ligma_object_get_memsize,
                                          gui_size);
}
