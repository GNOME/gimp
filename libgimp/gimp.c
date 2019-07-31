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
 * <https://www.gnu.org/licenses/>.
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

#include "gimp.h"
#include "gimp-private.h"
#include "gimpgpcompat.h"
#include "gimpgpparams.h"
#include "gimpplugin-private.h"
#include "gimpunitcache.h"

#include "libgimp-intl.h"


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


static gint       gimp_main_internal           (GType                 plug_in_type,
                                                const GimpPlugInInfo *info,
                                                gint                  argc,
                                                gchar                *argv[]);

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
static void       gimp_proc_run_internal       (GPProcRun       *proc_run,
                                                GimpProcedure   *procedure,
                                                GimpRunProc      run_proc,
                                                GPProcReturn    *proc_return);
static void       gimp_process_message         (GimpWireMessage *msg);
static void       gimp_single_message          (void);
static gboolean   gimp_extension_read          (GIOChannel      *channel,
                                                GIOCondition     condition,
                                                gpointer         data);

static void       gimp_set_pdb_error           (GimpValueArray  *return_vals);

#if defined G_OS_WIN32 && defined HAVE_EXCHNDL
static LPTOP_LEVEL_EXCEPTION_FILTER  _prevExceptionFilter    = NULL;
static gchar                         *plug_in_backtrace_path = NULL;
#endif

static GIOChannel                   *_gimp_readchannel       = NULL;
GIOChannel                          *_gimp_writechannel      = NULL;

#ifdef USE_WIN32_SHM
static HANDLE shm_handle;
#endif

static gint           _tile_width        = -1;
static gint           _tile_height       = -1;
static gint           _shm_ID            = -1;
static guchar        *_shm_addr          = NULL;
static gboolean       _show_help_button  = TRUE;
static gboolean       _export_profile    = FALSE;
static gboolean       _export_exif       = FALSE;
static gboolean       _export_xmp        = FALSE;
static gboolean       _export_iptc       = FALSE;
static GimpCheckSize  _check_size        = GIMP_CHECK_SIZE_MEDIUM_CHECKS;
static GimpCheckType  _check_type        = GIMP_CHECK_TYPE_GRAY_CHECKS;
static gint           _gdisp_ID          = -1;
static gchar         *_wm_class          = NULL;
static gchar         *_display_name      = NULL;
static gint           _monitor_number    = 0;
static guint32        _timestamp         = 0;
static gchar         *_icon_theme_dir    = NULL;
static const gchar   *progname           = NULL;

static gchar          write_buffer[WRITE_BUFFER_SIZE];
static gulong         write_buffer_index = 0;

static GimpStackTraceMode stack_trace_mode = GIMP_STACK_TRACE_NEVER;

GHashTable           *_gimp_temp_proc_ht = NULL;

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

static GimpPlugIn     *PLUG_IN      = NULL;
static GimpPlugInInfo  PLUG_IN_INFO = { 0, };

static GimpPDBStatusType  pdb_error_status   = GIMP_PDB_SUCCESS;
static gchar             *pdb_error_message  = NULL;


/**
 * gimp_main:
 * @plug_in_type: the type of the #GimpPlugIn subclass of the plug-in
 * @argc:         the number of arguments
 * @argv:         (array length=argc): the arguments
 *
 * The main procedure that must be called with the plug-in's
 * #GimpPlugIn subclass type and the 'argc' and 'argv' that are passed
 * to "main".
 *
 * Returns: an exit status as defined by the C library,
 *          on success EXIT_SUCCESS.
 **/
gint
gimp_main (GType  plug_in_type,
           gint   argc,
           gchar *argv[])
{
  return gimp_main_internal (plug_in_type, NULL, argc, argv);
}

/**
 * gimp_main_legacy:
 * @info: the #GimpPlugInInfo structure
 * @argc: the number of arguments
 * @argv: (array length=argc): the arguments
 *
 * The main procedure that must be called with the #GimpPlugInInfo
 * structure and the 'argc' and 'argv' that are passed to "main".
 *
 * Returns: an exit status as defined by the C library,
 *          on success EXIT_SUCCESS.
 **/
