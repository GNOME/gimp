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

#endif /* G_OS_WIN32 */

#if defined(ENABLE_RELOCATABLE_RESOURCES) && defined(__APPLE__)
#include <sys/param.h> /* PATH_MAX */
#include <libgen.h> /* dirname */
#include <sys/stat.h>
#endif /* __APPLE__ */

#ifndef GIMP_CONSOLE_COMPILATION
#include <gdk/gdk.h>
#else
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

#include <babl/babl.h>

#include "libgimpbase/gimpbase.h"

#include "pdb/pdb-types.h"

#include "config/gimpconfig-dump.h"

#include "core/gimp.h"
#include "core/gimpbacktrace.h"

#include "pdb/gimppdb.h"
#include "pdb/gimpprocedure.h"
#include "pdb/internal-procs.h"

#include "about.h"
#include "app.h"
#include "sanity.h"
#include "signals.h"
#include "unique.h"

#ifdef G_OS_WIN32
/* To get PROCESS_DEP_* defined we need _WIN32_WINNT at 0x0601. We still
 * use the API optionally only if present, though.
 */
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <conio.h>
#endif

#include "gimp-log.h"
#include "gimp-intl.h"
#include "gimp-version.h"


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
static gboolean            show_debug_menu   = TRUE;
static GimpStackTraceMode  stack_trace_mode  = GIMP_STACK_TRACE_QUERY;
static GimpPDBCompatMode   pdb_compat_mode   = GIMP_PDB_COMPAT_WARN;
#else
static gboolean            show_playground   = FALSE;
static gboolean            show_debug_menu   = FALSE;
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
    N_("Do not use shared memory between GIMP and plug-ins"), NULL
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
    "show-playground", 0, 0,
    G_OPTION_ARG_NONE, &show_playground,
    N_("Show a preferences page with experimental features"), NULL
  },
  {
    "show-debug-menu", 0, G_OPTION_FLAG_HIDDEN,
    G_OPTION_ARG_NONE, &show_debug_menu,
    N_("Show an image submenu with debug actions"), NULL
  },
  {
    G_OPTION_REMAINING, 0, 0,
    G_OPTION_ARG_FILENAME_ARRAY, &filenames,
    NULL, NULL
  },
  { NULL }
};

#if defined(ENABLE_RELOCATABLE_RESOURCES) && defined(__APPLE__)
static void
gimp_macos_setenv (const char * progname)
{
  /* helper to set environment variables for GIMP to be relocatable.
   * Due to the latest changes it is not recommended to set it in the shell
   * wrapper anymore.
   */
  gchar resolved_path[PATH_MAX];
  /* on some OSX installations open file limit is 256 and GIMP needs more */
  struct rlimit limit;
  limit.rlim_cur = 10000;
  limit.rlim_max = 10000;
  setrlimit (RLIMIT_NOFILE, &limit);
  if (realpath (progname, resolved_path) && !g_getenv ("GIMP_NO_WRAPPER"))
    {
      /* set path to the app folder to make sure that our python is called
       * instead of system one
       */
      static gboolean            show_playground   = TRUE;

      gchar *path;
      gchar  tmp[PATH_MAX];
      gchar *app_dir;
      gchar  res_dir[PATH_MAX];
      size_t path_len;
      struct stat sb;

      app_dir = g_path_get_dirname (resolved_path);
      g_snprintf (tmp, sizeof(tmp), "%s/../Resources", app_dir);
      if (realpath (tmp, res_dir) && !stat (res_dir,&sb) && S_ISDIR (sb.st_mode))
        g_print ("GIMP is started as MacOS application\n");
      else
        return;

      path_len = strlen (g_getenv ("PATH") ? g_getenv ("PATH") : "") + strlen (app_dir) + 2;
      path = g_try_malloc (path_len);
      if (path == NULL)
        {
          g_warning ("Failed to allocate memory");
          app_exit (EXIT_FAILURE);
        }
      if (g_getenv ("PATH"))
        g_snprintf (path, path_len, "%s:%s", app_dir, g_getenv ("PATH"));
      else
        g_snprintf (path, path_len, "%s", app_dir);
      g_free (app_dir);
      g_setenv ("PATH", path, TRUE);
      g_free (path);
      g_snprintf (tmp, sizeof(tmp), "%s/lib/gtk-3.0/3.0.0", res_dir);
      g_setenv ("GTK_PATH", tmp, TRUE);
      g_snprintf (tmp, sizeof(tmp), "%s/etc/gtk-3.0/gtk.immodules", res_dir);
      g_setenv ("GTK_IM_MODULE_FILE", tmp, TRUE);
      g_snprintf (tmp, sizeof(tmp), "%s/lib/gegl-0.4", res_dir);
      g_setenv ("GEGL_PATH", tmp, TRUE);
      g_snprintf (tmp, sizeof(tmp), "%s/lib/babl-0.1", res_dir);
      g_setenv ("BABL_PATH", tmp, TRUE);
      g_snprintf (tmp, sizeof(tmp), "%s/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache", res_dir);
      g_setenv ("GDK_PIXBUF_MODULE_FILE", tmp, TRUE);
      g_snprintf (tmp, sizeof(tmp), "%s/etc/fonts", res_dir);
      g_setenv ("FONTCONFIG_PATH", tmp, TRUE);
      g_snprintf (tmp, sizeof(tmp), "%s", res_dir);
      g_setenv ("PYTHONHOME", tmp, TRUE);
      g_snprintf (tmp, sizeof(tmp), "%s/lib/python2.7:%s/lib/gimp/2.0/python", res_dir, res_dir);
      g_setenv ("PYTHONPATH", tmp, TRUE);
      g_snprintf (tmp, sizeof(tmp), "%s/lib/gio/modules", res_dir);
      g_setenv ("GIO_MODULE_DIR", tmp, TRUE);
      g_snprintf (tmp, sizeof(tmp), "%s/share/libwmf/fonts", res_dir);
      g_setenv ("WMF_FONTDIR", tmp, TRUE);
      if (g_getenv ("HOME")!=NULL)
        {
          g_snprintf (tmp, sizeof(tmp),
                      "%s/Library/Application Support/GIMP/3.00/cache",
                      g_getenv("HOME"));
          g_setenv ("XDG_CACHE_HOME", tmp, TRUE);
        }
    }
}
#endif

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
  gchar          *backtrace_file     = NULL;
  gint            i;

