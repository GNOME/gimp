/* The GIMP -- an image manipulation program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "pdb-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpchannel.h"
#include "core/gimplayer.h"
#include "core/gimpparamspecs.h"
#include "core/gimpprogress.h"

#include "vectors/gimpvectors.h"

#include "plug-in/plug-in-run.h"

#include "gimpprocedure.h"
#include "procedural_db.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_procedure_free_strings (GimpProcedure *procedure);


/*  public functions  */

GimpProcedure  *
gimp_procedure_new (void)
{
  GimpProcedure *procedure = g_new0 (GimpProcedure, 1);

  return procedure;
}

void
gimp_procedure_free (GimpProcedure *procedure)
{
  gint i;

  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  gimp_procedure_free_strings (procedure);

  if (procedure->args)
    {
      for (i = 0; i < procedure->num_args; i++)
        g_param_spec_unref (procedure->args[i].pspec);

      g_free (procedure->args);
      procedure->args = NULL;
    }

  if (procedure->values)
    {
      for (i = 0; i < procedure->num_values; i++)
        g_param_spec_unref (procedure->values[i].pspec);

      g_free (procedure->values);
      procedure->values = NULL;
    }

  if (! procedure->static_proc)
    g_free (procedure);
}

GimpProcedure *
gimp_procedure_init (GimpProcedure *procedure,
                     gint           n_arguments,
                     gint           n_return_values)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), procedure);
  g_return_val_if_fail (procedure->args == NULL, procedure);
  g_return_val_if_fail (procedure->values == NULL, procedure);
  g_return_val_if_fail (n_arguments >= 0, procedure);
  g_return_val_if_fail (n_return_values >= 0, procedure);

  procedure->num_args = n_arguments;
  procedure->args     = g_new0 (ProcArg, n_arguments);

  procedure->num_values = n_return_values;
  procedure->values     = g_new0 (ProcArg, n_return_values);

  return procedure;
}

void
gimp_procedure_set_strings (GimpProcedure *procedure,
                            gchar         *name,
                            gchar         *original_name,
                            gchar         *blurb,
                            gchar         *help,
                            gchar         *author,
                            gchar         *copyright,
                            gchar         *date,
                            gchar         *deprecated)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  gimp_procedure_free_strings (procedure);

  procedure->name          = g_strdup (name);
  procedure->original_name = g_strdup (original_name);
  procedure->blurb         = g_strdup (blurb);
  procedure->help          = g_strdup (help);
  procedure->author        = g_strdup (author);
  procedure->copyright     = g_strdup (copyright);
  procedure->date          = g_strdup (date);
  procedure->deprecated    = g_strdup (deprecated);

  procedure->static_strings = FALSE;
}

void
gimp_procedure_set_static_strings (GimpProcedure *procedure,
                                   gchar         *name,
                                   gchar         *original_name,
                                   gchar         *blurb,
                                   gchar         *help,
                                   gchar         *author,
                                   gchar         *copyright,
                                   gchar         *date,
                                   gchar         *deprecated)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  gimp_procedure_free_strings (procedure);

  procedure->name          = name;
  procedure->original_name = original_name;
  procedure->blurb         = blurb;
  procedure->help          = help;
  procedure->author        = author;
  procedure->copyright     = copyright;
  procedure->date          = date;
  procedure->deprecated    = deprecated;

  procedure->static_strings = TRUE;
}

void
gimp_procedure_take_strings (GimpProcedure *procedure,
                             gchar         *name,
                             gchar         *original_name,
                             gchar         *blurb,
                             gchar         *help,
                             gchar         *author,
                             gchar         *copyright,
                             gchar         *date,
                             gchar         *deprecated)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  gimp_procedure_free_strings (procedure);

  procedure->name          = name;
  procedure->original_name = original_name;
  procedure->blurb         = blurb;
  procedure->help          = help;
  procedure->author        = author;
  procedure->copyright     = copyright;
  procedure->date          = date;
  procedure->deprecated    = deprecated;

  procedure->static_strings = FALSE;
}

