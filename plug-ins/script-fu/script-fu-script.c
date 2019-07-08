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

#include "script-fu-script.h"
#include "script-fu-scripts.h"
#include "script-fu-utils.h"

#include "script-fu-intl.h"


/*
 *  Local Functions
 */

static gboolean   script_fu_script_param_init (SFScript        *script,
                                               gint             nparams,
                                               const GimpParam *params,
                                               SFArgType        type,
                                               gint             n);


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
      SFArg *arg = &script->args[i];

      g_free (arg->label);

      switch (arg->type)
        {
        case SF_IMAGE:
        case SF_DRAWABLE:
        case SF_LAYER:
        case SF_CHANNEL:
        case SF_VECTORS:
        case SF_DISPLAY:
        case SF_COLOR:
        case SF_TOGGLE:
          break;

        case SF_VALUE:
        case SF_STRING:
        case SF_TEXT:
          g_free (arg->default_value.sfa_value);
          g_free (arg->value.sfa_value);
          break;

        case SF_ADJUSTMENT:
          break;

        case SF_FILENAME:
        case SF_DIRNAME:
          g_free (arg->default_value.sfa_file.filename);
          g_free (arg->value.sfa_file.filename);
          break;

        case SF_FONT:
          g_free (arg->default_value.sfa_font);
          g_free (arg->value.sfa_font);
          break;

        case SF_PALETTE:
          g_free (arg->default_value.sfa_palette);
          g_free (arg->value.sfa_palette);
          break;

        case SF_PATTERN:
          g_free (arg->default_value.sfa_pattern);
          g_free (arg->value.sfa_pattern);
          break;

        case SF_GRADIENT:
          g_free (arg->default_value.sfa_gradient);
          g_free (arg->value.sfa_gradient);
          break;

        case SF_BRUSH:
          g_free (arg->default_value.sfa_brush.name);
          g_free (arg->value.sfa_brush.name);
          break;

        case SF_OPTION:
          g_slist_free_full (arg->default_value.sfa_option.list,
                             (GDestroyNotify) g_free);
          break;

        case SF_ENUM:
          g_free (arg->default_value.sfa_enum.type_name);
          break;
        }
    }

  g_free (script->args);

  g_slice_free (SFScript, script);
}

void
script_fu_script_install_proc (SFScript    *script,
                               GimpRunProc  run_proc)
{
  const gchar  *menu_label = NULL;
  GimpParamDef *args;
  gint          i;

  g_return_if_fail (script != NULL);
  g_return_if_fail (run_proc != NULL);

  /* Allow scripts with no menus */
  if (strncmp (script->menu_label, "<None>", 6) != 0)
    menu_label = script->menu_label;

  args = g_new0 (GimpParamDef, script->n_args + 1);

  args[0].type        = GIMP_PDB_INT32;
  args[0].name        = "run-mode";
  args[0].description = "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }";

  for (i = 0; i < script->n_args; i++)
    {
      GimpPDBArgType  type = 0;
      const gchar    *name = NULL;

      switch (script->args[i].type)
        {
        case SF_IMAGE:
          type = GIMP_PDB_IMAGE;
          name = "image";
          break;

        case SF_DRAWABLE:
          type = GIMP_PDB_DRAWABLE;
          name = "drawable";
          break;

        case SF_LAYER:
          type = GIMP_PDB_LAYER;
          name = "layer";
          break;

        case SF_CHANNEL:
          type = GIMP_PDB_CHANNEL;
          name = "channel";
          break;

        case SF_VECTORS:
          type = GIMP_PDB_VECTORS;
          name = "vectors";
          break;

        case SF_DISPLAY:
          type = GIMP_PDB_DISPLAY;
          name = "display";
          break;

        case SF_COLOR:
          type = GIMP_PDB_COLOR;
          name = "color";
          break;

        case SF_TOGGLE:
          type = GIMP_PDB_INT32;
          name = "toggle";
          break;

        case SF_VALUE:
          type = GIMP_PDB_STRING;
          name = "value";
          break;

        case SF_STRING:
        case SF_TEXT:
          type = GIMP_PDB_STRING;
          name = "string";
          break;

        case SF_ADJUSTMENT:
          type = GIMP_PDB_FLOAT;
          name = "value";
          break;

        case SF_FILENAME:
          type = GIMP_PDB_STRING;
          name = "filename";
          break;

        case SF_DIRNAME:
          type = GIMP_PDB_STRING;
          name = "dirname";
          break;

        case SF_FONT:
          type = GIMP_PDB_STRING;
          name = "font";
          break;

        case SF_PALETTE:
          type = GIMP_PDB_STRING;
          name = "palette";
          break;

        case SF_PATTERN:
          type = GIMP_PDB_STRING;
          name = "pattern";
          break;

        case SF_BRUSH:
          type = GIMP_PDB_STRING;
          name = "brush";
          break;

        case SF_GRADIENT:
          type = GIMP_PDB_STRING;
          name = "gradient";
          break;

        case SF_OPTION:
          type = GIMP_PDB_INT32;
          name = "option";
          break;

        case SF_ENUM:
          type = GIMP_PDB_INT32;
          name = "enum";
          break;
        }

      args[i + 1].type        = type;
      args[i + 1].name        = (gchar *) name;
      args[i + 1].description = script->args[i].label;
    }

  gimp_install_temp_proc (script->name,
                          script->blurb,
                          "",
                          script->author,
                          script->copyright,
                          script->date,
                          menu_label,
                          script->image_types,
                          GIMP_TEMPORARY,
                          script->n_args + 1, 0,
                          args, NULL,
                          run_proc);

  g_free (args);
}

