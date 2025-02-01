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

#include <string.h>

#include <libgimp/gimp.h>

#include "tinyscheme/scheme-private.h"

#include "script-fu-types.h"
#include "script-fu-arg.h"
#include "script-fu-script.h"
#include "script-fu-run-func.h"

#include "script-fu-intl.h"


/*
 *  Local Functions
 */

static gboolean script_fu_script_param_init (SFScript             *script,
                                             GParamSpec          **pspecs,
                                             guint                 n_pspecs,
                                             GimpProcedureConfig  *config,
                                             SFArgType             type,
                                             gint                  n);
static void     script_fu_script_set_proc_metadata (
                                             GimpProcedure        *procedure,
                                             SFScript             *script);
static void     script_fu_script_set_proc_args (
                                             GimpProcedure        *procedure,
                                             SFScript             *script,
                                             guint                 first_conveyed_arg);
static void     script_fu_script_set_drawable_sensitivity (
                                             GimpProcedure        *procedure,
                                             SFScript             *script);

static void     script_fu_command_append_drawables (
                                             GString              *s,
                                             GimpDrawable        **drawables);
/*
 *  Function definitions
 */

SFScript *
script_fu_script_new (const gchar *name,
                      const gchar *menu_label,
                      const gchar *blurb,
                      const gchar *author,
                      const gchar *copyright,
                      const gchar *date,
                      const gchar *image_types,
                      gint         n_args)
{
  SFScript *script;

  script = g_slice_new0 (SFScript);

  script->name        = g_strdup (name);
  script->menu_label  = g_strdup (menu_label);
  script->blurb       = g_strdup (blurb);
  script->author      = g_strdup (author);
  script->copyright   = g_strdup (copyright);
  script->date        = g_strdup (date);
  script->image_types = g_strdup (image_types);
  script->i18n_domain_name           = NULL;
  script->i18n_catalog_relative_path = NULL;

  script->n_args = n_args;
  script->args   = g_new0 (SFArg, script->n_args);

  script->drawable_arity = SF_NO_DRAWABLE; /* default */

  return script;
}

void
script_fu_script_free (SFScript *script)
{
  gint i;

  g_return_if_fail (script != NULL);

  g_free (script->name);
  g_free (script->blurb);
  g_free (script->menu_label);
  g_free (script->author);
  g_free (script->copyright);
  g_free (script->date);
  g_free (script->image_types);
  g_free (script->i18n_domain_name);
  g_free (script->i18n_catalog_relative_path);

  for (i = 0; i < script->n_args; i++)
    {
      script_fu_arg_free (&script->args[i]);
    }

  g_free (script->args);

  g_slice_free (SFScript, script);
}


/*
 * From the script, create a temporary PDB procedure,
 * and install it as owned by the scriptfu extension PDB proc.
 */
void
script_fu_script_install_proc (GimpPlugIn  *plug_in,
                               SFScript    *script)
{
  GimpProcedure *procedure;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (script != NULL);

  procedure = script_fu_script_create_PDB_procedure (plug_in,
                                                     script,
                                                     GIMP_PDB_PROC_TYPE_TEMPORARY);

  gimp_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);
}


/*
 * Create and return a GimpProcedure or its subclass GimpImageProcedure.
 * Caller typically either:
 *    install it owned by self as TEMPORARY type procedure
 *    OR return it as the result of a create_procedure callback from GIMP (PLUGIN type procedure.)
 *
 * Caller must unref the procedure.
 *
 * Understands ScriptFu's internal run funcs for GimpProcedure and GimpImageProcedure
 */
