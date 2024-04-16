
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

#include <libgimp/gimp.h>

#include "script-fu-lib.h"

#include "script-fu-types.h"     /* SFScript */
#include "scheme-wrapper.h"      /* tinyscheme_init etc, */
#include "script-fu-scripts.h"   /* script_fu_find_scripts */
#include "script-fu-interface.h" /* script_fu_interface_is_active */
#include "script-fu-proc-factory.h"


/*
 * The purpose here is a small, clean API to the exported functions of the library,
 * hiding internal types of the library
 * and hiding functions not static but not exported.
 *
 * Some are simple delegation to scheme_wrapper functions,
 * but others adapt
 * and some call functions not in scheme_wrapper.c
 */


/*
 * Return whether extension-script-fu has an open dialog.
 * extension-script-fu is a single process.
 * It cannot have concurrent dialogs open in the GIMP app.
 *
 * Other plugins implementing PLUGIN type PDB procedures
 * in their own process (e.g. gimp-scheme-interpreter) do not need this.
 */
gboolean
script_fu_extension_is_busy (void)
{
  return script_fu_interface_is_active ();
}

/*
 * Find files at given paths, load them into the interpreter,
 * and register them as PDB procs of type TEMPORARY,
 * owned by the PDB proc of type PLUGIN for the given plugin.
 */
void
script_fu_find_and_register_scripts ( GimpPlugIn     *plugin,
                                      GList          *paths)
{
  script_fu_find_scripts (plugin, paths);
}

/*
 * Init the embedded interpreter.
 *
 * allow_register:
 * TRUE: allow loaded scripts to register PDB procedures.
 * The scheme functions script-fu-register and script-fu-menu-register are
 * defined to do something.
 * FALSE:  The scheme functions script-fu-register and script-fu-menu-register are
 * defined but do nothing.
 *
 * Note that the embedded interpreter always defines scheme functions
 * for all PDB procedures already existing when the interpreter starts
 * (currently bound at startup, but its possible to lazy bind.)
 * allow_register doesn't affect that.
 */
void
script_fu_init_embedded_interpreter ( GList          *paths,
                                      gboolean        allow_register,
                                      GimpRunMode     run_mode)
{
  g_debug ("script_fu_init_embedded_interpreter");
  tinyscheme_init (paths, allow_register);
  ts_set_run_mode (run_mode);
  /*
   * Ensure the embedded interpreter is running
   * and has loaded its internal Scheme scripts
   * and has defined existing PDB procs as Scheme foreign functions
   * (is ready to interpret PDB-like function calls in scheme scripts.)
   *
   * scripts/...init and scripts/...compat.scm are loaded
   * iff paths includes the "/scripts" dir.
   *
   * The .scm file(s) for plugins are loaded
   * iff paths includes their parent directory (e.g. /scripts)
   * Loaded does not imply yet registered in the PDB
   * (yet, they soon might be for some phases of the plugin.)
   */
}

void
script_fu_set_print_flag  (gboolean should_print)
{
  ts_set_print_flag (should_print);
}

/*
 * Make tinyscheme begin writing output to given gstring.
 */
void
script_fu_redirect_output_to_gstr (GString *output)
{
  ts_register_output_func (ts_gstring_output_func, output);
}

void
script_fu_redirect_output_to_stdout (void)
{
  ts_register_output_func (ts_stdout_output_func, NULL);
}

void
script_fu_print_welcome (void)
{
  ts_print_welcome ();
}

gboolean
script_fu_interpret_string (const gchar *text)
{
  /*converting from enum to boolean */
  return (gboolean) ts_interpret_string (text);
}

void
script_fu_set_run_mode (GimpRunMode run_mode)
{
  ts_set_run_mode (run_mode);
}

const gchar *
script_fu_get_success_msg (void)
{
  return ts_get_success_msg ();
}

/* Return an error message string for recent failure of script.
 *
 * Requires an interpretation just returned an error, else returns "Unknown".
 * Should be called exactly once per error, else second calls return "Unknown".
 *
 * Transfer ownership to caller, the string must be freed.
 */
const gchar *
script_fu_get_error_msg (void)
{
  return ts_get_error_msg ();
}

/* Return a GError for recent failure of script.
 *
 * Requires an interpretation just returned an error,
 * else returns a GError with message "Unknown".
 * Should be called exactly once per error
 *
 * You should call either get_error_msg, or get_gerror, but not both.
 *
 * Transfers ownership, caller must free the GError.
 */
GError *
script_fu_get_gerror (void)
{
  const gchar *error_message;
  GError      *result;

  error_message = script_fu_get_error_msg ();
  result = g_error_new_literal (g_quark_from_string ("scriptfu"), 0, error_message);
  g_free ((gpointer) error_message);
  return result;
}

void
script_fu_run_read_eval_print_loop (void)
{
  ts_interpret_stdin ();
}

void
script_fu_register_quit_callback (void (*func) (void))
{
  ts_register_quit_callback (func);
}

void
script_fu_register_post_command_callback (void (*func) (void))
{
  ts_register_post_command_callback (func);
}

/*
 * Return list of paths to directories containing .scm and .init scripts.
 * Usually at least GIMP's directory named like "/scripts."
 * List can also contain dirs custom or private to a user.
 " The GIMP dir often contain: plugins, init scripts, and utility scripts.
 *
 * Caller must free the returned list.
 */
GList *
script_fu_search_path (void)
{
  gchar *path_str;
  GList *path = NULL;

  path_str = gimp_gimprc_query ("script-fu-path");
  if (path_str)
    {
      GError *error = NULL;

      path = gimp_config_path_expand_to_files (path_str, &error);
      g_free (path_str);

      if (! path)
        {
          g_warning ("Can't convert script-fu-path to filesystem encoding: %s",
                     error->message);
          g_clear_error (&error);
        }
    }
  return path;
}


GimpProcedure *
script_fu_find_scripts_create_PDB_proc_plugin (GimpPlugIn  *plug_in,
                                               GList       *paths,
                                               const gchar *name)
{
  /* Delegate to factory. */
  return script_fu_proc_factory_make_PLUGIN (plug_in, paths, name);
}

GList *
script_fu_find_scripts_list_proc_names (GimpPlugIn *plug_in,
                                        GList      *paths)
{
  /* Delegate to factory. */
  return script_fu_proc_factory_list_names (plug_in, paths);
}
