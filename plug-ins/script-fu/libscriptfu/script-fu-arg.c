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

#include "script-fu-types.h"
#include "script-fu-arg.h"
#include "script-fu-utils.h"
#include "script-fu-version.h"


/*
 * Methods of SFArg.
 * SFArg is an informal class.
 * All methods take first argument SFArg*, i.e. self.
 *
 * A SFArg is similar to a GValue and a GParamSpec.
 * Like a GValue, it holds a value.
 * Like a GParamSpec, it is metadata and holds a default value.
 *
 * In GIMP 2, extension-script-fu stays running and keeps instances of SFArg in memory.
 * This is how ScriptFu "settings" aka "last values" are persistent for a session of GIMP.
 *
 * In GIMP 2, in the GUI implemented by ScriptFu (script-fu-interface.c),
 * initial values of widgets are taken from SFArg (s),
 * and result values of widgets are written back to SFArg.
 *
 * In GIMP 3, SFArg might be somewhat replaced with GimpConfig.
 * Then many of these methods are not needed.
 *
 * Roughly, the methods hide how to convert/represent SFArgs back/forth
 * to [GParamSpec, GValue, Scheme string representation.]
 *
 * Since SFArg is a union, similar to a GValue, the code is mostly switch on type.
 */

/*
 * An SFArg has a type SFArgType that denotes not only a type, but a kind of widget.
 * For example, SF_STRING denotes string type and a string entry widget,
 * while SF_TEXT denotes a string type and a multiline text editing widget.
 *
 * But the SFArgType:SF_ADJUSTMENT further specifies a kind of widget,
 * either spinner or slider.
 * I.E. SFArgType is not one-to-one with widget kind.
 *
 * Unlike PythonFu, there is no SFArgType.SF_INT.
 * Thus a ScriptFu author cannot specify an int-valued widget.
 * While Scheme speakers understand Scheme uses "numeric" for both float and int,
 * this might be confusing to script authors using other programming languages.
 *
 * SFArgType denotes not only a C type, but also a Scheme type.
 * For example, SF_ADJUSTMENT denotes the C type "float"
 * and the Scheme type "numeric" (which encompasses float and int.)
 * Another example, SF_PATTERN denotes the C type GimpPattern
 * and the Scheme type string (names of brushes are used in scripts.)
 */


static void pspec_set_default_file         (GParamSpec  *pspec,
                                            const gchar *filepath);
static void append_int_repr_from_gvalue    (GString     *result_string,
                                            GValue      *gvalue);
static void append_scheme_repr_of_c_string (const gchar *user_string,
                                            GString     *result_string);

static GString *script_fu_arg_get_blurb    (SFArg *arg);

/* Free any allocated members.
 * Somewhat hides what members of the SFArg struct are allocated.
 * !!! A few other places in the code do the allocations.
 * !!! A few other places in the code free members.
 */
void
script_fu_arg_free (SFArg *arg)
{
  g_free (arg->label);
  g_free (arg->property_name);

  switch (arg->type)
    {
    /* Integer ID's: primitives not needing free. */
    case SF_IMAGE:
    case SF_DRAWABLE:
    case SF_LAYER:
    case SF_CHANNEL:
    case SF_VECTORS:
    case SF_DISPLAY:
    case SF_COLOR:
    case SF_TOGGLE:
      break;

    case SF_BRUSH:
    case SF_FONT:
    case SF_GRADIENT:
    case SF_PALETTE:
    case SF_PATTERN:
      sf_resource_arg_free (arg);
      break;

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

    case SF_OPTION:
      g_slist_free_full (arg->default_value.sfa_option.list,
                         (GDestroyNotify) g_free);
      break;

    case SF_ENUM:
      g_free (arg->default_value.sfa_enum.type_name);
      break;
    }
}

/* Reset: copy the default value to current value.
 * This is only for old-style scripts and interface:
 * new-style scripts keep args in a ProcedureConfig, which is resettable.
 * Invoked when user chooses "Reset" button in the old-style dialog.
 *
 * Old-style reset is to the "factory default"
 * i.e. the default declared in the script.
 * New-style dialog also allows reset to "initial values"
 * which are the values when the dialog appeared,
 * which can be values from a previous run.
 */