GimpProcedure *
script_fu_script_create_PDB_procedure (GimpPlugIn     *plug_in,
                                       SFScript       *script,
                                       GimpPDBProcType plug_in_type)
{
  GimpProcedure *procedure;

  if (script->proc_class == GIMP_TYPE_IMAGE_PROCEDURE)
    {
      g_debug ("%s: %s, plugin type %i, image_proc", G_STRFUNC, script->name, plug_in_type);

      procedure = gimp_image_procedure_new (plug_in, script->name,
                                            plug_in_type,
                                            (GimpRunImageFunc) script_fu_run_image_procedure,
                                            script, /* user_data, pointer in extension-script-fu process */
                                            NULL);

      script_fu_script_set_proc_metadata (procedure, script);

      /* Script author does not declare image, drawable in script-fu-register-filter,
       * and we don't add to formal args in PDB.
       * The convenience class GimpImageProcedure already has formal args:
       * run_mode, image, drawables.
       * "0" means not skip any arguments declared in the script.
       */
      script_fu_script_set_proc_args (procedure, script, 0);

      /* The script declared arity, convey to GIMP. */
      script_fu_script_set_drawable_sensitivity (procedure, script);
    }
  else
    {
      /* Script declares a GimpProcedure.
       * Dispatch on which scheme function the script registered with.
       *
       * v2 had only script-fu-register.
       * For compatibility, we still support it,
       */
      g_assert (script->proc_class == GIMP_TYPE_PROCEDURE);

      if (script_fu_script_get_is_old_style (script))
        {
          g_debug ("%s: %s, plugin type %i, old-style", G_STRFUNC, script->name, plug_in_type);

          procedure = gimp_procedure_new (plug_in, script->name,
                                          plug_in_type,
                                          script_fu_run_procedure, /* old-style */
                                          script, NULL);

          script_fu_script_set_proc_metadata (procedure, script);

          gimp_procedure_add_enum_argument (procedure, "run-mode",
                                            "Run mode", "The run mode",
                                            GIMP_TYPE_RUN_MODE,
                                            GIMP_RUN_INTERACTIVE,
                                            G_PARAM_READWRITE);

          script_fu_script_set_proc_args (procedure, script, 0);

          /* !!! Author did not declare drawable arity, it was inferred. */
          script_fu_script_set_drawable_sensitivity (procedure, script);
        }
      else
        {
          g_debug ("%s: %s, plugin type %i, old-style", G_STRFUNC, script->name, plug_in_type);

          procedure = gimp_procedure_new (plug_in, script->name,
                                          plug_in_type,
                                          script_fu_run_regular_procedure,  /* new-style */
                                          script, NULL);

          script_fu_script_set_proc_metadata (procedure, script);

          gimp_procedure_add_enum_argument (procedure, "run-mode",
                                            "Run mode", "The run mode",
                                            GIMP_TYPE_RUN_MODE,
                                            GIMP_RUN_INTERACTIVE,
                                            G_PARAM_READWRITE);

          script_fu_script_set_proc_args (procedure, script, 0);

          /* The script did not declare arity.
           * new-style regular procedure always arity SF_NO_DRAWABLE.
           * This conveys to GIMP.
           */
          script_fu_script_set_drawable_sensitivity (procedure, script);
        }
    }
  return procedure;
}

void
script_fu_script_uninstall_proc (GimpPlugIn *plug_in,
                                 SFScript   *script)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (script != NULL);

  gimp_plug_in_remove_temp_procedure (plug_in, script->name);
}

gchar *
script_fu_script_get_title (SFScript *script)
{
  gchar *title;
  gchar *tmp;

  g_return_val_if_fail (script != NULL, NULL);

  /* strip mnemonics from the menupath */
  title = gimp_strip_uline (script->menu_label);

  /* if this looks like a full menu path, use only the last part */
  if (title[0] == '<' && (tmp = strrchr (title, '/')) && tmp[1])
    {
      tmp = g_strdup (tmp + 1);

      g_free (title);
      title = tmp;
    }

  /* cut off ellipsis */
  tmp = (strstr (title, "..."));
  if (! tmp)
    /* U+2026 HORIZONTAL ELLIPSIS */
    tmp = strstr (title, "\342\200\246");

  if (tmp && tmp == (title + strlen (title) - 3))
    *tmp = '\0';

  return title;
}

void
script_fu_script_reset (SFScript *script,
                        gboolean  reset_ids)
{
  gint i;

  g_return_if_fail (script != NULL);

  for (i = 0; i < script->n_args; i++)
    {
      script_fu_arg_reset (&script->args[i], reset_ids);
    }
}

