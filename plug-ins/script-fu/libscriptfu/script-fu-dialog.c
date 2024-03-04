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
 */

/* FUTURE: delete this after v3 is stable. */
#define DEBUG_CONFIG_PROPERTIES  TRUE

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

/* Run a dialog for a procedure, then interpret the script.
 *
 * Run dialog: create config, create dialog for config, show dialog, and return a config.
 * Interpret: marshal config into Scheme text for function call, then interpret script.
 *
 * One widget per param of the procedure.
 * Require the procedure registered with params of GTypes
 * corresponding to SFType the author declared in script-fu-register call.
 *
 * Require initial_args is not NULL or empty.
 * A caller must ensure a dialog is needed because args is not empty.
 */
GimpValueArray*
script_fu_dialog_run (GimpProcedure        *procedure,
                      SFScript             *script,
                      GimpImage            *image,
                      guint                 n_drawables,
                      GimpDrawable        **drawables,
                      GimpProcedureConfig  *config)

{
  GimpValueArray      *result = NULL;
  GimpProcedureDialog *dialog = NULL;
  gboolean             not_canceled;
  guint                n_specs;

  if ( (! G_IS_OBJECT (procedure)) || script == NULL)
    return gimp_procedure_new_return_values (procedure, GIMP_PDB_EXECUTION_ERROR, NULL);

  g_free (g_object_class_list_properties (G_OBJECT_GET_CLASS (config), &n_specs));
  if (n_specs < 2)
    return gimp_procedure_new_return_values (procedure, GIMP_PDB_EXECUTION_ERROR, NULL);

  /* We don't prevent concurrent dialogs as in script-fu-interface.c.
   * For extension-script-fu, Gimp is already preventing concurrent dialogs.
   * For gimp-script-fu-interpreter, each plugin is a separate process
   * so concurrent dialogs CAN occur.
   */
  /* There is no progress widget in GimpProcedureDialog.
   * Also, we don't need to update the progress in Gimp UI,
   * because Gimp shows progress: the name of all called PDB procedures.
   */

  /* Requires gimp_ui_init already called. */

#if DEBUG_CONFIG_PROPERTIES
  dump_properties (config);
  g_debug ("Len of initial_args %i", n_specs - 1);
  dump_objects (config);
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

  /* NULL means create widgets for all properties of the procedure
   * that we have not already created custom widgets for.
   */
  gimp_procedure_dialog_fill_list (dialog, NULL);

  not_canceled = gimp_procedure_dialog_run (dialog);
  /* Assert config holds validated arg values from a user interaction. */

#if DEBUG_CONFIG_PROPERTIES
  dump_objects (config);
#endif

  if (not_canceled)
    {
      result = script_fu_interpret_image_proc (procedure, script,
                                               image, n_drawables, drawables,
                                               config);
    }
  else
    {
      result = gimp_procedure_new_return_values (procedure, GIMP_PDB_CANCEL, NULL);
    }

  gtk_widget_destroy ((GtkWidget*) dialog);

  return result;
}