void
script_fu_arg_reset (SFArg *arg, gboolean should_reset_ids)
{
  SFArgValue *value         = &arg->value;
  SFArgValue *default_value = &arg->default_value;

  switch (arg->type)
    {
    case SF_IMAGE:
    case SF_DRAWABLE:
    case SF_LAYER:
    case SF_CHANNEL:
    case SF_VECTORS:
    case SF_DISPLAY:

      if (should_reset_ids)
        {
          /* !!! Use field name "sfa_image"; all these cases have same type in union.
           * The field type is an int, this is an ID.
           * We can use the same trick to group other cases, below.
           */
          value->sfa_image = default_value->sfa_image;
        }

      break;

    case SF_BRUSH:
    case SF_FONT:
    case SF_GRADIENT:
    case SF_PALETTE:
    case SF_PATTERN:
      if (should_reset_ids)
        sf_resource_arg_reset (arg);
      break;

    case SF_COLOR:
      g_clear_object (&value->sfa_color);
      value->sfa_color = gegl_color_duplicate (default_value->sfa_color);
      break;

    case SF_TOGGLE:
      value->sfa_toggle = default_value->sfa_toggle;
      break;

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

    case SF_OPTION:
      value->sfa_option.history = default_value->sfa_option.history;
      break;

    case SF_ENUM:
      value->sfa_enum.history = default_value->sfa_enum.history;
      break;
    }
}

/* The type of a function that declares a formal, resource argument to a PDB procedure. */
typedef void (*ResourceArgDeclareFunc) (
  GimpProcedure*, const gchar*, const gchar*, const gchar*,
  gboolean, GimpResource*, gboolean, GParamFlags);



static void
script_fu_add_resource_arg_default_from_context (
  GimpProcedure          *procedure,
  const gchar            *name,
  const gchar            *nick,
  const gchar            *blurb,
  ResourceArgDeclareFunc  func)
{
  /* Default from context, dynamically: default:NULL, default_from_context:TRUE */
  (*func) (procedure, name, nick, blurb,
            FALSE,  /* none OK */
            NULL,   /* default */
            TRUE,   /* default_from_context */
            G_PARAM_READWRITE);
}


static void
script_fu_add_resource_arg_with_default (
  GimpProcedure          *procedure,
  const gchar            *name,
  const gchar            *nick,
  const gchar            *blurb,
  ResourceArgDeclareFunc  func,
  GimpResource           *default_resource)
{
  (*func) (procedure, name, nick, blurb,
            FALSE,  /* none OK */
            default_resource,
            FALSE,   /* default_from_context */
            G_PARAM_READWRITE);
}


/* Formally declare a resource arg of a PDB procedure.
 * From the given SFArg.
 */
static void
script_fu_add_resource_arg (
  GimpProcedure          *procedure,
  const gchar            *name,
  const gchar            *nick,
  const gchar            *blurb,
  SFArg                  *arg,
  ResourceArgDeclareFunc  func)
{
  /* Switch on script author's declared resource name. */
  gchar *declared_name_of_default = sf_resource_arg_get_name_default (arg);

  if (g_strcmp0 (declared_name_of_default, "from context") == 0 ||
      g_utf8_strlen (declared_name_of_default, 256) == 0
     )
    {
      /* Author declared default name is empty or the special "from context". */
      script_fu_add_resource_arg_default_from_context (procedure, name, nick, blurb, func);
    }
  else
    {
      /* Author declared default name should name a resource.
       * Declare the formal arg to default to the named resource.
       *
       * The declared name should be untranslated.
       * Note that names might not be unique e.g. for fonts.
       * Note that some resources have translated names e.g. generated gradients.
       *
       * The default resource could still be NULL if the declared name does not match.
       * Warn in that case, to the console, at procedure creation time.
       * Note we still pass none_OK:FALSE meaning the procedure expects
       * a non-NULL resource at call time (after GUI.)
       * In interactive mode, the GUI may default from context when passed a NULL default.
       */
      GimpResource *default_resource = sf_resource_arg_get_default (arg);

      if (default_resource == NULL)
        {
          /* Fonts are loaded asynchronously so they may not be loaded
           * yet when plug-in procedures' arguments are declared. That's
           * OK though because resource argument defaults are not stored
           * anyway. And when the procedure will be actually called
           * later on, the fonts will likely have been loaded by then.
           */
          if (arg->default_value.sfa_resource.resource_type != GIMP_TYPE_FONT)
            g_warning ("%s declared resource name is invalid %s", G_STRFUNC, declared_name_of_default);
          script_fu_add_resource_arg_default_from_context (procedure, name, nick, blurb, func);
        }
      else
        {
          /* Declared name is valid name of resource. */
          script_fu_add_resource_arg_with_default (procedure, name, nick, blurb, func,
                                                   default_resource);
        }
    }
}