/* FUTURE: goes away when deprecated script-fu-register is obsoleted. */
gint
script_fu_script_collect_standard_args (SFScript             *script,
                                        GParamSpec          **pspecs,
                                        guint                 n_pspecs,
                                        GimpProcedureConfig  *config)
{
  gint params_consumed = 0;

  g_return_val_if_fail (script != NULL, 0);

  /*  the first parameter may be a DISPLAY id  */
  if (script_fu_script_param_init (script, pspecs, n_pspecs,
                                   config, SF_DISPLAY,
                                   params_consumed))
    {
      params_consumed++;
    }

  /*  an IMAGE id may come first or after the DISPLAY id  */
  if (script_fu_script_param_init (script, pspecs, n_pspecs,
                                   config, SF_IMAGE,
                                   params_consumed))
    {
      params_consumed++;

      /*  and may be followed by a DRAWABLE, LAYER, CHANNEL or
       *  PATH id
       */
      if (script_fu_script_param_init (script, pspecs, n_pspecs,
                                       config, SF_DRAWABLE,
                                       params_consumed) ||
          script_fu_script_param_init (script, pspecs, n_pspecs,
                                       config, SF_LAYER,
                                       params_consumed) ||
          script_fu_script_param_init (script, pspecs, n_pspecs,
                                       config, SF_CHANNEL,
                                       params_consumed) ||
          script_fu_script_param_init (script, pspecs, n_pspecs,
                                       config, SF_VECTORS,
                                       params_consumed))
        {
          params_consumed++;
        }
    }

  return params_consumed;
}

/* Methods that form "commands" i.e. texts in Scheme language
 * that represent calls to the inner run func defined in a script.
 */

gchar *
script_fu_script_get_command (SFScript *script)
{
  GString *s;
  gint     i;

  g_return_val_if_fail (script != NULL, NULL);

  s = g_string_new ("(");
  g_string_append (s, script->name);

  for (i = 0; i < script->n_args; i++)
    {
      g_string_append_c (s, ' ');

      script_fu_arg_append_repr_from_self (&script->args[i], s);
    }

  g_string_append_c (s, ')');

  return g_string_free (s, FALSE);
}

gchar *
script_fu_script_get_command_from_params (SFScript             *script,
                                          GParamSpec          **pspecs,
                                          guint                 n_pspecs,
                                          GimpProcedureConfig  *config)
{
  GString *s;
  gint     i;

  g_return_val_if_fail (script != NULL, NULL);

  s = g_string_new ("(");
  g_string_append (s, script->name);

  for (i = 0; i < script->n_args; i++)
    {
      GValue      value = G_VALUE_INIT;
      GParamSpec *pspec = pspecs[i + SF_ARG_TO_CONFIG_OFFSET];

      g_value_init (&value, pspec->value_type);
      g_object_get_property (G_OBJECT (config), pspec->name, &value);

      g_string_append_c (s, ' ');

      script_fu_arg_append_repr_from_gvalue (&script->args[i],
                                             s,
                                             &value);
      g_value_unset (&value);
    }

  g_string_append_c (s, ')');

  return g_string_free (s, FALSE);
}

/* Append a literal representing a Scheme container of numerics
 * where the numerics are the ID's of the given drawables.
 * Container is scheme vector, meaning its elements are all the same type.
 */
static void
script_fu_command_append_drawables (GString       *s,
                                    GimpDrawable **drawables)
{
  /* Require non-empty array of drawables. */
  g_assert (drawables != NULL && drawables[0] != NULL);

  /* !!! leading space to separate from prior args.
   * #() is scheme syntax for a vector.
   */
  g_string_append (s, " #(" );
  for (guint i = 0; drawables[i] != NULL; i++)
    {
      g_string_append_printf (s, " %d", gimp_item_get_id ((GimpItem*) drawables[i]));
    }
  g_string_append (s, ")" );
  /* Ensure string is like: " #( 1 2 3)" */
}


gchar *
script_fu_script_get_command_for_image_proc (SFScript            *script,
                                             GimpImage            *image,
                                             GimpDrawable        **drawables,
                                             GimpProcedureConfig  *config)
{
  GParamSpec **pspecs;
  guint        n_pspecs;
  GString     *s;
  gint         i;

  g_return_val_if_fail (script != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_PROCEDURE_CONFIG (config), NULL);

  s = g_string_new ("(");
  g_string_append (s, script->name);

  /* The command has no run mode. */

  /* scripts use integer ID's for Gimp objects. */
  g_string_append_printf (s, " %d", gimp_image_get_id (image));

  /* Append text repr for a container of all drawable ID's.
   * Even if script->drawable_arity = SF_PROC_IMAGE_SINGLE_DRAWABLE
   * since that means the inner run func takes many but will only process one.
   * We are not adapting to an inner run func that expects a single numeric.
   */
  script_fu_command_append_drawables (s, drawables);

  /* config contains the "other" args
   * Iterate over the GimpValueArray.
   * But script->args should be the same length, and types should match.
   */
  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config), &n_pspecs);

  /* config will have 1 additional property: "procedure". */
  for (i = 1; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];
      GValue      value = G_VALUE_INIT;

      g_string_append_c (s, ' ');

      g_value_init (&value, pspec->value_type);
      g_object_get_property (G_OBJECT (config), pspec->name, &value);
      script_fu_arg_append_repr_from_gvalue (&script->args[i - 1], s, &value);
      g_value_unset (&value);
    }

  g_string_append_c (s, ')');
  g_free (pspecs);

  return g_string_free (s, FALSE);
}

