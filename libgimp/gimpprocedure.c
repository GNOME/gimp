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

#include "config.h"

#include <stdarg.h>
#include <sys/types.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "gimp.h"

#include "libgimp-intl.h"


typedef enum
{
  GIMP_PDB_ERROR_FAILED,  /* generic error condition */
  GIMP_PDB_ERROR_CANCELLED,
  GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
  GIMP_PDB_ERROR_INVALID_ARGUMENT,
  GIMP_PDB_ERROR_INVALID_RETURN_VALUE,
  GIMP_PDB_ERROR_INTERNAL_ERROR
} GimpPdbErrorCode;

#define GIMP_PDB_ERROR (gimp_pdb_error_quark ())

static GQuark
gimp_pdb_error_quark (void)
{
  return g_quark_from_static_string ("gimp-pdb-error-quark");
}


struct _GimpProcedurePrivate
{
  gchar            *name;           /* procedure name                 */
  gchar            *label;
  gchar            *blurb;          /* Short procedure description    */
  gchar            *help;           /* Detailed help instructions     */
  gchar            *help_id;
  gchar            *author;         /* Author field                   */
  gchar            *copyright;      /* Copyright field                */
  gchar            *date;           /* Date field                     */

  gint32            num_args;       /* Number of procedure arguments  */
  GParamSpec      **args;           /* Array of procedure arguments   */

  gint32            num_values;     /* Number of return values        */
  GParamSpec      **values;         /* Array of return values         */

  GimpRunFunc       run_func;
  GimpRunFuncOld    run_func_old;
};


static void       gimp_procedure_finalize      (GObject         *object);

static void       gimp_procedure_free_strings  (GimpProcedure   *procedure);
static gboolean   gimp_procedure_validate_args (GimpProcedure   *procedure,
                                                GParamSpec     **param_specs,
                                                gint             n_param_specs,
                                                GimpValueArray  *args,
                                                gboolean         return_vals,
                                                GError         **error);


G_DEFINE_TYPE_WITH_PRIVATE (GimpProcedure, gimp_procedure, G_TYPE_OBJECT)

#define parent_class gimp_procedure_parent_class


static void
gimp_procedure_class_init (GimpProcedureClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_procedure_finalize;
}

static void
gimp_procedure_init (GimpProcedure *procedure)
{
  procedure->priv = gimp_procedure_get_instance_private (procedure);
}

static void
gimp_procedure_finalize (GObject *object)
{
  GimpProcedure *procedure = GIMP_PROCEDURE (object);
  gint           i;

  g_clear_pointer (&procedure->priv->name, g_free);

  gimp_procedure_free_strings (procedure);

  if (procedure->priv->args)
    {
      for (i = 0; i < procedure->priv->num_args; i++)
        g_param_spec_unref (procedure->priv->args[i]);

      g_clear_pointer (&procedure->priv->args, g_free);
    }

  if (procedure->priv->values)
    {
      for (i = 0; i < procedure->priv->num_values; i++)
        g_param_spec_unref (procedure->priv->values[i]);

      g_clear_pointer (&procedure->priv->values, g_free);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/*  public functions  */

GimpProcedure  *
gimp_procedure_new (const gchar *name,
                    GimpRunFunc  run_func)
{
  GimpProcedure *procedure;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (GIMP_TYPE_PROCEDURE, NULL);

  procedure->priv->name     = g_strdup (name);
  procedure->priv->run_func = run_func;

  return procedure;
}

GimpProcedure  *
gimp_procedure_new_legacy (const gchar    *name,
                           GimpRunFuncOld  run_func_old)
{
  GimpProcedure *procedure;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (run_func_old != NULL, NULL);

  procedure = g_object_new (GIMP_TYPE_PROCEDURE, NULL);

  procedure->priv->name         = g_strdup (name);
  procedure->priv->run_func_old = run_func_old;

  return procedure;
}

void
gimp_procedure_set_strings (GimpProcedure *procedure,
                            const gchar   *blurb,
                            const gchar   *help,
                            const gchar   *help_id,
                            const gchar   *author,
                            const gchar   *copyright,
                            const gchar   *date)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  gimp_procedure_free_strings (procedure);

  procedure->priv->blurb         = g_strdup (blurb);
  procedure->priv->help          = g_strdup (help);
  procedure->priv->help_id       = g_strdup (help_id);
  procedure->priv->author        = g_strdup (author);
  procedure->priv->copyright     = g_strdup (copyright);
  procedure->priv->date          = g_strdup (date);
}

const gchar *
gimp_procedure_get_name (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->name;
}

const gchar *
gimp_procedure_get_label (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->label;
}

const gchar *
gimp_procedure_get_blurb (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->blurb;
}

const gchar *
gimp_procedure_get_help_id (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->help_id;
}

GimpValueArray *
gimp_procedure_run (GimpProcedure   *procedure,
                    GimpValueArray  *args,
                    GError         **error)
{
  GimpValueArray *return_vals;
  GError         *my_error = NULL;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (args != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! gimp_procedure_validate_args (procedure,
                                      procedure->priv->args,
                                      procedure->priv->num_args,
                                      args, FALSE, &my_error))
    {
      return_vals = gimp_procedure_get_return_values (procedure, FALSE,
                                                      my_error);
      g_propagate_error (error, my_error);

      return return_vals;
    }

  /*  call the procedure  */
  return_vals = procedure->priv->run_func (procedure, args, error);

  if (! return_vals)
    {
      g_warning ("%s: no return values, shouldn't happen", G_STRFUNC);

      my_error = g_error_new (0, 0, 0,
                              _("Procedure '%s' returned no return values"),
                              gimp_procedure_get_name (procedure));

      return_vals = gimp_procedure_get_return_values (procedure, FALSE,
                                                      my_error);
      if (error && *error == NULL)
        g_propagate_error (error, my_error);
      else
        g_error_free (my_error);
    }

  return return_vals;
}

void
gimp_procedure_run_legacy (GimpProcedure    *procedure,
                           gint              n_params,
                           const GimpParam  *params,
                           gint             *n_return_vals,
                           GimpParam       **return_vals)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  /*  call the procedure  */
  procedure->priv->run_func_old (procedure,
                                 n_params, params,
                                 n_return_vals, return_vals);
}