/* Get the blurb of an argument.
 *
 * The blurb becomes the tooltip for widgets
 * and the description for the arg in the PDB.
 *
 * ScriptFu does not let authors declare blurb.
 * Instead we derive the blurb from a "label",
 * replacing the mnemonic/accelerator mark char.
 * An author declares a label i.e. menu item, with optional accelerator mark.
 * The label is a translated text.
 * SF also uses label for nick of the argument's param spec.
 *
 * For this implementation, the widget tooltip is redundant with the widget label,
 * but at least the PDB description is not empty.
 * The PDB description must not be empty because the author also cannot declare
 * the name (SF generates it) and the generated name is meaningless to a reader.
 * FUTURE: let author's declare a separate, translated blurb.
 * FUTURE: let author's declare a name for the arg (instead of generating one.)
 *
 * Caller owns the result.
 */
static GString*
script_fu_arg_get_blurb (SFArg *arg)
{
  GString *result = g_string_new (arg->label);

  g_string_replace (result, "_", "", 0);
  return result;
}


/* Return param spec that describes the arg.
 * Convert instance of SFArg to instance of GParamSpec.
 *
 * Used to specify an arg to the PDB proc which this script implements.
 * The GParamSpec is "floating" meaning ownership will transfer
 * to the GimpPDBProcedure.
 *
 * Ensure GParamSpec has a default except as noted below.
 * Default value from self.
 *
 * FUTURE: use GimpProcedureDialog
 * Because GimpProcedureDialog creates widgets from properties/paramspecs,
 * this should convey what SFArg denotes about desired widget kind,
 * but it doesn't fully do that yet.
 */
