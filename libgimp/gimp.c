/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimp.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#define _GNU_SOURCE  /* for the sigaction stuff */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef WAIT_ANY
#define WAIT_ANY -1
#endif

#include <gtk/gtk.h> /* need GDK_WINDOWING_FOO defines */

#ifndef G_OS_WIN32
#include "libgimpbase/gimpsignal.h"

#else

#ifdef HAVE_EXCHNDL
#include <time.h>
#include <exchndl.h>
#endif

#include <signal.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#if defined(USE_SYSV_SHM)

#ifdef HAVE_IPC_H
#include <sys/ipc.h>
#endif

#ifdef HAVE_SHM_H
#include <sys/shm.h>
#endif

#elif defined(USE_POSIX_SHM)

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <fcntl.h>
#include <sys/mman.h>

#endif /* USE_POSIX_SHM */

#ifdef GDK_WINDOWING_QUARTZ
#include <Cocoa/Cocoa.h>
#endif

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)
#  ifdef STRICT
#  undef STRICT
#  endif
#  define STRICT

#  ifdef _WIN32_WINNT
#  undef _WIN32_WINNT
#  endif
#  define _WIN32_WINNT 0x0601

#  include <windows.h>
#  include <tlhelp32.h>
#  undef RGB
#  define USE_WIN32_SHM 1
#endif

#include <locale.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpbase-private.h"
#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "gimp.h"
#include "gimpunitcache.h"

#include "libgimp-intl.h"


/**
 * SECTION: gimp
 * @title: Gimp
 * @short_description: Main functions needed for building a GIMP plug-in.
 *                     This header includes all other GIMP Library headers.
 *
 * Main functions needed for building a GIMP plug-in. This header
 * includes all other GIMP Library headers.
 **/


#define TILE_MAP_SIZE (_tile_width * _tile_height * 32)

#define ERRMSG_SHM_FAILED "Could not attach to gimp shared memory segment"

/* Maybe this should go in a public header if we add other things to it */
typedef enum
{
  GIMP_DEBUG_PID            = 1 << 0,
  GIMP_DEBUG_FATAL_WARNINGS = 1 << 1,
  GIMP_DEBUG_QUERY          = 1 << 2,
  GIMP_DEBUG_INIT           = 1 << 3,
  GIMP_DEBUG_RUN            = 1 << 4,
  GIMP_DEBUG_QUIT           = 1 << 5,

  GIMP_DEBUG_DEFAULT        = (GIMP_DEBUG_RUN | GIMP_DEBUG_FATAL_WARNINGS)
} GimpDebugFlag;

#define WRITE_BUFFER_SIZE  1024

void gimp_read_expect_msg   (GimpWireMessage *msg,
                             gint             type);


static void       gimp_close                   (void);
static void       gimp_debug_stop              (void);
static void       gimp_message_func            (const gchar    *log_domain,
                                                GLogLevelFlags  log_level,
                                                const gchar    *message,
                                                gpointer        data);
static void       gimp_fatal_func              (const gchar    *log_domain,
                                                GLogLevelFlags  flags,
                                                const gchar    *message,
                                                gpointer        data);
#ifdef G_OS_WIN32
#ifdef HAVE_EXCHNDL
static LONG WINAPI gimp_plugin_sigfatal_handler (PEXCEPTION_POINTERS pExceptionInfo);
#endif
#else
static void       gimp_plugin_sigfatal_handler (gint            sig_num);
#endif
static gboolean   gimp_plugin_io_error_handler (GIOChannel      *channel,
                                                GIOCondition     cond,
                                                gpointer         data);
static gboolean   gimp_write                   (GIOChannel      *channel,
                                                const guint8    *buf,
                                                gulong           count,
                                                gpointer         user_data);
static gboolean   gimp_flush                   (GIOChannel      *channel,
                                                gpointer         user_data);
static void       gimp_loop                    (void);
static void       gimp_config                  (GPConfig        *config);
static void       gimp_proc_run                (GPProcRun       *proc_run);
static void       gimp_temp_proc_run           (GPProcRun       *proc_run);
static void       gimp_process_message         (GimpWireMessage *msg);
static void       gimp_single_message          (void);
static gboolean   gimp_extension_read          (GIOChannel      *channel,
                                                GIOCondition     condition,
                                                gpointer         data);

static void       gimp_set_pdb_error           (const GimpParam *return_vals,
                                                gint             n_return_vals);


#if defined G_OS_WIN32 && defined HAVE_EXCHNDL
static LPTOP_LEVEL_EXCEPTION_FILTER  _prevExceptionFilter    = NULL;
static gchar                         *plug_in_backtrace_path = NULL;
#endif

static GIOChannel                   *_readchannel            = NULL;
GIOChannel                          *_writechannel           = NULL;

#ifdef USE_WIN32_SHM
static HANDLE shm_handle;
#endif

static gint           _tile_width        = -1;
static gint           _tile_height       = -1;
static gint           _shm_ID            = -1;
static guchar        *_shm_addr          = NULL;
static const gdouble  _gamma_val         = 2.2;
static gboolean       _install_cmap      = FALSE;
static gboolean       _show_tool_tips    = TRUE;
static gboolean       _show_help_button  = TRUE;
static gboolean       _export_exif       = FALSE;
static gboolean       _export_xmp        = FALSE;
static gboolean       _export_iptc       = FALSE;
static GimpCheckSize  _check_size        = GIMP_CHECK_SIZE_MEDIUM_CHECKS;
static GimpCheckType  _check_type        = GIMP_CHECK_TYPE_GRAY_CHECKS;
static gint           _min_colors        = 144;
static gint           _gdisp_ID          = -1;
static gchar         *_wm_class          = NULL;
static gchar         *_display_name      = NULL;
static gint           _monitor_number    = 0;
static guint32        _timestamp         = 0;
static const gchar   *progname           = NULL;

static gchar          write_buffer[WRITE_BUFFER_SIZE];
static gulong         write_buffer_index = 0;

static GimpStackTraceMode stack_trace_mode = GIMP_STACK_TRACE_NEVER;

static GHashTable    *temp_proc_ht       = NULL;

static guint          gimp_debug_flags   = 0;

static const GDebugKey gimp_debug_keys[] =
{
  { "pid",            GIMP_DEBUG_PID            },
  { "fatal-warnings", GIMP_DEBUG_FATAL_WARNINGS },
  { "fw",             GIMP_DEBUG_FATAL_WARNINGS },
  { "query",          GIMP_DEBUG_QUERY          },
  { "init",           GIMP_DEBUG_INIT           },
  { "run",            GIMP_DEBUG_RUN            },
  { "quit",           GIMP_DEBUG_QUIT           },
  { "on",             GIMP_DEBUG_DEFAULT        }
};

static GimpPlugInInfo PLUG_IN_INFO;


static GimpPDBStatusType  pdb_error_status   = GIMP_PDB_SUCCESS;
static gchar             *pdb_error_message  = NULL;


/**
 * gimp_main:
 * @info: the PLUG_IN_INFO structure
 * @argc: the number of arguments
 * @argv: the arguments
 *
 * The main procedure that must be called with the PLUG_IN_INFO structure
 * and the 'argc' and 'argv' that are passed to "main".
 *
 * Returns: an exit status as defined by the C library,
 *          on success %EXIT_SUCCESS.
 **/
