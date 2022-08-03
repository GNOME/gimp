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

#include "script-fu-types.h"
#include "script-fu-arg.h"
#include "script-fu-utils.h"


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
 * SF_VALUE probably should be obsoleted.
 * Search ChangeLog for mention of "SF_VALUE"
 * See below, the only difference is that one get string escaped.
 * Otherwise, SF_VALUE is identical to SF_STRING.
 * Probably SF_VALUE still exists just for backward compatibility.
 *
 * SFArgType denotes not only a C type, but also a Scheme type.
 * For example, SF_ADJUSTMENT denotes the C type "float"
 * and the Scheme type "numeric" (which encompasses float and int.)
 * Another example, SF_PATTERN denotes the C type GimpPattern
 * and the Scheme type string (names of brushes are used in scripts.)
 */


static void pspec_set_default_file (GParamSpec *pspec, const gchar *filepath);


/* Free any allocated members.
 * Somewhat hides what members of the SFArg struct are allocated.
 * !!! A few other places in the code do the allocations.
 * !!! A few other places in the code free members.
 */
void
script_fu_arg_free (SFArg *arg)
{
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

    /* FUTURE: font..gradient could all use the same code.
     * Since the type in the union are all the same: gchar*.
     * That is, group these cases with SF_VALUE.
     * But this method should go away altogether.
     */
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

/* Reset: copy the default value to current value. */
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

    /* FUTURE: font..gradient could all use the same code.
     * Since the type in the union are all the same: gchar*.
     * That is, group these cases with SF_VALUE.
     */
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
GParamSpec *
script_fu_arg_get_param_spec (SFArg       *arg,
                              const gchar *name,
                              const gchar *nick)
{
  GParamSpec * pspec = NULL;

  switch (arg->type)
    {
      /* No defaults for GIMP objects: Image, Item subclasses, Display */
    case SF_IMAGE:
      pspec = gimp_param_spec_image (name,
                                     nick,
                                     arg->label,
                                     TRUE,  /* None is valid. */
                                     G_PARAM_READWRITE);
      break;

    case SF_DRAWABLE:
      pspec = gimp_param_spec_drawable (name,
                                        nick,
                                        arg->label,
                                        TRUE,
                                        G_PARAM_READWRITE);
      break;

    case SF_LAYER:
      pspec = gimp_param_spec_layer (name,
                                     nick,
                                     arg->label,
                                     TRUE,
                                     G_PARAM_READWRITE);
      break;

    case SF_CHANNEL:
      pspec = gimp_param_spec_channel (name,
                                       nick,
                                       arg->label,
                                       TRUE,
                                       G_PARAM_READWRITE);
      break;

    case SF_VECTORS:
      pspec = gimp_param_spec_vectors (name,
                                       nick,
                                       arg->label,
                                       TRUE,
                                       G_PARAM_READWRITE);
      break;

    case SF_DISPLAY:
      pspec = gimp_param_spec_display (name,
                                       nick,
                                       arg->label,
                                       TRUE,
                                       G_PARAM_READWRITE);
      break;

    case SF_COLOR:
      /* Pass address of default color i.e. instance of GimpRGB.
       * Color is owned by ScriptFu and exists for lifetime of SF process.
       */
      pspec = gimp_param_spec_rgb (name,
                                   nick,
                                   arg->label,
                                   TRUE,  /* is alpha relevant */
                                   &arg->default_value.sfa_color,
                                   G_PARAM_READWRITE);
      /* FUTURE: Default not now appear in PDB browser, but appears in widgets? */
      break;

    case SF_TOGGLE:
      /* Implicit conversion from gint32 to gboolean. */
      pspec = g_param_spec_boolean (name,
                                    nick,
                                    arg->label,
                                    arg->default_value.sfa_toggle,
                                    G_PARAM_READWRITE);
      break;

    /* FUTURE special widgets for multiline text.
     * script-fu-interface does, but GimpProcedureDialog does not.
     */
    case SF_VALUE:
    case SF_STRING:
    case SF_TEXT:

    /* FUTURE special widgets.
     * script-fu-interface does, but GimpProcedureDialog does not?
     */
    case SF_FONT:
    case SF_PALETTE:
    case SF_PATTERN:
    case SF_GRADIENT:
      /* These cases the same type: gchar *.
       * Use arbitrary SFArg field of that type.
       */
      pspec = g_param_spec_string (name,
                                   nick,
                                   arg->label,
                                   arg->default_value.sfa_value,
                                   G_PARAM_READWRITE);
      break;

    case SF_BRUSH:
      /* FUTURE: brush object has more fields.
       * ?? Implement gimp_param_spec_brush
       */
      pspec = g_param_spec_string (name,
                                   nick,
                                   arg->label,
                                   arg->default_value.sfa_brush.name,
                                   G_PARAM_READWRITE);
      break;

    case SF_ADJUSTMENT:
      pspec = g_param_spec_double (name,
                                   nick,
                                   arg->label,
                                   arg->default_value.sfa_adjustment.lower,
                                   arg->default_value.sfa_adjustment.upper,
                                   arg->default_value.sfa_adjustment.value,
                                   G_PARAM_READWRITE);
      break;

    case SF_FILENAME:
    case SF_DIRNAME:
      pspec = g_param_spec_object (name,
                                   nick,
                                   arg->label,
                                   G_TYPE_FILE,
                                   G_PARAM_READWRITE |
                                   GIMP_PARAM_NO_VALIDATE);
      pspec_set_default_file (pspec, arg->default_value.sfa_file.filename);
      /* FUTURE: Default not now appear in PDB browser, but appears in widgets? */
      break;

    case SF_ENUM:
      /* history is the last used value AND the default. */
      pspec = g_param_spec_enum (name,
                                 nick,
                                 arg->label,
                                 g_type_from_name (arg->default_value.sfa_enum.type_name),
                                 arg->default_value.sfa_enum.history,
                                 G_PARAM_READWRITE);
      break;

    case SF_OPTION:
      pspec = g_param_spec_int (name,
                                nick,
                                arg->label,
                                0,        /* Always zero based. */
                                g_slist_length (arg->default_value.sfa_option.list),
                                arg->default_value.sfa_option.history,
                                G_PARAM_READWRITE);
      /* FUTURE: Model values not now appear in PDB browser NOR in widgets? */
      /* FUTURE: Does not show a combo box widget ??? */
      break;
    }

  return pspec;
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
  g_debug("script_fu_arg_append_repr_from_gvalue %s", arg->label);
  switch (arg->type)
    {
    case SF_IMAGE:
    case SF_DRAWABLE:
    case SF_LAYER:
    case SF_CHANNEL:
    case SF_VECTORS:
    case SF_DISPLAY:
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
        GimpRGB color;
        guchar  r, g, b;

        gimp_value_get_rgb (gvalue, &color);
        gimp_rgb_get_uchar (&color, &r, &g, &b);
        g_string_append_printf (result_string, "'(%d %d %d)",
                                (gint) r, (gint) g, (gint) b);
      }
      break;

    case SF_TOGGLE:
      g_string_append_printf (result_string, (g_value_get_boolean (gvalue) ?
                                  "TRUE" : "FALSE"));
      break;

    case SF_VALUE:
      g_string_append (result_string, g_value_get_string (gvalue));
      break;

    case SF_STRING:
    case SF_TEXT:
      {
        gchar *tmp;

        tmp = script_fu_strescape (g_value_get_string (gvalue));
        g_string_append_printf (result_string, "\"%s\"", tmp);
        g_free (tmp);
      }
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
                /* assert filepath not null. */
                /* Not escape special chars for whitespace or double quote. */
                g_string_append_printf (result_string, "\"%s\"", filepath);
                g_free (filepath);
              }
            else
              {
                gchar *msg = "Invalid GFile in gvalue.";
                g_warning ("%s", msg);
                g_string_append_printf (result_string, "\"%s\"", msg);
              }
          }
        else
          {
            gchar *msg = "Expecting GFile in gvalue.";
            g_warning ("%s", msg);
            g_string_append_printf (result_string, "\"%s\"", msg);
          }
        /* Ensure appended a filepath string OR an error string.*/
      }
      break;

    case SF_ADJUSTMENT:
      {
        gchar buffer[G_ASCII_DTOSTR_BUF_SIZE];

        g_ascii_dtostr (buffer, sizeof (buffer), g_value_get_double (gvalue));
        g_string_append (result_string, buffer);
      }
      break;

    case SF_FONT:
    case SF_PALETTE:
    case SF_PATTERN:
    case SF_GRADIENT:
    case SF_BRUSH:
      g_string_append_printf (result_string, "\"%s\"", g_value_get_string (gvalue));
      break;

    case SF_OPTION:
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

    case SF_COLOR:
      {
        guchar r, g, b;

        gimp_rgb_get_uchar (&arg_value->sfa_color, &r, &g, &b);
        g_string_append_printf (result_string, "'(%d %d %d)",
                                (gint) r, (gint) g, (gint) b);
      }
      break;

    case SF_TOGGLE:
      g_string_append (result_string, arg_value->sfa_toggle ? "TRUE" : "FALSE");
      break;

    case SF_VALUE:
      g_string_append (result_string, arg_value->sfa_value);
      break;

    case SF_STRING:
    case SF_TEXT:
      {
        gchar *tmp;

        tmp = script_fu_strescape (arg_value->sfa_value);
        g_string_append_printf (result_string, "\"%s\"", tmp);
        g_free (tmp);
      }
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
      {
        gchar *tmp;

        tmp = script_fu_strescape (arg_value->sfa_file.filename);
        g_string_append_printf (result_string, "\"%s\"", tmp);
        g_free (tmp);
      }
      break;

    case SF_FONT:
      g_string_append_printf (result_string, "\"%s\"", arg_value->sfa_font);
      break;

    case SF_PALETTE:
      g_string_append_printf (result_string, "\"%s\"", arg_value->sfa_palette);
      break;

    case SF_PATTERN:
      g_string_append_printf (result_string, "\"%s\"", arg_value->sfa_pattern);
      break;

    case SF_GRADIENT:
      g_string_append_printf (result_string, "\"%s\"", arg_value->sfa_gradient);
      break;

    case SF_BRUSH:
      {
        gchar buffer[G_ASCII_DTOSTR_BUF_SIZE];

        g_ascii_dtostr (buffer, sizeof (buffer),
                        arg_value->sfa_brush.opacity);
        g_string_append_printf (result_string, "'(\"%s\" %s %d %d)",
                                arg_value->sfa_brush.name,
                                buffer,
                                arg_value->sfa_brush.spacing,
                                arg_value->sfa_brush.paint_mode);
      }
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
static gint arg_count[SF_DISPLAY] = { 0, };

void
script_fu_arg_reset_name_generator (void)
{
  for (guint i = 0; i<SF_DISPLAY; i++)
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
 * e.g. a property named "image" "n_drawables" and "drawables"
 * So we avoid that name clash by starting with "otherImage"
 *
 * The name means nothing to human readers of the spec.
 * Instead, the nick is descriptive for human readers.
 *
 * The returned string is owned by the generator, a constant.
 * The caller need not copy it,
 * but usually does by creating a GParamSpec.
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

    case SF_VALUE:
      name = "value";
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

  *returned_name = numbered_name;

  /* nick is what the script author said describes the arg */
  *returned_nick = arg->label;
}


/* Set the default of a GParamSpec to a GFile for a path string.
 * The GFile is allocated and ownership is transferred to the GParamSpec.
 * The GFile is only a name and a so-named file might not exist.
 */
static void
pspec_set_default_file (GParamSpec *pspec, const gchar *filepath)
{
  GValue  gvalue = G_VALUE_INIT;
  GFile  *gfile  = NULL;

  g_value_init (&gvalue, G_TYPE_FILE);
  gfile = g_file_new_for_path (filepath);
  g_value_set_object (&gvalue, gfile);
  g_param_value_set_default (pspec, &gvalue);
}