void
script_fu_arg_add_argument (SFArg         *arg,
                            GimpProcedure *procedure,
                            const gchar   *name,
                            const gchar   *nick)
{
  GString               *blurb_gstring = script_fu_arg_get_blurb (arg);
  gchar                 *blurb         = blurb_gstring->str;
  GimpFileChooserAction  action        = GIMP_FILE_CHOOSER_ACTION_ANY;

  switch (arg->type)
    {
      /* No defaults for GIMP objects: Image, Item subclasses, Display */
    case SF_IMAGE:
      gimp_procedure_add_image_argument (procedure, name,
                                         nick, blurb,
                                         TRUE,  /* None is valid. */
                                         G_PARAM_READWRITE);
      break;

    case SF_DRAWABLE:
      gimp_procedure_add_drawable_argument (procedure, name,
                                            nick, blurb,
                                            TRUE,
                                            G_PARAM_READWRITE);
      break;

    case SF_LAYER:
      gimp_procedure_add_layer_argument (procedure, name,
                                         nick, blurb,
                                         TRUE,
                                         G_PARAM_READWRITE);
      break;

    case SF_CHANNEL:
      gimp_procedure_add_channel_argument (procedure, name,
                                           nick, blurb,
                                           TRUE,
                                           G_PARAM_READWRITE);
      break;

    case SF_VECTORS:
      gimp_procedure_add_path_argument (procedure, name,
                                        nick, blurb,
                                        TRUE,
                                        G_PARAM_READWRITE);
      break;

    case SF_DISPLAY:
      gimp_procedure_add_display_argument (procedure, name,
                                           nick, blurb,
                                           TRUE,
                                           G_PARAM_READWRITE);
      break;

    case SF_COLOR:
      {
        GeglColor *color = sf_color_arg_get_default_color (arg);

        gimp_procedure_add_color_argument (procedure, name, nick, blurb, TRUE,
                                           color, G_PARAM_READWRITE);
        g_object_unref (color);
      }
      break;

    case SF_TOGGLE:
      /* Implicit conversion from gint32 to gboolean. */
      gimp_procedure_add_boolean_argument (procedure, name,
                                           nick, blurb,
                                           arg->default_value.sfa_toggle,
                                           G_PARAM_READWRITE);
      break;

    /* FUTURE special widgets for multiline text.
     * script-fu-interface does, but GimpProcedureDialog does not.
     */
    case SF_STRING:
    case SF_TEXT:
      gimp_procedure_add_string_argument (procedure, name,
                                          nick, blurb,
                                          arg->default_value.sfa_value,
                                          G_PARAM_READWRITE);
      break;

    /* Subclasses of GimpResource.  Special widgets. */
    case SF_FONT:
      /* Avoid compiler warning: cast a type specific function to a generic function. */
      script_fu_add_resource_arg (procedure, name, nick, blurb, arg,
          (ResourceArgDeclareFunc) gimp_procedure_add_font_argument);
      break;

    case SF_PALETTE:
      script_fu_add_resource_arg (procedure, name, nick, blurb, arg,
          (ResourceArgDeclareFunc) gimp_procedure_add_palette_argument);
      break;

    case SF_PATTERN:
      script_fu_add_resource_arg (procedure, name, nick, blurb, arg,
          (ResourceArgDeclareFunc) gimp_procedure_add_pattern_argument);
      break;

    case SF_GRADIENT:
      script_fu_add_resource_arg (procedure, name, nick, blurb, arg,
          (ResourceArgDeclareFunc) gimp_procedure_add_gradient_argument);
      break;

    case SF_BRUSH:
      script_fu_add_resource_arg (procedure, name, nick, blurb, arg,
          (ResourceArgDeclareFunc) gimp_procedure_add_brush_argument);
      break;

    case SF_ADJUSTMENT:
      /* switch on number of decimal places aka "digits
       * !!! on the default value, not the current value.
       * Decimal places == 0 means type integer, else float
       */
      if (arg->default_value.sfa_adjustment.digits == 0)
        gimp_procedure_add_int_argument (procedure, name, nick, blurb,
                                         arg->default_value.sfa_adjustment.lower,
                                         arg->default_value.sfa_adjustment.upper,
                                         arg->default_value.sfa_adjustment.value,
                                         G_PARAM_READWRITE);
      else
        gimp_procedure_add_double_argument (procedure, name, nick, blurb,
                                            arg->default_value.sfa_adjustment.lower,
                                            arg->default_value.sfa_adjustment.upper,
                                            arg->default_value.sfa_adjustment.value,
                                            G_PARAM_READWRITE);
      break;

    case SF_FILENAME:
      action = GIMP_FILE_CHOOSER_ACTION_OPEN;
    case SF_DIRNAME:
        {
          GParamSpec *pspec = NULL;

          if (action == GIMP_FILE_CHOOSER_ACTION_ANY)
            action = GIMP_FILE_CHOOSER_ACTION_SELECT_FOLDER;

          gimp_procedure_add_file_argument (procedure, name,
                                            nick, blurb,
                                            action, TRUE, NULL,
                                            G_PARAM_READWRITE |
                                            GIMP_PARAM_NO_VALIDATE);
          pspec = gimp_procedure_find_argument (procedure, name);
          pspec_set_default_file (pspec, arg->default_value.sfa_file.filename);
        }
      break;

      /* Not necessary to have a more specific pspec:
       * pspec = gimp_param_spec_config_path (...GIMP_TYPE_CONFIG_PATH...)
       */

    case SF_ENUM:
      /* history is the last used value AND the default. */
      gimp_procedure_add_enum_argument (procedure, name,
                                        nick, blurb,
                                        g_type_from_name (arg->default_value.sfa_enum.type_name),
                                        arg->default_value.sfa_enum.history,
                                        G_PARAM_READWRITE);
      break;

    case SF_OPTION:
      gimp_procedure_add_int_argument (procedure, name,
                                       nick, blurb,
                                       0,        /* Always zero based. */
                                       g_slist_length (arg->default_value.sfa_option.list) - 1,
                                       arg->default_value.sfa_option.history,
                                       G_PARAM_READWRITE);
      /* FUTURE: Model values not now appear in PDB browser NOR in widgets? */
      /* FUTURE: Does not show a combo box widget ??? */
      break;
    }

  g_string_free (blurb_gstring, TRUE);
}

/* Warn of an error in a GFile argument
 * and append a repr of an unknown file to the result_string,
 * which is a scheme text being built.
 * The warning is to the console.
 * The warning may be innocuous if the script handles the case.
 */