void
script_fu_script_uninstall_proc (SFScript *script)
{
  g_return_if_fail (script != NULL);

  gimp_uninstall_temp_proc (script->name);
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
      SFArgValue *value         = &script->args[i].value;
      SFArgValue *default_value = &script->args[i].default_value;

      switch (script->args[i].type)
        {
        case SF_IMAGE:
        case SF_DRAWABLE:
        case SF_LAYER:
        case SF_CHANNEL:
        case SF_VECTORS:
        case SF_DISPLAY:
          if (reset_ids)
            value->sfa_image = default_value->sfa_image;
          break;

        case SF_COLOR:
          value->sfa_color = default_value->sfa_color;
          break;

        case SF_TOGGLE:
          value->sfa_toggle = default_value->sfa_toggle;
          break;

        case SF_VALUE:
        case SF_STRING:
        case SF_TEXT:
          g_free (value->sfa_value);
          value->sfa_value = g_strdup (default_value->sfa_value);
          break;

        case SF_ADJUSTMENT:
          value->sfa_adjustment.value = default_value->sfa_adjustment.value;
          break;

        case SF_FILENAME:
        case SF_DIRNAME:
          g_free (value->sfa_file.filename);
          value->sfa_file.filename = g_strdup (default_value->sfa_file.filename);
          break;

        case SF_FONT:
          g_free (value->sfa_font);
          value->sfa_font = g_strdup (default_value->sfa_font);
          break;

        case SF_PALETTE:
          g_free (value->sfa_palette);
          value->sfa_palette = g_strdup (default_value->sfa_palette);
          break;

        case SF_PATTERN:
          g_free (value->sfa_pattern);
          value->sfa_pattern = g_strdup (default_value->sfa_pattern);
          break;

        case SF_GRADIENT:
          g_free (value->sfa_gradient);
          value->sfa_gradient = g_strdup (default_value->sfa_gradient);
          break;

        case SF_BRUSH:
          g_free (value->sfa_brush.name);
          value->sfa_brush.name = g_strdup (default_value->sfa_brush.name);
          value->sfa_brush.opacity    = default_value->sfa_brush.opacity;
          value->sfa_brush.spacing    = default_value->sfa_brush.spacing;
          value->sfa_brush.paint_mode = default_value->sfa_brush.paint_mode;
          break;

        case SF_OPTION:
          value->sfa_option.history = default_value->sfa_option.history;
          break;

        case SF_ENUM:
          value->sfa_enum.history = default_value->sfa_enum.history;
          break;
        }
    }
}