gint
gimp_main (const GimpPlugInInfo *info,
           gint                  argc,
           gchar                *argv[])
{
  enum
  {
    ARG_PROGNAME,
    ARG_GIMP,
    ARG_READ_FD,
    ARG_WRITE_FD,
    ARG_MODE,
    ARG_STACK_TRACE_MODE,

    N_ARGS
  };

  gchar       *basename;
  const gchar *env_string;
  gchar       *debug_string;

#ifdef G_OS_WIN32
  gint i, j, k;

  /* Reduce risks */
  {
    typedef BOOL (WINAPI *t_SetDllDirectoryA) (LPCSTR lpPathName);
    t_SetDllDirectoryA p_SetDllDirectoryA;

    p_SetDllDirectoryA = (t_SetDllDirectoryA) GetProcAddress (GetModuleHandle ("kernel32.dll"),
                                                              "SetDllDirectoryA");
    if (p_SetDllDirectoryA)
      (*p_SetDllDirectoryA) ("");
  }

  /* On Windows, set DLL search path to $INSTALLDIR/bin so that GEGL
     file operations can find their respective file library DLLs (such
     as jasper, etc.) without needing to set external PATH. */
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

#ifdef HAVE_EXCHNDL
  /* Use Dr. Mingw (dumps backtrace on crash) if it is available. */
  {
    time_t  t;
    gchar  *filename;
    gchar  *dir;

    /* This has to be the non-roaming directory (i.e., the local
       directory) as backtraces correspond to the binaries on this
       system. */
    dir = g_build_filename (g_get_user_data_dir (),
                            GIMPDIR, GIMP_USER_VERSION, "CrashLog",
                            NULL);
    /* Ensure the path exists. */
    g_mkdir_with_parents (dir, 0700);

    time (&t);
    filename = g_strdup_printf ("%s-crash-%" G_GUINT64_FORMAT ".txt",
                                g_get_prgname(), t);
    plug_in_backtrace_path = g_build_filename (dir, filename, NULL);
    g_free (filename);
    g_free (dir);

    /* Similar to core crash handling in app/signals.c, the order here
     * is very important!
     */
    if (! _prevExceptionFilter)
      _prevExceptionFilter = SetUnhandledExceptionFilter (gimp_plugin_sigfatal_handler);

    ExcHndlInit ();
    ExcHndlSetLogFileNameA (plug_in_backtrace_path);
  }
#endif

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

  /* Group all our windows together on the taskbar */
  {
    typedef HRESULT (WINAPI *t_SetCurrentProcessExplicitAppUserModelID) (PCWSTR lpPathName);
    t_SetCurrentProcessExplicitAppUserModelID p_SetCurrentProcessExplicitAppUserModelID;

    p_SetCurrentProcessExplicitAppUserModelID = (t_SetCurrentProcessExplicitAppUserModelID) GetProcAddress (GetModuleHandle ("shell32.dll"),
                                                                                                            "SetCurrentProcessExplicitAppUserModelID");
    if (p_SetCurrentProcessExplicitAppUserModelID)
      (*p_SetCurrentProcessExplicitAppUserModelID) (L"gimp.GimpApplication");
  }

  /* Check for exe file name with spaces in the path having been split up
   * by buggy NT C runtime, or something. I don't know why this happens
   * on NT (including w2k), but not on w95/98.
   */

  for (i = 1; i < argc; i++)
    {
      k = strlen (argv[i]);

      if (k > 10)
        {
          if (g_ascii_strcasecmp (argv[i] + k - 4, ".exe") == 0)
            {
              /* Found the end of the executable name, most probably.
               * Splice the parts of the name back together.
               */
              GString *s;

              s = g_string_new (argv[ARG_PROGNAME]);

              for (j = 1; j <= i; j++)
                {
                  s = g_string_append_c (s, ' ');
                  s = g_string_append (s, argv[j]);
                }

              argv[ARG_PROGNAME] = s->str;

              /* Move rest of argv down */
              for (j = 1; j < argc - i; j++)
                argv[j] = argv[j + i];

              argv[argc - i] = NULL;
              argc -= i;

              break;
          }
       }
    }
#endif

  g_assert (info != NULL);

  PLUG_IN_INFO = *info;

  if ((argc != N_ARGS) || (strcmp (argv[ARG_GIMP], "-gimp") != 0))
    {
      g_printerr ("%s is a GIMP plug-in and must be run by GIMP to be used\n",
                  argv[ARG_PROGNAME]);
      return 1;
    }

  gimp_env_init (TRUE);

  progname = argv[ARG_PROGNAME];

  basename = g_path_get_basename (progname);

  g_set_prgname (basename);

  env_string = g_getenv ("GIMP_PLUGIN_DEBUG");

  if (env_string)
    {
      const gchar *debug_messages;

      debug_string = strchr (env_string, ',');

      if (debug_string)
        {
          gint len = debug_string - env_string;

          if ((strlen (basename) == len) &&
              (strncmp (basename, env_string, len) == 0))
            {
              gimp_debug_flags =
                g_parse_debug_string (debug_string + 1,
                                      gimp_debug_keys,
                                      G_N_ELEMENTS (gimp_debug_keys));
            }
        }
      else if (strcmp (env_string, basename) == 0)
        {
          gimp_debug_flags = GIMP_DEBUG_DEFAULT;
        }

      /*  make debug output visible by setting G_MESSAGES_DEBUG  */
      debug_messages = g_getenv ("G_MESSAGES_DEBUG");

      if (debug_messages)
        {
          gchar *tmp = g_strconcat (debug_messages, ",LibGimp", NULL);
          g_setenv ("G_MESSAGES_DEBUG", tmp, TRUE);
          g_free (tmp);
        }
      else
        {
          g_setenv ("G_MESSAGES_DEBUG", "LibGimp", TRUE);
        }
    }

  g_free (basename);

  stack_trace_mode = (GimpStackTraceMode) CLAMP (atoi (argv[ARG_STACK_TRACE_MODE]),
                                                 GIMP_STACK_TRACE_NEVER,
                                                 GIMP_STACK_TRACE_ALWAYS);

#ifndef G_OS_WIN32
  /* No use catching these on Win32, the user won't get any meaningful
   * stack trace from glib anyhow. It's better to let Windows inform
   * about the program error, and offer debugging if the plug-in
   * has been built with MSVC, and the user has MSVC installed.
   */
  gimp_signal_private (SIGHUP,  gimp_plugin_sigfatal_handler, 0);
  gimp_signal_private (SIGINT,  gimp_plugin_sigfatal_handler, 0);
  gimp_signal_private (SIGQUIT, gimp_plugin_sigfatal_handler, 0);
  gimp_signal_private (SIGTERM, gimp_plugin_sigfatal_handler, 0);

  gimp_signal_private (SIGABRT, gimp_plugin_sigfatal_handler, 0);
  gimp_signal_private (SIGBUS,  gimp_plugin_sigfatal_handler, 0);
  gimp_signal_private (SIGSEGV, gimp_plugin_sigfatal_handler, 0);
  gimp_signal_private (SIGFPE,  gimp_plugin_sigfatal_handler, 0);

  /* Ignore SIGPIPE from crashing Gimp */
  gimp_signal_private (SIGPIPE, SIG_IGN, 0);

  /* Restart syscalls interrupted by SIGCHLD */
  gimp_signal_private (SIGCHLD, SIG_DFL, SA_RESTART);
#endif

#ifdef G_OS_WIN32
  _readchannel  = g_io_channel_win32_new_fd (atoi (argv[ARG_READ_FD]));
  _writechannel = g_io_channel_win32_new_fd (atoi (argv[ARG_WRITE_FD]));
#else
  _readchannel  = g_io_channel_unix_new (atoi (argv[ARG_READ_FD]));
  _writechannel = g_io_channel_unix_new (atoi (argv[ARG_WRITE_FD]));
#endif

  g_io_channel_set_encoding (_readchannel, NULL, NULL);
  g_io_channel_set_encoding (_writechannel, NULL, NULL);

  g_io_channel_set_buffered (_readchannel, FALSE);
  g_io_channel_set_buffered (_writechannel, FALSE);

  g_io_channel_set_close_on_unref (_readchannel, TRUE);
  g_io_channel_set_close_on_unref (_writechannel, TRUE);

  gp_init ();

  gimp_wire_set_writer (gimp_write);
  gimp_wire_set_flusher (gimp_flush);

  gimp_enums_init ();

  /*  initialize units  */
  {
    GimpUnitVtable vtable;

    vtable.unit_get_number_of_units = _gimp_unit_cache_get_number_of_units;
    vtable.unit_get_number_of_built_in_units =
      _gimp_unit_cache_get_number_of_built_in_units;
    vtable.unit_new                 = _gimp_unit_cache_new;
    vtable.unit_get_deletion_flag   = _gimp_unit_cache_get_deletion_flag;
    vtable.unit_set_deletion_flag   = _gimp_unit_cache_set_deletion_flag;
    vtable.unit_get_factor          = _gimp_unit_cache_get_factor;
    vtable.unit_get_digits          = _gimp_unit_cache_get_digits;
    vtable.unit_get_identifier      = _gimp_unit_cache_get_identifier;
    vtable.unit_get_symbol          = _gimp_unit_cache_get_symbol;
    vtable.unit_get_abbreviation    = _gimp_unit_cache_get_abbreviation;
    vtable.unit_get_singular        = _gimp_unit_cache_get_singular;
    vtable.unit_get_plural          = _gimp_unit_cache_get_plural;

    gimp_base_init (&vtable);
  }

  /* initialize i18n support */

  setlocale (LC_ALL, "");

  bindtextdomain (GETTEXT_PACKAGE"-libgimp", gimp_locale_directory ());
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE"-libgimp", "UTF-8");
#endif


  /* set handler both for the "LibGimp" and "" domains */
  {
    const gchar * const log_domains[] =
    {
      "LibGimp",
      "LibGimpBase",
      "LibGimpColor",
      "LibGimpConfig",
      "LibGimpMath",
      "LibGimpModule",
      "LibGimpThumb",
      "LibGimpWidgets"
    };
    gint i;

    for (i = 0; i < G_N_ELEMENTS (log_domains); i++)
      g_log_set_handler (log_domains[i],
                         G_LOG_LEVEL_MESSAGE,
                         gimp_message_func,
                         NULL);

    g_log_set_handler (NULL,
                       G_LOG_LEVEL_MESSAGE,
                       gimp_message_func,
                       NULL);
  }

  if (gimp_debug_flags & GIMP_DEBUG_FATAL_WARNINGS)
    {
      GLogLevelFlags fatal_mask;

      fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
      fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
      g_log_set_always_fatal (fatal_mask);

      g_log_set_handler (NULL,
                         G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL |
                         G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL,
                         gimp_fatal_func, NULL);
    }
  else
    {
      g_log_set_handler (NULL,
                         G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL,
                         gimp_fatal_func, NULL);
    }

  if (strcmp (argv[ARG_MODE], "-query") == 0)
    {
      if (PLUG_IN_INFO.init_proc)
        gp_has_init_write (_writechannel, NULL);

      if (gimp_debug_flags & GIMP_DEBUG_QUERY)
        gimp_debug_stop ();

      if (PLUG_IN_INFO.query_proc)
        (* PLUG_IN_INFO.query_proc) ();

      gimp_close ();

      return EXIT_SUCCESS;
    }

  if (strcmp (argv[ARG_MODE], "-init") == 0)
    {
      if (gimp_debug_flags & GIMP_DEBUG_INIT)
        gimp_debug_stop ();

      if (PLUG_IN_INFO.init_proc)
        (* PLUG_IN_INFO.init_proc) ();

      gimp_close ();

      return EXIT_SUCCESS;
    }

  if (gimp_debug_flags & GIMP_DEBUG_RUN)
    gimp_debug_stop ();
  else if (gimp_debug_flags & GIMP_DEBUG_PID)
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Here I am!");

  temp_proc_ht = g_hash_table_new (g_str_hash, g_str_equal);

  g_io_add_watch (_readchannel,
                  G_IO_ERR | G_IO_HUP,
                  gimp_plugin_io_error_handler,
                  NULL);

  gimp_loop ();

  return EXIT_SUCCESS;
}

