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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef __GLIBC__
#include <malloc.h>
#endif

#include <locale.h>

#include <gio/gio.h>

#ifdef G_OS_WIN32
#include <io.h> /* get_osfhandle */
#endif

#ifndef GIMP_CONSOLE_COMPILATION
#include <gdk/gdk.h>
#endif

#include <babl/babl.h>

#include "libgimpbase/gimpbase.h"

#include "pdb/pdb-types.h"

#include "config/gimpconfig-dump.h"

#include "core/gimp.h"

#include "pdb/gimppdb.h"
#include "pdb/gimpprocedure.h"
#include "pdb/internal-procs.h"

#include "about.h"
#include "app.h"
#include "sanity.h"
#include "signals.h"
#include "unique.h"
#include "units.h"
#include "version.h"

#ifdef G_OS_WIN32
/* To get PROCESS_DEP_* defined we need _WIN32_WINNT at 0x0601. We still
 * use the API optionally only if present, though.
 */
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <conio.h>
#endif

#include "gimp-log.h"
#include "gimp-intl.h"


static gboolean  gimp_option_fatal_warnings   (const gchar  *option_name,
                                               const gchar  *value,
                                               gpointer      data,
                                               GError      **error);
static gboolean  gimp_option_stack_trace_mode (const gchar  *option_name,
                                               const gchar  *value,
                                               gpointer      data,
                                               GError      **error);
static gboolean  gimp_option_pdb_compat_mode  (const gchar  *option_name,
                                               const gchar  *value,
                                               gpointer      data,
                                               GError      **error);
static gboolean  gimp_option_dump_gimprc      (const gchar  *option_name,
                                               const gchar  *value,
                                               gpointer      data,
                                               GError      **error);
static gboolean  gimp_option_dump_pdb_procedures_deprecated
                                              (const gchar  *option_name,
                                               const gchar  *value,
                                               gpointer      data,
                                               GError      **error);

static void      gimp_show_version_and_exit   (void) G_GNUC_NORETURN;
static void      gimp_show_license_and_exit   (void) G_GNUC_NORETURN;

static void      gimp_init_i18n               (void);
static void      gimp_init_malloc             (void);

#if defined (G_OS_WIN32) && !defined (GIMP_CONSOLE_COMPILATION)
static void      gimp_open_console_window     (void);
#else
#define gimp_open_console_window() /* as nothing */
#endif

static const gchar        *system_gimprc     = NULL;
static const gchar        *user_gimprc       = NULL;
static const gchar        *session_name      = NULL;
static const gchar        *batch_interpreter = NULL;
static const gchar       **batch_commands    = NULL;
static const gchar       **filenames         = NULL;
static gboolean            as_new            = FALSE;
static gboolean            no_interface      = FALSE;
static gboolean            no_data           = FALSE;
static gboolean            no_fonts          = FALSE;
static gboolean            no_splash         = FALSE;
static gboolean            be_verbose        = FALSE;
static gboolean            new_instance      = FALSE;
#if defined (USE_SYSV_SHM) || defined (USE_POSIX_SHM) || defined (G_OS_WIN32)
static gboolean            use_shm           = TRUE;
#else
static gboolean            use_shm           = FALSE;
#endif
static gboolean            use_cpu_accel     = TRUE;
static gboolean            console_messages  = FALSE;
static gboolean            use_debug_handler = FALSE;

#ifdef GIMP_UNSTABLE
static gboolean            show_playground   = TRUE;
static GimpStackTraceMode  stack_trace_mode  = GIMP_STACK_TRACE_QUERY;
static GimpPDBCompatMode   pdb_compat_mode   = GIMP_PDB_COMPAT_WARN;
#else
static gboolean            show_playground   = FALSE;
static GimpStackTraceMode  stack_trace_mode  = GIMP_STACK_TRACE_NEVER;
static GimpPDBCompatMode   pdb_compat_mode   = GIMP_PDB_COMPAT_ON;
#endif