Argument *
gimp_procedure_execute (GimpProcedure *procedure,
                        Gimp          *gimp,
                        GimpContext   *context,
                        GimpProgress  *progress,
                        Argument      *args,
                        gint           n_args,
                        gint          *n_return_vals)
{
  Argument *return_vals = NULL;
  gint      i;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (n_return_vals != NULL, NULL);

  *n_return_vals = procedure->num_values + 1;

  for (i = 0; i < MIN (n_args, procedure->num_args); i++)
    {
      if (args[i].type != procedure->args[i].type)
        {
          gchar *type_name = procedural_db_type_name (procedure->args[i].type);
          gchar *got       = procedural_db_type_name (args[i].type);

          g_message (_("PDB calling error for procedure '%s':\n"
                       "Argument '%s' (#%d, type %s) type mismatch "
                       "(got %s)."),
                     procedure->name,
                     g_param_spec_get_name (procedure->args[i].pspec),
                     i + 1, type_name, got);

          g_free (type_name);
          g_free (got);

          return_vals = gimp_procedure_get_return_values (procedure, FALSE);
          g_value_set_enum (&return_vals->value, GIMP_PDB_CALLING_ERROR);

          return return_vals;
        }
      else if (! (procedure->args[i].pspec->flags & GIMP_PARAM_NO_VALIDATE))
        {
          GValue string_value = { 0, };

          g_value_init (&string_value, G_TYPE_STRING);

          if (g_value_type_transformable (args[i].value.g_type,
                                          G_TYPE_STRING))
            g_value_transform (&args[i].value, &string_value);
          else
            g_value_set_static_string (&string_value,
                                       "<not transformable to string>");

          if (g_param_value_validate (procedure->args[i].pspec,
                                      &args[i].value))
            {
              gchar *type_name;
              gchar *old_value;
              gchar *new_value;

              type_name = procedural_db_type_name (procedure->args[i].type);

              old_value = g_value_dup_string (&string_value);

              if (g_value_type_transformable (args[i].value.g_type,
                                              G_TYPE_STRING))
                g_value_transform (&args[i].value, &string_value);
              else
                g_value_set_static_string (&string_value,
                                           "<not transformable to string>");

              new_value = g_value_dup_string (&string_value);
              g_value_unset (&string_value);

              g_message (_("PDB calling error for procedure '%s':\n"
                           "Argument '%s' (#%d, type %s) out of bounds "
                           "(validation changed '%s' to '%s')"),
                         procedure->name,
                         g_param_spec_get_name (procedure->args[i].pspec),
                         i + 1, type_name,
                         old_value, new_value);

              g_free (type_name);
              g_free (old_value);
              g_free (new_value);

              return_vals = gimp_procedure_get_return_values (procedure, FALSE);
              g_value_set_enum (&return_vals->value, GIMP_PDB_CALLING_ERROR);

              return return_vals;
            }

          g_value_unset (&string_value);
        }
    }

  /*  call the procedure  */
  switch (procedure->proc_type)
    {
    case GIMP_INTERNAL:
      g_return_val_if_fail (n_args >= procedure->num_args, NULL);

      return_vals = procedure->exec_method.internal.marshal_func (procedure,
                                                                  gimp, context,
                                                                  progress,
                                                                  args);
      break;

    case GIMP_PLUGIN:
    case GIMP_EXTENSION:
    case GIMP_TEMPORARY:
      return_vals = plug_in_run (gimp, context, progress, procedure,
                                 args, n_args,
                                 TRUE, FALSE, -1);
      break;

    default:
      break;
    }

  /*  If there are no return arguments, assume an execution error  */
  if (! return_vals)
    {
      return_vals = gimp_procedure_get_return_values (procedure, FALSE);
      g_value_set_enum (&return_vals->value, GIMP_PDB_EXECUTION_ERROR);

      return return_vals;
    }

  return return_vals;
}