/**
 * gimp_quit:
 *
 * Forcefully causes the GIMP library to exit and close down its
 * connection to main gimp application. This function never returns.
 **/
void
gimp_quit (void)
{
  gimp_close ();

#if defined G_OS_WIN32 && defined HAVE_EXCHNDL
  if (plug_in_backtrace_path)
    g_free (plug_in_backtrace_path);
#endif

  exit (EXIT_SUCCESS);
}

/**
 * gimp_install_procedure:
 * @name:          the procedure's name.
 * @blurb:         a short text describing what the procedure does.
 * @help:          the help text for the procedure (usually considerably
 *                 longer than @blurb).
 * @author:        the procedure's author(s).
 * @copyright:     the procedure's copyright.
 * @date:          the date the procedure was added.
 * @menu_label:    the label to use for the procedure's menu entry,
 *                 or #NULL if the procedure has no menu entry.
 * @image_types:   the drawable types the procedure can handle.
 * @type:          the type of the procedure.
 * @n_params:      the number of parameters the procedure takes.
 * @n_return_vals: the number of return values the procedure returns.
 * @params:        the procedure's parameters.
 * @return_vals:   the procedure's return values.
 *
 * Installs a new procedure with the PDB (procedural database).
 *
 * Call this function from within your plug-in's query() function for
 * each procedure your plug-in implements.
 *
 * The @name parameter is mandatory and should be unique, or it will
 * overwrite an already existing procedure (overwrite procedures only
 * if you know what you're doing).
 *
 * The @blurb, @help, @author, @copyright and @date parameters are
 * optional but then you shouldn't write procedures without proper
 * documentation, should you.
 *
 * @menu_label defines the label that should be used for the
 * procedure's menu entry. The position where to register in the menu
 * hierarchy is chosen using gimp_plugin_menu_register().  This
 * function also still accepts the old (pre-2.2) way of registering a
 * menu entry and takes a string in the form
 * "&lt;Domain&gt;/Path/To/My/Menu"
 * (e.g. "&lt;Image&gt;/Filters/Render/Useless").
 *
 * It is possible to register a procedure only for keyboard-shortcut
 * activation by passing a @menu_label to gimp_install_procedure() but
 * not registering any menu path with gimp_plugin_menu_register(). In
 * this case, the given @menu_label will only be used as the
 * procedure's user-visible name in the keyboard shortcut editor.
 *
 * @image_types is a comma separated list of image types, or actually
 * drawable types, that this procedure can deal with. Wildcards are
 * possible here, so you could say "RGB*" instead of "RGB, RGBA" or
 * "*" for all image types. If the procedure doesn't need an image to
 * run, use the empty string.
 *
 * @type must be one of %GIMP_PLUGIN or %GIMP_EXTENSION. Note that
 * temporary procedures must be installed using
 * gimp_install_temp_proc().
 *
 * NOTE: Unlike the GIMP 1.2 API, %GIMP_EXTENSION no longer means
 * that the procedure's menu prefix is &lt;Toolbox&gt;, but that
 * it will install temporary procedures. Therefore, the GIMP core
 * will wait until the %GIMP_EXTENSION procedure has called
 * gimp_extension_ack(), which means that the procedure has done
 * its initialization, installed its temporary procedures and is
 * ready to run.
 *
 * <emphasis>Not calling gimp_extension_ack() from a %GIMP_EXTENSION
 * procedure will cause the GIMP core to lock up.</emphasis>
 *
 * Additionally, a %GIMP_EXTENSION procedure with no parameters
 * (@n_params == 0 and @params == #NULL) is an "automatic" extension
 * that will be automatically started on each GIMP startup.
 **/
void
gimp_install_procedure (const gchar        *name,
                        const gchar        *blurb,
                        const gchar        *help,
                        const gchar        *author,
                        const gchar        *copyright,
                        const gchar        *date,
                        const gchar        *menu_label,
                        const gchar        *image_types,
                        GimpPDBProcType     type,
                        gint                n_params,
                        gint                n_return_vals,
                        const GimpParamDef *params,
                        const GimpParamDef *return_vals)
{
  GPProcInstall proc_install;

  g_return_if_fail (name != NULL);
  g_return_if_fail (type != GIMP_INTERNAL);
  g_return_if_fail ((n_params == 0 && params == NULL) ||
                    (n_params > 0  && params != NULL));
  g_return_if_fail ((n_return_vals == 0 && return_vals == NULL) ||
                    (n_return_vals > 0  && return_vals != NULL));

  proc_install.name         = (gchar *) name;
  proc_install.blurb        = (gchar *) blurb;
  proc_install.help         = (gchar *) help;
  proc_install.author       = (gchar *) author;
  proc_install.copyright    = (gchar *) copyright;
  proc_install.date         = (gchar *) date;
  proc_install.menu_path    = (gchar *) menu_label;
  proc_install.image_types  = (gchar *) image_types;
  proc_install.type         = type;
  proc_install.nparams      = n_params;
  proc_install.nreturn_vals = n_return_vals;
  proc_install.params       = (GPParamDef *) params;
  proc_install.return_vals  = (GPParamDef *) return_vals;

  if (! gp_proc_install_write (_writechannel, &proc_install, NULL))
    gimp_quit ();
}

