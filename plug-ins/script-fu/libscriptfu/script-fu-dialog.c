/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * script-fu-dialog.c
 * Copyright (C) 2022 Lloyd Konneker
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

#ifdef GDK_WINDOWING_QUARTZ
#import <Cocoa/Cocoa.h>
#endif

#include "script-fu-types.h"    /* SFScript */
#include "script-fu-script.h"   /* get_title */
#include "script-fu-command.h"

#include "script-fu-dialog.h"
#include "script-fu-widgets-custom.h"


/* An informal class that shows a dialog for a script then runs the script.
 * It is internal to libscriptfu.
 *
 * The dialog is modal for the script:
 * OK button hides the dialog then runs the script once.
 *
 * The dialog is non-modal with respect to the GIMP app GUI, which remains responsive.
 *
 * When called from plugin extension-script-fu, the dialog is modal on the extension:
 * although GIMP app continues responsive, a user choosing a menu item
 * that is also implemented by a script and extension-script-fu
 * will not show a dialog until the first called script finishes.
 * TODO combine these paragraphs
 * We don't prevent concurrent dialogs as in script-fu-interface.c.
 * For extension-script-fu, Gimp is already preventing concurrent dialogs.
 * For gimp-script-fu-interpreter, each plugin is a separate process
 * so concurrent dialogs CAN occur.
 *
 * There is no progress widget in GimpProcedureDialog.
 * Also, we don't need to update the progress in Gimp UI,
 * because Gimp shows progress: the name of all called PDB procedures.
 *
 * Run dialog entails: create config, create dialog for config, show dialog, and return a config.
 * The config has one property for each arg of the script.
 * The dialog creates a widget for each such property of the config.
 * Each property has a ParamSpec.
 * Requires the procedure registered with args having ParamSpecs
 * corresponding to SFType the author declared in script-fu-register call.
 */

#define DEBUG_CONFIG_PROPERTIES  FALSE

#if DEBUG_CONFIG_PROPERTIES
static void
dump_properties (GimpProcedureConfig *config)
{
  GParamSpec **pspecs;
  guint        n_pspecs;

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config),
                                           &n_pspecs);
  for (guint i = 1; i < n_pspecs; i++)
    g_printerr ("%s %s\n", pspecs[i]->name, G_PARAM_SPEC_TYPE_NAME (pspecs[i]));
  g_free (pspecs);
}

static void
dump_objects (GimpProcedureConfig *config)
{
  /* Check it will return non-null objects. */
  GParamSpec **pspecs;
  guint        n_pspecs;

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config), &n_pspecs);

  /* config will have at least 1 property: "procedure". */
  if (n_pspecs == 1)
    {
      g_debug ("config holds no values");
      return;
    }

  for (gint i = 1; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];
      GValue      value = G_VALUE_INIT;

      g_value_init (&value, pspec->value_type);
      g_object_get_property (G_OBJECT (config), pspec->name, &value);

      if (G_VALUE_HOLDS_OBJECT (&value))
        if (g_value_get_object (&value) == NULL)
          g_debug ("gvalue %d holds NULL object", i);

      g_value_unset (&value);
    }
  g_free (pspecs);
}

#endif

/* Require conditions for running a dialog and interpreting.
 * Returns false when a dialog cannot be shown.
 *
 * A caller should not call when config has no properties that are args
 * of the procedure.
 */
static gboolean
sf_dialog_can_be_run (GimpProcedure        *procedure,
                      SFScript             *script,
                      GimpProcedureConfig  *config)
{
  guint n_specs;

  if ( (! G_IS_OBJECT (procedure)) ||
       (! GIMP_IS_PROCEDURE_CONFIG (config)) ||
       script == NULL)
    return FALSE;

  /* A config always has property "procedure"
   * It must have at least one more, an arg, else do not need a dialog.
   *
   * FUTURE: make a method of config.
   */
  g_free (g_object_class_list_properties (G_OBJECT_GET_CLASS (config), &n_specs));
  if (n_specs < 2)
    return FALSE;

  return TRUE;
}

/* Omit leading "procedure" and "run-mode" properties.
 * Caller must free the list.
 */
static GList*
sf_get_suffix_of_config_prop_names (GimpProcedureConfig  *config)
{
  guint n_specs;
  GParamSpec ** pspecs  = g_object_class_list_properties (G_OBJECT_GET_CLASS (config), &n_specs);
  GList *property_names = NULL;

  for (guint i=2; i<n_specs; i++)
    property_names = g_list_append (property_names, (void*) g_param_spec_get_name (pspecs[i]));

  g_free (pspecs);
  return property_names;
}

