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
  gchar            *menu_label;
  gchar            *blurb;          /* Short procedure description    */
  gchar            *help;           /* Detailed help instructions     */
  gchar            *help_id;
  gchar            *author;         /* Author field                   */
  gchar            *copyright;      /* Copyright field                */
  gchar            *date;           /* Date field                     */
  gchar            *image_types;

  GList            *menu_paths;

  gint32            n_args;         /* Number of procedure arguments  */
  GParamSpec      **args;           /* Array of procedure arguments   */

  gint32            n_values;       /* Number of return values        */
  GParamSpec      **values;         /* Array of return values         */

  GimpRunFunc       run_func;
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

  g_list_free_full (procedure->priv->menu_paths, g_free);
  procedure->priv->menu_paths = NULL;

  if (procedure->priv->args)
    {
      for (i = 0; i < procedure->priv->n_args; i++)
        g_param_spec_unref (procedure->priv->args[i]);

      g_clear_pointer (&procedure->priv->args, g_free);
    }

  if (procedure->priv->values)
    {
      for (i = 0; i < procedure->priv->n_values; i++)
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

void
gimp_procedure_set_strings (GimpProcedure *procedure,
                            const gchar   *menu_label,
                            const gchar   *blurb,
                            const gchar   *help,
                            const gchar   *help_id,
                            const gchar   *author,
                            const gchar   *copyright,
                            const gchar   *date,
                            const gchar   *image_types)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  gimp_procedure_free_strings (procedure);

  procedure->priv->menu_label    = g_strdup (menu_label);
  procedure->priv->blurb         = g_strdup (blurb);
  procedure->priv->help          = g_strdup (help);
  procedure->priv->help_id       = g_strdup (help_id);
  procedure->priv->author        = g_strdup (author);
  procedure->priv->copyright     = g_strdup (copyright);
  procedure->priv->date          = g_strdup (date);
  procedure->priv->image_types   = g_strdup (image_types);
}

const gchar *
gimp_procedure_get_name (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->name;
}

const gchar *
gimp_procedure_get_menu_label (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->menu_label;
}

const gchar *
gimp_procedure_get_blurb (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->blurb;
}

const gchar *
gimp_procedure_get_help (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->help;
}

const gchar *
gimp_procedure_get_help_id (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->help_id;
}

const gchar *
gimp_procedure_get_author (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->author;
}

const gchar *
gimp_procedure_get_copyright (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->copyright;
}

const gchar *
gimp_procedure_get_date (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->date;
}

const gchar *
gimp_procedure_get_image_types (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->image_types;
}

void
gimp_procedure_add_menu_path (GimpProcedure *procedure,
                              const gchar   *menu_path)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (menu_path != NULL);

  procedure->priv->menu_paths = g_list_append (procedure->priv->menu_paths,
                                               g_strdup (menu_path));
}

GList *
gimp_procedure_get_menu_paths (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->menu_paths;
}

void
gimp_procedure_add_argument (GimpProcedure *procedure,
                             GParamSpec    *pspec)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  procedure->priv->args = g_renew (GParamSpec *, procedure->priv->args,
                                   procedure->priv->n_args + 1);

  procedure->priv->args[procedure->priv->n_args] = pspec;

  g_param_spec_ref_sink (pspec);

  procedure->priv->n_args++;
}

void
gimp_procedure_add_return_value (GimpProcedure *procedure,
                                 GParamSpec    *pspec)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  procedure->priv->values = g_renew (GParamSpec *, procedure->priv->values,
                               procedure->priv->n_values + 1);

  procedure->priv->values[procedure->priv->n_values] = pspec;

  g_param_spec_ref_sink (pspec);

  procedure->priv->n_values++;
}

GParamSpec **
gimp_procedure_get_arguments (GimpProcedure *procedure,
                              gint          *n_arguments)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (n_arguments != NULL, NULL);

  *n_arguments = procedure->priv->n_args;

  return procedure->priv->args;
}

GParamSpec **
gimp_procedure_get_return_values (GimpProcedure *procedure,
                                  gint          *n_return_values)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (n_return_values != NULL, NULL);

  *n_return_values = procedure->priv->n_values;

  return procedure->priv->values;
}