/**
 * gimp_install_temp_proc:
 * @name:          the procedure's name.
 * @blurb:         a short text describing what the procedure does.
 * @help:          the help text for the procedure (usually considerably
 *                 longer than @blurb).
 * @author:        the procedure's author(s).
 * @copyright:     the procedure's copyright.
 * @date:          the date the procedure was added.
 * @menu_label:    the procedure's menu label, or #NULL if the procedure has
 *                 no menu entry.
 * @image_types:   the drawable types the procedure can handle.
 * @type:          the type of the procedure.
 * @n_params:      the number of parameters the procedure takes.
 * @n_return_vals: the number of return values the procedure returns.
 * @params:        the procedure's parameters.
 * @return_vals:   the procedure's return values.
 * @run_proc:      the function to call for executing the procedure.
 *
 * Installs a new temporary procedure with the PDB (procedural database).
 *
 * A temporary procedure is a procedure which is only available while
 * one of your plug-in's "real" procedures is running.
 *
 * See gimp_install_procedure() for most details.
 *
 * @type <emphasis>must</emphasis> be %GIMP_TEMPORARY or the function
 * will fail.
 *
 * @run_proc is the function which will be called to execute the
 * procedure.
 *
 * NOTE: Normally, plug-in communication is triggered by the plug-in
 * and the GIMP core only responds to the plug-in's requests. You must
 * explicitly enable receiving of temporary procedure run requests
 * using either gimp_extension_enable() or
 * gimp_extension_process(). See this functions' documentation for
 * details.
 **/
void
gimp_install_temp_proc (const gchar        *name,
                        const gchar        *blurb,
                        const gchar        *help,
                        const gchar        *author,
                        const gchar        *copyright,
                        const gchar        *date,
                        const gchar        *menu_label,
                        const gchar        *image_types,
                        GimpPDBProcType     type,
                        gint                n_params,
                        gint                n_return_vals,
                        const GimpParamDef *params,
                        const GimpParamDef *return_vals,
                        GimpRunProc         run_proc)
{
  g_return_if_fail (name != NULL);
  g_return_if_fail ((n_params == 0 && params == NULL) ||
                    (n_params > 0  && params != NULL));
  g_return_if_fail ((n_return_vals == 0 && return_vals == NULL) ||
                    (n_return_vals > 0  && return_vals != NULL));
  g_return_if_fail (type == GIMP_TEMPORARY);
  g_return_if_fail (run_proc != NULL);

  gimp_install_procedure (name,
                          blurb, help,
                          author, copyright, date,
                          menu_label,
                          image_types,
                          type,
                          n_params, n_return_vals,
                          params, return_vals);

  /*  Insert the temp proc run function into the hash table  */
  g_hash_table_insert (temp_proc_ht, g_strdup (name), (gpointer) run_proc);
}

/**
 * gimp_uninstall_temp_proc:
 * @name: the procedure's name
 *
 * Uninstalls a temporary procedure which has previously been
 * installed using gimp_install_temp_proc().
 **/
void
gimp_uninstall_temp_proc (const gchar *name)
{
  GPProcUninstall proc_uninstall;
  gpointer        hash_name;
  gboolean        found;

  g_return_if_fail (name != NULL);

  proc_uninstall.name = (gchar *) name;

  if (! gp_proc_uninstall_write (_writechannel, &proc_uninstall, NULL))
    gimp_quit ();

  found = g_hash_table_lookup_extended (temp_proc_ht, name, &hash_name, NULL);
  if (found)
    {
      g_hash_table_remove (temp_proc_ht, (gpointer) name);
      g_free (hash_name);
    }
}

/**
 * gimp_run_procedure:
 * @name:          the name of the procedure to run
 * @n_return_vals: return location for the number of return values
 * @...:           list of procedure parameters
 *
 * This function calls a GIMP procedure and returns its return values.
 *
 * The procedure's parameters are given by a va_list in the format
 * (type, value, type, value) and must be terminated by %GIMP_PDB_END.
 *
 * This function converts the va_list of parameters into an array and
 * passes them to gimp_run_procedure2(). Please look there for further
 * information.
 *
 * Return value: the procedure's return values unless there was an error,
 * in which case the zero-th return value will be the error status, and
 * the first return value will be a string detailing the error.
 **/
GimpParam *
gimp_run_procedure (const gchar *name,
                    gint        *n_return_vals,
                    ...)
{
  GimpPDBArgType  param_type;
  GimpParam      *return_vals;
  GimpParam      *params   = NULL;
  gint            n_params = 0;
  va_list         args;
  gint            i;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (n_return_vals != NULL, NULL);

  va_start (args, n_return_vals);
  param_type = va_arg (args, GimpPDBArgType);

  while (param_type != GIMP_PDB_END)
    {
      switch (param_type)
        {
        case GIMP_PDB_INT32:
        case GIMP_PDB_DISPLAY:
        case GIMP_PDB_IMAGE:
        case GIMP_PDB_ITEM:
        case GIMP_PDB_LAYER:
        case GIMP_PDB_CHANNEL:
        case GIMP_PDB_DRAWABLE:
        case GIMP_PDB_SELECTION:
        case GIMP_PDB_VECTORS:
        case GIMP_PDB_STATUS:
          (void) va_arg (args, gint);
          break;
        case GIMP_PDB_INT16:
          (void) va_arg (args, gint);
          break;
        case GIMP_PDB_INT8:
          (void) va_arg (args, gint);
          break;
        case GIMP_PDB_FLOAT:
          (void) va_arg (args, gdouble);
          break;
        case GIMP_PDB_STRING:
          (void) va_arg (args, gchar *);
          break;
        case GIMP_PDB_INT32ARRAY:
          (void) va_arg (args, gint32 *);
          break;
        case GIMP_PDB_INT16ARRAY:
          (void) va_arg (args, gint16 *);
          break;
        case GIMP_PDB_INT8ARRAY:
          (void) va_arg (args, gint8 *);
          break;
        case GIMP_PDB_FLOATARRAY:
          (void) va_arg (args, gdouble *);
          break;
        case GIMP_PDB_STRINGARRAY:
          (void) va_arg (args, gchar **);
          break;
        case GIMP_PDB_COLOR:
        case GIMP_PDB_COLORARRAY:
          (void) va_arg (args, GimpRGB *);
          break;
        case GIMP_PDB_PARASITE:
          (void) va_arg (args, GimpParasite *);
          break;
        case GIMP_PDB_END:
          break;
        }

      n_params++;

      param_type = va_arg (args, GimpPDBArgType);
    }

  va_end (args);

  params = g_new0 (GimpParam, n_params);

  va_start (args, n_return_vals);

  for (i = 0; i < n_params; i++)
    {
      params[i].type = va_arg (args, GimpPDBArgType);

      switch (params[i].type)
        {
        case GIMP_PDB_INT32:
          params[i].data.d_int32 = (gint32) va_arg (args, gint);
          break;
        case GIMP_PDB_INT16:
          params[i].data.d_int16 = (gint16) va_arg (args, gint);
          break;
        case GIMP_PDB_INT8:
          params[i].data.d_int8 = (guint8) va_arg (args, gint);
          break;
        case GIMP_PDB_FLOAT:
          params[i].data.d_float = (gdouble) va_arg (args, gdouble);
          break;
        case GIMP_PDB_STRING:
          params[i].data.d_string = va_arg (args, gchar *);
          break;
        case GIMP_PDB_INT32ARRAY:
          params[i].data.d_int32array = va_arg (args, gint32 *);
          break;
        case GIMP_PDB_INT16ARRAY:
          params[i].data.d_int16array = va_arg (args, gint16 *);
          break;
        case GIMP_PDB_INT8ARRAY:
          params[i].data.d_int8array = va_arg (args, guint8 *);
          break;
        case GIMP_PDB_FLOATARRAY:
          params[i].data.d_floatarray = va_arg (args, gdouble *);
          break;
        case GIMP_PDB_STRINGARRAY:
          params[i].data.d_stringarray = va_arg (args, gchar **);
          break;
        case GIMP_PDB_COLOR:
          params[i].data.d_color = *va_arg (args, GimpRGB *);
          break;
        case GIMP_PDB_ITEM:
          params[i].data.d_item = va_arg (args, gint32);
          break;
        case GIMP_PDB_DISPLAY:
          params[i].data.d_display = va_arg (args, gint32);
          break;
        case GIMP_PDB_IMAGE:
          params[i].data.d_image = va_arg (args, gint32);
          break;
        case GIMP_PDB_LAYER:
          params[i].data.d_layer = va_arg (args, gint32);
          break;
        case GIMP_PDB_CHANNEL:
          params[i].data.d_channel = va_arg (args, gint32);
          break;
        case GIMP_PDB_DRAWABLE:
          params[i].data.d_drawable = va_arg (args, gint32);
          break;
        case GIMP_PDB_SELECTION:
          params[i].data.d_selection = va_arg (args, gint32);
          break;
        case GIMP_PDB_COLORARRAY:
          params[i].data.d_colorarray = va_arg (args, GimpRGB *);
          break;
        case GIMP_PDB_VECTORS:
          params[i].data.d_vectors = va_arg (args, gint32);
          break;
        case GIMP_PDB_PARASITE:
          {
            GimpParasite *parasite = va_arg (args, GimpParasite *);

            if (parasite == NULL)
              {
                params[i].data.d_parasite.name = NULL;
                params[i].data.d_parasite.data = NULL;
              }
            else
              {
                params[i].data.d_parasite.name  = parasite->name;
                params[i].data.d_parasite.flags = parasite->flags;
                params[i].data.d_parasite.size  = parasite->size;
                params[i].data.d_parasite.data  = parasite->data;
              }
          }
          break;
        case GIMP_PDB_STATUS:
          params[i].data.d_status = va_arg (args, gint32);
          break;
        case GIMP_PDB_END:
          break;
        }
    }

  va_end (args);

  return_vals = gimp_run_procedure2 (name, n_return_vals, n_params, params);

  g_free (params);

  return return_vals;
}