static const GOptionEntry main_entries[] =
{
  { "version", 'v', G_OPTION_FLAG_NO_ARG,
    G_OPTION_ARG_CALLBACK, (GOptionArgFunc) gimp_show_version_and_exit,
    N_("Show version information and exit"), NULL
  },
  {
    "license", 0, G_OPTION_FLAG_NO_ARG,
    G_OPTION_ARG_CALLBACK, (GOptionArgFunc) gimp_show_license_and_exit,
    N_("Show license information and exit"), NULL
  },
  {
    "verbose", 0, 0,
    G_OPTION_ARG_NONE, &be_verbose,
    N_("Be more verbose"), NULL
  },
  {
    "new-instance", 'n', 0,
    G_OPTION_ARG_NONE, &new_instance,
    N_("Start a new GIMP instance"), NULL
  },
  {
    "as-new", 'a', 0,
    G_OPTION_ARG_NONE, &as_new,
    N_("Open images as new"), NULL
  },
  {
    "no-interface", 'i', 0,
    G_OPTION_ARG_NONE, &no_interface,
    N_("Run without a user interface"), NULL
  },
  {
    "no-data", 'd', 0,
    G_OPTION_ARG_NONE, &no_data,
    N_("Do not load brushes, gradients, patterns, ..."), NULL
  },
  {
    "no-fonts", 'f', 0,
    G_OPTION_ARG_NONE, &no_fonts,
    N_("Do not load any fonts"), NULL
  },
  {
    "no-splash", 's', 0,
    G_OPTION_ARG_NONE, &no_splash,
    N_("Do not show a splash screen"), NULL
  },
  {
    "no-shm", 0, G_OPTION_FLAG_REVERSE,
    G_OPTION_ARG_NONE, &use_shm,
    N_("Do not use shared memory between GIMP and plugins"), NULL
  },
  {
    "no-cpu-accel", 0, G_OPTION_FLAG_REVERSE,
    G_OPTION_ARG_NONE, &use_cpu_accel,
    N_("Do not use special CPU acceleration functions"), NULL
  },
  {
    "session", 0, 0,
    G_OPTION_ARG_FILENAME, &session_name,
    N_("Use an alternate sessionrc file"), "<name>"
  },
  {
    "gimprc", 'g', 0,
    G_OPTION_ARG_FILENAME, &user_gimprc,
    N_("Use an alternate user gimprc file"), "<filename>"
  },
  {
    "system-gimprc", 0, 0,
    G_OPTION_ARG_FILENAME, &system_gimprc,
    N_("Use an alternate system gimprc file"), "<filename>"
  },
  {
    "batch", 'b', 0,
    G_OPTION_ARG_STRING_ARRAY, &batch_commands,
    N_("Batch command to run (can be used multiple times)"), "<command>"
  },
  {
    "batch-interpreter", 0, 0,
    G_OPTION_ARG_STRING, &batch_interpreter,
    N_("The procedure to process batch commands with"), "<proc>"
  },
  {
    "console-messages", 'c', 0,
    G_OPTION_ARG_NONE, &console_messages,
    N_("Send messages to console instead of using a dialog"), NULL
  },
  {
    "pdb-compat-mode", 0, 0,
    G_OPTION_ARG_CALLBACK, gimp_option_pdb_compat_mode,
    /*  don't translate the mode names (off|on|warn)  */
    N_("PDB compatibility mode (off|on|warn)"), "<mode>"
  },
  {
    "stack-trace-mode", 0, 0,
    G_OPTION_ARG_CALLBACK, gimp_option_stack_trace_mode,
    /*  don't translate the mode names (never|query|always)  */
    N_("Debug in case of a crash (never|query|always)"), "<mode>"
  },
  {
    "debug-handlers", 0, 0,
    G_OPTION_ARG_NONE, &use_debug_handler,
    N_("Enable non-fatal debugging signal handlers"), NULL
  },
  {
    "g-fatal-warnings", 0, G_OPTION_FLAG_NO_ARG,
    G_OPTION_ARG_CALLBACK, gimp_option_fatal_warnings,
    N_("Make all warnings fatal"), NULL
  },
  {
    "dump-gimprc", 0, G_OPTION_FLAG_NO_ARG,
    G_OPTION_ARG_CALLBACK, gimp_option_dump_gimprc,
    N_("Output a gimprc file with default settings"), NULL
  },
  {
    "dump-gimprc-system", 0, G_OPTION_FLAG_NO_ARG | G_OPTION_FLAG_HIDDEN,
    G_OPTION_ARG_CALLBACK, gimp_option_dump_gimprc,
    NULL, NULL
  },
  {
    "dump-gimprc-manpage", 0, G_OPTION_FLAG_NO_ARG | G_OPTION_FLAG_HIDDEN,
    G_OPTION_ARG_CALLBACK, gimp_option_dump_gimprc,
    NULL, NULL
  },
  {
    "dump-pdb-procedures-deprecated", 0,
    G_OPTION_FLAG_NO_ARG | G_OPTION_FLAG_HIDDEN,
    G_OPTION_ARG_CALLBACK, gimp_option_dump_pdb_procedures_deprecated,
    N_("Output a sorted list of deprecated procedures in the PDB"), NULL
  },
  {
    "show-playground", 0, G_OPTION_FLAG_HIDDEN,
    G_OPTION_ARG_NONE, &show_playground,
    N_("Show a preferences page with experimental features"), NULL
  },
  {
    G_OPTION_REMAINING, 0, 0,
    G_OPTION_ARG_FILENAME_ARRAY, &filenames,
    NULL, NULL
  },
  { NULL }
};

