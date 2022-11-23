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
#include <sys/types.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "pdb-types.h"

#include "core/ligma.h"
#include "core/ligma-memsize.h"
#include "core/ligmachannel.h"
#include "core/ligmadisplay.h"
#include "core/ligmalayer.h"
#include "core/ligmaparamspecs.h"
#include "core/ligmaprogress.h"

#include "vectors/ligmavectors.h"

#include "ligmapdbcontext.h"
#include "ligmapdberror.h"
#include "ligmaprocedure.h"

#include "ligma-intl.h"


static void          ligma_procedure_finalize            (GObject         *object);

static gint64        ligma_procedure_get_memsize         (LigmaObject      *object,
                                                         gint64          *gui_size);

static const gchar * ligma_procedure_real_get_label      (LigmaProcedure   *procedure);
static const gchar * ligma_procedure_real_get_menu_label (LigmaProcedure   *procedure);
static const gchar * ligma_procedure_real_get_blurb      (LigmaProcedure   *procedure);
static const gchar * ligma_procedure_real_get_help_id    (LigmaProcedure   *procedure);
static gboolean      ligma_procedure_real_get_sensitive  (LigmaProcedure   *procedure,
                                                         LigmaObject      *object,
                                                         const gchar    **tooltip);
static LigmaValueArray * ligma_procedure_real_execute     (LigmaProcedure   *procedure,
                                                         Ligma            *ligma,
                                                         LigmaContext     *context,
                                                         LigmaProgress    *progress,
                                                         LigmaValueArray  *args,
                                                         GError         **error);
static void        ligma_procedure_real_execute_async    (LigmaProcedure   *procedure,
                                                         Ligma            *ligma,
                                                         LigmaContext     *context,
                                                         LigmaProgress    *progress,
                                                         LigmaValueArray  *args,
                                                         LigmaDisplay     *display);

static void          ligma_procedure_free_help           (LigmaProcedure   *procedure);
static void          ligma_procedure_free_attribution    (LigmaProcedure   *procedure);

static gboolean      ligma_procedure_validate_args       (LigmaProcedure   *procedure,
                                                         GParamSpec     **param_specs,
                                                         gint             n_param_specs,
                                                         LigmaValueArray  *args,
                                                         gboolean         return_vals,
                                                         GError         **error);


G_DEFINE_TYPE (LigmaProcedure, ligma_procedure, LIGMA_TYPE_VIEWABLE)

#define parent_class ligma_procedure_parent_class


static void
ligma_procedure_class_init (LigmaProcedureClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);

  object_class->finalize         = ligma_procedure_finalize;

  ligma_object_class->get_memsize = ligma_procedure_get_memsize;

  klass->get_label               = ligma_procedure_real_get_label;
  klass->get_menu_label          = ligma_procedure_real_get_menu_label;
  klass->get_blurb               = ligma_procedure_real_get_blurb;
  klass->get_help_id             = ligma_procedure_real_get_help_id;
  klass->get_sensitive           = ligma_procedure_real_get_sensitive;
  klass->execute                 = ligma_procedure_real_execute;
  klass->execute_async           = ligma_procedure_real_execute_async;
}

static void
ligma_procedure_init (LigmaProcedure *procedure)
{
  procedure->proc_type = LIGMA_PDB_PROC_TYPE_INTERNAL;
}