GimpValueArray *
gimp_procedure_new_arguments (GimpProcedure *procedure)
{
  GimpValueArray *args;
  GValue          value = G_VALUE_INIT;
  gint            i;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  args = gimp_value_array_new (procedure->priv->n_args);

  for (i = 0; i < procedure->priv->n_args; i++)
    {
      g_value_init (&value,
                    G_PARAM_SPEC_VALUE_TYPE (procedure->priv->args[i]));
      gimp_value_array_append (args, &value);
      g_value_unset (&value);
    }

  return args;
}

GimpValueArray *
gimp_procedure_new_return_values (GimpProcedure     *procedure,
                                  GimpPDBStatusType  status,
                                  const GError      *error)
{
  GimpValueArray *args;
  GValue          value = G_VALUE_INIT;
  gint            i;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (status != GIMP_PDB_PASS_THROUGH, NULL);

  switch (status)
    {
    case GIMP_PDB_SUCCESS:
    case GIMP_PDB_CANCEL:
      args = gimp_value_array_new (procedure->priv->n_values + 1);

      g_value_init (&value, GIMP_TYPE_PDB_STATUS_TYPE);
      g_value_set_enum (&value, status);
      gimp_value_array_append (args, &value);
      g_value_unset (&value);

      for (i = 0; i < procedure->priv->n_values; i++)
        {
          g_value_init (&value,
                        G_PARAM_SPEC_VALUE_TYPE (procedure->priv->values[i]));
          gimp_value_array_append (args, &value);
          g_value_unset (&value);
        }
      break;

    case GIMP_PDB_EXECUTION_ERROR:
    case GIMP_PDB_CALLING_ERROR:
      args = gimp_value_array_new ((error && error->message) ? 2 : 1);

      g_value_init (&value, GIMP_TYPE_PDB_STATUS_TYPE);
      g_value_set_enum (&value, status);
      gimp_value_array_append (args, &value);
      g_value_unset (&value);

      if (error && error->message)
        {
          g_value_init (&value, G_TYPE_STRING);
          g_value_set_string (&value, error->message);
          gimp_value_array_append (args, &value);
          g_value_unset (&value);
        }
      break;

    default:
      g_return_val_if_reached (NULL);
    }

  return args;
}

GimpValueArray *
gimp_procedure_run (GimpProcedure   *procedure,
                    GimpValueArray  *args)
{
  GimpValueArray *return_vals;
  GError         *error = NULL;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (args != NULL, NULL);

  if (! gimp_procedure_validate_args (procedure,
                                      procedure->priv->args,
                                      procedure->priv->n_args,
                                      args, FALSE, &error))
    {
      return_vals = gimp_procedure_new_return_values (procedure,
                                                      GIMP_PDB_CALLING_ERROR,
                                                      error);
      g_clear_error (&error);

      return return_vals;
    }

  /*  call the procedure  */
  return_vals = procedure->priv->run_func (procedure, args);

  if (! return_vals)
    {
      g_warning ("%s: no return values, shouldn't happen", G_STRFUNC);

      error = g_error_new (0, 0, 0,
                           _("Procedure '%s' returned no return values"),
                           gimp_procedure_get_name (procedure));

      return_vals = gimp_procedure_new_return_values (procedure,
                                                      GIMP_PDB_EXECUTION_ERROR,
                                                      error);
      g_clear_error (&error);
    }

  return return_vals;
}


/*  private functions  */

static void
gimp_procedure_free_strings (GimpProcedure *procedure)
{
  g_clear_pointer (&procedure->priv->menu_label,  g_free);
  g_clear_pointer (&procedure->priv->blurb,       g_free);
  g_clear_pointer (&procedure->priv->help,        g_free);
  g_clear_pointer (&procedure->priv->help_id,     g_free);
  g_clear_pointer (&procedure->priv->author,      g_free);
  g_clear_pointer (&procedure->priv->copyright,   g_free);
  g_clear_pointer (&procedure->priv->date,        g_free);
  g_clear_pointer (&procedure->priv->image_types, g_free);
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