int
main (int    argc,
      char **argv)
{
  GOptionContext *context;
  GError         *error = NULL;
  const gchar    *abort_message;
  gchar          *basename;
  GFile          *system_gimprc_file = NULL;
  GFile          *user_gimprc_file   = NULL;
  gint            i;

#if defined (__GNUC__) && defined (_WIN64)
  /* mingw-w64, at least the unstable build from late July 2008,
   * starts subsystem:windows programs in main(), but passes them
   * bogus argc and argv. __argc and __argv are OK, though, so just
   * use them.
   */
  argc = __argc;
  argv = __argv;
#endif

#ifdef G_OS_WIN32
  /* Reduce risks */
  {
    typedef BOOL (WINAPI *t_SetDllDirectoryA) (LPCSTR lpPathName);
    t_SetDllDirectoryA p_SetDllDirectoryA;

    p_SetDllDirectoryA = GetProcAddress (GetModuleHandle ("kernel32.dll"),
					 "SetDllDirectoryA");
    if (p_SetDllDirectoryA)
      (*p_SetDllDirectoryA) ("");
  }
#ifndef _WIN64
  {
    typedef BOOL (WINAPI *t_SetProcessDEPPolicy) (DWORD dwFlags);
    t_SetProcessDEPPolicy p_SetProcessDEPPolicy;

    p_SetProcessDEPPolicy = GetProcAddress (GetModuleHandle ("kernel32.dll"),
					    "SetProcessDEPPolicy");
    if (p_SetProcessDEPPolicy)
      (*p_SetProcessDEPPolicy) (PROCESS_DEP_ENABLE|PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION);
  }
#endif
#endif

#ifdef GIMP_UNSTABLE
  gimp_open_console_window ();
#endif

  gimp_init_malloc ();

  gimp_env_init (FALSE);

  gimp_log_init ();

  gimp_init_i18n ();

  g_set_application_name (GIMP_NAME);

#ifdef G_OS_WIN32
  argv = g_win32_get_command_line ();
#else
  argv = g_strdupv (argv);
#endif

  basename = g_path_get_basename (argv[0]);
  g_set_prgname (basename);
  g_free (basename);

  /* Check argv[] for "--verbose" first */
  for (i = 1; i < argc; i++)
    {
      const gchar *arg = argv[i];

      if (arg[0] != '-')
        continue;

      if ((strcmp (arg, "--verbose") == 0) || (strcmp (arg, "-v") == 0))
        {
          be_verbose = TRUE;
        }
    }

  /* Check argv[] for "--no-interface" before trying to initialize gtk+. */
  for (i = 1; i < argc; i++)
    {
      const gchar *arg = argv[i];

      if (arg[0] != '-')
        continue;

      if ((strcmp (arg, "--no-interface") == 0) || (strcmp (arg, "-i") == 0))
        {
          no_interface = TRUE;
        }
      else if ((strcmp (arg, "--version") == 0) || (strcmp (arg, "-v") == 0))
        {
          gimp_show_version_and_exit ();
        }
#if defined (G_OS_WIN32) && !defined (GIMP_CONSOLE_COMPILATION)
      else if ((strcmp (arg, "--help") == 0) ||
               (strcmp (arg, "-?") == 0) ||
               (strncmp (arg, "--help-", 7) == 0))
        {
          gimp_open_console_window ();
        }
#endif
    }

#ifdef GIMP_CONSOLE_COMPILATION
  no_interface = TRUE;
#endif

  context = g_option_context_new (_("[FILE|URI...]"));
  g_option_context_set_summary (context, GIMP_NAME);

  g_option_context_add_main_entries (context, main_entries, GETTEXT_PACKAGE);

  app_libs_init (context, no_interface);

  if (! g_option_context_parse_strv (context, &argv, &error))
    {
      if (error)
        {
          gimp_open_console_window ();
          g_print ("%s\n", error->message);
          g_error_free (error);
        }
      else
        {
          g_print ("%s\n",
                   _("GIMP could not initialize the graphical user interface.\n"
                     "Make sure a proper setup for your display environment "
                     "exists."));
        }

      app_exit (EXIT_FAILURE);
    }

  if (no_interface || be_verbose || console_messages || batch_commands != NULL)
    gimp_open_console_window ();

  if (no_interface)
    new_instance = TRUE;

#ifndef GIMP_CONSOLE_COMPILATION
  if (! new_instance && gimp_unique_open (filenames, as_new))
    {
      if (be_verbose)
	g_print ("%s\n",
		 _("Another GIMP instance is already running."));

      gdk_notify_startup_complete ();

      return EXIT_SUCCESS;
    }
#endif

  abort_message = sanity_check ();
  if (abort_message)
    app_abort (no_interface, abort_message);

  gimp_init_signal_handlers (stack_trace_mode);

  if (system_gimprc)
    system_gimprc_file = g_file_new_for_commandline_arg (system_gimprc);

  if (user_gimprc)
    user_gimprc_file = g_file_new_for_commandline_arg (user_gimprc);

  app_run (argv[0],
           filenames,
           system_gimprc_file,
           user_gimprc_file,
           session_name,
           batch_interpreter,
           batch_commands,
           as_new,
           no_interface,
           no_data,
           no_fonts,
           no_splash,
           be_verbose,
           use_shm,
           use_cpu_accel,
           console_messages,
           use_debug_handler,
           show_playground,
           stack_trace_mode,
           pdb_compat_mode);

  if (system_gimprc_file)
    g_object_unref (system_gimprc_file);

  if (user_gimprc_file)
    g_object_unref (user_gimprc_file);

  g_strfreev (argv);

  g_option_context_free (context);

  return EXIT_SUCCESS;
}


