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

#include "libgimpbase/gimpversion-private.h"

#include "libscriptfu/script-fu-lib.h"
#include "libscriptfu/script-fu-intl.h"

#include "script-fu-refresh.h"

GIMP_WARNING_API_BREAK("Reminder that we should probably get rid of Script-fu scripts in favor or proper Script-fu plug-ins, "
                       "port good scripts and get rid of obsolete ones (such as Refresh Scripts).")

/* The "Refresh Scripts" menu item is not in v3.
 *
 * Code is built, but the linker omits it,
 * since one call is commented out.
 *
 * This is not the only cruft, scheme_wrapper.c references
 * the PDB procedure by name.
 *
 * See #10596 and #10652.
 * When used, it throws CRITICAL.
 * There is also an MR that fixes the reason for the CRITICAL,
 * but #10652 suggests better alternatives.
 */

static GimpValueArray *
script_fu_refresh_proc (GimpProcedure        *procedure,
                        GimpProcedureConfig  *config,
                        gpointer              run_data)
{
  if (script_fu_extension_is_busy ())
    {
      g_message (_("You can not use \"Refresh Scripts\" while a "
                   "Script-Fu dialog box is open.  Please close "
                   "all Script-Fu windows and try again."));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               NULL);
    }
  else
    {
      /*  Reload all of the available scripts  */
      GList *path = script_fu_search_path ();

      script_fu_find_and_register_scripts (gimp_procedure_get_plug_in (procedure), path);

      g_list_free_full (path, (GDestroyNotify) g_object_unref);
    }

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

void
script_fu_register_refresh_procedure (GimpPlugIn *plug_in)
{
  GimpProcedure *procedure;

  procedure = gimp_procedure_new (plug_in, "script-fu-refresh",
                                  GIMP_PDB_PROC_TYPE_TEMPORARY,
                                  script_fu_refresh_proc, NULL, NULL);

  gimp_procedure_set_menu_label (procedure, _("_Refresh Scripts"));
  gimp_procedure_add_menu_path (procedure,
                                "<Image>/Filters/Development/Script-Fu");

  gimp_procedure_set_documentation (procedure,
                                    _("Re-read all available Script-Fu scripts"),
                                    "Re-read all available Script-Fu scripts",
                                    "script-fu-refresh");
  gimp_procedure_set_attribution (procedure,
                                  "Spencer Kimball & Peter Mattis",
                                  "Spencer Kimball & Peter Mattis",
                                  "1997");

  gimp_procedure_add_enum_argument (procedure, "run-mode",
                                    "Run mode",
                                    "The run mode",
                                    GIMP_TYPE_RUN_MODE,
                                    GIMP_RUN_INTERACTIVE,
                                    G_PARAM_READWRITE);

  gimp_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);
}
