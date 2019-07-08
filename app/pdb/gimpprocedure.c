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

#include "pdb-types.h"

#include "core/gimp.h"
#include "core/gimp-memsize.h"
#include "core/gimpchannel.h"
#include "core/gimplayer.h"
#include "core/gimpparamspecs.h"
#include "core/gimpprogress.h"

#include "vectors/gimpvectors.h"

#include "gimppdbcontext.h"
#include "gimppdberror.h"
#include "gimpprocedure.h"

#include "gimp-intl.h"


static void          gimp_procedure_finalize            (GObject         *object);

static gint64        gimp_procedure_get_memsize         (GimpObject      *object,
                                                         gint64          *gui_size);

static const gchar * gimp_procedure_real_get_label      (GimpProcedure   *procedure);
static const gchar * gimp_procedure_real_get_menu_label (GimpProcedure   *procedure);
static const gchar * gimp_procedure_real_get_blurb      (GimpProcedure   *procedure);
static const gchar * gimp_procedure_real_get_help_id    (GimpProcedure   *procedure);
static gboolean      gimp_procedure_real_get_sensitive  (GimpProcedure   *procedure,
                                                         GimpObject      *object,
                                                         const gchar    **tooltip);
static GimpValueArray * gimp_procedure_real_execute     (GimpProcedure   *procedure,
                                                         Gimp            *gimp,
                                                         GimpContext     *context,
                                                         GimpProgress    *progress,
                                                         GimpValueArray  *args,
                                                         GError         **error);
static void        gimp_procedure_real_execute_async    (GimpProcedure   *procedure,
                                                         Gimp            *gimp,
                                                         GimpContext     *context,
                                                         GimpProgress    *progress,
                                                         GimpValueArray  *args,
                                                         GimpObject      *display);

static void          gimp_procedure_free_strings        (GimpProcedure   *procedure);
static gboolean      gimp_procedure_validate_args       (GimpProcedure   *procedure,
                                                         GParamSpec     **param_specs,
                                                         gint             n_param_specs,
                                                         GimpValueArray  *args,
                                                         gboolean         return_vals,
                                                         GError         **error);


G_DEFINE_TYPE (GimpProcedure, gimp_procedure, GIMP_TYPE_VIEWABLE)

#define parent_class gimp_procedure_parent_class


static void
gimp_procedure_class_init (GimpProcedureClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  object_class->finalize         = gimp_procedure_finalize;

  gimp_object_class->get_memsize = gimp_procedure_get_memsize;

  klass->get_label               = gimp_procedure_real_get_label;
  klass->get_menu_label          = gimp_procedure_real_get_menu_label;
  klass->get_blurb               = gimp_procedure_real_get_blurb;
  klass->get_help_id             = gimp_procedure_real_get_help_id;
  klass->get_sensitive           = gimp_procedure_real_get_sensitive;
  klass->execute                 = gimp_procedure_real_execute;
  klass->execute_async           = gimp_procedure_real_execute_async;
}

static void
gimp_procedure_init (GimpProcedure *procedure)
{
  procedure->proc_type = GIMP_INTERNAL;
}

