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

#include "tinyscheme/scheme-private.h"
#include "script-fu-types.h"
#include "script-fu-scripts.h"
#include "script-fu-script.h"

#include "script-fu-proc-factory.h"

/* Local functions */
static void  script_fu_add_menu_to_procedure (GimpProcedure *procedure,
                                              SFScript      *script);


/* Methods to register PDB procs. A factory makes objects, here PDB procedures.
 *
 * Used by the outer script-fu-interpreter
 *
 * This is in libscriptfu to hide the SFScript type from outer plugins.
 * These methods use instances of type SFScript as specs for procedures.
 *
 * FUTURE: migrate code.
 * There are two flavors of factory-like code: for PDBProcType TEMPORARY and PLUGIN.
 * extension-script-fu outer plugin only makes TEMPORARY
 * script-fu-interpreter outer plugin only makes PLUGIN type
 * This source file supports only script-fu-interpreter.
 * script_fu_find_scripts() in script-fu-scripts.c is also a factory-like method,
 * and could be extracted to a separate source file.
 * Maybe more code sharing between the two flavors.
 */


/* Create and return a single PDB procedure of type PLUGIN,
 * for the given proc name, by reading the script file in the given paths.
 * Also add a menu for the procedure.
 *
 * PDB proc of type PLUGIN has permanent lifetime, unlike type TEMPORARY.
 *
 * The list of paths is usually just one directory, a subdir of /plug-ins.
 * The directory may contain many .scm files.
 * The plugin manager only queries one .scm file,
 * having the same name as its parent dir and and having execute permission.
 * But here we read all the .scm files in the directory.
 * Each .scm file may register (and define run func for) many PDB procedures.
 *
 * Here, one name is passed, and though we load all the .scm files,
 * we only create a PDB procedure for the passed name.
 */
GimpProcedure *
script_fu_proc_factory_make_PLUGIN (GimpPlugIn  *plug_in,
                                    GList       *paths,
                                    const gchar *proc_name)
{
  SFScript      * script    = NULL;
  GimpProcedure * procedure = NULL;

  /* Reads all .scm files at paths, even though only one is pertinent.
   * The returned script_tree is also in the state of the interpreter,
   * we don't need the result here.
  */
  (void) script_fu_find_scripts_into_tree (plug_in, paths);

  /* Get the pertinent script from the tree. */
  script = script_fu_find_script (proc_name);

  if (script)
    {
      procedure = script_fu_script_create_PDB_procedure (
        plug_in,
        script,
        GIMP_PDB_PROC_TYPE_PLUGIN);
      script_fu_add_menu_to_procedure (procedure, script);
    }
  else
    {
      g_warning ("Failed to find script: %s.", proc_name);
    }
  return procedure;
}

 /* Traverse the list of scripts, for each defined name of a PDB proc,
  * add it list whose handle is given.
  *
  * Order is not important.  Could just as well prepend.
  *
  * This is a GTraverseFunction
  */
static gboolean
script_fu_append_script_names (gpointer      *foo G_GNUC_UNUSED,
                               GList         *scripts,
                               GList        **name_list)
{
  for (GList * list = scripts; list; list = g_list_next (list))
    {
      SFScript *script   = list->data;

      if ( !script_fu_is_defined (script->name))
        {
          g_warning ("Run function not defined, or does not match PDB procedure name: %s",
                     script->name);
          continue;
        }

      /* Must assign result from g_list_append back to name_list */
      *name_list = g_list_append ( (GList *) *name_list, g_strdup (script->name));
    }
  return FALSE; /* We traversed all. */
 }

/* Load script texts (.scm files) in the given paths.
 * Iterate over all loaded scripts to get the PDB proc names they define.
 * Return a list of the names.
 */
GList *
script_fu_proc_factory_list_names (GimpPlugIn *plug_in,
                                   GList      *paths)
{
  GList * result_list = NULL;
  GTree * script_tree = NULL;

  /* Load (eval) all .scm files in all dirs in paths. */
  script_tree = script_fu_find_scripts_into_tree (plug_in, paths);

  /* Iterate over the tree, adding each script name to result list */
  g_tree_foreach (script_tree,
                  (GTraverseFunc) script_fu_append_script_names,
                  &result_list);

  return result_list;
}

/* From scriptfu's internal data, add any menu to given procedure in the PDB.
 * Requires that a script was just eval'ed so that scriptfu's list of menus
 * declared in a script is valid.
 * Requires the proc exists in PDB.
 *
 * Not ensure the PDB proc has a menu, when no menu was defined in the script.
 *
 * Derived from script_fu_install_menu, but that is specific to TEMPORARY procs.
 * Also, unlike script_fu_install_menu, we don't nuke the menu list as we proceed.
 *
 * For each "create" of a procedure, the gimp-script-fu-interpreter is started anew,
 * and a new script_menu_list is derived from the .scm file.
 * We don't traverse the menu list more than once per session, which soon exits.
 */
static void
script_fu_add_menu_to_procedure (GimpProcedure *procedure,
                                  SFScript      *script)
{
  GList    *menu_list;
  gboolean  did_add_menu = FALSE;

  menu_list = script_fu_get_menu_list ();
  /* menu_list might be NULL: for loop will have no iterations. */

  /* Each .scm file can declare many menu paths.
   * Traverse the list to find the menu path defined for the procedure.
   * Each SFMenu points to the procedure (SFScript) it belongs to.
   */
  for (GList * traverser = menu_list; traverser; traverser = g_list_next (traverser))
    {
      SFMenu *menu = traverser->data;
      if (menu->script == script)
        {
          g_debug ("Add menu: %s", menu->menu_path);
          gimp_procedure_add_menu_path (procedure, menu->menu_path);
          did_add_menu = TRUE;
          break;
        }
    }

  /* Some procedures don't have menu path.
   * It is normal, but not common, to define procs of type PLUGIN that don't appear in the menus.
   * No part of GIMP defaults a menu path for procedures.
   * A menu label without a menu path is probably a mistake by the script author.
   */
  if ( ! did_add_menu )
    {
      /* Unusual for a .scm file to have no menu paths, but not an error. */
      g_debug ("No menu paths! Does the procedure name in script-fu-menu-register match?");
      /* FUTURE if the script defines a menu *label*, declare an error. */
    }
  /* script_menu_list is a reference we do not need to free. */
 }