gint
script_fu_script_collect_standard_args (SFScript        *script,
                                        gint             n_params,
                                        const GimpParam *params)
{
  gint params_consumed = 0;

  g_return_val_if_fail (script != NULL, 0);

  /*  the first parameter may be a DISPLAY id  */
  if (script_fu_script_param_init (script,
                                   n_params, params, SF_DISPLAY,
                                   params_consumed))
    {
      params_consumed++;
    }

  /*  an IMAGE id may come first or after the DISPLAY id  */
  if (script_fu_script_param_init (script,
                                   n_params, params, SF_IMAGE,
                                   params_consumed))
    {
      params_consumed++;

      /*  and may be followed by a DRAWABLE, LAYER, CHANNEL or
       *  VECTORS id
       */
      if (script_fu_script_param_init (script,
                                       n_params, params, SF_DRAWABLE,
                                       params_consumed) ||
          script_fu_script_param_init (script,
                                       n_params, params, SF_LAYER,
                                       params_consumed) ||
          script_fu_script_param_init (script,
                                       n_params, params, SF_CHANNEL,
                                       params_consumed) ||
          script_fu_script_param_init (script,
                                       n_params, params, SF_VECTORS,
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
      SFArgValue *arg_value = &script->args[i].value;

      g_string_append_c (s, ' ');

      switch (script->args[i].type)
        {
        case SF_IMAGE:
        case SF_DRAWABLE:
        case SF_LAYER:
        case SF_CHANNEL:
        case SF_VECTORS:
        case SF_DISPLAY:
          g_string_append_printf (s, "%d", arg_value->sfa_image);
          break;

        case SF_COLOR:
          {
            guchar r, g, b;

            gimp_rgb_get_uchar (&arg_value->sfa_color, &r, &g, &b);
            g_string_append_printf (s, "'(%d %d %d)",
                                    (gint) r, (gint) g, (gint) b);
          }
          break;

        case SF_TOGGLE:
          g_string_append (s, arg_value->sfa_toggle ? "TRUE" : "FALSE");
          break;

        case SF_VALUE:
          g_string_append (s, arg_value->sfa_value);
          break;

        case SF_STRING:
        case SF_TEXT:
          {
            gchar *tmp;

            tmp = script_fu_strescape (arg_value->sfa_value);
            g_string_append_printf (s, "\"%s\"", tmp);
            g_free (tmp);
          }
          break;

        case SF_ADJUSTMENT:
          {
            gchar buffer[G_ASCII_DTOSTR_BUF_SIZE];

            g_ascii_dtostr (buffer, sizeof (buffer),
                            arg_value->sfa_adjustment.value);
            g_string_append (s, buffer);
          }
          break;

        case SF_FILENAME:
        case SF_DIRNAME:
          {
            gchar *tmp;

            tmp = script_fu_strescape (arg_value->sfa_file.filename);
            g_string_append_printf (s, "\"%s\"", tmp);
            g_free (tmp);
          }
          break;

        case SF_FONT:
          g_string_append_printf (s, "\"%s\"", arg_value->sfa_font);
          break;

        case SF_PALETTE:
          g_string_append_printf (s, "\"%s\"", arg_value->sfa_palette);
          break;

        case SF_PATTERN:
          g_string_append_printf (s, "\"%s\"", arg_value->sfa_pattern);
          break;

        case SF_GRADIENT:
          g_string_append_printf (s, "\"%s\"", arg_value->sfa_gradient);
          break;

        case SF_BRUSH:
          {
            gchar buffer[G_ASCII_DTOSTR_BUF_SIZE];

            g_ascii_dtostr (buffer, sizeof (buffer),
                            arg_value->sfa_brush.opacity);
            g_string_append_printf (s, "'(\"%s\" %s %d %d)",
                                    arg_value->sfa_brush.name,
                                    buffer,
                                    arg_value->sfa_brush.spacing,
                                    arg_value->sfa_brush.paint_mode);
          }
          break;

        case SF_OPTION:
          g_string_append_printf (s, "%d", arg_value->sfa_option.history);
          break;

        case SF_ENUM:
          g_string_append_printf (s, "%d", arg_value->sfa_enum.history);
          break;
        }
    }

  g_string_append_c (s, ')');

  return g_string_free (s, FALSE);
}

gchar *
script_fu_script_get_command_from_params (SFScript        *script,
                                          const GimpParam *params)
{
  GString *s;
  gint     i;

  g_return_val_if_fail (script != NULL, NULL);

  s = g_string_new ("(");
  g_string_append (s, script->name);

  for (i = 0; i < script->n_args; i++)
    {
      const GimpParam *param = &params[i + 1];

      g_string_append_c (s, ' ');

      switch (script->args[i].type)
        {
        case SF_IMAGE:
        case SF_DRAWABLE:
        case SF_LAYER:
        case SF_CHANNEL:
        case SF_VECTORS:
        case SF_DISPLAY:
          g_string_append_printf (s, "%d", param->data.d_int32);
          break;

        case SF_COLOR:
          {
            guchar r, g, b;

            gimp_rgb_get_uchar (&param->data.d_color, &r, &g, &b);
            g_string_append_printf (s, "'(%d %d %d)",
                                    (gint) r, (gint) g, (gint) b);
          }
          break;

        case SF_TOGGLE:
          g_string_append_printf (s, (param->data.d_int32 ? "TRUE" : "FALSE"));
          break;

        case SF_VALUE:
          g_string_append (s, param->data.d_string);
          break;

        case SF_STRING:
        case SF_TEXT:
        case SF_FILENAME:
        case SF_DIRNAME:
          {
            gchar *tmp;

            tmp = script_fu_strescape (param->data.d_string);
            g_string_append_printf (s, "\"%s\"", tmp);
            g_free (tmp);
          }
          break;

        case SF_ADJUSTMENT:
          {
            gchar buffer[G_ASCII_DTOSTR_BUF_SIZE];

            g_ascii_dtostr (buffer, sizeof (buffer), param->data.d_float);
            g_string_append (s, buffer);
          }
          break;

        case SF_FONT:
        case SF_PALETTE:
        case SF_PATTERN:
        case SF_GRADIENT:
        case SF_BRUSH:
          g_string_append_printf (s, "\"%s\"", param->data.d_string);
          break;

        case SF_OPTION:
        case SF_ENUM:
          g_string_append_printf (s, "%d", param->data.d_int32);
          break;
        }
    }

  g_string_append_c (s, ')');

  return g_string_free (s, FALSE);
}


/*
 *  Local Functions
 */

static gboolean
script_fu_script_param_init (SFScript        *script,
                             gint             nparams,
                             const GimpParam *params,
                             SFArgType        type,
                             gint             n)
{
  SFArg *arg = &script->args[n];

  if (script->n_args > n && arg->type == type && nparams > n + 1)
    {
      switch (type)
        {
        case SF_IMAGE:
          if (params[n + 1].type == GIMP_PDB_IMAGE)
            {
              arg->value.sfa_image = params[n + 1].data.d_image;
              return TRUE;
            }
          break;

        case SF_DRAWABLE:
          if (params[n + 1].type == GIMP_PDB_DRAWABLE)
            {
              arg->value.sfa_drawable = params[n + 1].data.d_drawable;
              return TRUE;
            }
          break;

        case SF_LAYER:
          if (params[n + 1].type == GIMP_PDB_LAYER)
            {
              arg->value.sfa_layer = params[n + 1].data.d_layer;
              return TRUE;
            }
          break;

        case SF_CHANNEL:
          if (params[n + 1].type == GIMP_PDB_CHANNEL)
            {
              arg->value.sfa_channel = params[n + 1].data.d_channel;
              return TRUE;
            }
          break;

        case SF_VECTORS:
          if (params[n + 1].type == GIMP_PDB_VECTORS)
            {
              arg->value.sfa_vectors = params[n + 1].data.d_vectors;
              return TRUE;
            }
          break;

        case SF_DISPLAY:
          if (params[n + 1].type == GIMP_PDB_DISPLAY)
            {
              arg->value.sfa_display = params[n + 1].data.d_display;
              return TRUE;
            }
          break;

        default:
          break;
        }
    }

  return FALSE;
}