/* Create and run a dialog for a procedure.
 * Returns true when not canceled.
 * Side effects on config.
 *
 * When should_skip_runmode, the config has a run-mode property
 * the dialog does not show.
 *
 * Procedure may be GimpImageProcedure or ordinary GimpProcedure.
 *
 * Requires gimp_ui_init already called.
 */
static gboolean
sf_dialog_run (GimpProcedure        *procedure,
               SFScript             *script,
               GimpProcedureConfig  *config,
               gboolean              should_skip_runmode)
{
  GimpProcedureDialog *dialog = NULL;
  gboolean            not_canceled;

#if DEBUG_CONFIG_PROPERTIES
  dump_properties (config);
  dump_objects (config);
#endif

#ifdef GDK_WINDOWING_QUARTZ
  /* Ensure this process doesn't appear in Dock as separate app.
   * Set as accessory (helper) app instead of regular app.
   */
  [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
#endif

  /* Create a dialog having properties (describing arguments of the procedure)
   * taken from the config.
   *
   * Title dialog with the menu item, not the procedure name.
   * Assert menu item is localized.
   */
  dialog = (GimpProcedureDialog*) gimp_procedure_dialog_new (
                                      procedure,
                                      config,
                                      script_fu_script_get_title (script));
  /* dialog has no widgets except standard buttons. */

  /* Create custom widget where the stock widget is not adequate. */
  script_fu_widgets_custom_add (dialog, script);

  /* Create widgets for all properties of the procedure not already created. */
  if (should_skip_runmode)
    {
      GList *property_names = sf_get_suffix_of_config_prop_names (config);

      gimp_procedure_dialog_fill_list (dialog, property_names);
      g_list_free (property_names);
    }
  else
    {
      /* NULL means: all properties. */
      gimp_procedure_dialog_fill_list (dialog, NULL);
    }

#ifdef GDK_WINDOWING_QUARTZ

  /* The user chose a plugin from gimp app, now ensure this process is active.
   * The gimp app was active, but now the plugin should be active.
   * This is also called in gimpui_init(), but that is not sufficient
   * for second calls of plugins served by long-running extension-script-fu.
   * The user can still raise other gimp windows, hiding the dialog.
   */
  [NSApp activateIgnoringOtherApps:YES];
#endif

  not_canceled = gimp_procedure_dialog_run (dialog);

#if DEBUG_CONFIG_PROPERTIES
  dump_objects (config);
#endif

  gtk_widget_destroy ((GtkWidget*) dialog);
  return not_canceled;
}


/* Run a dialog for an ImageProcedure, then interpret the script.
 *
 * Interpret: marshal config into Scheme text for function call, then interpret script.
 */
GimpValueArray*
script_fu_dialog_run_image_proc (
                      GimpProcedure        *procedure,
                      SFScript             *script,
                      GimpImage            *image,
                      GimpDrawable        **drawables,
                      GimpProcedureConfig  *config)

{
  GimpValueArray *result = NULL;
  gboolean        not_canceled;

  if (! sf_dialog_can_be_run (procedure, script, config))
    return gimp_procedure_new_return_values (procedure, GIMP_PDB_EXECUTION_ERROR, NULL);

  not_canceled = sf_dialog_run (procedure, script, config, FALSE);
  /* Assert config holds validated arg values from a user interaction. */

  if (not_canceled)
    result = script_fu_interpret_image_proc (procedure, script,
                                             image, drawables,
                                             config);
  else
    result = gimp_procedure_new_return_values (procedure, GIMP_PDB_CANCEL, NULL);


  return result;
}

/* Run a dialog for a generic GimpProcedure, then interpret the script. */
GimpValueArray*
script_fu_dialog_run_regular_proc (GimpProcedure        *procedure,
                                   SFScript             *script,
                                   GimpProcedureConfig  *config)

{
  GimpValueArray *result = NULL;
  gboolean        not_canceled;

  if (! sf_dialog_can_be_run (procedure, script, config))
    return gimp_procedure_new_return_values (procedure, GIMP_PDB_EXECUTION_ERROR, NULL);

  not_canceled = sf_dialog_run (procedure, script, config,
                                TRUE);  /* skip run-mode prop of config. */
  /* Assert config holds validated arg values from a user interaction. */

  if (not_canceled)
    result = script_fu_interpret_regular_proc (procedure, script, config);
  else
    result = gimp_procedure_new_return_values (procedure, GIMP_PDB_CANCEL, NULL);


  return result;
}