static void
gimp_procedure_finalize (GObject *object)
{
  GimpProcedure *procedure = GIMP_PROCEDURE (object);
  gint           i;

  gimp_procedure_free_strings (procedure);

  if (procedure->args)
    {
      for (i = 0; i < procedure->num_args; i++)
        g_param_spec_unref (procedure->args[i]);

      g_clear_pointer (&procedure->args, g_free);
    }

  if (procedure->values)
    {
      for (i = 0; i < procedure->num_values; i++)
        g_param_spec_unref (procedure->values[i]);

      g_clear_pointer (&procedure->values, g_free);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_procedure_get_memsize (GimpObject *object,
                            gint64     *gui_size)
{
  GimpProcedure *procedure = GIMP_PROCEDURE (object);
  gint64         memsize   = 0;
  gint           i;

  if (! procedure->static_strings)
    {
      memsize += gimp_string_get_memsize (procedure->original_name);
      memsize += gimp_string_get_memsize (procedure->blurb);
      memsize += gimp_string_get_memsize (procedure->help);
      memsize += gimp_string_get_memsize (procedure->author);
      memsize += gimp_string_get_memsize (procedure->copyright);
      memsize += gimp_string_get_memsize (procedure->date);
      memsize += gimp_string_get_memsize (procedure->deprecated);
    }

  memsize += procedure->num_args * sizeof (GParamSpec *);

  for (i = 0; i < procedure->num_args; i++)
    memsize += gimp_g_param_spec_get_memsize (procedure->args[i]);

  memsize += procedure->num_values * sizeof (GParamSpec *);

  for (i = 0; i < procedure->num_values; i++)
    memsize += gimp_g_param_spec_get_memsize (procedure->values[i]);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static const gchar *
gimp_procedure_real_get_label (GimpProcedure *procedure)
{
  return gimp_object_get_name (procedure); /* lame fallback */
}

static const gchar *
gimp_procedure_real_get_menu_label (GimpProcedure *procedure)
{
  return gimp_procedure_get_label (procedure);
}

static const gchar *
gimp_procedure_real_get_blurb (GimpProcedure *procedure)
{
  return procedure->blurb;
}

static const gchar *
gimp_procedure_real_get_help_id (GimpProcedure *procedure)
{
  return NULL;
}

static gboolean
gimp_procedure_real_get_sensitive (GimpProcedure  *procedure,
                                   GimpObject     *object,
                                   const gchar   **tooltip)
{
  return TRUE /* random fallback */;
}

static GimpValueArray *
gimp_procedure_real_execute (GimpProcedure   *procedure,
                             Gimp            *gimp,
                             GimpContext     *context,
                             GimpProgress    *progress,
                             GimpValueArray  *args,
                             GError         **error)
{
  g_return_val_if_fail (gimp_value_array_length (args) >=
                        procedure->num_args, NULL);

  return procedure->marshal_func (procedure, gimp,
                                  context, progress,
                                  args, error);
}

static void
gimp_procedure_real_execute_async (GimpProcedure  *procedure,
                                   Gimp           *gimp,
                                   GimpContext    *context,
                                   GimpProgress   *progress,
                                   GimpValueArray *args,
                                   GimpObject     *display)
{
  GimpValueArray *return_vals;
  GError         *error = NULL;

  g_return_if_fail (gimp_value_array_length (args) >= procedure->num_args);

  return_vals = GIMP_PROCEDURE_GET_CLASS (procedure)->execute (procedure,
                                                               gimp,
                                                               context,
                                                               progress,
                                                               args,
                                                               &error);

  gimp_value_array_unref (return_vals);

  if (error)
    {
      gimp_message_literal (gimp, G_OBJECT (progress), GIMP_MESSAGE_ERROR,
                            error->message);
      g_error_free (error);
    }
}


/*  public functions  */

GimpProcedure  *
gimp_procedure_new (GimpMarshalFunc marshal_func)
{
  GimpProcedure *procedure;

  g_return_val_if_fail (marshal_func != NULL, NULL);

  procedure = g_object_new (GIMP_TYPE_PROCEDURE, NULL);

  procedure->marshal_func = marshal_func;

  return procedure;
}

void
gimp_procedure_set_strings (GimpProcedure *procedure,
                            const gchar   *original_name,
                            const gchar   *blurb,
                            const gchar   *help,
                            const gchar   *author,
                            const gchar   *copyright,
                            const gchar   *date,
                            const gchar   *deprecated)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  gimp_procedure_free_strings (procedure);

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
                                   const gchar   *original_name,
                                   const gchar   *blurb,
                                   const gchar   *help,
                                   const gchar   *author,
                                   const gchar   *copyright,
                                   const gchar   *date,
                                   const gchar   *deprecated)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  gimp_procedure_free_strings (procedure);

  procedure->original_name = (gchar *) original_name;
  procedure->blurb         = (gchar *) blurb;
  procedure->help          = (gchar *) help;
  procedure->author        = (gchar *) author;
  procedure->copyright     = (gchar *) copyright;
  procedure->date          = (gchar *) date;
  procedure->deprecated    = (gchar *) deprecated;

  procedure->static_strings = TRUE;
}

void
gimp_procedure_take_strings (GimpProcedure *procedure,
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

  procedure->original_name = original_name;
  procedure->blurb         = blurb;
  procedure->help          = help;
  procedure->author        = author;
  procedure->copyright     = copyright;
  procedure->date          = date;
  procedure->deprecated    = deprecated;

  procedure->static_strings = FALSE;
}

const gchar *
gimp_procedure_get_label (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return GIMP_PROCEDURE_GET_CLASS (procedure)->get_label (procedure);
}

const gchar *
gimp_procedure_get_menu_label (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return GIMP_PROCEDURE_GET_CLASS (procedure)->get_menu_label (procedure);
}

const gchar *
gimp_procedure_get_blurb (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return GIMP_PROCEDURE_GET_CLASS (procedure)->get_blurb (procedure);
}

const gchar *
gimp_procedure_get_help_id (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return GIMP_PROCEDURE_GET_CLASS (procedure)->get_help_id (procedure);
}

gboolean
gimp_procedure_get_sensitive (GimpProcedure  *procedure,
                              GimpObject     *object,
                              const gchar   **tooltip)
{
  const gchar *my_tooltip = NULL;
  gboolean     sensitive;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), FALSE);
  g_return_val_if_fail (object == NULL || GIMP_IS_OBJECT (object), FALSE);

  sensitive = GIMP_PROCEDURE_GET_CLASS (procedure)->get_sensitive (procedure,
                                                                   object,
                                                                   &my_tooltip);

  if (tooltip)
    *tooltip = my_tooltip;

  return sensitive;
}

