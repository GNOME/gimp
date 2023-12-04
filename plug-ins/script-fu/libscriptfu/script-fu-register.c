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

#include <glib.h>

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <libgimp/gimp.h>

#include "tinyscheme/scheme-private.h"

#include "script-fu-types.h"
#include "script-fu-script.h"
#include "script-fu-register.h"
#include "script-fu-errors.h"
#include "script-fu-resource.h"
#include "scheme-marshal.h"


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


/* Parse a default spec from registration data.
 *
 * Side effects on arg.
 *
 * Returns sc->NIL on success.
 * Returns pointer to a foreign_error on parsing errors.
 *
 * A default_spec can be an atom or a list.
 * An atom is a single default value.
 * A list for SF-COLOR is also a default value.
 * In other cases, a list is a default and constraints.
 * Some constraints are declared to and enforced by the PDB.
 * Some constraints also convey to the widget for the arg.
 *
 * We check that each list is the correct length,
 * so we don't car off the end of the list.
 *
 * We don't check the types of list elements.
 * When they are not the correct type,
 * they are *some* scheme object and getting the numeric value
 * will return *some* value probably not the intended value,
 * but at least not a memory error or crash.
 */
static pointer
script_fu_parse_default_spec (scheme   *sc,
                              pointer   default_spec,
                              SFArg    *arg)
{
  switch (arg->type)
    {
    case SF_IMAGE:
    case SF_DRAWABLE:
    case SF_LAYER:
    case SF_CHANNEL:
    case SF_VECTORS:
    case SF_DISPLAY:
      if (!sc->vptr->is_integer (default_spec))
        return registration_error (sc, "default IDs must be integers");

      arg->default_value.sfa_image =
        sc->vptr->ivalue (default_spec);
      break;

    case SF_COLOR:
      if (sc->vptr->is_string (default_spec))
        {
          /* default_spec is name of default */
          gchar *name_of_default = sc->vptr->string_value (default_spec);

          if (! sf_color_arg_set_default_by_name (arg, name_of_default))
            return registration_error (sc, "invalid default color name");
        }
      else if (sc->vptr->is_list (sc, default_spec))
        {
          /* default_spec is list of numbers. */
          GeglColor *color = marshal_component_list_to_color (sc, default_spec);

          if (color == NULL)
            {
              return registration_error (sc, "color default list of integers not encode a color");
            }
          else
            {
              sf_color_arg_set_default_by_color (arg, color);
              g_object_unref (color);
            }
        }
      else
        {
          return registration_error (sc, "color defaults must be a list of 3 integers or a color name");
        }
      break;

    case SF_TOGGLE:
      /* Accept scheme boolean or int.
       * This does not vary by language version, and makes language v2 more lenient.
       */
      /* Note storing internally as a C int, which the widget wants.
       * Elsewhere we marshal back to a Scheme data.
       *
       * Note that is_false is not exported from scheme.c, we compare Scheme pointers.
       *
       * The default value is from evaluating a Scheme expression.
       * More in keeping with Scheme, should convert any value other than #f to C truth.
       * Instead, convert only literal #t to C truth.
       */
      if (sc->vptr->is_integer (default_spec))
        arg->default_value.sfa_toggle = (sc->vptr->ivalue (default_spec)) ? TRUE : FALSE;
      else if (default_spec == sc->T)
        arg->default_value.sfa_toggle = 1;
      else if (default_spec == sc->F)
        arg->default_value.sfa_toggle = 0;
      else
        return registration_error (sc, "toggle default must yield an integer, #t, or #f");
      break;

    case SF_VALUE:
      if (!sc->vptr->is_string (default_spec))
        return registration_error (sc, "value defaults must be strings");

      arg->default_value.sfa_value =
        g_strdup (sc->vptr->string_value (default_spec));
      break;

    case SF_STRING:
    case SF_TEXT:
      if (!sc->vptr->is_string (default_spec))
        return registration_error (sc, "string defaults must be strings");

      arg->default_value.sfa_value =
        g_strdup (sc->vptr->string_value (default_spec));
      break;

    case SF_ADJUSTMENT:
      {
        pointer adj_list;

        if (!sc->vptr->is_list (sc, default_spec) &&
            sc->vptr->list_length (sc, default_spec) != 7)
          return registration_error (sc, "adjustment defaults must be a list of 7 elements");

        adj_list = default_spec;
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
      if (!sc->vptr->is_string (default_spec))
        return registration_error (sc, "filename defaults must be strings");
      /* fallthrough */

    case SF_DIRNAME:
      if (!sc->vptr->is_string (default_spec))
        return registration_error (sc, "dirname defaults must be strings");

      arg->default_value.sfa_file.filename =
        g_strdup (sc->vptr->string_value (default_spec));

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
    case SF_PALETTE:
    case SF_PATTERN:
    case SF_BRUSH:
    case SF_GRADIENT:
      /* Default_spec given by author is a name. */
      if (!sc->vptr->is_string (default_spec))
        return registration_error (sc, "resource defaults must be strings");

      sf_resource_set_default (&arg->default_value.sfa_resource,
                               sc->vptr->string_value (default_spec));
      break;

    case SF_OPTION:
      {
        pointer option_list;

        if (!sc->vptr->is_list (sc, default_spec) ||
            sc->vptr->list_length(sc, default_spec) < 1 )
          return registration_error (sc, "option defaults must be a non-empty list");

        for (option_list = default_spec;
              option_list != sc->NIL;
              option_list = sc->vptr->pair_cdr (option_list))
          {
            pointer option = (sc->vptr->pair_car (option_list));
            if (sc->vptr->is_string (option))
              arg->default_value.sfa_option.list =
                g_slist_append (arg->default_value.sfa_option.list,
                                g_strdup (sc->vptr->string_value (option)));
            else
              return registration_error (sc, "options must be strings");
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

        if (!sc->vptr->is_list (sc, default_spec))
          return registration_error (sc, "enum defaults must be a list");

        option_list = default_spec;
        if (!sc->vptr->is_string (sc->vptr->pair_car (option_list)))
          return registration_error (sc, "first element in enum defaults must be a type-name");

        val = sc->vptr->string_value (sc->vptr->pair_car (option_list));

        if (g_str_has_prefix (val, "Gimp"))
          type_name = g_strdup (val);
        else
          type_name = g_strconcat ("Gimp", val, NULL);

        enum_type = g_type_from_name (type_name);
        if (! G_TYPE_IS_ENUM (enum_type))
          {
            g_free (type_name);
            return registration_error (sc, "first element in enum defaults must be the name of a registered type");
          }

        arg->default_value.sfa_enum.type_name = type_name;

        option_list = sc->vptr->pair_cdr (option_list);
        if (!sc->vptr->is_string (sc->vptr->pair_car (option_list)))
          return registration_error (sc, "second element in enum defaults must be a string");

        enum_value =
          g_enum_get_value_by_nick (g_type_class_peek (enum_type),
                                    sc->vptr->string_value (sc->vptr->pair_car (option_list)));
        if (enum_value)
          arg->default_value.sfa_enum.history = enum_value->value;
      }
      break;
    } /* end switch */

  /* success */
  return sc->NIL;
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
            return registration_error (sc, "argument types must be integers");

          arg->type = sc->vptr->ivalue (sc->vptr->pair_car (a));
          a = sc->vptr->pair_cdr (a);
        }
      else
        return registration_error (sc, "missing type specifier");

      if (a != sc->NIL)
        {
          if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
            return registration_error (sc, "argument labels must be strings");

          arg->label = g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
          a = sc->vptr->pair_cdr (a);
        }
      else
        return registration_error (sc, "missing arguments label");

      if (a != sc->NIL)
        {
          /* a is pointing into a sequence, grouped in three.
           * (car a) is the third part of three.
           * (cdr a) is the rest of the sequence
           */
          pointer default_spec =sc->vptr->pair_car (a);
          pointer error = script_fu_parse_default_spec (sc, default_spec, arg);

          if (error != sc->NIL)
            return error;
          else
            /* advance to next group of three in the sequence. */
            a = sc->vptr->pair_cdr (a);
        }
      else
        {
          return registration_error (sc, "missing default argument");
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
