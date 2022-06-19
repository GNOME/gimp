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
#include <libgimp/gimp.h>

#include "libscriptfu/script-fu-lib.h"

#include "script-fu-interpreter.h"

/* Implementation of the outer ScriptFuInterpreter.
 * This understands ScriptFu internals
 * i.e. uses libscriptfu shared with other ScriptFu plugins e.g. extension-script-fu.
 */

/* We don't need to load (into the interpreter) any .scm files handled by extension-script-fu.
 * Namely, the .scm in the GIMP installation /scripts or in the user local /scripts dirs.
 *
 * During startup, GIMP might call gimp-script-fu-interpreter
 * to query new files such as /plug-ins/fu/fu.scm.
 * This is before extension-script-fu starts.
 * But all the .scm files handled by extension-script-fu are type TEMPORARY and not needed
 * for a /plug-ins/fu.scm to be queried.
 * The only Scheme executed during query are calls to script-fu-register.
 * Later, when a /plug-ins/fu.scm is run, it can call temporary PDB procedures
 * that extension-script-fu provides.
 *
 * When we call script_fu_init_embedded_interpreter(),
 * the passed paths should include the path to /scripts
 * because that is the location of scripts for initialization and compatibility
 * (script-fu.init, plug-in-compat.init and script-fu-compat.init,
 * which are really scheme files.)
 *
 * scrip-fu-interpreter always inits embedded interpreter(allow_register=TRUE)
 * In the "run" phase, you don't need script-fu-register to be defined, but its harmless.
 */

static GFile *script_fu_get_plugin_parent_path (const gchar *path_to_this_script);
static void   script_fu_free_path_list         (GList      **list);


/* Return a list of PDB procedure names defined in all .scm files in
 * the parent dir of the given path, which is a filename of the one being queried.
 *
 * Each .scm file may contain many calls to script-fu-register, which defines a PDB procedure.
 * All .scm files in the parent dir are searched.
 *
 * This executable is named script-fu-interpreter
 * but no PDB procedure is named "script-fu-interpreter".
 * Instead, the interpreter registers PDB procs named from name strings
 * give in the in script-fu-register() calls in the interpreted scripts.
 *
 * Caller must free the list.
 */
GList *
script_fu_interpreter_list_defined_proc_names (GimpPlugIn  *plug_in,
                                               const gchar *path_to_this_script)
{
  GList *name_list = NULL;  /* list of strings */
  GList *path_list = NULL;  /* list of GFile */

  /* path_list is /scripts dir etc. from which we will load compat and init scripts.
   * second argument TRUE means define script-fu-register into the interpreter.
   */
  path_list = script_fu_search_path ();
  script_fu_init_embedded_interpreter (path_list, TRUE, GIMP_RUN_NONINTERACTIVE);
  script_fu_free_path_list (&path_list);

  /* Reuse path_list, now a list of one path, the parent dir of the queried script. */
  path_list = g_list_append (path_list,
                             script_fu_get_plugin_parent_path (path_to_this_script));
  name_list = script_fu_find_scripts_list_proc_names (plug_in, path_list);
  script_fu_free_path_list (&path_list);

  /* Usually name_list is not NULL i.e. not empty.
   * But an .scm file that is not an actual GIMP plugin, or broken, may yield empty list.
   */
  return name_list;
}


/* Create a PDB proc of type PLUGIN with the given name.
 * Unlike extension-script-fu, create proc of type PLUGIN.
 *
 * We are in "create procedure" phase of call from GIMP.
 * Create a PDB procedure that the script-fu-interpreter wraps.
 *
 * A GimpPDBProcedure has a run function, here script_fu_script_proc()
 * of this outer interpreter.
 * Sometime after the create, GIMP calls the run func, passing a name aka command.
 * In ScriptFu, the same name is used for the PDB proc and the Scheme function
 * which is the inner run func defined in the script.
 * script_fu_script_proc calls the TinyScheme interpreter to evaluate
 * the inner run func in the script.
 */
GimpProcedure *
script_fu_interpreter_create_proc_at_path (GimpPlugIn  *plug_in,
                                           const gchar *proc_name,
                                           const gchar *path_to_this_script
                                          )
{
  GimpProcedure *procedure = NULL;
  GList         *path_list = NULL;  /* list of GFile */

  g_debug ("script_fu_interpreter_create_proc_at_path, name: %s", proc_name);

  /* Require proc_name is a suitable name for a PDB procedure eg "script-fu-test".
   * (Not tested for canonical name "script-fu-<something>")
   * Require proc_name is a name that was queried earlier.
   * Require the proc_name was defined in some .scm file
   * in the same directory as the .scm file passed as argv[0].
   * The name of the .scm file eg "/plug-ins/fu/fu.scm"
   * can be entirely different from proc_name.
   *
   * Otherwise, we simply won't find the proc_name defined in any .scm file,
   * and will fail gracefully, returning NULL.
   */

  path_list = script_fu_search_path ();
  path_list = g_list_append (path_list,
                             script_fu_get_plugin_parent_path (path_to_this_script));
  /* path_list are the /scripts dir, for .init and compat.scm, plus the path to this.
   * second arg TRUE means define script-fu-register so it is effective.
   */
  script_fu_init_embedded_interpreter (path_list, TRUE, GIMP_RUN_NONINTERACTIVE);

  /* Reuse path_list, now a list of only the path to this script. */
  script_fu_free_path_list (&path_list);
  path_list = g_list_append (path_list,
                             script_fu_get_plugin_parent_path (path_to_this_script));

  procedure = script_fu_find_scripts_create_PDB_proc_plugin (plug_in, path_list, proc_name);
  script_fu_free_path_list (&path_list);

  /* When procedure is not NULL, assert:
   *    some .scm was evaluated.
   *    the script defined many PDB procedures locally, i.e. in script-tree
   *    we created a single PDB procedure (but not put it in the GIMP PDB)
   *
   * Ensure procedure is-a GimpProcedure or NULL.
   * GIMP is the caller and will put non-NULL procedure in the PDB.
   */
  return procedure;
}


/* Return GFile of the parent directory of this plugin, whose filename is given.
 *
 * Caller must free the GFile.
 */
static GFile *
script_fu_get_plugin_parent_path (const gchar *path_to_this_script)
{
  GFile *path        = NULL;
  GFile *parent_path = NULL;

  /* A libgimp GimpPlugin does not know its path,
   * but its path was passed in argv to this interpreter.
   * The path is to a file being queried e.g. "~/.config/GIMP/2.99/plug-ins/fu/fu.scm"
   */
  g_debug ("path to this plugin %s", path_to_this_script);
  path = g_file_new_for_path (path_to_this_script);
  parent_path = g_file_get_parent (path);
  g_object_unref (path);
  return parent_path;
}

/* Free a list of paths at the given handle.
 * Ensures that the pointer to the list is NULL, prevents "dangling."
 * g_list_free_full alone does not do that.
 */
static void
script_fu_free_path_list (GList **list)
{
  /* !!! g_steal_pointer takes a handle. */
  g_list_free_full (g_steal_pointer (list), g_object_unref);
}
