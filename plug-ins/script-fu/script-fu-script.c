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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "script-fu-types.h"

#include "script-fu-script.h"


/*
 *  Function definitions
 */

SFScript *
script_fu_script_new (const gchar *name,
                      const gchar *menu_path,
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
  script->menu_path   = g_strdup (menu_path);
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
  g_free (script->menu_path);
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
          g_slist_foreach (arg->default_value.sfa_option.list,
                           (GFunc) g_free, NULL);
          g_slist_free (arg->default_value.sfa_option.list);
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
  const gchar  *menu_path = NULL;
  GimpParamDef *args;
  gint          i;

  g_return_if_fail (script != NULL);
  g_return_if_fail (run_proc != NULL);

  /* Allow scripts with no menus */
  if (strncmp (script->menu_path, "<None>", 6) != 0)
    menu_path = script->menu_path;

  args = g_new0 (GimpParamDef, script->n_args + 1);

  args[0].type        = GIMP_PDB_INT32;
  args[0].name        = "run-mode";
  args[0].description = "Interactive, non-interactive";

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
                          menu_path,
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