static void
sf_append_file_error_repr (gchar *err_message, GString *result_string )
{
  g_warning ("%s", err_message);
  /* Represent unknown file by literal for empty string: "" */
  g_string_append_printf (result_string, "\"\"");
}

/* Append a Scheme representation of the arg value from the given gvalue.
 * Append to a Scheme text to be interpreted.
 *
 * The SFArg only specifies the type,
 * but the GType held by the GValue must be the same or convertable.
 *
 * The repr comes from the value of the GValue, not the value of the SFArg.
 *
 * Used when GIMP is calling the PDB procedure implemented by the script,
 * passing a GValueArray.
 */
void
script_fu_arg_append_repr_from_gvalue (SFArg       *arg,
                                       GString     *result_string,
                                       GValue      *gvalue)
{
  g_debug ("script_fu_arg_append_repr_from_gvalue %s", arg->label);
  switch (arg->type)
    {
    case SF_IMAGE:
    case SF_DRAWABLE:
    case SF_LAYER:
    case SF_CHANNEL:
    case SF_VECTORS:
    case SF_DISPLAY:

    /* The GValue is a GObject of type inheriting GimpResource, having id prop */
    case SF_BRUSH:
    case SF_FONT:
    case SF_GRADIENT:
    case SF_PALETTE:
    case SF_PATTERN:

      {
        GObject *object = g_value_get_object (gvalue);
        gint     id     = -1;

        if (object)
          g_object_get (object, "id", &id, NULL);

        g_string_append_printf (result_string, "%d", id);
      }
      break;

    case SF_COLOR:
      {
        /* Since v3 PDB procedures traffic in GeglColor.
         * But ScriptFu still represents as RGB uint8 in a list.
         */
        GeglColor *color;

        color = g_value_get_object (gvalue);
        if (color)
          {
            gchar *repr = sf_color_get_repr_from_gegl_color (color);
            g_string_append (result_string, repr);
            g_free (repr);
          }
        else
          {
            gchar *msg = "Invalid color object in gvalue.";
            g_warning ("%s", msg);
            g_string_append_printf (result_string, "\"%s\"", msg);
          }
      }
      break;

    case SF_TOGGLE:
      if (is_interpret_v3_dialect ())
        {
          g_string_append (result_string, (g_value_get_boolean (gvalue) ? "#t" : "#f"));
        }
      else
        g_string_append (result_string, (g_value_get_boolean (gvalue) ? "TRUE" : "FALSE"));
      break;

    case SF_STRING:
    case SF_TEXT:
      append_scheme_repr_of_c_string (g_value_get_string (gvalue), result_string);
      break;

    case SF_FILENAME:
    case SF_DIRNAME:
      {
        if (G_VALUE_HOLDS_OBJECT (gvalue) && G_VALUE_TYPE (gvalue) == G_TYPE_FILE)
          {
            GFile *file = g_value_get_object (gvalue);

            /* Catch: GValue initialized to hold a GFile, but not hold one.
             * Specificially, GimpProcedureDialog can yield that condition;
             * the dialog shows "(None)" meaning user has not chosen a file yet.
             */
            if (G_IS_FILE (file))
              {
                /* Not checking file exists, only creating a descriptive string.
                 * I.E. not g_file_get_path, which can return NULL.
                 */
                gchar *filepath = g_file_get_parse_name (file);
                gchar *esc_path = script_fu_strescape (filepath);

                /* assert filepath not null. */
                g_string_append_printf (result_string, "\"%s\"", esc_path);
                g_free (filepath);
                g_free (esc_path);
              }
            else
              {
                /* This is usually a C NULL.
                 * Some file chooser widgets may not support a default file
                 * and/or may return NULL when the user doesn't choose a file.
                 */
                sf_append_file_error_repr ("Unknown file in GFile to script.",
                                           result_string);
              }
          }
        else
          {
            /* Programming error: GValue should hold a GFile. */
            sf_append_file_error_repr ("Expecting GFile in GValue to script.",
                                       result_string);
          }
        /* Ensure appended a filepath string OR a string repr of an unknown file. */
      }
      break;

    case SF_ADJUSTMENT:
      {
        if (arg->default_value.sfa_adjustment.digits != 0)
          {
            gchar buffer[G_ASCII_DTOSTR_BUF_SIZE];

            g_ascii_dtostr (buffer, sizeof (buffer), g_value_get_double (gvalue));
            g_string_append (result_string, buffer);
          }
        else
          {
            append_int_repr_from_gvalue (result_string, gvalue);
          }
      }
      break;

    case SF_OPTION:
      append_int_repr_from_gvalue (result_string, gvalue);
      break;

    case SF_ENUM:
      if (G_VALUE_HOLDS_ENUM (gvalue))
        {
          /* Effectively upcasting to a less restrictive Scheme class Integer. */
          g_string_append_printf (result_string, "%d", g_value_get_enum (gvalue));
        }
      else
        {
          /* For now, occurs when GimpConfig or GimpProcedureDialog does not support GParamEnum. */
          g_warning ("Expecting GValue holding a GEnum.");
          /* Append arbitrary int, so no errors in signature of Scheme call.
           * The call might not yield result the user intended.
           */
          g_string_append (result_string, "1");
        }
      break;
    }
}

