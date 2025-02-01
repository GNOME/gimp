
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
#include "script-fu-script.h"    /* script_fu_script_get_i18n */
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

static gboolean _script_fu_report_progress = FALSE;

/**
 * script_fu_report_progress:
 *
 * Returns: whether we should report progress status automatically
 *          (normally only within the "extension-script-fu" persistent plug-in).
 */
gboolean
script_fu_report_progress (void)
{
  return _script_fu_report_progress;
}

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
script_fu_init_embedded_interpreter (GList       *paths,
                                     gboolean     allow_register,
                                     GimpRunMode  run_mode,
                                     gboolean     report_progress)
{
  g_debug ("script_fu_init_embedded_interpreter");
  _script_fu_report_progress = report_progress;
  tinyscheme_init (paths, allow_register);
  ts_set_run_mode (run_mode);
  /*
   * Ensure the embedded interpreter is running and:
   *    loaded its internal Scheme scripts e.g. init.scm
   *    defined existing PDB procs as Scheme foreign functions
   *      (is ready to interpret PDB-like function calls in scheme scripts.)
   *    has loaded other init and compat scripts in /scripts/init
   *      e.g. script-fu-compat.scm
   *
   * The .scm file(s) for plugins in /scripts are NOT loaded.
   * Any util scripts in /scripts are NOT loaded, e.g. script-fu-utils.scm.
   */
}

/* Load script files at paths.
 * Side effect: create state script_tree.
 * Requires interpreter initialized.
 */
void
script_fu_load_scripts_into_tree (GimpPlugIn *plugin,
                                  GList      *paths)
{
  script_fu_scripts_load_into_tree (plugin, paths);
}

/* Has the interpreter been initialized and a script loaded
 * i.e. interpreted for registration into interpreter state: script_tree.
 */
gboolean
script_fu_is_scripts_loaded (void)
{
  return script_fu_scripts_are_loaded ();
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
 *
 * List has two elements, in this order:
 *    directory for user scripts
 *    directory for system-wide scripts, distributed with GIMP
 * The dirs usually contain: plugins, init scripts, and utility scripts.
 *
 * The returned type is GList of GFile.
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

/* Returns path to a subdirectory of the given path,
 * the subdirectory containing init scripts.
 *
 * SF keeps init scripts segregated in a subdirectory, since 3.0.
 * This lets them have the proper suffix ".scm"
 * without being confused with plugin scripts.
 * Also lets other scripts in that directory not be loaded at startup.
 *
 * This hides the name "init".
 *
 * Caller must free the result.
 */
gchar *
script_fu_get_init_subdirectory (GFile *dir)
{
  GFile *subdirectory = g_file_get_child (dir, "scriptfu-init");
  gchar *result_path  = g_file_get_path (subdirectory);

  g_object_unref (subdirectory);

  /* result is a string path to a directory which might not exist. */
  return result_path;
}

/* The directory containing init scripts installed with GIMP.
 * Caller must free the result string.
 */
gchar *
script_fu_sys_init_directory (void)
{
  GList *paths = script_fu_search_path ();
  GList *list_element;
  gchar *result_path;

  /* The second list element is the sys scripts dir. */
  list_element = g_list_next (paths);

  result_path = script_fu_get_init_subdirectory (list_element->data);

  g_list_free_full (paths, (GDestroyNotify) g_object_unref);

  return result_path;
}

/* The directory containing user init scripts.
 * Caller must free the result string.
 */
gchar *
script_fu_user_init_directory (void)
{
  GList *paths = script_fu_search_path ();
  GList *list_element;
  gchar *result_path;

  /* The first list element is the user scripts dir. */
  list_element = paths;

  result_path = script_fu_get_init_subdirectory (list_element->data);

  g_list_free_full (paths, (GDestroyNotify) g_object_unref);

  return result_path;
}

gboolean

/* Is the given dir named like an init dir for SF?
 *
 * This is lax, and doesn't check the dir is in one of
 * the two locations for SF's init directories.
 * Users should not use the name in their own directories
 * in the tree of script directories.
 */
script_fu_is_init_directory (GFile *dir)
{
  char    *basename = g_file_get_basename (dir);
  gboolean result;

  result = g_strcmp0 (basename, "scriptfu-init") == 0;
  g_free (basename);
  return result;
}

/* Create a PDB procedure from the SFScript for the given proc name.
 * Does not register into the PDB.
 * Requires scripts already loaded i.e. SFScript exist.
 */
GimpProcedure *
script_fu_create_PDB_proc_plugin (GimpPlugIn  *plug_in,
                                  const gchar *name)
{
  /* Delegate to factory. */
  return script_fu_proc_factory_make_PLUGIN (plug_in, name);
}

GList *
script_fu_find_scripts_list_proc_names (GimpPlugIn *plug_in,
                                        GList      *paths)
{
  /* Delegate to factory. */
  return script_fu_proc_factory_list_names (plug_in, paths);
}

/* Requires scripts already loaded. */
void
script_fu_get_i18n_for_proc (const gchar *proc_name,
                             gchar      **declared_i18n_domain,
                             gchar      **declared_i18n_catalog)
{
  SFScript *script = script_fu_find_script (proc_name);
  script_fu_script_get_i18n (script, declared_i18n_domain, declared_i18n_catalog);
}