#ifdef ENABLE_WIN32_DEBUG_CONSOLE
  gimp_open_console_window ();
#endif
#if defined(ENABLE_RELOCATABLE_RESOURCES) && defined(__APPLE__)
  /* remove MacOS session identifier from the command line args */
  gint newargc = 0;
  for (gint i = 0; i < argc; i++)
    {
      if (!g_str_has_prefix (argv[i], "-psn_"))
        {
          argv[newargc] = argv[i];
          newargc++;
        }
    }
  if (argc > newargc)
    {
      argv[newargc] = NULL; /* glib expects NULL terminated array */
      argc = newargc;
    }

  gimp_macos_setenv (argv[0]);
#endif

#if defined (__GNUC__) && defined (_WIN64)
  /* mingw-w64, at least the unstable build from late July 2008,
   * starts subsystem:windows programs in main(), but passes them
   * bogus argc and argv. __argc and __argv are OK, though, so just
   * use them.
   */
  argc = __argc;
  argv = __argv;
#endif

  /* Initialize GimpBacktrace early on.  In particular, we want the
   * Windows backend to catch the SET_THREAD_NAME exceptions of newly
   * created threads.
   */
  gimp_backtrace_init ();

  /* Start signal handlers early. */
  gimp_init_signal_handlers (&backtrace_file);

#ifdef G_OS_WIN32
  /* Reduce risks */
  {
    typedef BOOL (WINAPI *t_SetDllDirectoryA) (LPCSTR lpPathName);
    t_SetDllDirectoryA p_SetDllDirectoryA;

    p_SetDllDirectoryA =
      (t_SetDllDirectoryA) GetProcAddress (GetModuleHandle ("kernel32.dll"),
                                           "SetDllDirectoryA");
    if (p_SetDllDirectoryA)
      (*p_SetDllDirectoryA) ("");
  }

  /* On Windows, set DLL search path to $INSTALLDIR/bin so that .exe
     plug-ins in the plug-ins directory can find libgimp and file
     library DLLs without needing to set external PATH. */
  {
    const gchar *install_dir;
    gchar       *bin_dir;
    LPWSTR       w_bin_dir;
    int          n;

    w_bin_dir = NULL;
    install_dir = gimp_installation_directory ();
    bin_dir = g_build_filename (install_dir, "bin", NULL);

    n = MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS,
                             bin_dir, -1, NULL, 0);
    if (n == 0)
      goto out;

    w_bin_dir = g_malloc_n (n + 1, sizeof (wchar_t));
    n = MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS,
                             bin_dir, -1,
                             w_bin_dir, (n + 1) * sizeof (wchar_t));
    if (n == 0)
      goto out;

    SetDllDirectoryW (w_bin_dir);

  out:
    if (w_bin_dir)
      g_free (w_bin_dir);
    g_free (bin_dir);
  }

#ifndef _WIN64
  {
    typedef BOOL (WINAPI *t_SetProcessDEPPolicy) (DWORD dwFlags);
    t_SetProcessDEPPolicy p_SetProcessDEPPolicy;

    p_SetProcessDEPPolicy =
      (t_SetProcessDEPPolicy) GetProcAddress (GetModuleHandle ("kernel32.dll"),
                                              "SetProcessDEPPolicy");
    if (p_SetProcessDEPPolicy)
      (*p_SetProcessDEPPolicy) (PROCESS_DEP_ENABLE|PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION);
  }
#endif

  /* Group all our windows together on the taskbar */
  {
    typedef HRESULT (WINAPI *t_SetCurrentProcessExplicitAppUserModelID) (PCWSTR lpPathName);
    t_SetCurrentProcessExplicitAppUserModelID p_SetCurrentProcessExplicitAppUserModelID;

    p_SetCurrentProcessExplicitAppUserModelID =
      (t_SetCurrentProcessExplicitAppUserModelID) GetProcAddress (GetModuleHandle ("shell32.dll"),
                                                                  "SetCurrentProcessExplicitAppUserModelID");
    if (p_SetCurrentProcessExplicitAppUserModelID)
      (*p_SetCurrentProcessExplicitAppUserModelID) (L"gimp.GimpApplication");
  }
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

      if (batch_commands)
        gimp_unique_batch_run (batch_interpreter, batch_commands);

      gdk_notify_startup_complete ();

      return EXIT_SUCCESS;
    }
#endif

  abort_message = sanity_check_early ();
  if (abort_message)
    app_abort (no_interface, abort_message);

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
           show_debug_menu,
           stack_trace_mode,
           pdb_compat_mode,
           backtrace_file);

  if (backtrace_file)
    g_free (backtrace_file);

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

      babl_init ();
      gimp = g_object_new (GIMP_TYPE_GIMP, NULL);
      gimp_load_config (gimp, NULL, NULL);

      success = gimp_config_dump (G_OBJECT (gimp), format);

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

  babl_init ();
  gimp = g_object_new (GIMP_TYPE_GIMP, NULL);
  gimp_load_config (gimp, NULL, NULL);

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

      g_print ("%s\n", gimp_object_get_name (procedure));
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