static void
ligma_procedure_finalize (GObject *object)
{
  LigmaProcedure *procedure = LIGMA_PROCEDURE (object);
  gint           i;

  ligma_procedure_free_help        (procedure);
  ligma_procedure_free_attribution (procedure);

  g_clear_pointer (&procedure->deprecated, g_free);
  g_clear_pointer (&procedure->label,      g_free);

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
ligma_procedure_get_memsize (LigmaObject *object,
                            gint64     *gui_size)
{
  LigmaProcedure *procedure = LIGMA_PROCEDURE (object);
  gint64         memsize   = 0;
  gint           i;

  if (! procedure->static_help)
    {
      memsize += ligma_string_get_memsize (procedure->blurb);
      memsize += ligma_string_get_memsize (procedure->help);
      memsize += ligma_string_get_memsize (procedure->help_id);
    }

  if (! procedure->static_attribution)
    {
      memsize += ligma_string_get_memsize (procedure->authors);
      memsize += ligma_string_get_memsize (procedure->copyright);
      memsize += ligma_string_get_memsize (procedure->date);
    }

  memsize += ligma_string_get_memsize (procedure->deprecated);

  memsize += procedure->num_args * sizeof (GParamSpec *);

  for (i = 0; i < procedure->num_args; i++)
    memsize += ligma_g_param_spec_get_memsize (procedure->args[i]);

  memsize += procedure->num_values * sizeof (GParamSpec *);

  for (i = 0; i < procedure->num_values; i++)
    memsize += ligma_g_param_spec_get_memsize (procedure->values[i]);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static const gchar *
ligma_procedure_real_get_label (LigmaProcedure *procedure)
{
  gchar *ellipsis;
  gchar *label;

  if (procedure->label)
    return procedure->label;

  label = ligma_strip_uline (ligma_procedure_get_menu_label (procedure));

  ellipsis = strstr (label, "...");

  if (! ellipsis)
    ellipsis = strstr (label, "\342\200\246" /* U+2026 HORIZONTAL ELLIPSIS */);

  if (ellipsis && ellipsis == (label + strlen (label) - 3))
    *ellipsis = '\0';

  procedure->label = label;

  return procedure->label;
}

static const gchar *
ligma_procedure_real_get_menu_label (LigmaProcedure *procedure)
{
  return ligma_object_get_name (procedure); /* lame fallback */
}

static const gchar *
ligma_procedure_real_get_blurb (LigmaProcedure *procedure)
{
  return procedure->blurb;
}

static const gchar *
ligma_procedure_real_get_help_id (LigmaProcedure *procedure)
{
  if (procedure->help_id)
    return procedure->help_id;

  return ligma_object_get_name (procedure);
}

static gboolean
ligma_procedure_real_get_sensitive (LigmaProcedure  *procedure,
                                   LigmaObject     *object,
                                   const gchar   **tooltip)
{
  return TRUE /* random fallback */;
}

static LigmaValueArray *
ligma_procedure_real_execute (LigmaProcedure   *procedure,
                             Ligma            *ligma,
                             LigmaContext     *context,
                             LigmaProgress    *progress,
                             LigmaValueArray  *args,
                             GError         **error)
{
  g_return_val_if_fail (ligma_value_array_length (args) >=
                        procedure->num_args, NULL);

  return procedure->marshal_func (procedure, ligma,
                                  context, progress,
                                  args, error);
}

static void
ligma_procedure_real_execute_async (LigmaProcedure  *procedure,
                                   Ligma           *ligma,
                                   LigmaContext    *context,
                                   LigmaProgress   *progress,
                                   LigmaValueArray *args,
                                   LigmaDisplay    *display)
{
  LigmaValueArray *return_vals;
  GError         *error = NULL;

  g_return_if_fail (ligma_value_array_length (args) >= procedure->num_args);

  return_vals = LIGMA_PROCEDURE_GET_CLASS (procedure)->execute (procedure,
                                                               ligma,
                                                               context,
                                                               progress,
                                                               args,
                                                               &error);

  ligma_value_array_unref (return_vals);

  if (error)
    {
      ligma_message_literal (ligma, G_OBJECT (progress), LIGMA_MESSAGE_ERROR,
                            error->message);
      g_error_free (error);
    }
}


/*  public functions  */

LigmaProcedure  *
ligma_procedure_new (LigmaMarshalFunc marshal_func)
{
  LigmaProcedure *procedure;

  g_return_val_if_fail (marshal_func != NULL, NULL);

  procedure = g_object_new (LIGMA_TYPE_PROCEDURE, NULL);

  procedure->marshal_func = marshal_func;

  return procedure;
}

void
ligma_procedure_set_help (LigmaProcedure *procedure,
                         const gchar   *blurb,
                         const gchar   *help,
                         const gchar   *help_id)
{
  g_return_if_fail (LIGMA_IS_PROCEDURE (procedure));

  ligma_procedure_free_help (procedure);

  procedure->blurb   = g_strdup (blurb);
  procedure->help    = g_strdup (help);
  procedure->help_id = g_strdup (help_id);

  procedure->static_help = FALSE;
}

void
ligma_procedure_set_static_help (LigmaProcedure *procedure,
                                const gchar   *blurb,
                                const gchar   *help,
                                const gchar   *help_id)
{
  g_return_if_fail (LIGMA_IS_PROCEDURE (procedure));

  ligma_procedure_free_help (procedure);

  procedure->blurb   = (gchar *) blurb;
  procedure->help    = (gchar *) help;
  procedure->help_id = (gchar *) help_id;

  procedure->static_help = TRUE;
}

void
ligma_procedure_take_help (LigmaProcedure *procedure,
                          gchar         *blurb,
                          gchar         *help,
                          gchar         *help_id)
{
  g_return_if_fail (LIGMA_IS_PROCEDURE (procedure));

  ligma_procedure_free_help (procedure);

  procedure->blurb   = blurb;
  procedure->help    = help;
  procedure->help_id = help_id;

  procedure->static_help = FALSE;
}

void
ligma_procedure_set_attribution (LigmaProcedure *procedure,
                                const gchar   *authors,
                                const gchar   *copyright,
                                const gchar   *date)
{
  g_return_if_fail (LIGMA_IS_PROCEDURE (procedure));

  ligma_procedure_free_attribution (procedure);

  procedure->authors   = g_strdup (authors);
  procedure->copyright = g_strdup (copyright);
  procedure->date      = g_strdup (date);

  procedure->static_attribution = FALSE;
}

void
ligma_procedure_set_static_attribution (LigmaProcedure *procedure,
                                       const gchar   *authors,
                                       const gchar   *copyright,
                                       const gchar   *date)
{
  g_return_if_fail (LIGMA_IS_PROCEDURE (procedure));

  ligma_procedure_free_attribution (procedure);

  procedure->authors   = (gchar *) authors;
  procedure->copyright = (gchar *) copyright;
  procedure->date      = (gchar *) date;

  procedure->static_attribution = TRUE;
}

void
ligma_procedure_take_attribution (LigmaProcedure *procedure,
                                 gchar         *authors,
                                 gchar         *copyright,
                                 gchar         *date)
{
  g_return_if_fail (LIGMA_IS_PROCEDURE (procedure));

  ligma_procedure_free_attribution (procedure);

  procedure->authors   = authors;
  procedure->copyright = copyright;
  procedure->date      = date;

  procedure->static_attribution = FALSE;
}

void
ligma_procedure_set_deprecated (LigmaProcedure *procedure,
                               const gchar   *deprecated)
{
  g_return_if_fail (LIGMA_IS_PROCEDURE (procedure));

  g_free (procedure->deprecated);
  procedure->deprecated = g_strdup (deprecated);
}

const gchar *
ligma_procedure_get_label (LigmaProcedure *procedure)
{
  g_return_val_if_fail (LIGMA_IS_PROCEDURE (procedure), NULL);

  return LIGMA_PROCEDURE_GET_CLASS (procedure)->get_label (procedure);
}

const gchar *
ligma_procedure_get_menu_label (LigmaProcedure *procedure)
{
  g_return_val_if_fail (LIGMA_IS_PROCEDURE (procedure), NULL);

  return LIGMA_PROCEDURE_GET_CLASS (procedure)->get_menu_label (procedure);
}

const gchar *
ligma_procedure_get_blurb (LigmaProcedure *procedure)
{
  g_return_val_if_fail (LIGMA_IS_PROCEDURE (procedure), NULL);

  return LIGMA_PROCEDURE_GET_CLASS (procedure)->get_blurb (procedure);
}

const gchar *
ligma_procedure_get_help (LigmaProcedure *procedure)
{
  g_return_val_if_fail (LIGMA_IS_PROCEDURE (procedure), NULL);

  return procedure->help;
}

const gchar *
ligma_procedure_get_help_id (LigmaProcedure *procedure)
{
  g_return_val_if_fail (LIGMA_IS_PROCEDURE (procedure), NULL);

  return LIGMA_PROCEDURE_GET_CLASS (procedure)->get_help_id (procedure);
}

gboolean
ligma_procedure_get_sensitive (LigmaProcedure  *procedure,
                              LigmaObject     *object,
                              const gchar   **reason)
{
  const gchar *my_reason  = NULL;
  gboolean     sensitive;

  g_return_val_if_fail (LIGMA_IS_PROCEDURE (procedure), FALSE);
  g_return_val_if_fail (object == NULL || LIGMA_IS_OBJECT (object), FALSE);

  sensitive = LIGMA_PROCEDURE_GET_CLASS (procedure)->get_sensitive (procedure,
                                                                   object,
                                                                   &my_reason);

  if (reason)
    *reason = my_reason;

  return sensitive;
}

LigmaValueArray *
ligma_procedure_execute (LigmaProcedure   *procedure,
                        Ligma            *ligma,
                        LigmaContext     *context,
                        LigmaProgress    *progress,
                        LigmaValueArray  *args,
                        GError         **error)
{
  LigmaValueArray *return_vals;
  GError         *pdb_error = NULL;

  g_return_val_if_fail (LIGMA_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (args != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! ligma_procedure_validate_args (procedure,
                                      procedure->args, procedure->num_args,
                                      args, FALSE, &pdb_error))
    {
      return_vals = ligma_procedure_get_return_values (procedure, FALSE,
                                                      pdb_error);
      if (! error)
        /* If we can't properly propagate the error, at least print it
         * to standard error stream. This makes debugging easier.
         */
        g_printerr ("%s failed to validate arguments: %s\n", G_STRFUNC, pdb_error->message);

      g_propagate_error (error, pdb_error);

      return return_vals;
    }

  if (LIGMA_IS_PDB_CONTEXT (context))
    context = g_object_ref (context);
  else
    context = ligma_pdb_context_new (ligma, context, TRUE);

  if (progress)
    g_object_ref (progress);

  /*  call the procedure  */
  return_vals = LIGMA_PROCEDURE_GET_CLASS (procedure)->execute (procedure,
                                                               ligma,
                                                               context,
                                                               progress,
                                                               args,
                                                               error);

  if (progress)
    g_object_unref (progress);

  g_object_unref (context);

  if (return_vals)
    {
      switch (g_value_get_enum (ligma_value_array_index (return_vals, 0)))
        {
        case LIGMA_PDB_CALLING_ERROR:
        case LIGMA_PDB_EXECUTION_ERROR:
          /*  If the error has not already been set, construct one
           *  from the error message that is optionally passed with
           *  the return values.
           */
          if (error && *error == NULL &&
              ligma_value_array_length (return_vals) > 1 &&
              G_VALUE_HOLDS_STRING (ligma_value_array_index (return_vals, 1)))
            {
              GValue      *value   = ligma_value_array_index (return_vals, 1);
              const gchar *message = g_value_get_string (value);

              if (message)
                g_set_error_literal (error, LIGMA_PDB_ERROR,
                                     LIGMA_PDB_ERROR_FAILED,
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

      pdb_error = g_error_new (LIGMA_PDB_ERROR,
                               LIGMA_PDB_ERROR_INVALID_RETURN_VALUE,
                               _("Procedure '%s' returned no return values"),
                               ligma_object_get_name (procedure));

      return_vals = ligma_procedure_get_return_values (procedure, FALSE,
                                                      pdb_error);
      if (error && *error == NULL)
        g_propagate_error (error, pdb_error);
      else
        g_error_free (pdb_error);

    }

  return return_vals;
}

void
ligma_procedure_execute_async (LigmaProcedure  *procedure,
                              Ligma           *ligma,
                              LigmaContext    *context,
                              LigmaProgress   *progress,
                              LigmaValueArray *args,
                              LigmaDisplay    *display,
                              GError        **error)
{
  g_return_if_fail (LIGMA_IS_PROCEDURE (procedure));
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));
  g_return_if_fail (args != NULL);
  g_return_if_fail (display == NULL || LIGMA_IS_DISPLAY (display));
  g_return_if_fail (error == NULL || *error == NULL);

  if (ligma_procedure_validate_args (procedure,
                                    procedure->args, procedure->num_args,
                                    args, FALSE, error))
    {
      if (LIGMA_IS_PDB_CONTEXT (context))
        context = g_object_ref (context);
      else
        context = ligma_pdb_context_new (ligma, context, TRUE);

      if (progress)
        g_object_ref (progress);

      LIGMA_PROCEDURE_GET_CLASS (procedure)->execute_async (procedure, ligma,
                                                           context, progress,
                                                           args, display);

      if (progress)
        g_object_unref (progress);

      g_object_unref (context);
    }
}

LigmaValueArray *
ligma_procedure_get_arguments (LigmaProcedure *procedure)
{
  LigmaValueArray *args;
  GValue          value = G_VALUE_INIT;
  gint            i;

  g_return_val_if_fail (LIGMA_IS_PROCEDURE (procedure), NULL);

  args = ligma_value_array_new (procedure->num_args);

  for (i = 0; i < procedure->num_args; i++)
    {
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (procedure->args[i]));
      g_param_value_set_default (procedure->args[i], &value);
      ligma_value_array_append (args, &value);
      g_value_unset (&value);
    }

  return args;
}

LigmaValueArray *
ligma_procedure_get_return_values (LigmaProcedure *procedure,
                                  gboolean       success,
                                  const GError  *error)
{
  LigmaValueArray *args;
  GValue          value = G_VALUE_INIT;
  gint            i;

  g_return_val_if_fail (success == FALSE || LIGMA_IS_PROCEDURE (procedure),
                        NULL);

  if (success)
    {
      args = ligma_value_array_new (procedure->num_values + 1);

      g_value_init (&value, LIGMA_TYPE_PDB_STATUS_TYPE);
      g_value_set_enum (&value, LIGMA_PDB_SUCCESS);
      ligma_value_array_append (args, &value);
      g_value_unset (&value);

      for (i = 0; i < procedure->num_values; i++)
        {
          g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (procedure->values[i]));
          g_param_value_set_default (procedure->values[i], &value);
          ligma_value_array_append (args, &value);
          g_value_unset (&value);
        }
    }
  else
    {
      args = ligma_value_array_new ((error && error->message) ? 2 : 1);

      g_value_init (&value, LIGMA_TYPE_PDB_STATUS_TYPE);

      /*  errors in the LIGMA_PDB_ERROR domain are calling errors  */
      if (error && error->domain == LIGMA_PDB_ERROR)
        {
          switch ((LigmaPdbErrorCode) error->code)
            {
            case LIGMA_PDB_ERROR_FAILED:
            case LIGMA_PDB_ERROR_PROCEDURE_NOT_FOUND:
            case LIGMA_PDB_ERROR_INVALID_ARGUMENT:
            case LIGMA_PDB_ERROR_INVALID_RETURN_VALUE:
            case LIGMA_PDB_ERROR_INTERNAL_ERROR:
              g_value_set_enum (&value, LIGMA_PDB_CALLING_ERROR);
              break;

            case LIGMA_PDB_ERROR_CANCELLED:
              g_value_set_enum (&value, LIGMA_PDB_CANCEL);
              break;

            default:
              ligma_assert_not_reached ();
            }
        }
      else
        {
          g_value_set_enum (&value, LIGMA_PDB_EXECUTION_ERROR);
        }

      ligma_value_array_append (args, &value);
      g_value_unset (&value);

      if (error && error->message)
        {
          g_value_init (&value, G_TYPE_STRING);
          g_value_set_string (&value, error->message);
          ligma_value_array_append (args, &value);
          g_value_unset (&value);
        }
    }

  return args;
}

void
ligma_procedure_add_argument (LigmaProcedure *procedure,
                             GParamSpec    *pspec)
{
  g_return_if_fail (LIGMA_IS_PROCEDURE (procedure));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  procedure->args = g_renew (GParamSpec *, procedure->args,
                             procedure->num_args + 1);

  procedure->args[procedure->num_args] = pspec;

  g_param_spec_ref_sink (pspec);

  procedure->num_args++;
}

void
ligma_procedure_add_return_value (LigmaProcedure *procedure,
                                 GParamSpec    *pspec)
{
  g_return_if_fail (LIGMA_IS_PROCEDURE (procedure));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  procedure->values = g_renew (GParamSpec *, procedure->values,
                               procedure->num_values + 1);

  procedure->values[procedure->num_values] = pspec;

  g_param_spec_ref_sink (pspec);

  procedure->num_values++;
}

/**
 * ligma_procedure_create_override:
 * @procedure:
 * @new_marshal_func:
 *
 * Creates a new LigmaProcedure that can be used to override the
 * existing @procedure.
 *
 * Returns: The new #LigmaProcedure.
 **/
LigmaProcedure *
ligma_procedure_create_override (LigmaProcedure   *procedure,
                                LigmaMarshalFunc  new_marshal_func)
{
  LigmaProcedure *new_procedure = NULL;
  const gchar   *name          = NULL;
  int            i             = 0;

  new_procedure = ligma_procedure_new (new_marshal_func);
  name          = ligma_object_get_name (procedure);

  ligma_object_set_static_name (LIGMA_OBJECT (new_procedure), name);

  for (i = 0; i < procedure->num_args; i++)
    ligma_procedure_add_argument (new_procedure, procedure->args[i]);

  for (i = 0; i < procedure->num_values; i++)
    ligma_procedure_add_return_value (new_procedure, procedure->values[i]);

  return new_procedure;
}

gint
ligma_procedure_name_compare (LigmaProcedure *proc1,
                             LigmaProcedure *proc2)
{
  /* Assume there always is a name, don't bother with NULL checks */
  return strcmp (ligma_object_get_name (proc1),
                 ligma_object_get_name (proc2));
}

/*  private functions  */

static void
ligma_procedure_free_help (LigmaProcedure *procedure)
{
  if (! procedure->static_help)
    {
      g_free (procedure->blurb);
      g_free (procedure->help);
      g_free (procedure->help_id);
    }

  procedure->blurb   = NULL;
  procedure->help    = NULL;
  procedure->help_id = NULL;

  procedure->static_help = FALSE;
}

static void
ligma_procedure_free_attribution (LigmaProcedure *procedure)
{
  if (! procedure->static_attribution)
    {
      g_free (procedure->authors);
      g_free (procedure->copyright);
      g_free (procedure->date);
    }

  procedure->authors   = NULL;
  procedure->copyright = NULL;
  procedure->date      = NULL;

  procedure->static_attribution = FALSE;
}

static gboolean
ligma_procedure_validate_args (LigmaProcedure  *procedure,
                              GParamSpec    **param_specs,
                              gint            n_param_specs,
                              LigmaValueArray *args,
                              gboolean        return_vals,
                              GError        **error)
{
  gint i;

  for (i = 0; i < MIN (ligma_value_array_length (args), n_param_specs); i++)
    {
      GValue     *arg       = ligma_value_array_index (args, i);
      GParamSpec *pspec     = param_specs[i];
      GType       arg_type  = G_VALUE_TYPE (arg);
      GType       spec_type = G_PARAM_SPEC_VALUE_TYPE (pspec);

      if (! g_type_is_a (arg_type, spec_type))
        {
          if (return_vals)
            {
              g_set_error (error,
                           LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_RETURN_VALUE,
                           _("Procedure '%s' returned a wrong value type "
                             "for return value '%s' (#%d). "
                             "Expected %s, got %s."),
                           ligma_object_get_name (procedure),
                           g_param_spec_get_name (pspec),
                           i + 1, g_type_name (spec_type),
                           g_type_name (arg_type));
            }
          else
            {
              g_set_error (error,
                           LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                           _("Procedure '%s' has been called with a "
                             "wrong value type for argument '%s' (#%d). "
                             "Expected %s, got %s."),
                           ligma_object_get_name (procedure),
                           g_param_spec_get_name (pspec),
                           i + 1, g_type_name (spec_type),
                           g_type_name (arg_type));
            }

          return FALSE;
        }
      else if (! (pspec->flags & LIGMA_PARAM_NO_VALIDATE))
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
              if (LIGMA_IS_PARAM_SPEC_DRAWABLE (pspec) &&
                  g_value_get_object (arg) == NULL)
                {
                  if (return_vals)
                    {
                      g_set_error (error,
                                   LIGMA_PDB_ERROR,
                                   LIGMA_PDB_ERROR_INVALID_RETURN_VALUE,
                                   _("Procedure '%s' returned an "
                                     "invalid ID for argument '%s'. "
                                     "Most likely a plug-in is trying "
                                     "to work on a layer that doesn't "
                                     "exist any longer."),
                                   ligma_object_get_name (procedure),
                                   g_param_spec_get_name (pspec));
                    }
                  else
                    {
                      g_set_error (error,
                                   LIGMA_PDB_ERROR,
                                   LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                                   _("Procedure '%s' has been called with an "
                                     "invalid ID for argument '%s'. "
                                     "Most likely a plug-in is trying "
                                     "to work on a layer that doesn't "
                                     "exist any longer."),
                                   ligma_object_get_name (procedure),
                                   g_param_spec_get_name (pspec));
                    }
                }
              else if (LIGMA_IS_PARAM_SPEC_IMAGE (pspec) &&
                       g_value_get_object (arg) == NULL)
                {
                  if (return_vals)
                    {
                      g_set_error (error,
                                   LIGMA_PDB_ERROR,
                                   LIGMA_PDB_ERROR_INVALID_RETURN_VALUE,
                                   _("Procedure '%s' returned an "
                                     "invalid ID for argument '%s'. "
                                     "Most likely a plug-in is trying "
                                     "to work on an image that doesn't "
                                     "exist any longer."),
                                   ligma_object_get_name (procedure),
                                   g_param_spec_get_name (pspec));
                    }
                  else
                    {
                      g_set_error (error,
                                   LIGMA_PDB_ERROR,
                                   LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                                   _("Procedure '%s' has been called with an "
                                     "invalid ID for argument '%s'. "
                                     "Most likely a plug-in is trying "
                                     "to work on an image that doesn't "
                                     "exist any longer."),
                                   ligma_object_get_name (procedure),
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
                                   LIGMA_PDB_ERROR,
                                   LIGMA_PDB_ERROR_INVALID_RETURN_VALUE,
                                   _("Procedure '%s' returned "
                                     "'%s' as return value '%s' "
                                     "(#%d, type %s). "
                                     "This value is out of range."),
                                   ligma_object_get_name (procedure),
                                   value,
                                   g_param_spec_get_name (pspec),
                                   i + 1, g_type_name (spec_type));
                    }
                  else
                    {
                      g_set_error (error,
                                   LIGMA_PDB_ERROR,
                                   LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                                   _("Procedure '%s' has been called with "
                                     "value '%s' for argument '%s' "
                                     "(#%d, type %s). "
                                     "This value is out of range."),
                                   ligma_object_get_name (procedure),
                                   value,
                                   g_param_spec_get_name (pspec),
                                   i + 1, g_type_name (spec_type));
                    }
                }

              g_value_unset (&string_value);

              return FALSE;
            }

          /*  UTT-8 validate all strings  */
          if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_STRING ||
              (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_BOXED &&
               G_PARAM_SPEC_VALUE_TYPE (pspec) == G_TYPE_STRV))
            {
              gboolean valid = TRUE;

              if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_STRING)
                {
                  const gchar *string = g_value_get_string (arg);

                  if (string)
                    valid = g_utf8_validate (string, -1, NULL);
                }
              else
                {
                  const gchar **array = g_value_get_boxed (arg);

                  if (array)
                    {
                      gint i;

                      for (i = 0; array[i]; i++)
                        {
                          valid = g_utf8_validate (array[i], -1, NULL);
                          if (! valid)
                            break;
                        }
                    }
                }

              if (! valid)
                {
                  if (return_vals)
                    {
                      g_set_error (error,
                                   LIGMA_PDB_ERROR,
                                   LIGMA_PDB_ERROR_INVALID_RETURN_VALUE,
                                   _("Procedure '%s' returned an "
                                     "invalid UTF-8 string for argument '%s'."),
                                   ligma_object_get_name (procedure),
                                   g_param_spec_get_name (pspec));
                    }
                  else
                    {
                      g_set_error (error,
                                   LIGMA_PDB_ERROR,
                                   LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                                   _("Procedure '%s' has been called with an "
                                     "invalid UTF-8 string for argument '%s'."),
                                   ligma_object_get_name (procedure),
                                   g_param_spec_get_name (pspec));
                    }

                  g_value_unset (&string_value);

                  return FALSE;
                }
            }

          g_value_unset (&string_value);
        }
    }

  return TRUE;
}
