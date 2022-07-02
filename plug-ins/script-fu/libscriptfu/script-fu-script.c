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
#include <libgimp/gimpui.h>

#include "tinyscheme/scheme-private.h"

#include "script-fu-types.h"
#include "script-fu-arg.h"
#include "script-fu-script.h"
#include "script-fu-intl.h"


/*
 *  Local Functions
 */

static gboolean   script_fu_script_param_init (SFScript             *script,
                                               const GimpValueArray *args,
                                               SFArgType             type,
                                               gint                  n);




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

  script->n_args = n_args;
  script->args   = g_new0 (SFArg, script->n_args);

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
                               SFScript    *script,
                               GimpRunFunc  run_func)
{
  GimpProcedure *procedure;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (script != NULL);
  g_return_if_fail (run_func != NULL);

  procedure = script_fu_script_create_PDB_procedure (plug_in,
                                                     script,
                                                     run_func,
                                                     GIMP_PDB_PROC_TYPE_TEMPORARY);

  gimp_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);
}


/*
 * Create and return a GimpProcedure.
 * Caller typically either:
 *    install it owned by self as TEMPORARY type procedure
 *    OR return it as the result of a create_procedure callback from GIMP (PLUGIN type procedure.)
 *
 * Caller must unref the procedure.
 */
GimpProcedure *
script_fu_script_create_PDB_procedure (GimpPlugIn     *plug_in,
                                       SFScript       *script,
                                       GimpRunFunc     run_func,
                                       GimpPDBProcType plug_in_type)
{
  GimpProcedure *procedure;
  const gchar   *menu_label            = NULL;

  g_debug ("script_fu_script_create_PDB_procedure: %s of type %i", script->name, plug_in_type);

  /* Allow scripts with no menus */
  if (strncmp (script->menu_label, "<None>", 6) != 0)
    menu_label = script->menu_label;

  procedure = gimp_procedure_new (plug_in, script->name,
                                  plug_in_type,
                                  run_func, script, NULL);

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

  gimp_procedure_add_argument (procedure,
                               g_param_spec_enum ("run-mode",
                                                  "Run mode",
                                                  "The run mode",
                                                  GIMP_TYPE_RUN_MODE,
                                                  GIMP_RUN_INTERACTIVE,
                                                  G_PARAM_READWRITE));

  script_fu_arg_reset_name_generator ();
  for (gint i = 0; i < script->n_args; i++)
    {
      GParamSpec  *pspec = NULL;
      const gchar *name  = NULL;
      const gchar *nick  = NULL;

      script_fu_arg_generate_name_and_nick (&script->args[i], &name, &nick);
      pspec = script_fu_arg_get_param_spec (&script->args[i],
                                            name,
                                            nick);
      gimp_procedure_add_argument (procedure, pspec);
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

gint
script_fu_script_collect_standard_args (SFScript             *script,
                                        const GimpValueArray *args)
{
  gint params_consumed = 0;

  g_return_val_if_fail (script != NULL, 0);

  /*  the first parameter may be a DISPLAY id  */
  if (script_fu_script_param_init (script,
                                   args, SF_DISPLAY,
                                   params_consumed))
    {
      params_consumed++;
    }

  /*  an IMAGE id may come first or after the DISPLAY id  */
  if (script_fu_script_param_init (script,
                                   args, SF_IMAGE,
                                   params_consumed))
    {
      params_consumed++;

      /*  and may be followed by a DRAWABLE, LAYER, CHANNEL or
       *  VECTORS id
       */
      if (script_fu_script_param_init (script,
                                       args, SF_DRAWABLE,
                                       params_consumed) ||
          script_fu_script_param_init (script,
                                       args, SF_LAYER,
                                       params_consumed) ||
          script_fu_script_param_init (script,
                                       args, SF_CHANNEL,
                                       params_consumed) ||
          script_fu_script_param_init (script,
                                       args, SF_VECTORS,
                                       params_consumed))
        {
          params_consumed++;
        }
    }

  return params_consumed;
}

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
                                          const GimpValueArray *args)
{
  GString *s;
  gint     i;

  g_return_val_if_fail (script != NULL, NULL);

  s = g_string_new ("(");
  g_string_append (s, script->name);

  for (i = 0; i < script->n_args; i++)
    {
      GValue *value = gimp_value_array_index (args, i + 1);

      g_string_append_c (s, ' ');

      script_fu_arg_append_repr_from_gvalue (&script->args[i],
                                             s,
                                             value);
    }

  g_string_append_c (s, ')');

  return g_string_free (s, FALSE);
}


/*
 *  Local Functions
 */

static gboolean
script_fu_script_param_init (SFScript             *script,
                             const GimpValueArray *args,
                             SFArgType             type,
                             gint                  n)
{
  SFArg *arg = &script->args[n];

  if (script->n_args > n &&
      arg->type == type  &&
      gimp_value_array_length (args) > n + 1)
    {
      GValue *value = gimp_value_array_index (args, n + 1);

      switch (type)
        {
        case SF_IMAGE:
          if (GIMP_VALUE_HOLDS_IMAGE (value))
            {
              GimpImage *image = g_value_get_object (value);

              arg->value.sfa_image = gimp_image_get_id (image);
              return TRUE;
            }
          break;

        case SF_DRAWABLE:
          if (GIMP_VALUE_HOLDS_DRAWABLE (value))
            {
              GimpItem *item = g_value_get_object (value);

              arg->value.sfa_drawable = gimp_item_get_id (item);
              return TRUE;
            }
          break;

        case SF_LAYER:
          if (GIMP_VALUE_HOLDS_LAYER (value))
            {
              GimpItem *item = g_value_get_object (value);

              arg->value.sfa_layer = gimp_item_get_id (item);
              return TRUE;
            }
          break;

        case SF_CHANNEL:
          if (GIMP_VALUE_HOLDS_CHANNEL (value))
            {
              GimpItem *item = g_value_get_object (value);

              arg->value.sfa_channel = gimp_item_get_id (item);
              return TRUE;
            }
          break;

        case SF_VECTORS:
          if (GIMP_VALUE_HOLDS_VECTORS (value))
            {
              GimpItem *item = g_value_get_object (value);

              arg->value.sfa_vectors = gimp_item_get_id (item);
              return TRUE;
            }
          break;

        case SF_DISPLAY:
          if (GIMP_VALUE_HOLDS_DISPLAY (value))
            {
              GimpDisplay *display = g_value_get_object (value);

              arg->value.sfa_display = gimp_display_get_id (display);
              return TRUE;
            }
          break;

        default:
          break;
        }
    }

  return FALSE;
}