/* Append a Scheme representation of the arg value from self's value.
 * Append to a Scheme text to be interpreted.
 *
 * Used when the PDB procedure implemented by the script is being calling interactively,
 * after a GUI dialog has written user's choices into self's value.
 *
 * This method is slated for deletion when script-fu-interface.c is deleted.
 */
void
script_fu_arg_append_repr_from_self (SFArg       *arg,
                                     GString     *result_string)
{
  SFArgValue *arg_value = &arg->value;

  switch (arg->type)
    {
    case SF_IMAGE:
    case SF_DRAWABLE:
    case SF_LAYER:
    case SF_CHANNEL:
    case SF_VECTORS:
    case SF_DISPLAY:
      g_string_append_printf (result_string, "%d", arg_value->sfa_image);
      break;

    case SF_BRUSH:
    case SF_FONT:
    case SF_GRADIENT:
    case SF_PALETTE:
    case SF_PATTERN:
      {
        gchar *repr = sf_resource_arg_get_repr (arg);
        g_string_append (result_string, repr);
        g_free (repr);
      }
      break;

    case SF_COLOR:
      {
        gchar *repr = sf_color_get_repr (arg_value->sfa_color);
        g_string_append (result_string, repr);
        g_free (repr);
      }
      break;

    case SF_TOGGLE:
      if (is_interpret_v3_dialect ())
        g_string_append (result_string, arg_value->sfa_toggle ? "#t" : "#f");
      else
        g_string_append (result_string, arg_value->sfa_toggle ? "TRUE" : "FALSE");
      break;

    case SF_STRING:
    case SF_TEXT:
      append_scheme_repr_of_c_string (arg_value->sfa_value, result_string);
      break;

    case SF_ADJUSTMENT:
      {
        gchar buffer[G_ASCII_DTOSTR_BUF_SIZE];

        g_ascii_dtostr (buffer, sizeof (buffer),
                        arg_value->sfa_adjustment.value);
        g_string_append (result_string, buffer);
      }
      break;

    case SF_FILENAME:
    case SF_DIRNAME:
      append_scheme_repr_of_c_string (arg_value->sfa_file.filename, result_string);
      break;

    case SF_OPTION:
      g_string_append_printf (result_string, "%d", arg_value->sfa_option.history);
      break;

    case SF_ENUM:
      g_string_append_printf (result_string, "%d", arg_value->sfa_enum.history);
      break;
    }
}


/* Array the size of the enum
 * Counts names generated per SF type per generator session.
 */
static gint arg_count[SF_DISPLAY + 1] = { 0, };

void
script_fu_arg_reset_name_generator (void)
{
  for (guint i = 0; i <= SF_DISPLAY; i++)
    arg_count[i] = 0;
}

/*
 * Return a unique name, and non-unique nick, for self.
 *
 * Self's label came from a call to script-fu-register ()
 * and was not lexically checked so is unsuitable for a property name.
 * ScriptFu does not require script author to provide a unique name
 * for args in a call to script-fu-register.
 *
 * This is a generator.
 * Returned name is a canonical name for a GParamSpec, i.e. a property name.
 * It meets the lexical requirements for a property name.
 * It is unique among all names returned between resets of the generator.
 * Thus name meets uniquity for names of properties of one object.
 *
 * !!! GimpImageProcedures already have properties for convenience arguments,
 * e.g. a property named "image" and "drawables"
 * So we avoid that name clash by starting with "otherImage"
 *
 * The name means nothing to human readers of the spec.
 * Instead, the nick is descriptive for human readers.
 *
 * The returned string is owned by the generator, a constant.
 * The caller need not copy it,
 * but usually does by creating a GParamSpec.
 *
 * As a side effect, a copy of the string is kept in arg->property_name.
 */
