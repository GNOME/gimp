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

#include <glib.h>
#include <glib/gstdio.h>

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

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)
#  ifdef STRICT
#  undef STRICT
#  endif
#  define STRICT

#  include <windows.h>
#  include <tlhelp32.h>
#  undef RGB
#endif

#include <locale.h>

#include "gimp.h"

#include "libgimpbase/gimpbase-private.h"
#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "gimp-debug.h"
#include "gimp-private.h"
#include "gimp-shm.h"
#include "gimpgpparams.h"
#include "gimppdb-private.h"
#include "gimpplugin-private.h"

#include "libgimp-intl.h"


static void   gimp_close        (void);


#ifdef G_OS_WIN32

#ifdef HAVE_EXCHNDL
static LONG WINAPI gimp_plugin_sigfatal_handler (PEXCEPTION_POINTERS pExceptionInfo);

static LPTOP_LEVEL_EXCEPTION_FILTER  _prevExceptionFilter    = NULL;
static gchar                         *plug_in_backtrace_path = NULL;
#endif

#else /* ! G_OS_WIN32 */

static void        gimp_plugin_sigfatal_handler (gint sig_num);

#endif /* G_OS_WIN32 */


static GimpPlugIn         *PLUG_IN               = NULL;
static GimpPDB            *PDB                   = NULL;

static gint                _tile_width           = -1;
static gint                _tile_height          = -1;
static gboolean            _show_help_button     = TRUE;
static gboolean            _export_color_profile = FALSE;
static gboolean            _export_comment       = FALSE;
static gboolean            _export_exif          = FALSE;
static gboolean            _export_xmp           = FALSE;
static gboolean            _export_iptc          = FALSE;
static gboolean            _export_thumbnail     = TRUE;
static gint32              _num_processors       = 1;
static GimpCheckSize       _check_size           = GIMP_CHECK_SIZE_MEDIUM_CHECKS;
static GimpCheckType       _check_type           = GIMP_CHECK_TYPE_GRAY_CHECKS;
static GeglColor          *_check_custom_color1  = NULL;
static GeglColor          *_check_custom_color2  = NULL;
static gint                _default_display_id   = -1;
static gchar              *_wm_class             = NULL;
static gchar              *_display_name         = NULL;
static gint                _monitor_number       = 0;
static guint32             _timestamp            = 0;
static gchar              *_icon_theme_dir       = NULL;
static const gchar        *progname              = NULL;

static GimpStackTraceMode  stack_trace_mode      = GIMP_STACK_TRACE_NEVER;


/**
 * gimp_main:
 * @plug_in_type: the type of the #GimpPlugIn subclass of the plug-in
 * @argc:         the number of arguments
 * @argv:         (array length=argc): the arguments
 *
 * The main plug-in function that must be called with the plug-in's
 * #GimpPlugIn subclass #GType and the 'argc' and 'argv' that are passed
 * to the platform's main().
 *
 * See also: GIMP_MAIN(), #GimpPlugIn.
 *
 * Returns: an exit status as defined by the C library,
 *          on success EXIT_SUCCESS.
 *
 * Since: 3.0
 **/
gint
gimp_main (GType  plug_in_type,
           gint   argc,
           gchar *argv[])
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

  GIOChannel *read_channel;
  GIOChannel *write_channel;
  gchar      *basename;
  gint        protocol_version;

#ifdef G_OS_WIN32

  gint i, j, k;

  /* Reduce risks */
  SetDllDirectoryW (L"");

  /* On Windows, set DLL search path to $INSTALLDIR/bin so that GEGL
   * file operations can find their respective file library DLLs (such
   * as jasper, etc.) without needing to set external PATH.
   */
  {
    const gchar *install_dir;
    gchar       *bin_dir;
    LPWSTR       w_bin_dir;

    w_bin_dir = NULL;
    install_dir = gimp_installation_directory ();
    bin_dir = g_build_filename (install_dir, "bin", NULL);

    w_bin_dir = g_utf8_to_utf16 (bin_dir, -1, NULL, NULL, NULL);
    if (w_bin_dir)
      {
        SetDllDirectoryW (w_bin_dir);
        g_free (w_bin_dir);
      }

    g_free (bin_dir);
  }

