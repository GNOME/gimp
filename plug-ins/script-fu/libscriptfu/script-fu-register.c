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

#include <glib.h>

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <libligma/ligma.h>

#include "tinyscheme/scheme-private.h"

#include "script-fu-types.h"
#include "script-fu-script.h"
#include "script-fu-register.h"

/* Methods for a script's call to script-fu-register or script-fu-register-filter.
 * Such calls declare a PDB procedure, that ScriptFu will register in the PDB,
 * that the script implements by its inner run func.
 * These methods are only creating structs local to ScriptFu, used later to register.
 */



/* Traverse Scheme argument list
 * creating a new SFScript with metadata, but empty SFArgs (formal arg specs)
 *
 * Takes a handle to a pointer into the argument list.
 * Advances the pointer past the metadata args.
 *
 * Returns new SFScript.
 */
SFScript*
script_fu_script_new_from_metadata_args (scheme  *sc,
                                         pointer *handle)
{
  SFScript    *script;
  const gchar *name;
  const gchar *menu_label;
  const gchar *blurb;
  const gchar *author;
  const gchar *copyright;
  const gchar *date;
  const gchar *image_types;
  guint        n_args;

  /* dereference handle into local pointer. */
  pointer a = *handle;

  g_debug ("script_fu_script_new_from_metadata_args");

  /* Require list_length starting at a is >=7
   * else strange parsing errors at plugin query time.
   */

  name = sc->vptr->string_value (sc->vptr->pair_car (a));
  a = sc->vptr->pair_cdr (a);
  menu_label = sc->vptr->string_value (sc->vptr->pair_car (a));
  a = sc->vptr->pair_cdr (a);
  blurb = sc->vptr->string_value (sc->vptr->pair_car (a));
  a = sc->vptr->pair_cdr (a);
  author = sc->vptr->string_value (sc->vptr->pair_car (a));
  a = sc->vptr->pair_cdr (a);
  copyright = sc->vptr->string_value (sc->vptr->pair_car (a));
  a = sc->vptr->pair_cdr (a);
  date = sc->vptr->string_value (sc->vptr->pair_car (a));
  a = sc->vptr->pair_cdr (a);

  if (sc->vptr->is_pair (a))
    {
      image_types = sc->vptr->string_value (sc->vptr->pair_car (a));
      a = sc->vptr->pair_cdr (a);
    }
  else
    {
      image_types = sc->vptr->string_value (a);
      a = sc->NIL;
    }

  /* Store local, advanced pointer at handle from caller. */
  *handle = a;

  /* Calculate supplied number of formal arguments of the PDB procedure,
   * each takes three actual args from Scheme call.
   */
  n_args = sc->vptr->list_length (sc, a) / 3;

  /* This allocates empty array of SFArg. Hereafter, script knows its n_args. */
  script = script_fu_script_new (name,
                                 menu_label,
                                 blurb,
                                 author,
                                 copyright,
                                 date,
                                 image_types,
                                 n_args);
  return script;
}

/* Traverse suffix of Scheme argument list,
 * creating SFArgs (formal arg specs) from triplets.
 *
 * Takes a handle to a pointer into the argument list.
 * Advances the pointer past the triplets.
 * Changes state of SFScript.args[]
 *
 * Returns a foreign_error or NIL.
 */