GimpValueArray *
gimp_procedure_execute (GimpProcedure   *procedure,
                        Gimp            *gimp,
                        GimpContext     *context,
                        GimpProgress    *progress,
                        GimpValueArray  *args,
                        GError         **error)
{
  GimpValueArray *return_vals;
  GError         *pdb_error = NULL;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (args != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! gimp_procedure_validate_args (procedure,
                                      procedure->args, procedure->num_args,
                                      args, FALSE, &pdb_error))
    {
      return_vals = gimp_procedure_get_return_values (procedure, FALSE,
                                                      pdb_error);
      g_propagate_error (error, pdb_error);

      return return_vals;
    }

  if (GIMP_IS_PDB_CONTEXT (context))
    context = g_object_ref (context);
  else
    context = gimp_pdb_context_new (gimp, context, TRUE);

  if (progress)
    g_object_ref (progress);

  /*  call the procedure  */
  return_vals = GIMP_PROCEDURE_GET_CLASS (procedure)->execute (procedure,
                                                               gimp,
                                                               context,
                                                               progress,
                                                               args,
                                                               error);

  if (progress)
    g_object_unref (progress);

  g_object_unref (context);

  if (return_vals)
    {
      switch (g_value_get_enum (gimp_value_array_index (return_vals, 0)))
        {
        case GIMP_PDB_CALLING_ERROR:
        case GIMP_PDB_EXECUTION_ERROR:
          /*  If the error has not already been set, construct one
           *  from the error message that is optionally passed with
           *  the return values.
           */
          if (error && *error == NULL &&
              gimp_value_array_length (return_vals) > 1 &&
              G_VALUE_HOLDS_STRING (gimp_value_array_index (return_vals, 1)))
            {
              GValue      *value   = gimp_value_array_index (return_vals, 1);
              const gchar *message = g_value_get_string (value);

              if (message)
                g_set_error_literal (error, GIMP_PDB_ERROR,
                                     GIMP_PDB_ERROR_FAILED,
                                     message);
            }
          break;

        default:
          break;
        }
    }
  else
    {
      g_warning ("%s: no return values, shouldn't happen", G_STRFUNC);

      pdb_error = g_error_new (GIMP_PDB_ERROR,
                               GIMP_PDB_ERROR_INVALID_RETURN_VALUE,
                               _("Procedure '%s' returned no return values"),
                               gimp_object_get_name (procedure));

      return_vals = gimp_procedure_get_return_values (procedure, FALSE,
                                                      pdb_error);
      if (error && *error == NULL)
        g_propagate_error (error, pdb_error);
      else
        g_error_free (pdb_error);

    }

  return return_vals;
}