GimpValueArray *
gimp_procedure_get_arguments (GimpProcedure *procedure)
{
  GimpValueArray *args;
  GValue          value = G_VALUE_INIT;
  gint            i;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  args = gimp_value_array_new (procedure->priv->num_args);

  for (i = 0; i < procedure->priv->num_args; i++)
    {
      g_value_init (&value,
                    G_PARAM_SPEC_VALUE_TYPE (procedure->priv->args[i]));
      gimp_value_array_append (args, &value);
      g_value_unset (&value);
    }

  return args;
}

GimpValueArray *
gimp_procedure_get_return_values (GimpProcedure *procedure,
                                  gboolean       success,
                                  const GError  *error)
{
  GimpValueArray *args;
  GValue          value = G_VALUE_INIT;
  gint            i;

  g_return_val_if_fail (success == FALSE || GIMP_IS_PROCEDURE (procedure),
                        NULL);

  if (success)
    {
      args = gimp_value_array_new (procedure->priv->num_values + 1);

      g_value_init (&value, GIMP_TYPE_PDB_STATUS_TYPE);
      g_value_set_enum (&value, GIMP_PDB_SUCCESS);
      gimp_value_array_append (args, &value);
      g_value_unset (&value);

      for (i = 0; i < procedure->priv->num_values; i++)
        {
          g_value_init (&value,
                        G_PARAM_SPEC_VALUE_TYPE (procedure->priv->values[i]));
          gimp_value_array_append (args, &value);
          g_value_unset (&value);
        }
    }
  else
    {
      args = gimp_value_array_new ((error && error->message) ? 2 : 1);

      g_value_init (&value, GIMP_TYPE_PDB_STATUS_TYPE);

      /*  errors in the GIMP_PDB_ERROR domain are calling errors  */
      if (error && error->domain == GIMP_PDB_ERROR)
        {
          switch ((GimpPdbErrorCode) error->code)
            {
            case GIMP_PDB_ERROR_FAILED:
            case GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND:
            case GIMP_PDB_ERROR_INVALID_ARGUMENT:
            case GIMP_PDB_ERROR_INVALID_RETURN_VALUE:
            case GIMP_PDB_ERROR_INTERNAL_ERROR:
              g_value_set_enum (&value, GIMP_PDB_CALLING_ERROR);
              break;

            case GIMP_PDB_ERROR_CANCELLED:
              g_value_set_enum (&value, GIMP_PDB_CANCEL);
              break;

            default:
              g_assert_not_reached ();
            }
        }
      else
        {
          g_value_set_enum (&value, GIMP_PDB_EXECUTION_ERROR);
        }

      gimp_value_array_append (args, &value);
      g_value_unset (&value);

      if (error && error->message)
        {
          g_value_init (&value, G_TYPE_STRING);
          g_value_set_string (&value, error->message);
          gimp_value_array_append (args, &value);
          g_value_unset (&value);
        }
    }

  return args;
}