#ifdef HAVE_EXCHNDL
  /* Use Dr. Mingw (dumps backtrace on crash) if it is available. */
  {
    time_t   t;
    gchar   *filename;
    gchar   *dir;
    wchar_t *plug_in_backtrace_path_utf16;

    /* This has to be the non-roaming directory (i.e., the local
     * directory) as backtraces correspond to the binaries on this
     * system.
     */
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

    plug_in_backtrace_path_utf16 = g_utf8_to_utf16 (plug_in_backtrace_path,
                                                    -1, NULL, NULL, NULL);
    if (plug_in_backtrace_path_utf16)
      {
        ExcHndlSetLogFileNameW (plug_in_backtrace_path_utf16);
        g_free (plug_in_backtrace_path_utf16);
      }
  }
#endif /* HAVE_EXCHNDL */

#ifndef _WIN64
  {
    typedef BOOL (WINAPI *t_SetProcessDEPPolicy) (DWORD dwFlags);
    t_SetProcessDEPPolicy p_SetProcessDEPPolicy;

    p_SetProcessDEPPolicy = GetProcAddress (GetModuleHandleW (L"kernel32.dll"),
                                            "SetProcessDEPPolicy");
    if (p_SetProcessDEPPolicy)
      (*p_SetProcessDEPPolicy) (PROCESS_DEP_ENABLE|PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION);
  }