void
gimp_procedure_execute_async (GimpProcedure  *procedure,
                              Gimp           *gimp,
                              GimpContext    *context,
                              GimpProgress   *progress,
                              GimpValueArray *args,
                              GimpObject     *display,
                              GError        **error)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (args != NULL);
  g_return_if_fail (display == NULL || GIMP_IS_OBJECT (display));
  g_return_if_fail (error == NULL || *error == NULL);

  if (gimp_procedure_validate_args (procedure,
                                    procedure->args, procedure->num_args,
                                    args, FALSE, error))
    {
      if (GIMP_IS_PDB_CONTEXT (context))
        context = g_object_ref (context);
      else
        context = gimp_pdb_context_new (gimp, context, TRUE);

      if (progress)
        g_object_ref (progress);

      GIMP_PROCEDURE_GET_CLASS (procedure)->execute_async (procedure, gimp,
                                                           context, progress,
                                                           args, display);

      if (progress)
        g_object_unref (progress);

      g_object_unref (context);
    }
}

GimpValueArray *
gimp_procedure_get_arguments (GimpProcedure *procedure)
{
  GimpValueArray *args;
  GValue          value = G_VALUE_INIT;
  gint            i;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  args = gimp_value_array_new (procedure->num_args);

  for (i = 0; i < procedure->num_args; i++)
    {
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (procedure->args[i]));
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
      args = gimp_value_array_new (procedure->num_values + 1);

      g_value_init (&value, GIMP_TYPE_PDB_STATUS_TYPE);
      g_value_set_enum (&value, GIMP_PDB_SUCCESS);
      gimp_value_array_append (args, &value);
      g_value_unset (&value);

      for (i = 0; i < procedure->num_values; i++)
        {
          g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (procedure->values[i]));
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
              gimp_assert_not_reached ();
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

  procedure->args = g_renew (GParamSpec *, procedure->args,
                             procedure->num_args + 1);

  procedure->args[procedure->num_args] = pspec;

  g_param_spec_ref_sink (pspec);

  procedure->num_args++;
}

void
gimp_procedure_add_return_value (GimpProcedure *procedure,
                                 GParamSpec    *pspec)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  procedure->values = g_renew (GParamSpec *, procedure->values,
                               procedure->num_values + 1);

  procedure->values[procedure->num_values] = pspec;

  g_param_spec_ref_sink (pspec);

  procedure->num_values++;
}

/**
 * gimp_procedure_create_override:
 * @procedure:
 * @new_marshal_func:
 *
 * Creates a new GimpProcedure that can be used to override the
 * existing @procedure.
 *
 * Returns: The new #GimpProcedure.
 **/
GimpProcedure *
gimp_procedure_create_override (GimpProcedure   *procedure,
                                GimpMarshalFunc  new_marshal_func)
{
  GimpProcedure *new_procedure = NULL;
  const gchar   *name          = NULL;
  int            i             = 0;

  new_procedure = gimp_procedure_new (new_marshal_func);
  name          = gimp_object_get_name (procedure);

  gimp_object_set_static_name (GIMP_OBJECT (new_procedure), name);

  for (i = 0; i < procedure->num_args; i++)
    gimp_procedure_add_argument (new_procedure, procedure->args[i]);

  for (i = 0; i < procedure->num_values; i++)
    gimp_procedure_add_return_value (new_procedure, procedure->values[i]);

  return new_procedure;
}