gint
gimp_main_legacy (const GimpPlugInInfo *info,
                  gint                  argc,
                  gchar                *argv[])
{
  return gimp_main_internal (G_TYPE_NONE, info, argc, argv);
}

static gint
gimp_main_internal (GType                 plug_in_type,
                    const GimpPlugInInfo *info,
                    gint                  argc,
                    gchar                *argv[])
{
  enum
  {
    ARG_PROGNAME,
    ARG_GIMP,
    ARG_PROTOCOL_VERSION,
    ARG_READ_FD,
    ARG_WRITE_FD,
    ARG_MODE,
    ARG_STACK_TRACE_MODE,

    N_ARGS
  };

  gchar       *basename;
  const gchar *env_string;
  gchar       *debug_string;
  gint         protocol_version;

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

  g_assert ((plug_in_type != G_TYPE_NONE && info == NULL) ||
            (plug_in_type == G_TYPE_NONE && info != NULL));

  if (plug_in_type != G_TYPE_NONE)
    {
      PLUG_IN = g_object_new (plug_in_type, NULL);

      g_assert (GIMP_IS_PLUG_IN (PLUG_IN));
    }
  else
    {
      PLUG_IN_INFO = *info;
    }

  if ((argc != N_ARGS) || (strcmp (argv[ARG_GIMP], "-gimp") != 0))
    {
      g_printerr ("%s is a GIMP plug-in and must be run by GIMP to be used\n",
                  argv[ARG_PROGNAME]);
      return EXIT_FAILURE;
    }

  gimp_env_init (TRUE);

  progname = argv[ARG_PROGNAME];

  basename = g_path_get_basename (progname);

  g_set_prgname (basename);

  protocol_version = atoi (argv[ARG_PROTOCOL_VERSION]);

  if (protocol_version < GIMP_PROTOCOL_VERSION)
    {
      g_printerr ("Could not execute plug-in \"%s\"\n(%s)\n"
                  "because GIMP is using an older version of the "
                  "plug-in protocol.\n",
                  gimp_filename_to_utf8 (g_get_prgname ()),
                  gimp_filename_to_utf8 (progname));
      return EXIT_FAILURE;
    }
  else if (protocol_version > GIMP_PROTOCOL_VERSION)
    {
      g_printerr ("Could not execute plug-in \"%s\"\n(%s)\n"
                  "because it uses an obsolete version of the "
                  "plug-in protocol.\n",
                  gimp_filename_to_utf8 (g_get_prgname ()),
                  gimp_filename_to_utf8 (progname));
      return EXIT_FAILURE;
    }

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
  _gimp_readchannel  = g_io_channel_win32_new_fd (atoi (argv[ARG_READ_FD]));
  _gimp_writechannel = g_io_channel_win32_new_fd (atoi (argv[ARG_WRITE_FD]));
#else
  _gimp_readchannel  = g_io_channel_unix_new (atoi (argv[ARG_READ_FD]));
  _gimp_writechannel = g_io_channel_unix_new (atoi (argv[ARG_WRITE_FD]));
#endif

  g_io_channel_set_encoding (_gimp_readchannel, NULL, NULL);
  g_io_channel_set_encoding (_gimp_writechannel, NULL, NULL);

  g_io_channel_set_buffered (_gimp_readchannel, FALSE);
  g_io_channel_set_buffered (_gimp_writechannel, FALSE);

  g_io_channel_set_close_on_unref (_gimp_readchannel, TRUE);
  g_io_channel_set_close_on_unref (_gimp_writechannel, TRUE);

  gp_init ();

  gimp_wire_set_writer (gimp_write);
  gimp_wire_set_flusher (gimp_flush);

  /*  initialize GTypes, they need to be known to g_type_from_name()  */
  {
    GType init_types[] =
    {
      GIMP_TYPE_INT32,         GIMP_TYPE_PARAM_INT32,
      GIMP_TYPE_INT16,         GIMP_TYPE_PARAM_INT16,
      GIMP_TYPE_INT8,          GIMP_TYPE_PARAM_INT8,

      GIMP_TYPE_PARAM_STRING,

      GIMP_TYPE_ARRAY,         GIMP_TYPE_PARAM_ARRAY,
      GIMP_TYPE_INT8_ARRAY,    GIMP_TYPE_PARAM_INT8_ARRAY,
      GIMP_TYPE_INT16_ARRAY,   GIMP_TYPE_PARAM_INT16_ARRAY,
      GIMP_TYPE_INT32_ARRAY,   GIMP_TYPE_PARAM_INT32_ARRAY,
      GIMP_TYPE_FLOAT_ARRAY,   GIMP_TYPE_PARAM_FLOAT_ARRAY,
      GIMP_TYPE_STRING_ARRAY,  GIMP_TYPE_PARAM_STRING_ARRAY,
      GIMP_TYPE_RGB_ARRAY,     GIMP_TYPE_PARAM_RGB_ARRAY,

      GIMP_TYPE_DISPLAY_ID,    GIMP_TYPE_PARAM_DISPLAY_ID,
      GIMP_TYPE_IMAGE_ID,      GIMP_TYPE_PARAM_IMAGE_ID,
      GIMP_TYPE_ITEM_ID,       GIMP_TYPE_PARAM_ITEM_ID,
      GIMP_TYPE_DRAWABLE_ID,   GIMP_TYPE_PARAM_DRAWABLE_ID,
      GIMP_TYPE_LAYER_ID,      GIMP_TYPE_PARAM_LAYER_ID,
      GIMP_TYPE_CHANNEL_ID,    GIMP_TYPE_PARAM_CHANNEL_ID,
      GIMP_TYPE_LAYER_MASK_ID, GIMP_TYPE_PARAM_LAYER_MASK_ID,
      GIMP_TYPE_SELECTION_ID,  GIMP_TYPE_PARAM_SELECTION_ID,
      GIMP_TYPE_VECTORS_ID,    GIMP_TYPE_PARAM_VECTORS_ID
    };

    gint i;

    for (i = 0; i < G_N_ELEMENTS (init_types); i++, i++)
      {
        GType type = init_types[i];

        if (G_TYPE_IS_CLASSED (type))
          g_type_class_ref (type);
      }

    gimp_enums_init ();
  }

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
      if (PLUG_IN)
        {
          if (GIMP_PLUG_IN_GET_CLASS (PLUG_IN)->init_procedures)
            gp_has_init_write (_gimp_writechannel, NULL);
        }
      else
        {
          if (PLUG_IN_INFO.init_proc)
            gp_has_init_write (_gimp_writechannel, NULL);
        }

      if (gimp_debug_flags & GIMP_DEBUG_QUERY)
        gimp_debug_stop ();

      if (PLUG_IN)
        {
          _gimp_plug_in_query (PLUG_IN);
        }
      else
        {
          if (PLUG_IN_INFO.query_proc)
            PLUG_IN_INFO.query_proc ();
        }

      gimp_close ();

      return EXIT_SUCCESS;
    }

  if (strcmp (argv[ARG_MODE], "-init") == 0)
    {
      if (gimp_debug_flags & GIMP_DEBUG_INIT)
        gimp_debug_stop ();

      if (PLUG_IN)
        {
          _gimp_plug_in_init (PLUG_IN);
        }
      else
        {
          if (PLUG_IN_INFO.init_proc)
            PLUG_IN_INFO.init_proc ();
        }

      gimp_close ();

      return EXIT_SUCCESS;
    }

  if (gimp_debug_flags & GIMP_DEBUG_RUN)
    gimp_debug_stop ();
  else if (gimp_debug_flags & GIMP_DEBUG_PID)
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Here I am!");

  _gimp_temp_proc_ht = g_hash_table_new (g_str_hash, g_str_equal);

  g_io_add_watch (_gimp_readchannel,
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

void
_gimp_read_expect_msg (GimpWireMessage *msg,
                       gint             type)
{
  while (TRUE)
    {
      if (! gimp_wire_read_msg (_gimp_readchannel, msg, NULL))
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

GimpValueArray *
gimp_run_procedure_with_array (const gchar    *name,
                               GimpValueArray *arguments)
{
  GPProcRun        proc_run;
  GPProcReturn    *proc_return;
  GimpWireMessage  msg;
  GimpValueArray  *return_values;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (arguments != NULL, NULL);

  proc_run.name    = (gchar *) name;
  proc_run.nparams = gimp_value_array_length (arguments);
  proc_run.params  = _gimp_value_array_to_gp_params (arguments, FALSE);

  gp_lock ();
  if (! gp_proc_run_write (_gimp_writechannel, &proc_run, NULL))
    gimp_quit ();

  _gimp_read_expect_msg (&msg, GP_PROC_RETURN);
  gp_unlock ();

  proc_return = msg.data;

  return_values = _gimp_gp_params_to_value_array (NULL, 0,
                                                  proc_return->params,
                                                  proc_return->nparams,
                                                  TRUE, FALSE);

  gimp_wire_destroy (&msg);

  gimp_set_pdb_error (return_values);

  return return_values;
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
 * gimp_export_color_profile:
 *
 * Returns whether file plug-ins should default to exporting the
 * image's color profile.
 *
 * Return value: TRUE if preferences are set to export the color profile.
 *
 * Since: 2.10.4
 **/
gboolean
gimp_export_color_profile (void)
{
  return _export_profile;
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
 * gimp_get_icon_theme_dir:
 *
 * Returns the directory of the current icon theme.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Return value: the icon theme directory
 *
 * Since: 2.10.4
 **/
const gchar *
gimp_icon_theme_dir (void)
{
  return _icon_theme_dir;
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
  if (! gp_extension_ack_write (_gimp_writechannel, NULL))
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
      g_io_add_watch (_gimp_readchannel, G_IO_IN | G_IO_PRI,
                      gimp_extension_read,
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
      FD_SET (g_io_channel_unix_get_fd (_gimp_readchannel), &readfds);

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

  g_io_channel_win32_make_pollfd (_gimp_readchannel, G_IO_IN, &pollfd);

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

  if (PLUG_IN)
    {
      _gimp_plug_in_quit (PLUG_IN);
    }
  else
    {
      if (PLUG_IN_INFO.quit_proc)
        PLUG_IN_INFO.quit_proc ();
    }

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

  gp_quit_write (_gimp_writechannel, NULL);
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
      if (! gimp_wire_read_msg (_gimp_readchannel, &msg, NULL))
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
  GFile *file;
  gchar *path;

  _tile_width       = config->tile_width;
  _tile_height      = config->tile_height;
  _shm_ID           = config->shm_ID;
  _check_size       = config->check_size;
  _check_type       = config->check_type;
  _show_help_button = config->show_help_button ? TRUE : FALSE;
  _export_profile   = config->export_profile   ? TRUE : FALSE;
  _export_exif      = config->export_exif      ? TRUE : FALSE;
  _export_xmp       = config->export_xmp       ? TRUE : FALSE;
  _export_iptc      = config->export_iptc      ? TRUE : FALSE;
  _gdisp_ID         = config->gdisp_ID;
  _wm_class         = g_strdup (config->wm_class);
  _display_name     = g_strdup (config->display_name);
  _monitor_number   = config->monitor_number;
  _timestamp        = config->timestamp;
  _icon_theme_dir   = g_strdup (config->icon_theme_dir);

  if (config->app_name)
    g_set_application_name (config->app_name);

  gimp_cpu_accel_set_use (config->use_cpu_accel);

  file = gimp_file_new_for_config_path (config->swap_path, NULL);
  path = g_file_get_path (file);

  g_object_set (gegl_config (),
                "tile-cache-size",     config->tile_cache_size,
                "swap",                path,
                "threads",             (gint) config->num_processors,
                "use-opencl",          config->use_opencl,
                "application-license", "GPL3",
                NULL);

  g_free (path);
  g_object_unref (file);

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
  GPProcReturn proc_return;

  if (PLUG_IN)
    {
      GimpProcedure *procedure;

      procedure = gimp_plug_in_create_procedure (PLUG_IN, proc_run->name);

      if (procedure)
        {
          gimp_proc_run_internal (proc_run, procedure, NULL,
                                  &proc_return);

          g_object_unref (procedure);
        }
    }
  else if (PLUG_IN_INFO.run_proc)
    {
      gimp_proc_run_internal (proc_run, NULL, PLUG_IN_INFO.run_proc,
                              &proc_return);
    }

  if (! gp_proc_return_write (_gimp_writechannel, &proc_return, NULL))
    gimp_quit ();
}

static void
gimp_temp_proc_run (GPProcRun *proc_run)
{
  GPProcReturn proc_return;

  if (PLUG_IN)
    {
      GimpProcedure *procedure;

      procedure = gimp_plug_in_get_temp_procedure (PLUG_IN, proc_run->name);

      if (procedure)
        {
          gimp_proc_run_internal (proc_run, procedure, NULL,
                                  &proc_return);
        }
    }
  else
    {
      GimpRunProc run_proc = g_hash_table_lookup (_gimp_temp_proc_ht,
                                                  proc_run->name);

      if (run_proc)
        {
#ifdef GDK_WINDOWING_QUARTZ
          if (proc_run->params &&
              proc_run->params[0].data.d_int == GIMP_RUN_INTERACTIVE)
            {
              [NSApp activateIgnoringOtherApps: YES];
            }
#endif

          gimp_proc_run_internal (proc_run, NULL, run_proc,
                                  &proc_return);
        }
    }

  if (! gp_temp_proc_return_write (_gimp_writechannel, &proc_return, NULL))
    gimp_quit ();
}

static void
gimp_proc_run_internal (GPProcRun     *proc_run,
                        GimpProcedure *procedure,
                        GimpRunProc    run_proc,
                        GPProcReturn  *proc_return)
{
  GimpValueArray *arguments;
  GimpValueArray *return_values = NULL;

  arguments = _gimp_gp_params_to_value_array (NULL, 0,
                                              proc_run->params,
                                              proc_run->nparams,
                                              FALSE, FALSE);

  if (procedure)
    {
      return_values = gimp_procedure_run (procedure, arguments);
    }
  else
    {
      GimpParam *params;
      GimpParam *return_vals;
      gint       n_params;
      gint       n_return_vals;

      n_params = gimp_value_array_length (arguments);
      params   = _gimp_value_array_to_params (arguments, FALSE);

      run_proc (proc_run->name,
                n_params,       params,
                &n_return_vals, &return_vals);

      return_values = _gimp_params_to_value_array (return_vals,
                                                   n_return_vals,
                                                   FALSE);

      g_free (params);
    }

  gimp_value_array_unref (arguments);

  proc_return->name    = proc_run->name;
  proc_return->nparams = gimp_value_array_length (return_values);
  proc_return->params  = _gimp_value_array_to_gp_params (return_values, TRUE);

  gimp_value_array_unref (return_values);
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
  if (! gimp_wire_read_msg (_gimp_readchannel, &msg, NULL))
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
gimp_set_pdb_error (GimpValueArray *return_values)
{
  g_clear_pointer (&pdb_error_message, g_free);
  pdb_error_status = GIMP_PDB_SUCCESS;

  if (gimp_value_array_length (return_values) > 0)
    {
      pdb_error_status =
        g_value_get_enum (gimp_value_array_index (return_values, 0));

      switch (pdb_error_status)
        {
        case GIMP_PDB_SUCCESS:
        case GIMP_PDB_PASS_THROUGH:
          break;

        case GIMP_PDB_EXECUTION_ERROR:
        case GIMP_PDB_CALLING_ERROR:
        case GIMP_PDB_CANCEL:
          if (gimp_value_array_length (return_values) > 1)
            {
              GValue *value = gimp_value_array_index (return_values, 1);

              if (G_VALUE_HOLDS_STRING (value))
                pdb_error_message = g_value_dup_string (value);
            }
          break;
        }
    }
}