/* The difference between count of config properties and count of script args.
* config will have 2 extra, leading property: "procedure" and "run_mode".
*/
#define SF_ARGS_TO_CONFIG_OFFSET 2

gchar *
script_fu_script_get_command_for_regular_proc (SFScript            *script,
                                               GimpProcedureConfig *config)
{
  GParamSpec **pspecs;
  guint        n_pspecs;
  GString     *s;
  gint         i;

  g_return_val_if_fail (script != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_PROCEDURE_CONFIG (config), NULL);

  s = g_string_new ("(");
  g_string_append (s, script->name);

  /* The command has no run mode. */

  /* config contains the "other" args
   * Iterate over the GimpValueArray, starting at an offset.
   * But script->args should be the same length, and types should match.
   */
  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config), &n_pspecs);

  for (i = SF_ARGS_TO_CONFIG_OFFSET; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];
      GValue      value = G_VALUE_INIT;

      g_string_append_c (s, ' ');

      g_value_init (&value, pspec->value_type);
      g_object_get_property (G_OBJECT (config), pspec->name, &value);
      script_fu_arg_append_repr_from_gvalue (&script->args[i - SF_ARGS_TO_CONFIG_OFFSET],
                                             s, &value);
      g_value_unset (&value);
    }

  g_string_append_c (s, ')');
  g_free (pspecs);

  return g_string_free (s, FALSE);
}

/* Infer whether the script, defined using v2 script-fu-register,
 * which does not specify the arity for drawables,
 * is actually a script that takes one and only one drawable.
 * Such plugins are deprecated in v3: each plugin must take container of drawables
 * and declare its drawable arity via gimp_procedure_set_sensitivity_mask.
 */
void
script_fu_script_infer_drawable_arity (SFScript *script)
{
  if ((script->n_args > 1) &&
      script->args[0].type == SF_IMAGE &&
      script->args[1].type == SF_DRAWABLE)
    {
      g_debug ("Inferring drawable arity one.");
      script->drawable_arity = SF_ONE_DRAWABLE;
    }
}

/*
 *  Local Functions
 */

/* FUTURE: goes away when deprecated script-fu-register is obsoleted. */
static gboolean
script_fu_script_param_init (SFScript             *script,
                             GParamSpec          **pspecs,
                             guint                 n_pspecs,
                             GimpProcedureConfig  *config,
                             SFArgType             type,
                             gint                  n)
{
  SFArg   *arg    = &script->args[n];
  gboolean result = FALSE;

  if (script->n_args > n &&
      arg->type == type  &&
      /* The first pspec is "procedure", the second is "run-mode". */
      n_pspecs > n + SF_ARG_TO_CONFIG_OFFSET)
    {
      GValue      value = G_VALUE_INIT;
      GParamSpec *pspec = pspecs[n + SF_ARG_TO_CONFIG_OFFSET];

      g_value_init (&value, pspec->value_type);
      g_object_get_property (G_OBJECT (config), pspec->name, &value);
      /* When value is object, must be unreffed. */

      switch (type)
        {
        case SF_IMAGE:
          if (GIMP_VALUE_HOLDS_IMAGE (&value))
            {
              GimpImage *image = g_value_get_object (&value);

              arg->value.sfa_image = gimp_image_get_id (image);
              result = TRUE;
            }
          break;

        case SF_DRAWABLE:
          if (GIMP_VALUE_HOLDS_DRAWABLE (&value))
            {
              GimpItem *item = g_value_get_object (&value);

              arg->value.sfa_drawable = gimp_item_get_id (item);
              result = TRUE;
            }
          break;

        case SF_LAYER:
          if (GIMP_VALUE_HOLDS_LAYER (&value))
            {
              GimpItem *item = g_value_get_object (&value);

              arg->value.sfa_layer = gimp_item_get_id (item);
              result = TRUE;
            }
          break;

        case SF_CHANNEL:
          if (GIMP_VALUE_HOLDS_CHANNEL (&value))
            {
              GimpItem *item = g_value_get_object (&value);

              arg->value.sfa_channel = gimp_item_get_id (item);
              result = TRUE;
            }
          break;

        case SF_VECTORS:
          if (GIMP_VALUE_HOLDS_PATH (&value))
            {
              GimpItem *item = g_value_get_object (&value);

              arg->value.sfa_vectors = gimp_item_get_id (item);
              result = TRUE;
            }
          break;

        case SF_DISPLAY:
          if (GIMP_VALUE_HOLDS_DISPLAY (&value))
            {
              GimpDisplay *display = g_value_get_object (&value);

              arg->value.sfa_display = gimp_display_get_id (display);
              result = TRUE;
            }
          break;

        default:
          break;
        }

      /* unset will unref when is-a object. */
      g_value_unset (&value);
    }
    /* Else not enough args or pspecs */

  return result;
}