Argument *
gimp_procedure_get_arguments (GimpProcedure *procedure)
{
  Argument *args;
  gint      i;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  args = g_new0 (Argument, procedure->num_args);

  for (i = 0; i < procedure->num_args; i++)
    procedural_db_argument_init (&args[i], &procedure->args[i]);

  return args;
}

Argument *
gimp_procedure_get_return_values (GimpProcedure *procedure,
                                  gboolean       success)
{
  Argument *args;
  gint      n_args;
  gint      i;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure) ||
                        success == FALSE, NULL);

  if (procedure)
    n_args = procedure->num_values + 1;
  else
    n_args = 1;

  args = g_new0 (Argument, n_args);

  procedural_db_compat_arg_init (&args[0], GIMP_PDB_STATUS);

  if (success)
    g_value_set_enum (&args[0].value, GIMP_PDB_SUCCESS);
  else
    g_value_set_enum (&args[0].value, GIMP_PDB_EXECUTION_ERROR);

  if (procedure)
    for (i = 0; i < procedure->num_values; i++)
      procedural_db_argument_init (&args[i + 1], &procedure->values[i]);

  return args;
}

void
gimp_procedure_add_argument (GimpProcedure  *procedure,
                             GimpPDBArgType  arg_type,
                             GParamSpec     *pspec)
{
  gint i;

  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  for (i = 0; i < procedure->num_args; i++)
    if (procedure->args[i].pspec == NULL)
      {
        procedure->args[i].type  = arg_type;
        procedure->args[i].pspec = pspec;

        g_param_spec_ref (pspec);
        g_param_spec_sink (pspec);

        return;
      }

  g_warning ("%s: can't register more than %d arguments for procedure %s",
             G_STRFUNC, procedure->num_args, procedure->name);
}

void
gimp_procedure_add_return_value (GimpProcedure  *procedure,
                                 GimpPDBArgType  arg_type,
                                 GParamSpec     *pspec)
{
  gint i;

  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  for (i = 0; i < procedure->num_values; i++)
    if (procedure->values[i].pspec == NULL)
      {
        procedure->values[i].type  = arg_type;
        procedure->values[i].pspec = pspec;

        g_param_spec_ref (pspec);
        g_param_spec_sink (pspec);

        return;
      }

  g_warning ("%s: can't register more than %d return values for procedure %s",
             G_STRFUNC, procedure->num_values, procedure->name);
}