void
gimp_read_expect_msg (GimpWireMessage *msg,
                      gint             type)
{
  while (TRUE)
    {
      if (! gimp_wire_read_msg (_readchannel, msg, NULL))
        gimp_quit ();

      if (msg->type == type)
        return; /* up to the caller to call wire_destroy() */

      if (msg->type == GP_TEMP_PROC_RUN || msg->type == GP_QUIT)
        {
          gimp_process_message (msg);
        }
      else
        {
          g_error ("unexpected message: %d", msg->type);
        }

      gimp_wire_destroy (msg);
    }
}

/**
 * gimp_run_procedure2:
 * @name:          the name of the procedure to run
 * @n_return_vals: return location for the number of return values
 * @n_params:      the number of parameters the procedure takes.
 * @params:        the procedure's parameters array.
 *
 * This function calls a GIMP procedure and returns its return values.
 * To get more information about the available procedures and the
 * parameters they expect, please have a look at the Procedure Browser
 * as found in the Xtns menu in GIMP's toolbox.
 *
 * As soon as you don't need the return values any longer, you should
 * free them using gimp_destroy_params().
 *
 * Return value: the procedure's return values unless there was an error,
 * in which case the zero-th return value will be the error status, and
 * if there are two values returned, the other return value will be a
 * string detailing the error.
 **/
GimpParam *
gimp_run_procedure2 (const gchar     *name,
                     gint            *n_return_vals,
                     gint             n_params,
                     const GimpParam *params)
{
  GPProcRun        proc_run;
  GPProcReturn    *proc_return;
  GimpWireMessage  msg;
  GimpParam       *return_vals;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (n_return_vals != NULL, NULL);

  proc_run.name    = (gchar *) name;
  proc_run.nparams = n_params;
  proc_run.params  = (GPParam *) params;

  gp_lock ();
  if (! gp_proc_run_write (_writechannel, &proc_run, NULL))
    gimp_quit ();

  gimp_read_expect_msg (&msg, GP_PROC_RETURN);
  gp_unlock ();

  proc_return = msg.data;

  *n_return_vals = proc_return->nparams;
  return_vals    = (GimpParam *) proc_return->params;

  proc_return->nparams = 0;
  proc_return->params  = NULL;

  gimp_wire_destroy (&msg);

  gimp_set_pdb_error (return_vals, *n_return_vals);

  return return_vals;
}

/**
 * gimp_destroy_params:
 * @params:   the #GimpParam array to destroy
 * @n_params: the number of elements in the array
 *
 * Destroys a #GimpParam array as returned by gimp_run_procedure() or
 * gimp_run_procedure2().
 **/
void
gimp_destroy_params (GimpParam *params,
                     gint       n_params)
{
  gp_params_destroy ((GPParam *) params, n_params);
}

/**
 * gimp_destroy_paramdefs:
 * @paramdefs: the #GimpParamDef array to destroy
 * @n_params:  the number of elements in the array
 *
 * Destroys a #GimpParamDef array as returned by
 * gimp_procedural_db_proc_info().
 **/
void
gimp_destroy_paramdefs (GimpParamDef *paramdefs,
                        gint          n_params)
{
  while (n_params--)
    {
      g_free (paramdefs[n_params].name);
      g_free (paramdefs[n_params].description);
    }

  g_free (paramdefs);
}

/**
 * gimp_get_pdb_error:
 *
 * Retrieves the error message from the last procedure call.
 *
 * If a procedure call fails, then it might pass an error message with
 * the return values. Plug-ins that are using the libgimp C wrappers
 * don't access the procedure return values directly. Thus libgimp
 * stores the error message and makes it available with this
 * function. The next procedure call unsets the error message again.
 *
 * The returned string is owned by libgimp and must not be freed or
 * modified.
 *
 * Return value: the error message
 *
 * Since: 2.6
 **/
const gchar *
gimp_get_pdb_error (void)
{
  if (pdb_error_message && strlen (pdb_error_message))
    return pdb_error_message;

  switch (pdb_error_status)
    {
    case GIMP_PDB_SUCCESS:
      /*  procedure executed successfully  */
      return _("success");

    case GIMP_PDB_EXECUTION_ERROR:
      /*  procedure execution failed       */
      return _("execution error");

    case GIMP_PDB_CALLING_ERROR:
      /*  procedure called incorrectly     */
      return _("calling error");

    case GIMP_PDB_CANCEL:
      /*  procedure execution cancelled    */
      return _("cancelled");

    default:
      return "invalid return status";
    }
}

/**
 * gimp_get_pdb_status:
 *
 * Retrieves the status from the last procedure call.
 *
 * Return value: the #GimpPDBStatusType.
 *
 * Since: 2.10
 **/
GimpPDBStatusType
gimp_get_pdb_status (void)
{
  return pdb_error_status;
}

/**
 * gimp_tile_width:
 *
 * Returns the tile width GIMP is using.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Return value: the tile_width
 **/
guint
gimp_tile_width (void)
{
  return _tile_width;
}

/**
 * gimp_tile_height:
 *
 * Returns the tile height GIMP is using.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Return value: the tile_height
 **/
guint
gimp_tile_height (void)
{
  return _tile_height;
}

/**
 * gimp_shm_ID:
 *
 * Returns the shared memory ID used for passing tile data between the
 * GIMP core and the plug-in.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Return value: the shared memory ID
 **/
gint
gimp_shm_ID (void)
{
  return _shm_ID;
}

/**
 * gimp_shm_addr:
 *
 * Returns the address of the shared memory segment used for passing
 * tile data between the GIMP core and the plug-in.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Return value: the shared memory address
 **/
guchar *
gimp_shm_addr (void)
{
  return _shm_addr;
}

/**
 * gimp_gamma:
 *
 * Returns the global gamma value GIMP and all its plug-ins should
 * use.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * NOTE: This function will always return 2.2, the gamma value for
 * sRGB. There's currently no way to change this and all operations
 * should assume that pixel data is in the sRGB colorspace.
 *
 * Return value: the gamma value
 **/
gdouble
gimp_gamma (void)
{
  return _gamma_val;
}

/**
 * gimp_show_tool_tips:
 *
 * Returns whether or not the plug-in should show tool-tips.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Return value: the show_tool_tips boolean
 **/
gboolean
gimp_show_tool_tips (void)
{
  return _show_tool_tips;
}

/**
 * gimp_show_help_button:
 *
 * Returns whether or not GimpDialog should automatically add a help
 * button if help_func and help_id are given.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Return value: the show_help_button boolean
 *
 * Since: 2.2
 **/
gboolean
gimp_show_help_button (void)
{
  return _show_help_button;
}

/**
 * gimp_export_exif:
 *
 * Returns whether file plug-ins should default to exporting Exif
 * metadata, according preferences (original settings is #FALSE since
 * metadata can contain sensitive information).
 *
 * Return value: TRUE if preferences are set to export Exif.
 *
 * Since: 2.10
 **/