static void
script_fu_script_set_proc_metadata (GimpProcedure *procedure,
                                    SFScript      *script)
{
  const gchar *menu_label = NULL;

  /* Allow scripts with no menus */
  if (strncmp (script->menu_label, "<None>", 6) != 0)
    menu_label = script->menu_label;

  gimp_procedure_set_image_types (procedure, script->image_types);

  if (menu_label && strlen (menu_label))
    gimp_procedure_set_menu_label (procedure, menu_label);

  gimp_procedure_set_documentation (procedure,
                                    script->blurb,
                                    NULL,
                                    script->name);
  gimp_procedure_set_attribution (procedure,
                                  script->author,
                                  script->copyright,
                                  script->date);
}

/* Convey formal arguments from SFArg to the PDB. */
static void
script_fu_script_set_proc_args (GimpProcedure *procedure,
                                SFScript      *script,
                                guint          first_conveyed_arg)
{
  script_fu_arg_reset_name_generator ();
  for (gint i = first_conveyed_arg; i < script->n_args; i++)
    {
      const gchar *name = NULL;
      const gchar *nick = NULL;

      script_fu_arg_generate_name_and_nick (&script->args[i], &name, &nick);
      script_fu_arg_add_argument (&script->args[i], procedure, name, nick);
    }
}

/* Convey drawable arity to the PDB.
 * !!! Unless set, sensitivity defaults to drawable arity 1.
 * See libgimp/gimpprocedure.c gimp_procedure_set_sensitivity_mask
 */
static void
script_fu_script_set_drawable_sensitivity (GimpProcedure *procedure, SFScript *script)
{
  switch (script->drawable_arity)
    {
    case SF_TWO_OR_MORE_DRAWABLE:
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLES);
      break;
    case SF_ONE_OR_MORE_DRAWABLE:
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE |
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLES);
      break;
    case SF_ONE_DRAWABLE:
      gimp_procedure_set_sensitivity_mask (procedure, GIMP_PROCEDURE_SENSITIVE_DRAWABLE);
      break;
    case SF_NO_DRAWABLE:
      /* menu item always sensitive. */
      break;
    default:
      /* Fail to set sensitivy mask. */
      g_warning ("Unhandled case for SFDrawableArity");
    }
}

/* Set drawable arity of script to NONE, meaning a regular procedure,
 * always enabled (sensitive)
 */
void
script_fu_script_set_drawable_arity_none (SFScript *script)
{
  script->drawable_arity = SF_NO_DRAWABLE;
}

/* Only settable, never cleared, defaults to false. */
void
script_fu_script_set_is_old_style (SFScript *script)
{
  script->is_old_style = TRUE;
}

gboolean
script_fu_script_get_is_old_style (SFScript *script)
{
  return script->is_old_style;
}

/* Set script's i18n from strings owned by inner interpreter.
 * First free any existing data since might have been set already:
 * a script author may mistakenly call script-fu-register-i18n twice
 * for the same procedure.
 */
void
script_fu_script_set_i18n (SFScript  *script,
                           gchar     *domain,
                           gchar     *catalog)
{
  g_free (script->i18n_domain_name);
  g_free (script->i18n_catalog_relative_path);
  script->i18n_domain_name           = g_strdup (domain);
  script->i18n_catalog_relative_path = g_strdup (catalog);
}

/* Return a copy of script's i18n, to the handles.
 *
 * Require *handles is NULL, not already allocated.
 *
 * May return NULL, when script author has not called script-fu-register-i18n.
 */
void
script_fu_script_get_i18n (SFScript  *script,
                           gchar    **domain,
                           gchar    **catalog)
{
  *domain  = g_strdup (script->i18n_domain_name);
  *catalog = g_strdup (script->i18n_catalog_relative_path);
}