static GParamSpec *
gimp_procedure_compat_pspec (Gimp           *gimp,
                             GimpPDBArgType  arg_type,
                             const gchar    *name,
                             const gchar    *desc)
{
  GParamSpec *pspec = NULL;

  switch (arg_type)
    {
    case GIMP_PDB_INT32:
      pspec = g_param_spec_int (name, name, desc,
                                G_MININT32, G_MAXINT32, 0,
                                G_PARAM_READWRITE);
      break;

    case GIMP_PDB_INT16:
      pspec = g_param_spec_int (name, name, desc,
                                G_MININT16, G_MAXINT16, 0,
                                G_PARAM_READWRITE);
      break;

    case GIMP_PDB_INT8:
      pspec = g_param_spec_uint (name, name, desc,
                                 0, G_MAXUINT8, 0,
                                 G_PARAM_READWRITE);
      break;

    case GIMP_PDB_FLOAT:
      pspec = g_param_spec_double (name, name, desc,
                                   -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                                   G_PARAM_READWRITE);
      break;

    case GIMP_PDB_STRING:
      pspec = g_param_spec_string (name, name, desc,
                                   NULL,
                                   G_PARAM_READWRITE);
      break;

    case GIMP_PDB_INT32ARRAY:
    case GIMP_PDB_INT16ARRAY:
    case GIMP_PDB_INT8ARRAY:
    case GIMP_PDB_FLOATARRAY:
    case GIMP_PDB_STRINGARRAY:
      pspec = g_param_spec_pointer (name, name, desc,
                                    G_PARAM_READWRITE);
      break;

    case GIMP_PDB_COLOR:
      pspec = gimp_param_spec_rgb (name, name, desc,
                                   NULL,
                                   G_PARAM_READWRITE);
      break;

    case GIMP_PDB_REGION:
      break;

    case GIMP_PDB_DISPLAY:
      pspec = gimp_param_spec_display_id (name, name, desc,
                                          gimp,
                                          G_PARAM_READWRITE);
      break;

    case GIMP_PDB_IMAGE:
      pspec = gimp_param_spec_image_id (name, name, desc,
                                        gimp,
                                        G_PARAM_READWRITE);
      break;

    case GIMP_PDB_LAYER:
      pspec = gimp_param_spec_item_id (name, name, desc,
                                       gimp, GIMP_TYPE_LAYER,
                                       G_PARAM_READWRITE);
      break;

    case GIMP_PDB_CHANNEL:
      pspec = gimp_param_spec_item_id (name, name, desc,
                                       gimp, GIMP_TYPE_CHANNEL,
                                       G_PARAM_READWRITE);
      break;

    case GIMP_PDB_DRAWABLE:
      pspec = gimp_param_spec_item_id (name, name, desc,
                                       gimp, GIMP_TYPE_DRAWABLE,
                                       G_PARAM_READWRITE);
      break;

    case GIMP_PDB_SELECTION:
      pspec = gimp_param_spec_item_id (name, name, desc,
                                       gimp, GIMP_TYPE_CHANNEL,
                                       G_PARAM_READWRITE);
      break;

    case GIMP_PDB_BOUNDARY:
      break;

    case GIMP_PDB_VECTORS:
      pspec = gimp_param_spec_item_id (name, name, desc,
                                       gimp, GIMP_TYPE_VECTORS,
                                       G_PARAM_READWRITE);
      break;

    case GIMP_PDB_PARASITE:
      pspec = gimp_param_spec_parasite (name, name, desc,
                                        G_PARAM_READWRITE);
      break;

    case GIMP_PDB_STATUS:
      pspec = g_param_spec_enum (name, name, desc,
                                 GIMP_TYPE_PDB_STATUS_TYPE,
                                 GIMP_PDB_EXECUTION_ERROR,
                                 G_PARAM_READWRITE);
      break;

    case GIMP_PDB_END:
      break;
    }

  if (! pspec)
    g_warning ("%s: returning NULL for %s (%s)",
               G_STRFUNC, name, procedural_db_type_name (arg_type));

  return pspec;
}

void
gimp_procedure_add_compat_arg (GimpProcedure  *procedure,
                               Gimp           *gimp,
                               GimpPDBArgType  arg_type,
                               const gchar    *name,
                               const gchar    *desc)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (name != NULL);

  gimp_procedure_add_argument (procedure, arg_type,
                               gimp_procedure_compat_pspec (gimp, arg_type,
                                                            name, desc));
}

void
gimp_procedure_add_compat_value (GimpProcedure  *procedure,
                                 Gimp           *gimp,
                                 GimpPDBArgType  arg_type,
                                 const gchar    *name,
                                 const gchar    *desc)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (name != NULL);

  gimp_procedure_add_return_value (procedure, arg_type,
                                   gimp_procedure_compat_pspec (gimp, arg_type,
                                                                name, desc));
}


/*  private functions  */

static void
gimp_procedure_free_strings (GimpProcedure *procedure)
{
  if (! procedure->static_strings)
    {
      g_free (procedure->name);
      g_free (procedure->original_name);
      g_free (procedure->blurb);
      g_free (procedure->help);
      g_free (procedure->author);
      g_free (procedure->copyright);
      g_free (procedure->date);
      g_free (procedure->deprecated);
    }

  procedure->name          = NULL;
  procedure->original_name = NULL;
  procedure->blurb         = NULL;
  procedure->help          = NULL;
  procedure->author        = NULL;
  procedure->copyright     = NULL;
  procedure->date          = NULL;
  procedure->deprecated    = NULL;

  procedure->static_strings = FALSE;
}