gint
gimp_procedure_name_compare (GimpProcedure *proc1,
                             GimpProcedure *proc2)
{
  /* Assume there always is a name, don't bother with NULL checks */
  return strcmp (proc1->original_name,
                 proc2->original_name);
}

/*  private functions  */

static void
gimp_procedure_free_strings (GimpProcedure *procedure)
{
  if (! procedure->static_strings)
    {
      g_free (procedure->original_name);
      g_free (procedure->blurb);
      g_free (procedure->help);
      g_free (procedure->author);
      g_free (procedure->copyright);
      g_free (procedure->date);
      g_free (procedure->deprecated);
    }

  procedure->original_name = NULL;
  procedure->blurb         = NULL;
  procedure->help          = NULL;
  procedure->author        = NULL;
  procedure->copyright     = NULL;
  procedure->date          = NULL;
  procedure->deprecated    = NULL;

  procedure->static_strings = FALSE;
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
                           gimp_object_get_name (procedure),
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
                           gimp_object_get_name (procedure),
                           g_param_spec_get_name (pspec),
                           i + 1, g_type_name (spec_type),
                           g_type_name (arg_type));
            }

          return FALSE;
        }
      else if (! (pspec->flags & GIMP_PARAM_NO_VALIDATE))
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
              if (GIMP_IS_PARAM_SPEC_DRAWABLE_ID (pspec) &&
                  g_value_get_int (arg) == -1)
                {
                  if (return_vals)
                    {
                      g_set_error (error,
                                   GIMP_PDB_ERROR,
                                   GIMP_PDB_ERROR_INVALID_RETURN_VALUE,
                                   _("Procedure '%s' returned an "
                                     "invalid ID for argument '%s'. "
                                     "Most likely a plug-in is trying "
                                     "to work on a layer that doesn't "
                                     "exist any longer."),
                                   gimp_object_get_name (procedure),
                                   g_param_spec_get_name (pspec));
                    }
                  else
                    {
                      g_set_error (error,
                                   GIMP_PDB_ERROR,
                                   GIMP_PDB_ERROR_INVALID_ARGUMENT,
                                   _("Procedure '%s' has been called with an "
                                     "invalid ID for argument '%s'. "
                                     "Most likely a plug-in is trying "
                                     "to work on a layer that doesn't "
                                     "exist any longer."),
                                   gimp_object_get_name (procedure),
                                   g_param_spec_get_name (pspec));
                    }
                }
              else if (GIMP_IS_PARAM_SPEC_IMAGE_ID (pspec) &&
                       g_value_get_int (arg) == -1)
                {
                  if (return_vals)
                    {
                      g_set_error (error,
                                   GIMP_PDB_ERROR,
                                   GIMP_PDB_ERROR_INVALID_RETURN_VALUE,
                                   _("Procedure '%s' returned an "
                                     "invalid ID for argument '%s'. "
                                     "Most likely a plug-in is trying "
                                     "to work on an image that doesn't "
                                     "exist any longer."),
                                   gimp_object_get_name (procedure),
                                   g_param_spec_get_name (pspec));
                    }
                  else
                    {
                      g_set_error (error,
                                   GIMP_PDB_ERROR,
                                   GIMP_PDB_ERROR_INVALID_ARGUMENT,
                                   _("Procedure '%s' has been called with an "
                                     "invalid ID for argument '%s'. "
                                     "Most likely a plug-in is trying "
                                     "to work on an image that doesn't "
                                     "exist any longer."),
                                   gimp_object_get_name (procedure),
                                   g_param_spec_get_name (pspec));
                    }
                }
              else
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
                                   gimp_object_get_name (procedure),
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
                                   gimp_object_get_name (procedure),
                                   value,
                                   g_param_spec_get_name (pspec),
                                   i + 1, g_type_name (spec_type));
                    }
                }

              g_value_unset (&string_value);

              return FALSE;
            }

          g_value_unset (&string_value);
        }
    }

  return TRUE;
}