#ifdef G_OS_WIN32

/* Provide WinMain in case we build GIMP as a subsystem:windows
 * application. Well, we do. When built with mingw, though, user code
 * execution still starts in main() in that case. So WinMain() gets
 * used on MSVC builds only.
 */

#ifdef __GNUC__
#  ifndef _stdcall
#    define _stdcall  __attribute__((stdcall))
#  endif
#endif

int _stdcall
WinMain (struct HINSTANCE__ *hInstance,
         struct HINSTANCE__ *hPrevInstance,
         char               *lpszCmdLine,
         int                 nCmdShow)
{
  return main (__argc, __argv);
}

#ifndef GIMP_CONSOLE_COMPILATION

static void
wait_console_window (void)
{
  FILE *console = fopen ("CONOUT$", "w");

  SetConsoleTitleW (g_utf8_to_utf16 (_("GIMP output. Type any character to close this window."), -1, NULL, NULL, NULL));
  fprintf (console, _("(Type any character to close this window)\n"));
  fflush (console);
  _getch ();
}

static void
gimp_open_console_window (void)
{
  if (((HANDLE) _get_osfhandle (fileno (stdout)) == INVALID_HANDLE_VALUE ||
       (HANDLE) _get_osfhandle (fileno (stderr)) == INVALID_HANDLE_VALUE) && AllocConsole ())
    {
      if ((HANDLE) _get_osfhandle (fileno (stdout)) == INVALID_HANDLE_VALUE)
        freopen ("CONOUT$", "w", stdout);

      if ((HANDLE) _get_osfhandle (fileno (stderr)) == INVALID_HANDLE_VALUE)
        freopen ("CONOUT$", "w", stderr);

      SetConsoleTitleW (g_utf8_to_utf16 (_("GIMP output. You can minimize this window, but don't close it."), -1, NULL, NULL, NULL));

      atexit (wait_console_window);
    }
}
#endif

#endif /* G_OS_WIN32 */


static gboolean
gimp_option_fatal_warnings (const gchar  *option_name,
                            const gchar  *value,
                            gpointer      data,
                            GError      **error)
{
  GLogLevelFlags fatal_mask;

  fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
  fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;

  g_log_set_always_fatal (fatal_mask);

  return TRUE;
}

static gboolean
gimp_option_stack_trace_mode (const gchar  *option_name,
                              const gchar  *value,
                              gpointer      data,
                              GError      **error)
{
  if (strcmp (value, "never") == 0)
    stack_trace_mode = GIMP_STACK_TRACE_NEVER;
  else if (strcmp (value, "query") == 0)
    stack_trace_mode = GIMP_STACK_TRACE_QUERY;
  else if (strcmp (value, "always") == 0)
    stack_trace_mode = GIMP_STACK_TRACE_ALWAYS;
  else
    return FALSE;

  return TRUE;
}