gboolean
gimp_export_exif (void)
{
  return _export_exif;
}

/**
 * gimp_export_xmp:
 *
 * Returns whether file plug-ins should default to exporting XMP
 * metadata, according preferences (original settings is #FALSE since
 * metadata can contain sensitive information).
 *
 * Return value: TRUE if preferences are set to export XMP.
 *
 * Since: 2.10
 **/
gboolean
gimp_export_xmp (void)
{
  return _export_xmp;
}

/**
 * gimp_export_iptc:
 *
 * Returns whether file plug-ins should default to exporting IPTC
 * metadata, according preferences (original settings is #FALSE since
 * metadata can contain sensitive information).
 *
 * Return value: TRUE if preferences are set to export IPTC.
 *
 * Since: 2.10
 **/
gboolean
gimp_export_iptc (void)
{
  return _export_iptc;
}

/**
 * gimp_check_size:
 *
 * Returns the size of the checkerboard to be used in previews.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Return value: the check_size value
 *
 * Since: 2.2
 **/
GimpCheckSize
gimp_check_size (void)
{
  return _check_size;
}

/**
 * gimp_check_type:
 *
 * Returns the type of the checkerboard to be used in previews.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Return value: the check_type value
 *
 * Since: 2.2
 **/
GimpCheckType
gimp_check_type (void)
{
  return _check_type;
}

/**
 * gimp_default_display:
 *
 * Returns the default display ID. This corresponds to the display the
 * running procedure's menu entry was invoked from.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Return value: the default display ID
 **/
gint32
gimp_default_display (void)
{
  return _gdisp_ID;
}

/**
 * gimp_wm_class:
 *
 * Returns the window manager class to be used for plug-in windows.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Return value: the window manager class
 **/
const gchar *
gimp_wm_class (void)
{
  return _wm_class;
}

/**
 * gimp_display_name:
 *
 * Returns the display to be used for plug-in windows.
 *
 * This is a constant value given at plug-in configuration time.
 * Will return #NULL if GIMP has been started with no GUI, either
 * via "--no-interface" flag, or a console build.
 *
 * Return value: the display name
 **/
const gchar *
gimp_display_name (void)
{
  return _display_name;
}

/**
 * gimp_monitor_number:
 *
 * Returns the monitor number to be used for plug-in windows.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Return value: the monitor number
 **/
gint
gimp_monitor_number (void)
{
  return _monitor_number;
}

/**
 * gimp_user_time:
 *
 * Returns the timestamp of the user interaction that should be set on
 * the plug-in window. This is handled transparently, plug-in authors
 * do not have to care about it.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Return value: timestamp for plug-in window
 *
 * Since: 2.6
 **/
guint32
gimp_user_time (void)
{
  return _timestamp;
}

/**
 * gimp_get_progname:
 *
 * Returns the plug-in's executable name.
 *
 * Return value: the executable name
 **/
const gchar *
gimp_get_progname (void)
{
  return progname;
}

/**
 * gimp_extension_ack:
 *
 * Notify the main GIMP application that the extension has been properly
 * initialized and is ready to run.
 *
 * This function <emphasis>must</emphasis> be called from every
 * procedure that was registered as #GIMP_EXTENSION.
 *
 * Subsequently, extensions can process temporary procedure run
 * requests using either gimp_extension_enable() or
 * gimp_extension_process().
 *
 * See also: gimp_install_procedure(), gimp_install_temp_proc()
 **/
void
gimp_extension_ack (void)
{
  if (! gp_extension_ack_write (_writechannel, NULL))
    gimp_quit ();
}

/**
 * gimp_extension_enable:
 *
 * Enables asynchronous processing of messages from the main GIMP
 * application.
 *
 * Normally, a plug-in is not called by GIMP except for the call to
 * the procedure it implements. All subsequent communication is
 * triggered by the plug-in and all messages sent from GIMP to the
 * plug-in are just answers to requests the plug-in made.
 *
 * If the plug-in however registered temporary procedures using
 * gimp_install_temp_proc(), it needs to be able to receive requests
 * to execute them. Usually this will be done by running
 * gimp_extension_process() in an endless loop.
 *
 * If the plug-in cannot use gimp_extension_process(), i.e. if it has
 * a GUI and is hanging around in a #GMainLoop, it must call
 * gimp_extension_enable().
 *
 * Note that the plug-in does not need to be a #GIMP_EXTENSION to
 * register temporary procedures.
 *
 * See also: gimp_install_procedure(), gimp_install_temp_proc()
 **/
void
gimp_extension_enable (void)
{
  static gboolean callback_added = FALSE;

  if (! callback_added)
    {
      g_io_add_watch (_readchannel, G_IO_IN | G_IO_PRI, gimp_extension_read,
                      NULL);

      callback_added = TRUE;
    }
}

/**
 * gimp_extension_process:
 * @timeout: The timeout (in ms) to use for the select() call.
 *
 * Processes one message sent by GIMP and returns.
 *
 * Call this function in an endless loop after calling
 * gimp_extension_ack() to process requests for running temporary
 * procedures.
 *
 * See gimp_extension_enable() for an asynchronous way of doing the
 * same if running an endless loop is not an option.
 *
 * See also: gimp_install_procedure(), gimp_install_temp_proc()
 **/
void
gimp_extension_process (guint timeout)
{
#ifndef G_OS_WIN32
  gint select_val;

  do
    {
      fd_set readfds;
      struct timeval  tv;
      struct timeval *tvp;

      if (timeout)
        {
          tv.tv_sec  = timeout / 1000;
          tv.tv_usec = (timeout % 1000) * 1000;
          tvp = &tv;
        }
      else
        tvp = NULL;

      FD_ZERO (&readfds);
      FD_SET (g_io_channel_unix_get_fd (_readchannel), &readfds);

      if ((select_val = select (FD_SETSIZE, &readfds, NULL, NULL, tvp)) > 0)
        {
          gimp_single_message ();
        }
      else if (select_val == -1 && errno != EINTR)
        {
          perror ("gimp_extension_process");
          gimp_quit ();
        }
    }
  while (select_val == -1 && errno == EINTR);
#else
  /* Zero means infinite wait for us, but g_poll and
   * g_io_channel_win32_poll use -1 to indicate
   * infinite wait.
   */
  GPollFD pollfd;

  if (timeout == 0)
    timeout = -1;

  g_io_channel_win32_make_pollfd (_readchannel, G_IO_IN, &pollfd);

  if (g_io_channel_win32_poll (&pollfd, 1, timeout) == 1)
    gimp_single_message ();
#endif
}


/*  private functions  */

static void
gimp_close (void)
{
  if (gimp_debug_flags & GIMP_DEBUG_QUIT)
    gimp_debug_stop ();

  if (PLUG_IN_INFO.quit_proc)
    (* PLUG_IN_INFO.quit_proc) ();

#if defined(USE_SYSV_SHM)

  if ((_shm_ID != -1) && _shm_addr)
    shmdt ((char *) _shm_addr);

#elif defined(USE_WIN32_SHM)

  if (shm_handle)
    CloseHandle (shm_handle);

#elif defined(USE_POSIX_SHM)

  if ((_shm_ID != -1) && (_shm_addr != MAP_FAILED))
    munmap (_shm_addr, TILE_MAP_SIZE);

#endif

  gp_quit_write (_writechannel, NULL);
}

static void
gimp_debug_stop (void)
{
#ifndef G_OS_WIN32

  g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Waiting for debugger...");
  raise (SIGSTOP);

#else

  HANDLE        hThreadSnap = NULL;
  THREADENTRY32 te32        = { 0 };
  pid_t         opid        = getpid ();

  g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
         "Debugging (restart externally): %ld",
         (long int) opid);

  hThreadSnap = CreateToolhelp32Snapshot (TH32CS_SNAPTHREAD, 0);
  if (hThreadSnap == INVALID_HANDLE_VALUE)
    {
      g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
             "error getting threadsnap - debugging impossible");
      return;
    }

  te32.dwSize = sizeof (THREADENTRY32);

  if (Thread32First (hThreadSnap, &te32))
    {
      do
        {
          if (te32.th32OwnerProcessID == opid)
            {
              HANDLE hThread = OpenThread (THREAD_SUSPEND_RESUME, FALSE,
                                           te32.th32ThreadID);
              SuspendThread (hThread);
              CloseHandle (hThread);
            }
        }
      while (Thread32Next (hThreadSnap, &te32));
    }
  else
    {
      g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "error getting threads");
    }

  CloseHandle (hThreadSnap);

