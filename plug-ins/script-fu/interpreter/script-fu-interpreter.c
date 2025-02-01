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
 * e.g. init.scm.
 *
 * scrip-fu-interpreter always inits embedded interpreter(allow_register=TRUE)
 * In the "run" phase, you don't need script-fu-register to be defined, but its harmless.
 *
 * The usual sequence of phases and callbacks from GimpPlugin is:
 *   query phase
 *      set_i18n          script_fu_interpreter_get_i18n
 *      query procedures  script_fu_interpreter_list_defined_proc_names
 *   run phase
 *      set_i18n          script_fu_interpreter_get_i18n
 *      create procedure  script_fu_interpreter_create_proc
 *      set_i18n          script_fu_interpreter_get_i18n
 *      run procedure     (calls directly a C func defined in script-fu-run-func.c)
 * We only init interpreter and load scripts once per phase.
 */

static GFile *script_fu_get_plugin_parent_path (const gchar *path_to_this_script);
static void   script_fu_free_path_list         (GList      **list);
static void   script_fu_interpreter_init_inner (void);
static void   script_fu_interpreter_load_script (
                                                GimpPlugIn  *plug_in,
                                                const gchar *path_to_script);
static void   script_fu_interpreter_init_and_load_script (
                                                GimpPlugIn  *plug_in,
                                                const gchar *path_to_script);

/* Return a list of PDB procedure names defined in all .scm files in
 * the parent dir of the given path, which is a filename of the one being queried.
 * This is called in the "query" phase, and subsequently the interpreter will exit.
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

  script_fu_interpreter_init_inner();

  /* List one path, the parent dir of the queried script. */
  path_list = g_list_append (path_list,
                             script_fu_get_plugin_parent_path (path_to_this_script));
  name_list = script_fu_find_scripts_list_proc_names (plug_in, path_list);
  script_fu_free_path_list (&path_list);

  /* Usually name_list is not NULL i.e. not empty.
   * But an .scm file that is not an actual GIMP plugin, or broken, may yield empty list.
   */
  return name_list;
}

GimpProcedure *
script_fu_interpreter_create_proc (GimpPlugIn  *plug_in,
                                   const gchar *proc_name,
                                   const gchar *path_to_script)
{
  GimpProcedure *procedure = NULL;

  g_debug ("%s name: %s", G_STRFUNC, proc_name);

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
  /* Load scripts.
   * We loaded scripts prior for callback gimp_plugin_set_18n,
   * but then i18n was not in effect.
   * Load again, but now i18n will translate GUI strings for declared args.
   */
  script_fu_interpreter_load_script (plug_in, path_to_script);

  /* Assert loaded scripts has proc_name.  Else this fails, returns NULL. */
  procedure = script_fu_create_PDB_proc_plugin (plug_in, proc_name);

  /* When procedure is not NULL, the caller GIMP will it in the PDB. */
  return procedure;
}


/* Return i18n domain and catalog declared by script at path.
 * Returns NULL when script did not call script-fu-register-i18n.
 * Returned strings are new allocated.
 * Returned strings are returned at the given handles.
 *
 * This is only used by the interpreter.
 * extension-script-fu does not let old-style scripts in /scripts
 * declare i18n; such scripts use only the shared translations gimp30-script-fu.mo.
 *
 * This is called twice when GIMP is running the procedure:
 * 1. before create
 * 2. before run
 * For the first call, we initialize the interpreter and load the script.
 * For the second call, the script is still loaded,
 * and we return the same results as the first call.
 */
void
script_fu_interpreter_get_i18n (GimpPlugIn  *plug_in,
                                const gchar *proc_name,
                                const gchar *path_to_this_script,
                                gchar       **i18n_domain, /* OUT handles */
                                gchar       **i18n_catalog)
{
  /* As discussed above, this may be called many times in same interpreter session.
   * Only init interpreter once.
   */
  if (! script_fu_is_scripts_loaded ())
    {
      script_fu_interpreter_init_and_load_script (plug_in, path_to_this_script);
    }

  /* This will allocate strings and set the handles. */
  return script_fu_get_i18n_for_proc (proc_name, i18n_domain, i18n_catalog);
}

/* Load plugin .scm files from directory at path.
 * Side effects on the interpreter's tree of scripts.
 *
 * This may load many files which define many PDB procedures.
 * This does not install the PDB procedures defined by the scripts.
 *
 * This can be called sequentially but the effect is not cumulative:
 * it frees any scripts already loaded into internal tree.
 * A second call reloads the tree.
 */
static void
script_fu_interpreter_load_script (GimpPlugIn  *plug_in,
                                   const gchar *path_to_script)
{
  GList *path_list   = NULL;  /* list of GFile */

  /* Convert file name to list of one parent path.
   * A SF independently interpreted file must be in its own dir,
   * and load_scripts wants a dir, not a file.
   */
  path_list = g_list_append (path_list,
                             script_fu_get_plugin_parent_path (path_to_script));

  /* Get scripts into global state: scripts_tree. */
  (void) script_fu_load_scripts_into_tree (plug_in, path_list);

  script_fu_free_path_list (&path_list);

  /* Not ensure script_fu_is_scripts_loaded(),
   * when the path is bad or is to a file that is not a valid SF script file.
   * Not ensure that the file declared any particular procedure name.
   */
}



/* Init the SF interpreter and load plugin script files at path.
 *
 * Is an error to call more than once.
 *
 * The path is to a directory expected to contain one or more plugin .scm file.
 *
 * Initting the interpreter also "loads" non-plugin .scm files,
 * where "load" means in Scheme: read and evaluate.
 */
/* FUTURE we should not need to pass plug_in.  See script_fu_remove_script */
static void
script_fu_interpreter_init_and_load_script (GimpPlugIn  *plug_in,
                                            const gchar *path_to_script)
{
  if (script_fu_is_scripts_loaded ())
    {
      g_error ("%s interpreter already init", G_STRFUNC);
      return;
    }

  script_fu_interpreter_init_inner();

  script_fu_interpreter_load_script (plug_in, path_to_script);
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

/* Init the TinyScheme interpreter
 * and the ScriptFu interpreter that wraps it.
 *
 * Side effects only, on the state of the interpreter.
 *
 * Ensures:
 *    Innermost TinyScheme interpreter is initialized.
 *    It has loaded init.scm (and some other scripts in /scripts/init)
 *    The ScriptFu registration functions are defined
 *       (and other functions unique to ScriptFu outer interpreter.)
 *
 * It has NOT loaded plugin scripts:
 *   in /scripts, served by extension-script-fu
 *   in /plug-ins, served by independent interpreter
 */
static void
script_fu_interpreter_init_inner (void)
{
  GList *path_list = NULL;  /* list of GFile */

  g_debug ("%s", G_STRFUNC);

  path_list = script_fu_search_path ();
  /* path_list is /scripts dir which has subdir /init of compat and init scripts. */

  /* Second argument TRUE means define script-fu-register
   * and other registration functions into the interpreter.
   * So that plugin scripts WILL load IN THE FUTURE.
   * This does not load any plugins,
   * but subsequently, the interpreter will recognize registration functions
   * when interpreter loads a plugin .scm file.
   *
   * Fourth argument FALSE means use no progress reporting.
   */
  script_fu_init_embedded_interpreter (path_list, TRUE, GIMP_RUN_NONINTERACTIVE, FALSE);
  script_fu_free_path_list (&path_list);
}
