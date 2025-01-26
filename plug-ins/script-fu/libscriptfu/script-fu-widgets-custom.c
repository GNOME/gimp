/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2024 Lloyd Konneker
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

#include <libgimp/gimpui.h>

#include "script-fu-types.h"
#include "script-fu-widgets-custom.h"

/* Widgets in GimpProcedureDialog *custom* to ScriptFu.
 *
 * GimpProcedureDialog provides stock widgets
 * based on the type of a procedure argument.
 * A custom widget is the same kind of widget as stock, but specialized.
 *
 * Both stock and custom widgets are keyed to a property name
 * of the GimpProcedure for the script.
 * You add custom widgets, then call gimp_procedure_dialog_fill_list()
 * which adds a stock widget for any property name not having a custom widget.
 *
 * FUTURE: implement these in GimpProcedureDialog.
 * That can't be done now because GParamSpecs are limited
 * in the types they can describe.
 * E.G. SF-OPTION describes an enumerated type
 * that can't be described completely in a GParamSpec.
 * E.G. property type GFile is a general type
 * whose subtypes: existing-file, existing-dir, or file-to-save
 * can't be described in a GParamSpec.
 *
 * Most SFArg types are supported by stock widgets.
 *
 * SF-FILENAME is stock.
 * For property of type G_TYPE_FILE.
 * A GtkFileChooser in the mode for user to open a existing file.
 *
 * SF-DIRNAME is custom.
 * Also a GtkFileChooser but in mode for user to choose a directory.
 *
 * FUTURE:
 * SF-SAVE-FILENAME is custom.
 * Also a GtkFileChooser but in mode for user to save a file
 * (choose a file to replace, or enter filename and choose a directory.)
 *
 * SF-OPTION is custom.
 * Is an int combo box widget, for an enumeration defined by the script
 * (versus SF-ENUM for enums defined by GIMP.)
 * Note SF-OPTION does not create symbols in the interpreter.
 * The script itself, and all other plugins, must use integer literals
 * to denote values.
 *
 * SF-ADJUSTMENT:SF-SLIDER is custom.
 * Default widget for propety of type DOUBLE is an entry w/ spinner.
 * Override with a slider w/ spinner.
 */

/* Does SFArg type need custom widget? */
static gboolean
sf_arg_type_is_custom (SFScript *script,
                       guint     arg_index)
{
  gboolean result=FALSE;

  switch (script->args[arg_index].type)
    {
    case SF_OPTION:
      result = TRUE;
      break;
    case SF_ADJUSTMENT:
      if (script->args[arg_index].default_value.sfa_adjustment.type == SF_SLIDER)
        result = TRUE;
      break;
    default:
      result = FALSE;
    }
  return result;
}

/* Returns new GtkListStore from SFArg declaration.
 * Transfers ownership of the allocated, returned store.
 * Ownership transfers to a dialog, which frees it on dialog destroy.
 *
 * Returned value is never NULL.
 * Returned store can be empty if the declaration of the arg is non-sensical.
 */
static GtkListStore *
sf_widget_custom_new_int_store (SFArg *arg)
{
  GtkListStore *result;
  GSList       *list;
  guint         counter = 0; /* SF enumerations start at 0. */

  /* Create empty store. */
  result = g_object_new (GIMP_TYPE_INT_STORE, NULL);

  /* Iterate over list of names of enumerated values,
   * appending each to store having value equal to the iteration index.
   *
   * ScriptFu does NOT define constant Scheme symbols in the interpreter;
   * the names of enumerated values are not available to the script,
   * which must use integer literals or (define other symbols for the values.)
   */
  for (list = arg->default_value.sfa_option.list;
       list;
       list = g_slist_next (list))
    {
      GtkTreeIter iter;

      gtk_list_store_append (result, &iter);
      gtk_list_store_set (result, &iter,
                          GIMP_INT_STORE_VALUE, counter,
                          GIMP_INT_STORE_LABEL, list->data,
                          -1);
      counter++;
    }

  return result;
}

/* Adds widget for arg of type SF-OPTION to the dialog.
 * Widget is a combobox widget.
 * Specializes the widget by a custom store i.e. model
 * derived from the arg's declaration.
 */
static void
sf_widget_custom_option (GimpProcedureDialog *dialog,
                         SFArg               *arg)
{
  GtkListStore *store;

  store = sf_widget_custom_new_int_store (arg);
  gimp_procedure_dialog_get_int_combo (dialog,
                                       arg->property_name,
                                       GIMP_INT_STORE (store));
}

/* Adds widget for arg of type SF-ADJUSTMENT:SF_SLIDER to the dialog.
 * Specializes the widget: slider instead of entry, and a spinner
 */
static void
sf_widget_custom_slider (GimpProcedureDialog *dialog,
                         SFArg               *arg)
{
  /* Widget belongs to dialog, discard the returned ref. */
  (void) gimp_procedure_dialog_get_widget (dialog,
                                           arg->property_name,
                                           GIMP_TYPE_SCALE_ENTRY);
}

/* Add a custom widget for a script's arg to the script's dialog.
 * Does nothing when the type of the arg does not need a custom widget.
 *
 * Widget is associated with the arg's property, by name of the property.
 * The arg's property is a property of the Procedure.
 * The widget will be positioned in the dialog by order of property.
 */
static void
sf_widget_custom_add_to_dialog (GimpProcedureDialog *dialog,
                                SFArg               *arg)
{
  /* Handles same cases as sf_arg_type_is_custom() */
  switch (arg->type)
    {
    case SF_OPTION:
      sf_widget_custom_option (dialog, arg);
      break;
    case SF_ADJUSTMENT:
      /* We already know sfa_adjustment.type == SF_SLIDER. */
      sf_widget_custom_slider (dialog, arg);
      break;
    default:
      g_warning ("%s Unhandled custom widget type.", G_STRFUNC);
      break;
    }
}


/* Add custom widgets to dialog, for certain type of args of script. */
void
script_fu_widgets_custom_add (GimpProcedureDialog *dialog,
                              SFScript            *script)
{
  for (int arg_index=0; arg_index < script->n_args; arg_index++)
    {
      if (sf_arg_type_is_custom (script, arg_index))
        {
          sf_widget_custom_add_to_dialog (dialog, &script->args[arg_index]);
        }
    }
}