#endif
}

static void
gimp_message_func (const gchar    *log_domain,
                   GLogLevelFlags  log_level,
                   const gchar    *message,
                   gpointer        data)
{
  gimp_message (message);
}

static void
gimp_fatal_func (const gchar    *log_domain,
                 GLogLevelFlags  flags,
                 const gchar    *message,
                 gpointer        data)
{
  const gchar *level;

  switch (flags & G_LOG_LEVEL_MASK)
    {
    case G_LOG_LEVEL_WARNING:
      level = "WARNING";
      break;
    case G_LOG_LEVEL_CRITICAL:
      level = "CRITICAL";
      break;
    case G_LOG_LEVEL_ERROR:
      level = "ERROR";
      break;
    default:
      level = "FATAL";
      break;
    }

  g_printerr ("%s: %s: %s\n",
              progname, level, message);

#ifndef G_OS_WIN32
  switch (stack_trace_mode)
    {
    case GIMP_STACK_TRACE_NEVER:
      break;

    case GIMP_STACK_TRACE_QUERY:
        {
          sigset_t sigset;

          sigemptyset (&sigset);
          sigprocmask (SIG_SETMASK, &sigset, NULL);
          gimp_stack_trace_query (progname);
        }
      break;

    case GIMP_STACK_TRACE_ALWAYS:
        {
          sigset_t sigset;

          sigemptyset (&sigset);
          sigprocmask (SIG_SETMASK, &sigset, NULL);
          gimp_stack_trace_print (progname, stdout, NULL);
        }
      break;
    }
#endif

  /* Do not end with gimp_quit().
   * We want the plug-in to continue its normal crash course, otherwise
   * we won't get the "Plug-in crashed" error in GIMP.
   */
  exit (EXIT_FAILURE);
}

#ifdef G_OS_WIN32

