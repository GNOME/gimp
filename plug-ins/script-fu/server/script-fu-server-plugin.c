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

#include "script-fu-server.h"

#include "libscriptfu/script-fu-lib.h"
#include "libscriptfu/script-fu-intl.h"


#define SCRIPT_FU_SERVER_TYPE (script_fu_server_get_type ())
G_DECLARE_FINAL_TYPE (ScriptFuServer, script_fu_server, SCRIPT, FU_SERVER, GimpPlugIn)

struct _ScriptFuServer
{
  GimpPlugIn parent_instance;
};

static GList          * script_fu_server_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * script_fu_server_create_procedure (GimpPlugIn           *plug_in,
                                                           const gchar          *name);

static GimpValueArray * script_fu_server_outer_run        (GimpProcedure        *procedure,
                                                           GimpProcedureConfig  *config,
                                                           gpointer              run_data);
static void             script_fu_server_run_init         (GimpProcedure        *procedure,
                                                           GimpRunMode           run_mode);


G_DEFINE_TYPE (ScriptFuServer, script_fu_server, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (SCRIPT_FU_SERVER_TYPE)
DEFINE_STD_SET_I18N


static void
script_fu_server_class_init (ScriptFuServerClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = script_fu_server_query_procedures;
  plug_in_class->create_procedure = script_fu_server_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
script_fu_server_init (ScriptFuServer *script_fu_server)
{
}

static GList *
script_fu_server_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup ("plug-in-script-fu-server"));

  return list;
}

static GimpProcedure *
script_fu_server_create_procedure (GimpPlugIn  *plug_in,
                                   const gchar *name)
{
  GimpProcedure *procedure = NULL;

  /* The run func script_fu_server_outer_run is defined in this source file. */
  procedure = gimp_procedure_new (plug_in, name,
                                  GIMP_PDB_PROC_TYPE_PLUGIN,
                                  script_fu_server_outer_run, NULL, NULL);

  gimp_procedure_set_menu_label (procedure, _("_Start Server..."));
  gimp_procedure_add_menu_path (procedure,
                                "<Image>/Filters/Development/Script-Fu");

  gimp_procedure_set_documentation (procedure,
                                    _("Server for remote Script-Fu "
                                      "operation"),
                                    "Provides a server for remote "
                                    "script-fu operation. NOTE that for "
                                    "security reasons this procedure's "
                                    "API was changed in an incompatible "
                                    "way since GIMP 2.8.12. You now have "
                                    "to pass the IP to listen on as "
                                    "first parameter. Calling this "
                                    "procedure with the old API will "
                                    "fail on purpose.",
                                    name);
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

  gimp_procedure_add_string_argument (procedure, "ip",
                                      "IP",
                                      "The IP on which to listen for requests",
                                      NULL,
                                      G_PARAM_READWRITE);

  gimp_procedure_add_int_argument (procedure, "port",
                                   "Port",
                                   "The port on which to listen for requests",
                                   0, G_MAXINT, 0,
                                   G_PARAM_READWRITE);

  /* FUTURE: gimp_procedure_add_file_argument, but little benefit, need change script-fu-server.c */
  gimp_procedure_add_string_argument (procedure, "logfile",
                                      "Log File",
                                      "The file to log activity to",
                                      NULL,
                                      G_PARAM_READWRITE);

  return procedure;
}


/*
 * Test cases:
 *
 * Normal starting is from GIMP GUI: "Filters>Development>Script-Fu>Start server...""
 * Expect a dialog to enter IP, etc.
 * Expect a console msg: "ScriptFu server: initialized and listening...""
 *
 * Does not have settings.  After the above,
 * Expect "Filters>Repeat Last" and "Reshow Last" to be disabled (greyed out)
 *
 * Execute the server from headless GIMP:
 * gimp -i --batch-interpreter='plug-in-script-fu-eval'
 " -c '(plug-in-script-fu-server 1 "127.0.0.1" 10008 "/tmp/gimp-log")'
 *
 * Execute the binary from command line fails with:
 * "script-fu-server is a GIMP plug-in and must be run by GIMP to be used"
 *
 * The PDB procedure plug-in-script-fu-server CAN be called from another procedure
 * (but shouldn't be.)
 * Expect plug-in-script-fu-server to never return
 */

/*
 * ScriptFu Server listens and responds to clients.
 * The server responds with a success indication,
 * and a string representation of the result
 * (a representation in Lisp, human-readable but also interpretable.)
 * A client may also expect side-effects, e.g. on images and image files.
 *
 * The server has its own logging.
 * Its logging defaults to stdout, but optionally to a file.
 * The server logs errors in interpretation of the stream from the client.
 *
 * A client may quit the server by eval "(gimp-quit)"
 * Otherwise, the server blocks on IO from the client.
 *
 * A server that dies leaves the client with a broken connection,
 * but does not affect extension-script-fu.
 *
 * A server is a child process of a GIMP process.
 * Quitting or killing the parent process also kills the server.
 */
static GimpValueArray *
script_fu_server_outer_run (GimpProcedure        *procedure,
                            GimpProcedureConfig  *config,
                            gpointer              run_data)
{
  GimpValueArray *return_vals = NULL;
  GimpRunMode     run_mode    = GIMP_RUN_NONINTERACTIVE;

  g_object_get (config, "run-mode", &run_mode, NULL);
  script_fu_server_run_init (procedure, run_mode);

  /* Remind any users watching the console. */
  g_debug ("Starting. Further logging by server might be to a log file.");

  /*
   * Call the inner run func, defined in script-fu-server.c
   * !!! This does not return unless a client evals "(gimp-quit)"
   */
  return_vals = script_fu_server_run (procedure, config);

  /*
   * The server returns SUCCESS but no other values (to the caller)
   * if it was quit properly.
   * The server also returns ERROR if it can't start properly.
   */
  g_assert (return_vals != NULL);
  return return_vals;
}

/*
 * The server is just the interpreter.
 * It does not register any scripts as TEMPORARY procs.
 * extension-script-fu should also be running, to register its TEMPORARY procs
 * (those defined by .scm files in /scripts)
 * in the PDB, and to execute the TEMPORARY PDB procs.
 *
 * We do load initialization and compatibility scripts.
 */
static void
script_fu_server_run_init (GimpProcedure *procedure,
                           GimpRunMode    run_mode)
{
  GList *path;

  /*
   * Non-null path so we load init and compat scripts
   * which are jumbled in same dir as TEMPORARY procedure scripts.
   */
  path = script_fu_search_path ();

  /*  Init the interpreter, not allow register scripts */
  script_fu_init_embedded_interpreter (path, FALSE, run_mode);

  g_list_free_full (path, (GDestroyNotify) g_object_unref);
}