#endif /* _WIN64 */

  /* Group all our windows together on the taskbar */
  {
    typedef HRESULT (WINAPI *t_SetCurrentProcessExplicitAppUserModelID) (PCWSTR lpPathName);
    t_SetCurrentProcessExplicitAppUserModelID p_SetCurrentProcessExplicitAppUserModelID;

    p_SetCurrentProcessExplicitAppUserModelID = (t_SetCurrentProcessExplicitAppUserModelID) GetProcAddress (GetModuleHandleW (L"shell32.dll"),
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

#endif /* G_OS_WIN32 */

  g_assert (plug_in_type != G_TYPE_NONE);

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

  _gimp_debug_init (basename);

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

#endif /* ! G_OS_WIN32 */

#ifdef G_OS_WIN32
  read_channel  = g_io_channel_win32_new_fd (atoi (argv[ARG_READ_FD]));
  write_channel = g_io_channel_win32_new_fd (atoi (argv[ARG_WRITE_FD]));
#else
  read_channel  = g_io_channel_unix_new (atoi (argv[ARG_READ_FD]));
  write_channel = g_io_channel_unix_new (atoi (argv[ARG_WRITE_FD]));
#endif

  g_io_channel_set_encoding (read_channel, NULL, NULL);
  g_io_channel_set_encoding (write_channel, NULL, NULL);

  g_io_channel_set_buffered (read_channel, FALSE);
  g_io_channel_set_buffered (write_channel, FALSE);

  g_io_channel_set_close_on_unref (read_channel, TRUE);
  g_io_channel_set_close_on_unref (write_channel, TRUE);

  /*  initialize GTypes, they need to be known to g_type_from_name()  */
  {
    GType init_types[] =
    {
      G_TYPE_INT,              G_TYPE_PARAM_INT,
      G_TYPE_UCHAR,            G_TYPE_PARAM_UCHAR,

      G_TYPE_STRING,           G_TYPE_PARAM_STRING,
      G_TYPE_STRV,             G_TYPE_PARAM_BOXED,

      G_TYPE_BYTES,            G_TYPE_PARAM_BOXED,

      GIMP_TYPE_ARRAY,         GIMP_TYPE_PARAM_ARRAY,
      GIMP_TYPE_INT32_ARRAY,   GIMP_TYPE_PARAM_INT32_ARRAY,
      GIMP_TYPE_FLOAT_ARRAY,   GIMP_TYPE_PARAM_FLOAT_ARRAY,
      GIMP_TYPE_OBJECT_ARRAY,  GIMP_TYPE_PARAM_OBJECT_ARRAY,

      GIMP_TYPE_DISPLAY,       GIMP_TYPE_PARAM_DISPLAY,
      GIMP_TYPE_IMAGE,         GIMP_TYPE_PARAM_IMAGE,
      GIMP_TYPE_ITEM,          GIMP_TYPE_PARAM_ITEM,
      GIMP_TYPE_DRAWABLE,      GIMP_TYPE_PARAM_DRAWABLE,
      GIMP_TYPE_LAYER,         GIMP_TYPE_PARAM_LAYER,
      GIMP_TYPE_TEXT_LAYER,    GIMP_TYPE_PARAM_TEXT_LAYER,
      GIMP_TYPE_GROUP_LAYER,   GIMP_TYPE_PARAM_GROUP_LAYER,
      GIMP_TYPE_CHANNEL,       GIMP_TYPE_PARAM_CHANNEL,
      GIMP_TYPE_LAYER_MASK,    GIMP_TYPE_PARAM_LAYER_MASK,
      GIMP_TYPE_SELECTION,     GIMP_TYPE_PARAM_SELECTION,
      GIMP_TYPE_PATH,          GIMP_TYPE_PARAM_PATH,

      GIMP_TYPE_BRUSH,         GIMP_TYPE_PARAM_BRUSH,
      GIMP_TYPE_FONT,          GIMP_TYPE_PARAM_FONT,
      GIMP_TYPE_GRADIENT,      GIMP_TYPE_PARAM_GRADIENT,
      GIMP_TYPE_PALETTE,       GIMP_TYPE_PARAM_PALETTE,
      GIMP_TYPE_PATTERN,       GIMP_TYPE_PARAM_PATTERN,

      GIMP_TYPE_UNIT,          GIMP_TYPE_PARAM_UNIT
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
    GimpUnitVtable vtable = { 0 };

    vtable.get_deletion_flag = _gimp_unit_get_deletion_flag;
    vtable.set_deletion_flag = _gimp_unit_set_deletion_flag;
    vtable.get_data          = _gimp_unit_get_data;

    gimp_base_init (&vtable);
  }

  /*  initialize i18n support  */
  setlocale (LC_ALL, "");

  gimp_bind_text_domain (GETTEXT_PACKAGE"-libgimp", gimp_locale_directory ());
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE"-libgimp", "UTF-8");
#endif

  _gimp_debug_configure (stack_trace_mode);

  PLUG_IN = g_object_new (plug_in_type,
                          "program-name",  progname,
                          "read-channel",  read_channel,
                          "write-channel", write_channel,
                          NULL);

  g_assert (GIMP_IS_PLUG_IN (PLUG_IN));

  if (strcmp (argv[ARG_MODE], "-query") == 0)
    {
      if (_gimp_get_debug_flags () & GIMP_DEBUG_QUERY)
        _gimp_debug_stop ();

      _gimp_plug_in_query (PLUG_IN);

      gimp_close ();

      return EXIT_SUCCESS;
    }

  if (strcmp (argv[ARG_MODE], "-init") == 0)
    {
      if (_gimp_get_debug_flags () & GIMP_DEBUG_INIT)
        _gimp_debug_stop ();

      _gimp_plug_in_init (PLUG_IN);

      gimp_close ();

      return EXIT_SUCCESS;
    }

  if (_gimp_get_debug_flags () & GIMP_DEBUG_RUN)
    _gimp_debug_stop ();
  else if (_gimp_get_debug_flags () & GIMP_DEBUG_PID)
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Here I am!");

  _gimp_plug_in_run (PLUG_IN);

  gimp_close ();
  g_io_channel_unref (read_channel);
  g_io_channel_unref (write_channel);
  g_clear_object (&_check_custom_color1);
  g_clear_object (&_check_custom_color2);

  return EXIT_SUCCESS;
}

/**
 * gimp_get_plug_in:
 *
 * This function returns the plug-in's #GimpPlugIn instance, which is
 * a a singleton that can exist exactly once per running plug-in.
 *
 * Returns: (transfer none) (nullable): The plug-in's #GimpPlugIn singleton.
 *
 * Since: 3.0
 **/
GimpPlugIn *
gimp_get_plug_in (void)
{
  return PLUG_IN;
}

/**
 * gimp_get_pdb:
 *
 * This function returns the plug-in's #GimpPDB instance, which is a
 * singleton that can exist exactly once per running plug-in.
 *
 * Returns: (transfer none) (nullable): The plug-in's #GimpPDB singleton.
 *
 * Since: 3.0
 **/
GimpPDB *
gimp_get_pdb (void)
{
  if (! PDB)
    PDB = _gimp_pdb_new (PLUG_IN);

  return PDB;
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
 * gimp_tile_width:
 *
 * Returns the tile width GIMP is using.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Returns: the tile_width
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
 * Returns: the tile_height
 **/
guint
gimp_tile_height (void)
{
  return _tile_height;
}

/**
 * gimp_show_help_button:
 *
 * Returns whether or not GimpDialog should automatically add a help
 * button if help_func and help_id are given.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Returns: the show_help_button boolean
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
 * Returns: TRUE if preferences are set to export the color profile.
 *
 * Since: 2.10.4
 **/
gboolean
gimp_export_color_profile (void)
{
  return _export_color_profile;
}

/**
 * gimp_export_comment:
 *
 * Returns whether file plug-ins should default to exporting the
 * image's comment.
 *
 * Returns: TRUE if preferences are set to export the comment.
 *
 * Since: 3.0
 **/
gboolean
gimp_export_comment (void)
{
  return _export_comment;
}

/**
 * gimp_export_exif:
 *
 * Returns whether file plug-ins should default to exporting Exif
 * metadata, according preferences (original settings is %FALSE since
 * metadata can contain sensitive information).
 *
 * Returns: TRUE if preferences are set to export Exif.
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
 * metadata, according preferences (original settings is %FALSE since
 * metadata can contain sensitive information).
 *
 * Returns: TRUE if preferences are set to export XMP.
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
 * metadata, according preferences (original settings is %FALSE since
 * metadata can contain sensitive information).
 *
 * Returns: TRUE if preferences are set to export IPTC.
 *
 * Since: 2.10
 **/
gboolean
gimp_export_iptc (void)
{
  return _export_iptc;
}

/**
 * gimp_export_thumbnail:
 *
 * Returns whether file plug-ins should default to exporting the
 * image's comment.
 *
 * Returns: TRUE if preferences are set to export the thumbnail.
 *
 * Since: 3.0
 **/
gboolean
gimp_export_thumbnail (void)
{
  return _export_thumbnail;
}

/**
 * gimp_get_num_processors:
 *
 * Returns the number of threads set explicitly by the user in the
 * preferences. This information can be used by plug-ins wishing to
 * follow user settings for multi-threaded implementations.
 *
 * Returns: the preferred number of threads to use.
 *
 * Since: 3.0
 **/
gint32
gimp_get_num_processors (void)
{
  return _num_processors;
}

/**
 * gimp_check_size:
 *
 * Returns the size of the checkerboard to be used in previews.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Returns: the check_size value
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
 * Returns: the check_type value
 *
 * Since: 2.2
 **/
GimpCheckType
gimp_check_type (void)
{
  return _check_type;
}

/**
 * gimp_check_custom_color1:
 *
 * Returns the first checkerboard custom color that can
 * be used in previews.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Return value: the _check_custom_color1 value
 *
 * Since: 3.0
 **/
const GeglColor *
gimp_check_custom_color1 (void)
{
  return (const GeglColor *) _check_custom_color1;
}

/**
 * gimp_check_custom_color2:
 *
 * Returns the second checkerboard custom color that can
 * be used in previews.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Return value: the _check_custom_color2 value
 *
 * Since: 3.0
 **/
const GeglColor *
gimp_check_custom_color2 (void)
{
  return (const GeglColor *) _check_custom_color2;
}

/**
 * gimp_default_display:
 *
 * Returns the default display ID. This corresponds to the display the
 * running procedure's menu entry was invoked from.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Returns: (transfer none): the default display ID
 *          The object belongs to libgimp and you should not free it.
 **/
GimpDisplay *
gimp_default_display (void)
{
  return gimp_display_get_by_id (_default_display_id);
}

/**
 * gimp_wm_class:
 *
 * Returns the window manager class to be used for plug-in windows.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Returns: the window manager class
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
 * Will return %NULL if GIMP has been started with no GUI, either
 * via "--no-interface" flag, or a console build.
 *
 * Returns: the display name
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
 * Returns: the monitor number
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
 * Returns: timestamp for plug-in window
 *
 * Since: 2.6
 **/
guint32
gimp_user_time (void)
{
  return _timestamp;
}

/**
 * gimp_icon_theme_dir:
 *
 * Returns the directory of the current icon theme.
 *
 * This is a constant value given at plug-in configuration time.
 *
 * Returns: the icon theme directory
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
 * Returns: the executable name
 **/
const gchar *
gimp_get_progname (void)
{
  return progname;
}


/*  private functions  */

static void
gimp_close (void)
{
  if (_gimp_get_debug_flags () & GIMP_DEBUG_QUIT)
    _gimp_debug_stop ();

  gimp_base_exit ();

  _gimp_plug_in_quit (PLUG_IN);

  if (PDB)
    g_object_run_dispose (G_OBJECT (PDB));
  g_clear_object (&PDB);

  g_object_run_dispose (G_OBJECT (PLUG_IN));
  g_clear_object (&PLUG_IN);
}




#ifdef G_OS_WIN32

#ifdef HAVE_EXCHNDL
static LONG WINAPI
gimp_plugin_sigfatal_handler (PEXCEPTION_POINTERS pExceptionInfo)
{
  g_printerr ("Plugin signal handler: %s: fatal error\n", progname);

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

      stream = g_fopen (plug_in_backtrace_path, "r");
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
#endif /* HAVE_EXCHNDL */

#else /* ! G_OS_WIN32 */

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

#endif /* G_OS_WIN32 */

void
_gimp_config (GPConfig *config)
{
  GFile        *file;
  gchar        *path;
  gsize         bpp;
  const guint8 *pixel;
  const guint8 *icc;
  gsize         icc_length;
  const Babl   *format = NULL;
  const Babl   *space  = NULL;

  _tile_width           = config->tile_width;
  _tile_height          = config->tile_height;
  _check_size           = config->check_size;
  _check_type           = config->check_type;

  _show_help_button     = config->show_help_button ? TRUE : FALSE;
  _export_color_profile = config->export_color_profile   ? TRUE : FALSE;
  _export_exif          = config->export_exif      ? TRUE : FALSE;
  _export_xmp           = config->export_xmp       ? TRUE : FALSE;
  _export_iptc          = config->export_iptc      ? TRUE : FALSE;
  _export_comment       = config->export_comment;
  _num_processors       = config->num_processors;
  _default_display_id   = config->default_display_id;
  _wm_class             = g_strdup (config->wm_class);
  _display_name         = g_strdup (config->display_name);
  _monitor_number       = config->monitor_number;
  _timestamp            = config->timestamp;
  _icon_theme_dir       = g_strdup (config->icon_theme_dir);

  if (config->app_name)
    g_set_application_name (config->app_name);

  gimp_cpu_accel_set_use (config->use_cpu_accel);

  file = gimp_file_new_for_config_path (config->swap_path, NULL);
  path = g_file_get_path (file);

  g_object_set (gegl_config (),
                "tile-cache-size",     config->tile_cache_size,
                "swap",                path,
                "swap-compression",    config->swap_compression,
                "threads",             (gint) config->num_processors,
                "use-opencl",          config->use_opencl,
                "application-license", "GPL3",
                NULL);

  /* XXX Running gegl_init() before gegl_config() is not appreciated by
   * GEGL and generates a bunch of CRITICALs.
   */
  babl_init ();

  g_clear_object (&_check_custom_color1);
  _check_custom_color1 = gegl_color_new (NULL);
  pixel = g_bytes_get_data (config->check_custom_color1, &bpp);
  icc   = g_bytes_get_data (config->check_custom_icc1, &icc_length);
  space = babl_space_from_icc ((const char *) icc, (int) icc_length,
                               BABL_ICC_INTENT_RELATIVE_COLORIMETRIC,
                               NULL);
  format = babl_format_with_space (config->check_custom_encoding1, space);
  if (bpp != babl_format_get_bytes_per_pixel (format))
    {
      g_warning ("%s: checker board color 1's format expects %d bpp but %" G_GSIZE_FORMAT " bytes were passed.",
                 G_STRFUNC, babl_format_get_bytes_per_pixel (format), bpp);
      gegl_color_set_pixel (_check_custom_color1, babl_format ("R'G'B'A double"), GIMP_CHECKS_CUSTOM_COLOR1);
    }
  else
    {
      gegl_color_set_pixel (_check_custom_color1, format, pixel);
    }

  g_clear_object (&_check_custom_color2);
  _check_custom_color2 = gegl_color_new (NULL);
  pixel = g_bytes_get_data (config->check_custom_color2, &bpp);
  icc   = g_bytes_get_data (config->check_custom_icc2, &icc_length);
  space = babl_space_from_icc ((const char *) icc, (int) icc_length,
                               BABL_ICC_INTENT_RELATIVE_COLORIMETRIC,
                               NULL);
  format = babl_format_with_space (config->check_custom_encoding2, space);
  if (bpp != babl_format_get_bytes_per_pixel (format))
    {
      g_warning ("%s: checker board color 2's format expects %d bpp but %" G_GSIZE_FORMAT " bytes were passed.",
                 G_STRFUNC, babl_format_get_bytes_per_pixel (format), bpp);
      gegl_color_set_pixel (_check_custom_color2, babl_format ("R'G'B'A double"), GIMP_CHECKS_CUSTOM_COLOR2);
    }
  else
    {
      gegl_color_set_pixel (_check_custom_color2, format, pixel);
    }

  g_free (path);
  g_object_unref (file);

  _gimp_shm_open (config->shm_id);
}