void
gimp_procedure_add_argument (GimpProcedure *procedure,
                             GParamSpec    *pspec)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  procedure->priv->args = g_renew (GParamSpec *, procedure->priv->args,
                                   procedure->priv->num_args + 1);

  procedure->priv->args[procedure->priv->num_args] = pspec;

  g_param_spec_ref_sink (pspec);

  procedure->priv->num_args++;
}

void
gimp_procedure_add_return_value (GimpProcedure *procedure,
                                 GParamSpec    *pspec)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  procedure->priv->values = g_renew (GParamSpec *, procedure->priv->values,
                               procedure->priv->num_values + 1);

  procedure->priv->values[procedure->priv->num_values] = pspec;

  g_param_spec_ref_sink (pspec);

  procedure->priv->num_values++;
}


/*  private functions  */

static void
gimp_procedure_free_strings (GimpProcedure *procedure)
{
  g_clear_pointer (&procedure->priv->blurb,     g_free);
  g_clear_pointer (&procedure->priv->help,      g_free);
  g_clear_pointer (&procedure->priv->help_id,   g_free);
  g_clear_pointer (&procedure->priv->author,    g_free);
  g_clear_pointer (&procedure->priv->copyright, g_free);
  g_clear_pointer (&procedure->priv->date,      g_free);
}

static gboolean
gimp_procedure_validate_args (GimpProcedure  *procedure,
                              GParamSpec    **param_specs,
                              gint            n_param_specs,
                              GimpValueArray *args,
                              gboolean        return_vals,
                              GError        **error)
{
  gint i;

  for (i = 0; i < MIN (gimp_value_array_length (args), n_param_specs); i++)
    {
      GValue     *arg       = gimp_value_array_index (args, i);
      GParamSpec *pspec     = param_specs[i];
      GType       arg_type  = G_VALUE_TYPE (arg);
      GType       spec_type = G_PARAM_SPEC_VALUE_TYPE (pspec);

      if (arg_type != spec_type)
        {
          if (return_vals)
            {
              g_set_error (error,
                           GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_RETURN_VALUE,
                           _("Procedure '%s' returned a wrong value type "
                             "for return value '%s' (#%d). "
                             "Expected %s, got %s."),
                           gimp_procedure_get_name (procedure),
                           g_param_spec_get_name (pspec),
                           i + 1, g_type_name (spec_type),
                           g_type_name (arg_type));
            }
          else
            {
              g_set_error (error,
                           GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                           _("Procedure '%s' has been called with a "
                             "wrong value type for argument '%s' (#%d). "
                             "Expected %s, got %s."),
                           gimp_procedure_get_name (procedure),
                           g_param_spec_get_name (pspec),
                           i + 1, g_type_name (spec_type),
                           g_type_name (arg_type));
            }

          return FALSE;
        }
      else /* if (! (pspec->flags & GIMP_PARAM_NO_VALIDATE)) */
        {
          GValue string_value = G_VALUE_INIT;

          g_value_init (&string_value, G_TYPE_STRING);

          if (g_value_type_transformable (arg_type, G_TYPE_STRING))
            g_value_transform (arg, &string_value);
          else
            g_value_set_static_string (&string_value,
                                       "<not transformable to string>");

          if (g_param_value_validate (pspec, arg))
            {
              const gchar *value = g_value_get_string (&string_value);

              if (value == NULL)
                value = "(null)";

              if (return_vals)
                {
                  g_set_error (error,
                               GIMP_PDB_ERROR,
                               GIMP_PDB_ERROR_INVALID_RETURN_VALUE,
                               _("Procedure '%s' returned "
                                 "'%s' as return value '%s' "
                                 "(#%d, type %s). "
                                 "This value is out of range."),
                               gimp_procedure_get_name (procedure),
                               value,
                               g_param_spec_get_name (pspec),
                               i + 1, g_type_name (spec_type));
                }
              else
                {
                  g_set_error (error,
                               GIMP_PDB_ERROR,
                               GIMP_PDB_ERROR_INVALID_ARGUMENT,
                               _("Procedure '%s' has been called with "
                                 "value '%s' for argument '%s' "
                                 "(#%d, type %s). "
                                 "This value is out of range."),
                               gimp_procedure_get_name (procedure),
                               value,
                               g_param_spec_get_name (pspec),
                               i + 1, g_type_name (spec_type));
                }

              g_value_unset (&string_value);

              return FALSE;
            }

          g_value_unset (&string_value);
        }
    }

  return TRUE;
}
