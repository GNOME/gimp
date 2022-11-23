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

/* This file understands how to make a LigmaPlugin of the interpreter.
 * This is mostly boilerplate for any plugin.
 * It understands little about ScriptFu internals,
 * hidden by script-fu-interpreter.[ch] and libscriptfu.
 */

#include "config.h"
#include <glib.h>
#include <libligma/ligma.h>

#include "libscriptfu/script-fu-intl.h"

#include "script-fu-interpreter.h"


/* ScriptFuInterpreter subclasses LigmaPlugIn */

#define SCRIPT_FU_INTERPRETER_TYPE (script_fu_interpreter_get_type ())
G_DECLARE_FINAL_TYPE (ScriptFuInterpreter, script_fu_interpreter, SCRIPT, FU_INTERPRETER, LigmaPlugIn)

struct _ScriptFuInterpreter
{
  LigmaPlugIn      parent_instance;
};

static GList          * script_fu_interpreter_query_procedures (LigmaPlugIn  *plug_in);
static LigmaProcedure  * script_fu_interpreter_create_procedure (LigmaPlugIn  *plug_in,
                                                                const gchar *name);

G_DEFINE_TYPE (ScriptFuInterpreter, script_fu_interpreter, LIGMA_TYPE_PLUG_IN)

/* An alias to argv[1], which is the path to the .scm file.
 * Each instance of ScriptFuInterpreter is specialized by the script passed in argv[1]
 * This var need not belong to the class or to an instance,
 * because there will only be one instance of ScriptFuInterpreter per plugin process.
 */
static gchar * path_to_this_script;

/* Connect to Ligma.  See libligma/ligma.c.
 *
 * Can't use LIGMA_MAIN macro, it doesn't omit argv[0].
 *
 * First arg is app cited in the shebang.
 * Second arg is the .scm file containing the shebang.
 * Second to last arg is the "phase" e.g. -query or -run
 * Last arg is the mode for crash dumps.
 * Typical argv:
 *   ligma-script-fu-interpreter-3.0 ~/.config/LIGMA/2.99/plug-ins/fu/fu
 *          -ligma 270 12 11 -query 1
 */
int main (int argc, char *argv[])
{
  g_debug ("Enter script-fu-interpreter main");

  /* Alias path to this plugin's script file. */
  path_to_this_script = argv[1];

  /* ligma_main will create an instance of the class given by the first arg, a GType.
   * The class is a subclass of LigmaPlugIn (with overridden query and create methods.)
   * LIGMA will subsequently callback the query or create methods,
   * or the run_func of the PDB procedure of the plugin,
   * depending on the "phase" arg in argv,
   * which is set by the ligma plugin manager, which is invoking this interpreter.
   */
  /* Omit argv[0] when passing to ligma */
  ligma_main (SCRIPT_FU_INTERPRETER_TYPE, argc-1, &argv[1] );

  g_debug ("Exit script-fu-interpreter.");
}

DEFINE_STD_SET_I18N

static void
script_fu_interpreter_class_init (ScriptFuInterpreterClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = script_fu_interpreter_query_procedures;
  plug_in_class->create_procedure = script_fu_interpreter_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}


/* called by the GType system to initialize instance of the class. */
static void
script_fu_interpreter_init (ScriptFuInterpreter *script_fu)
{
  /* Nothing to do. */
}


/* Return the names of PDB procedures implemented. A callback from LIGMA. */
static GList *
script_fu_interpreter_query_procedures (LigmaPlugIn *plug_in)
{
  GList *result = NULL;

  g_debug ("queried");

  result = script_fu_interpreter_list_defined_proc_names (plug_in, path_to_this_script);
  if (g_list_length (result) < 1)
    g_warning ("No procedures defined in %s", path_to_this_script);

  /* Caller is LIGMA and it will free the list. */
  return result;
}


/* Create and return a LigmaPDBProcedure,
 * for the named one of the PDB procedures that the script implements.
 * A callback from LIGMA.
 *
 * Also set attributes on the procedure, most importantly, menu items (optional.)
 * Also create any menus/submenus that the script defines e.g. Filters>My
 */
static LigmaProcedure *
script_fu_interpreter_create_procedure (LigmaPlugIn  *plug_in,
                                        const gchar *proc_name)
{
  return script_fu_interpreter_create_proc_at_path (plug_in,
                                                    proc_name,
                                                    path_to_this_script);
}