pointer
script_fu_script_create_formal_args (scheme   *sc,
                                     pointer  *handle,
                                     SFScript *script)
{
  /* dereference handle into local pointer. */
  pointer a = *handle;

  g_debug ("script_fu_script_create_formal_args");

  for (guint i = 0; i < script->n_args; i++)
    {
      SFArg *arg = &script->args[i];

      if (a != sc->NIL)
        {
          if (!sc->vptr->is_integer (sc->vptr->pair_car (a)))
            return foreign_error (sc, "script-fu-register: argument types must be integer values", 0);

          arg->type = sc->vptr->ivalue (sc->vptr->pair_car (a));
          a = sc->vptr->pair_cdr (a);
        }
      else
        return foreign_error (sc, "script-fu-register: missing type specifier", 0);

      if (a != sc->NIL)
        {
          if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
            return foreign_error (sc, "script-fu-register: argument labels must be strings", 0);

          arg->label = g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
          a = sc->vptr->pair_cdr (a);
        }
      else
        return foreign_error (sc, "script-fu-register: missing arguments label", 0);

      if (a != sc->NIL)
        {
          switch (arg->type)
            {
            case SF_IMAGE:
            case SF_DRAWABLE:
            case SF_LAYER:
            case SF_CHANNEL:
            case SF_VECTORS:
            case SF_DISPLAY:
              if (!sc->vptr->is_integer (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: default IDs must be integer values", 0);

              arg->default_value.sfa_image =
                sc->vptr->ivalue (sc->vptr->pair_car (a));
              break;

            case SF_COLOR:
              if (sc->vptr->is_string (sc->vptr->pair_car (a)))
                {
                  if (! ligma_rgb_parse_css (&arg->default_value.sfa_color,
                                            sc->vptr->string_value (sc->vptr->pair_car (a)),
                                            -1))
                    return foreign_error (sc, "script-fu-register: invalid default color name", 0);

                  ligma_rgb_set_alpha (&arg->default_value.sfa_color, 1.0);
                }
              else if (sc->vptr->is_list (sc, sc->vptr->pair_car (a)) &&
                       sc->vptr->list_length(sc, sc->vptr->pair_car (a)) == 3)
                {
                  pointer color_list;
                  guchar  r, g, b;

                  color_list = sc->vptr->pair_car (a);
                  r = CLAMP (sc->vptr->ivalue (sc->vptr->pair_car (color_list)), 0, 255);
                  color_list = sc->vptr->pair_cdr (color_list);
                  g = CLAMP (sc->vptr->ivalue (sc->vptr->pair_car (color_list)), 0, 255);
                  color_list = sc->vptr->pair_cdr (color_list);
                  b = CLAMP (sc->vptr->ivalue (sc->vptr->pair_car (color_list)), 0, 255);

                  ligma_rgb_set_uchar (&arg->default_value.sfa_color, r, g, b);
                }
              else
                {
                  return foreign_error (sc, "script-fu-register: color defaults must be a list of 3 integers or a color name", 0);
                }
              break;

            case SF_TOGGLE:
              if (!sc->vptr->is_integer (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: toggle default must be an integer value", 0);

              arg->default_value.sfa_toggle =
                (sc->vptr->ivalue (sc->vptr->pair_car (a))) ? TRUE : FALSE;
              break;

            case SF_VALUE:
              if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: value defaults must be string values", 0);

              arg->default_value.sfa_value =
                g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
              break;

            case SF_STRING:
            case SF_TEXT:
              if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: string defaults must be string values", 0);

              arg->default_value.sfa_value =
                g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
              break;

            case SF_ADJUSTMENT:
              {
                pointer adj_list;

                if (!sc->vptr->is_list (sc, a))
                  return foreign_error (sc, "script-fu-register: adjustment defaults must be a list", 0);

                adj_list = sc->vptr->pair_car (a);
                arg->default_value.sfa_adjustment.value =
                  sc->vptr->rvalue (sc->vptr->pair_car (adj_list));

                adj_list = sc->vptr->pair_cdr (adj_list);
                arg->default_value.sfa_adjustment.lower =
                  sc->vptr->rvalue (sc->vptr->pair_car (adj_list));

                adj_list = sc->vptr->pair_cdr (adj_list);
                arg->default_value.sfa_adjustment.upper =
                  sc->vptr->rvalue (sc->vptr->pair_car (adj_list));

                adj_list = sc->vptr->pair_cdr (adj_list);
                arg->default_value.sfa_adjustment.step =
                  sc->vptr->rvalue (sc->vptr->pair_car (adj_list));

                adj_list = sc->vptr->pair_cdr (adj_list);
                arg->default_value.sfa_adjustment.page =
                  sc->vptr->rvalue (sc->vptr->pair_car (adj_list));

                adj_list = sc->vptr->pair_cdr (adj_list);
                arg->default_value.sfa_adjustment.digits =
                  sc->vptr->ivalue (sc->vptr->pair_car (adj_list));

                adj_list = sc->vptr->pair_cdr (adj_list);
                arg->default_value.sfa_adjustment.type =
                  sc->vptr->ivalue (sc->vptr->pair_car (adj_list));
              }
              break;

            case SF_FILENAME:
              if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: filename defaults must be string values", 0);
              /* fallthrough */

            case SF_DIRNAME:
              if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: dirname defaults must be string values", 0);

              arg->default_value.sfa_file.filename =
                g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));

#ifdef G_OS_WIN32
              {
                /* Replace POSIX slashes with Win32 backslashes. This
                 * is just so script-fus can be written with only
                 * POSIX directory separators.
                 */
                gchar *filename = arg->default_value.sfa_file.filename;

                while (*filename)
                  {
                    if (*filename == '/')
                      *filename = G_DIR_SEPARATOR;

                    filename++;
                  }
              }
#endif
              break;

            case SF_FONT:
              if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: font defaults must be string values", 0);

              arg->default_value.sfa_font =
                g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
              break;

            case SF_PALETTE:
              if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: palette defaults must be string values", 0);

              arg->default_value.sfa_palette =
                g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
              break;

            case SF_PATTERN:
              if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: pattern defaults must be string values", 0);

              arg->default_value.sfa_pattern =
                g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
              break;

            case SF_BRUSH:
              {
                pointer brush_list;

                if (!sc->vptr->is_list (sc, a))
                  return foreign_error (sc, "script-fu-register: brush defaults must be a list", 0);

                brush_list = sc->vptr->pair_car (a);
                arg->default_value.sfa_brush.name =
                  g_strdup (sc->vptr->string_value (sc->vptr->pair_car (brush_list)));

                brush_list = sc->vptr->pair_cdr (brush_list);
                arg->default_value.sfa_brush.opacity =
                  sc->vptr->rvalue (sc->vptr->pair_car (brush_list));

                brush_list = sc->vptr->pair_cdr (brush_list);
                arg->default_value.sfa_brush.spacing =
                  sc->vptr->ivalue (sc->vptr->pair_car (brush_list));

                brush_list = sc->vptr->pair_cdr (brush_list);
                arg->default_value.sfa_brush.paint_mode =
                  sc->vptr->ivalue (sc->vptr->pair_car (brush_list));
              }
              break;

            case SF_GRADIENT:
              if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: gradient defaults must be string values", 0);

              arg->default_value.sfa_gradient =
                g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
              break;

            case SF_OPTION:
              {
                pointer option_list;

                if (!sc->vptr->is_list (sc, a))
                  return foreign_error (sc, "script-fu-register: option defaults must be a list", 0);

                for (option_list = sc->vptr->pair_car (a);
                     option_list != sc->NIL;
                     option_list = sc->vptr->pair_cdr (option_list))
                  {
                    arg->default_value.sfa_option.list =
                      g_slist_append (arg->default_value.sfa_option.list,
                                      g_strdup (sc->vptr->string_value
                                                (sc->vptr->pair_car (option_list))));
                  }
              }
              break;

            case SF_ENUM:
              {
                pointer      option_list;
                const gchar *val;
                gchar       *type_name;
                GEnumValue  *enum_value;
                GType        enum_type;

                if (!sc->vptr->is_list (sc, a))
                  return foreign_error (sc, "script-fu-register: enum defaults must be a list", 0);

                option_list = sc->vptr->pair_car (a);
                if (!sc->vptr->is_string (sc->vptr->pair_car (option_list)))
                  return foreign_error (sc, "script-fu-register: first element in enum defaults must be a type-name", 0);

                val = sc->vptr->string_value (sc->vptr->pair_car (option_list));

                if (g_str_has_prefix (val, "Ligma"))
                  type_name = g_strdup (val);
                else
                  type_name = g_strconcat ("Ligma", val, NULL);

                enum_type = g_type_from_name (type_name);
                if (! G_TYPE_IS_ENUM (enum_type))
                  {
                    g_free (type_name);
                    return foreign_error (sc, "script-fu-register: first element in enum defaults must be the name of a registered type", 0);
                  }

                arg->default_value.sfa_enum.type_name = type_name;

                option_list = sc->vptr->pair_cdr (option_list);
                if (!sc->vptr->is_string (sc->vptr->pair_car (option_list)))
                  return foreign_error (sc, "script-fu-register: second element in enum defaults must be a string", 0);

                enum_value =
                  g_enum_get_value_by_nick (g_type_class_peek (enum_type),
                                            sc->vptr->string_value (sc->vptr->pair_car (option_list)));
                if (enum_value)
                  arg->default_value.sfa_enum.history = enum_value->value;
              }
              break;
            }

          a = sc->vptr->pair_cdr (a);
        }
      else
        {
          return foreign_error (sc, "script-fu-register: missing default argument", 0);
        }
    } /* end for */

  /* Store local, advanced pointer at handle from caller. */
  *handle = a;

  return sc->NIL;
}

/* Traverse next arg in Scheme argument list.
 * Set SFScript.drawable_arity from the argument.
 * Used only by script-fu-register-filter.
 *
 * Return foreign_error or NIL.
 */
pointer
script_fu_script_parse_drawable_arity_arg (scheme   *sc,
                                           pointer  *handle,
                                           SFScript *script)
{
  /* dereference handle into local pointer. */
  pointer a = *handle;

  /* argument must be an int, usually a symbol from enum e.g. SF-MULTIPLE-DRAWABLE */
  if (!sc->vptr->is_integer (sc->vptr->pair_car (a)))
    return foreign_error (sc, "script-fu-register-filter: drawable arity must be integer value", 0);
  script->drawable_arity = sc->vptr->ivalue (sc->vptr->pair_car (a));

  /* Advance the pointer into script. */
  a = sc->vptr->pair_cdr (a);
  *handle = a;
  return sc->NIL;
}