void
script_fu_arg_generate_name_and_nick (SFArg        *arg,
                                      const gchar **returned_name,
                                      const gchar **returned_nick)
{
  static gchar  numbered_name[64];
  gchar        *name = NULL;

  switch (arg->type)
    {
    case SF_IMAGE:
      name = "otherImage";  /* !!! Avoid name clash. */
      break;

    case SF_DRAWABLE:
      name = "drawable";
      break;

    case SF_LAYER:
      name = "layer";
      break;

    case SF_CHANNEL:
      name = "channel";
      break;

    case SF_VECTORS:
      name = "vectors";
      break;

    case SF_DISPLAY:
      name = "display";
      break;

    case SF_COLOR:
      name = "color";
      break;

    case SF_TOGGLE:
      name = "toggle";
      break;

    case SF_STRING:
      name = "string";
      break;

    case SF_TEXT:
      name = "text";
      break;

    case SF_ADJUSTMENT:
      name = "adjustment";
      break;

    case SF_FILENAME:
      name = "filename";
      break;

    case SF_DIRNAME:
      name = "dirname";
      break;

    case SF_FONT:
      name = "font";
      break;

    case SF_PALETTE:
      name = "palette";
      break;

    case SF_PATTERN:
      name = "pattern";
      break;

    case SF_BRUSH:
      name = "brush";
      break;

    case SF_GRADIENT:
      name = "gradient";
      break;

    case SF_OPTION:
      name = "option";
      break;

    case SF_ENUM:
      name = "enum";
      break;
    }

  if (arg_count[arg->type] == 0)
    {
      g_strlcpy (numbered_name, name, sizeof (numbered_name));
    }
  else
    {
      g_snprintf (numbered_name, sizeof (numbered_name),
                  "%s-%d", name, arg_count[arg->type] + 1);
    }

  arg_count[arg->type]++;

  /* Side effect: remember the generated name. */
  arg->property_name = g_strdup (numbered_name);

  *returned_name = numbered_name;

  /* nick is what the script author declared for menu label */
  *returned_nick = arg->label;
}

/* Set the default of a GParamSpec to a GFile for a path string.
 * The GFile is allocated and ownership is transferred to the GParamSpec.
 * The GFile is only a name and a so-named file might not exist.
 */
static void
pspec_set_default_file (GParamSpec *pspec, const gchar *filepath)
{
  GFile *gfile  = NULL;

  if (filepath != NULL && strlen (filepath) > 0)
    gfile = g_file_new_for_path (filepath);

  gimp_param_spec_object_set_default (pspec, G_OBJECT (gfile));

  g_clear_object (&gfile);
}

/* Append a string repr of an integer valued gvalue to given GString.
 * When the gvalue doesn't hold an integer, warn and append arbitrary int literal.
 */
static void
append_int_repr_from_gvalue (GString *result_string, GValue *gvalue)
{
  if (G_VALUE_HOLDS_INT (gvalue))
    {
      g_string_append_printf (result_string, "%d", g_value_get_int (gvalue));
    }
  else
    {
      g_warning ("Expecting GValue holding an int.");
      /* Append arbitrary int, so no errors in signature of Scheme call.
       * The call might not yield result the user intended.
       */
      g_string_append (result_string, "1");
    }
}


/* For the given C string IN, which can be user input so almost anything,
 * append the Scheme repr to the GString OUT.
 *
 * The Scheme repr is:
 *    - escaped certain characters
 *    - surrounded by double quotes.
 *
 * When user input string is NULL, appends just a pair of quotes
 * i.e. the Scheme representation of an empty string.
 *
 * Caller owns the result_string, and it is a GString, appendable.
 */
static void
append_scheme_repr_of_c_string (const gchar *user_string, GString *result_string)
{
  gchar *tmp;

  if (user_string == NULL)
    {
      g_string_append (result_string, "\"\"");
    }
  else
    {
      /* strescape requires user_string not null. */
      tmp = script_fu_strescape (user_string);
      g_string_append_printf (result_string, "\"%s\"", tmp);
      g_free (tmp);
    }
}

