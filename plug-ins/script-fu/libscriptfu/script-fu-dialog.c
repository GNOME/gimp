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

static gint
get_length (GimpProcedureConfig *config)
{
  GParamSpec **pspecs;
  guint        n_pspecs;

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config),
                                           &n_pspecs);
  g_free (pspecs);
  g_debug ("length config: %d", n_pspecs);

  return n_pspecs;
}

/* Fill a new (length zero) gva with new gvalues (empty but holding the correct type)
 from the config.
 */
static void
fill_gva_from (GimpProcedureConfig *config,
               GimpValueArray      *gva)
{
  GParamSpec **pspecs;
  guint        n_pspecs;

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config),
                                           &n_pspecs);
  /* !!! Start at property 1 */
  for (guint i = 1; i < n_pspecs; i++)
    {
      g_debug ("%s %s\n", pspecs[i]->name, G_PARAM_SPEC_TYPE_NAME (pspecs[i]));
      /* append empty gvalue */
      gimp_value_array_append (gva, NULL);
    }

  g_free (pspecs);
}

static void
dump_objects (GimpProcedureConfig  *config)
{
  /* Check it will return non-null objects. */
  GimpValueArray *args;
  gint length;

  /* Need one less gvalue !!! */
  args = gimp_value_array_new (get_length (config) - 1);
  /* The array still has length zero. */
  g_debug ("GVA length: %d", gimp_value_array_length (args));

  fill_gva_from (config, args);

  gimp_procedure_config_get_values (config, args);
  if (args == NULL)
    {
      g_debug ("config holds no values");
      return;
    }
  length = gimp_value_array_length (args);

  for (guint i = 1; i < length; i++)
    {
      GValue *gvalue = gimp_value_array_index (args, i);
      if (G_VALUE_HOLDS_OBJECT (gvalue))
        if (g_value_get_object (gvalue) == NULL)
          g_debug ("gvalue %d holds NULL object", i);
    }
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

  /* Script's menu label */
  gimp_ui_init (script_fu_script_get_title (script));

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

  /* It is possible to create custom widget where the provided widget is not adequate.
   * Then gimp_procedure_dialog_fill_list will create the rest.
   * For now, the provided widgets should be adequate.
   */

  /* NULL means create widgets for all properties of the procedure
   * that we have not already created widgets for.
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