#ifdef HAVE_EXCHNDL
static LONG WINAPI
gimp_plugin_sigfatal_handler (PEXCEPTION_POINTERS pExceptionInfo)
{
  g_printerr ("%s: fatal error\n", progname);

  SetUnhandledExceptionFilter (_prevExceptionFilter);

  /* For simplicity, do not make a difference between QUERY and ALWAYS
   * on Windows (at least not for now).
   */
  if (stack_trace_mode != GIMP_STACK_TRACE_NEVER &&
      g_file_test (plug_in_backtrace_path, G_FILE_TEST_IS_REGULAR))
    {
      FILE   *stream;
      guchar  buffer[256];
      size_t  read_len;

      stream = fopen (plug_in_backtrace_path, "r");
      do
        {
          /* Just read and output directly the file content. */
          read_len = fread (buffer, 1, sizeof (buffer) - 1, stream);
          buffer[read_len] = '\0';
          g_printerr ("%s", buffer);
        }
      while (read_len);
      fclose (stream);
    }

  if (_prevExceptionFilter && _prevExceptionFilter != gimp_plugin_sigfatal_handler)
    return _prevExceptionFilter (pExceptionInfo);
  else
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

#else
static void
gimp_plugin_sigfatal_handler (gint sig_num)
{
  switch (sig_num)
    {
    case SIGHUP:
    case SIGINT:
    case SIGQUIT:
    case SIGTERM:
      g_printerr ("%s terminated: %s\n", progname, g_strsignal (sig_num));
      break;

    case SIGABRT:
    case SIGBUS:
    case SIGSEGV:
    case SIGFPE:
    case SIGPIPE:
    default:
      g_printerr ("%s: fatal error: %s\n", progname, g_strsignal (sig_num));
      switch (stack_trace_mode)
        {
        case GIMP_STACK_TRACE_NEVER:
          break;

        case GIMP_STACK_TRACE_QUERY:
          {
            sigset_t sigset;

            sigemptyset (&sigset);
            sigprocmask (SIG_SETMASK, &sigset, NULL);
            gimp_stack_trace_query (progname);
          }
          break;

        case GIMP_STACK_TRACE_ALWAYS:
          {
            sigset_t sigset;

            sigemptyset (&sigset);
            sigprocmask (SIG_SETMASK, &sigset, NULL);
            gimp_stack_trace_print (progname, stdout, NULL);
          }
          break;
        }
      break;
    }

  /* Do not end with gimp_quit().
   * We want the plug-in to continue its normal crash course, otherwise
   * we won't get the "Plug-in crashed" error in GIMP.
   */
  exit (EXIT_FAILURE);
}
#endif

static gboolean
gimp_plugin_io_error_handler (GIOChannel   *channel,
                              GIOCondition  cond,
                              gpointer      data)
{
  g_printerr ("%s: fatal error: GIMP crashed\n", progname);
  gimp_quit ();

  /* never reached */
  return TRUE;
}

static gboolean
gimp_write (GIOChannel   *channel,
            const guint8 *buf,
            gulong        count,
            gpointer      user_data)
{
  gulong bytes;

  while (count > 0)
    {
      if ((write_buffer_index + count) >= WRITE_BUFFER_SIZE)
        {
          bytes = WRITE_BUFFER_SIZE - write_buffer_index;
          memcpy (&write_buffer[write_buffer_index], buf, bytes);
          write_buffer_index += bytes;
          if (! gimp_wire_flush (channel, NULL))
            return FALSE;
        }
      else
        {
          bytes = count;
          memcpy (&write_buffer[write_buffer_index], buf, bytes);
          write_buffer_index += bytes;
        }

      buf += bytes;
      count -= bytes;
    }

  return TRUE;
}

static gboolean
gimp_flush (GIOChannel *channel,
            gpointer    user_data)
{
  GIOStatus  status;
  GError    *error = NULL;
  gsize      count;
  gsize      bytes;

  if (write_buffer_index > 0)
    {
      count = 0;
      while (count != write_buffer_index)
        {
          do
            {
              bytes = 0;
              status = g_io_channel_write_chars (channel,
                                                 &write_buffer[count],
                                                 (write_buffer_index - count),
                                                 &bytes,
                                                 &error);
            }
          while (status == G_IO_STATUS_AGAIN);

          if (status != G_IO_STATUS_NORMAL)
            {
              if (error)
                {
                  g_warning ("%s: gimp_flush(): error: %s",
                             g_get_prgname (), error->message);
                  g_error_free (error);
                }
              else
                {
                  g_warning ("%s: gimp_flush(): error", g_get_prgname ());
                }

              return FALSE;
            }

          count += bytes;
        }

      write_buffer_index = 0;
    }

  return TRUE;
}

static void
gimp_loop (void)
{
  GimpWireMessage msg;

  while (TRUE)
    {
      if (! gimp_wire_read_msg (_readchannel, &msg, NULL))
        {
          gimp_close ();
          return;
        }

      switch (msg.type)
        {
        case GP_QUIT:
          gimp_wire_destroy (&msg);
          gimp_close ();
          return;

        case GP_CONFIG:
          gimp_config (msg.data);
          break;

        case GP_TILE_REQ:
        case GP_TILE_ACK:
        case GP_TILE_DATA:
          g_warning ("unexpected tile message received (should not happen)");
          break;

        case GP_PROC_RUN:
          gimp_proc_run (msg.data);
          gimp_wire_destroy (&msg);
          gimp_close ();
          return;

        case GP_PROC_RETURN:
          g_warning ("unexpected proc return message received (should not happen)");
          break;

        case GP_TEMP_PROC_RUN:
          g_warning ("unexpected temp proc run message received (should not happen");
          break;

        case GP_TEMP_PROC_RETURN:
          g_warning ("unexpected temp proc return message received (should not happen");
          break;

        case GP_PROC_INSTALL:
          g_warning ("unexpected proc install message received (should not happen)");
          break;

        case GP_HAS_INIT:
          g_warning ("unexpected has init message received (should not happen)");
          break;
        }

      gimp_wire_destroy (&msg);
    }
}

static void
gimp_config (GPConfig *config)
{
  if (config->version < GIMP_PROTOCOL_VERSION)
    {
      g_message ("Could not execute plug-in \"%s\"\n(%s)\n"
                 "because GIMP is using an older version of the "
                 "plug-in protocol.",
                 gimp_filename_to_utf8 (g_get_prgname ()),
                 gimp_filename_to_utf8 (progname));
      gimp_quit ();
    }
  else if (config->version > GIMP_PROTOCOL_VERSION)
    {
      g_message ("Could not execute plug-in \"%s\"\n(%s)\n"
                 "because it uses an obsolete version of the "
                 "plug-in protocol.",
                 gimp_filename_to_utf8 (g_get_prgname ()),
                 gimp_filename_to_utf8 (progname));
      gimp_quit ();
    }

  _tile_width       = config->tile_width;
  _tile_height      = config->tile_height;
  _shm_ID           = config->shm_ID;
  _check_size       = config->check_size;
  _check_type       = config->check_type;
  _install_cmap     = config->install_cmap     ? TRUE : FALSE;
  _show_tool_tips   = config->show_tooltips    ? TRUE : FALSE;
  _show_help_button = config->show_help_button ? TRUE : FALSE;
  _export_exif      = config->export_exif      ? TRUE : FALSE;
  _export_xmp       = config->export_xmp       ? TRUE : FALSE;
  _export_iptc      = config->export_iptc      ? TRUE : FALSE;
  _min_colors       = config->min_colors;
  _gdisp_ID         = config->gdisp_ID;
  _wm_class         = g_strdup (config->wm_class);
  _display_name     = g_strdup (config->display_name);
  _monitor_number   = config->monitor_number;
  _timestamp        = config->timestamp;

  if (config->app_name)
    g_set_application_name (config->app_name);

  gimp_cpu_accel_set_use (config->use_cpu_accel);

  g_object_set (gegl_config (),
                "use-opencl",          config->use_opencl,
                "application-license", "GPL3",
                NULL);

  if (_shm_ID != -1)
    {
#if defined(USE_SYSV_SHM)

      /* Use SysV shared memory mechanisms for transferring tile data. */

      _shm_addr = (guchar *) shmat (_shm_ID, NULL, 0);

      if (_shm_addr == (guchar *) -1)
        {
          g_error ("shmat() failed: %s\n" ERRMSG_SHM_FAILED,
                   g_strerror (errno));
        }

#elif defined(USE_WIN32_SHM)

      /* Use Win32 shared memory mechanisms for transferring tile data. */

      gchar fileMapName[128];

      /* From the id, derive the file map name */
      g_snprintf (fileMapName, sizeof (fileMapName), "GIMP%d.SHM", _shm_ID);

      /* Open the file mapping */
      shm_handle = OpenFileMapping (FILE_MAP_ALL_ACCESS,
                                    0, fileMapName);
      if (shm_handle)
        {
          /* Map the shared memory into our address space for use */
          _shm_addr = (guchar *) MapViewOfFile (shm_handle,
                                                FILE_MAP_ALL_ACCESS,
                                                0, 0, TILE_MAP_SIZE);

          /* Verify that we mapped our view */
          if (!_shm_addr)
            {
              g_error ("MapViewOfFile error: %lu... " ERRMSG_SHM_FAILED,
                       GetLastError ());
            }
        }
      else
        {
          g_error ("OpenFileMapping error: %lu... " ERRMSG_SHM_FAILED,
                   GetLastError ());
        }

#elif defined(USE_POSIX_SHM)

      /* Use POSIX shared memory mechanisms for transferring tile data. */

      gchar map_file[32];
      gint  shm_fd;

      /* From the id, derive the file map name */
      g_snprintf (map_file, sizeof (map_file), "/gimp-shm-%d", _shm_ID);

      /* Open the file mapping */
      shm_fd = shm_open (map_file, O_RDWR, 0600);

      if (shm_fd != -1)
        {
          /* Map the shared memory into our address space for use */
          _shm_addr = (guchar *) mmap (NULL, TILE_MAP_SIZE,
                                       PROT_READ | PROT_WRITE, MAP_SHARED,
                                       shm_fd, 0);

          /* Verify that we mapped our view */
          if (_shm_addr == MAP_FAILED)
            {
              g_error ("mmap() failed: %s\n" ERRMSG_SHM_FAILED,
                       g_strerror (errno));
            }

          close (shm_fd);
        }
      else
        {
          g_error ("shm_open() failed: %s\n" ERRMSG_SHM_FAILED,
                   g_strerror (errno));
        }

#endif
    }
}

static void
gimp_proc_run (GPProcRun *proc_run)
{
  if (PLUG_IN_INFO.run_proc)
    {
      GPProcReturn  proc_return;
      GimpParam    *return_vals;
      gint          n_return_vals;

      (* PLUG_IN_INFO.run_proc) (proc_run->name,
                                 proc_run->nparams,
                                 (GimpParam *) proc_run->params,
                                 &n_return_vals, &return_vals);

      proc_return.name    = proc_run->name;
      proc_return.nparams = n_return_vals;
      proc_return.params  = (GPParam *) return_vals;

      if (! gp_proc_return_write (_writechannel, &proc_return, NULL))
        gimp_quit ();
    }
}

static void
gimp_temp_proc_run (GPProcRun *proc_run)
{
  GimpRunProc run_proc = g_hash_table_lookup (temp_proc_ht, proc_run->name);

  if (run_proc)
    {
      GPProcReturn  proc_return;
      GimpParam    *return_vals;
      gint          n_return_vals;

#ifdef GDK_WINDOWING_QUARTZ
      if (proc_run->params &&
          proc_run->params[0].data.d_int32 == GIMP_RUN_INTERACTIVE)
        {
          [NSApp activateIgnoringOtherApps: YES];
        }
#endif

      (* run_proc) (proc_run->name,
                    proc_run->nparams,
                    (GimpParam *) proc_run->params,
                    &n_return_vals, &return_vals);

      proc_return.name    = proc_run->name;
      proc_return.nparams = n_return_vals;
      proc_return.params  = (GPParam *) return_vals;

      if (! gp_temp_proc_return_write (_writechannel, &proc_return, NULL))
        gimp_quit ();
    }
}

static void
gimp_process_message (GimpWireMessage *msg)
{
  switch (msg->type)
    {
    case GP_QUIT:
      gimp_quit ();
      break;
    case GP_CONFIG:
      gimp_config (msg->data);
      break;
    case GP_TILE_REQ:
    case GP_TILE_ACK:
    case GP_TILE_DATA:
      g_warning ("unexpected tile message received (should not happen)");
      break;
    case GP_PROC_RUN:
      g_warning ("unexpected proc run message received (should not happen)");
      break;
    case GP_PROC_RETURN:
      g_warning ("unexpected proc return message received (should not happen)");
      break;
    case GP_TEMP_PROC_RUN:
      gimp_temp_proc_run (msg->data);
      break;
    case GP_TEMP_PROC_RETURN:
      g_warning ("unexpected temp proc return message received (should not happen)");
      break;
    case GP_PROC_INSTALL:
      g_warning ("unexpected proc install message received (should not happen)");
      break;
    case GP_HAS_INIT:
      g_warning ("unexpected has init message received (should not happen)");
      break;
    }
}

static void
gimp_single_message (void)
{
  GimpWireMessage msg;

  /* Run a temp function */
  if (! gimp_wire_read_msg (_readchannel, &msg, NULL))
    gimp_quit ();

  gimp_process_message (&msg);

  gimp_wire_destroy (&msg);
}

static gboolean
gimp_extension_read (GIOChannel  *channel,
                     GIOCondition condition,
                     gpointer     data)
{
  gimp_single_message ();

  return TRUE;
}

static void
gimp_set_pdb_error (const GimpParam *return_vals,
                    gint             n_return_vals)
{
  if (pdb_error_message)
    {
      g_free (pdb_error_message);
      pdb_error_message = NULL;
    }

  pdb_error_status = return_vals[0].data.d_status;

  switch (pdb_error_status)
    {
    case GIMP_PDB_SUCCESS:
    case GIMP_PDB_PASS_THROUGH:
      break;

    case GIMP_PDB_EXECUTION_ERROR:
    case GIMP_PDB_CALLING_ERROR:
    case GIMP_PDB_CANCEL:
      if (n_return_vals > 1 && return_vals[1].type == GIMP_PDB_STRING)
        {
          pdb_error_message = g_strdup (return_vals[1].data.d_string);
        }
      break;
    }
}