static gboolean
gimp_option_pdb_compat_mode (const gchar  *option_name,
                             const gchar  *value,
                             gpointer      data,
                             GError      **error)
{
  if (! strcmp (value, "off"))
    pdb_compat_mode = GIMP_PDB_COMPAT_OFF;
  else if (! strcmp (value, "on"))
    pdb_compat_mode = GIMP_PDB_COMPAT_ON;
  else if (! strcmp (value, "warn"))
    pdb_compat_mode = GIMP_PDB_COMPAT_WARN;
  else
    return FALSE;

  return TRUE;
}

static gboolean
gimp_option_dump_gimprc (const gchar  *option_name,
                         const gchar  *value,
                         gpointer      data,
                         GError      **error)
{
  GimpConfigDumpFormat format = GIMP_CONFIG_DUMP_NONE;

  gimp_open_console_window ();

  if (strcmp (option_name, "--dump-gimprc") == 0)
    format = GIMP_CONFIG_DUMP_GIMPRC;
  if (strcmp (option_name, "--dump-gimprc-system") == 0)
    format = GIMP_CONFIG_DUMP_GIMPRC_SYSTEM;
  else if (strcmp (option_name, "--dump-gimprc-manpage") == 0)
    format = GIMP_CONFIG_DUMP_GIMPRC_MANPAGE;

  if (format)
    {
      Gimp     *gimp;
      gboolean  success;

      gimp = g_object_new (GIMP_TYPE_GIMP, NULL);

      units_init (gimp);
      babl_init ();

      success = gimp_config_dump (format);

      g_object_unref (gimp);

      app_exit (success ? EXIT_SUCCESS : EXIT_FAILURE);
    }

  return FALSE;
}

static gboolean
gimp_option_dump_pdb_procedures_deprecated (const gchar  *option_name,
                                            const gchar  *value,
                                            gpointer      data,
                                            GError      **error)
{
  Gimp  *gimp;
  GList *deprecated_procs;
  GList *iter;

  gimp = g_object_new (GIMP_TYPE_GIMP, NULL);

  /* Make sure to turn on compatibility mode so deprecated procedures
   * are included
   */
  gimp->pdb_compat_mode = GIMP_PDB_COMPAT_ON;

  /* Initialize the list of procedures */
  internal_procs_init (gimp->pdb);

  /* Get deprecated procedures */
  deprecated_procs = gimp_pdb_get_deprecated_procedures (gimp->pdb);

  for (iter = deprecated_procs; iter; iter = g_list_next (iter))
    {
      GimpProcedure *procedure = GIMP_PROCEDURE (iter->data);

      g_print ("%s\n", procedure->original_name);
    }

  g_list_free (deprecated_procs);

  g_object_unref (gimp);

  app_exit (EXIT_SUCCESS);

  return FALSE;
}

static void
gimp_show_version_and_exit (void)
{
  gimp_open_console_window ();
  gimp_version_show (be_verbose);

  app_exit (EXIT_SUCCESS);
}

static void
gimp_show_license_and_exit (void)
{
  gimp_open_console_window ();
  gimp_version_show (be_verbose);

  g_print ("\n");
  g_print (GIMP_LICENSE);
  g_print ("\n\n");

  app_exit (EXIT_SUCCESS);
}

static void
gimp_init_malloc (void)
{
#ifdef GIMP_GLIB_MEM_PROFILER
  g_mem_set_vtable (glib_mem_profiler_table);
  g_atexit (g_mem_profile);
#endif

#ifdef __GLIBC__
  /* Tweak memory allocation so that memory allocated in chunks >= 4k
   * (64x64 pixel 1bpp tile) gets returned to the system when free()'d.
   *
   * The default value for M_MMAP_THRESHOLD in glibc-2.3 is 128k.
   * This is said to be an empirically derived value that works well
   * in most systems. Lowering it to 4k is thus probably not the ideal
   * solution.
   *
   * An alternative to tuning this parameter would be to use
   * malloc_trim(), for example after releasing a large tile-manager.
   */
#if 0
  mallopt (M_MMAP_THRESHOLD, TILE_WIDTH * TILE_HEIGHT);
#endif
#endif
}

static void
gimp_init_i18n (void)
{
  /*  We may change the locale later if the user specifies a language
   *  in the gimprc file. Here we are just initializing the locale
   *  according to the environment variables and set up the paths to
   *  the message catalogs.
   */

  setlocale (LC_ALL, "");

  bindtextdomain (GETTEXT_PACKAGE"-libgimp", gimp_locale_directory ());
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE"-libgimp", "UTF-8");
#endif

  bindtextdomain (GETTEXT_PACKAGE, gimp_locale_directory ());
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

  textdomain (GETTEXT_PACKAGE);
}
