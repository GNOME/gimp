/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#define _GNU_SOURCE  /* need the POSIX signal API */

#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core/core-types.h"

#include "core/gimp.h"

#include "errors.h"

#ifdef G_OS_WIN32
#include <windows.h>
#endif


/*  private variables  */

static Gimp                *the_errors_gimp   = NULL;
static gboolean             use_debug_handler = FALSE;
static GimpStackTraceMode   stack_trace_mode  = GIMP_STACK_TRACE_QUERY;
static gchar               *full_prog_name    = NULL;


/*  local function prototypes  */

static G_GNUC_NORETURN void  gimp_eek (const gchar *reason,
                                       const gchar *message,
                                       gboolean     use_handler);

static void   gimp_message_log_func (const gchar        *log_domain,
                                     GLogLevelFlags      flags,
                                     const gchar        *message,
                                     gpointer            data);
static void   gimp_error_log_func   (const gchar        *domain,
                                     GLogLevelFlags      flags,
                                     const gchar        *message,
                                     gpointer            data) G_GNUC_NORETURN;



/*  public functions  */

void
errors_init (Gimp               *gimp,
             const gchar        *_full_prog_name,
             gboolean            _use_debug_handler,
             GimpStackTraceMode  _stack_trace_mode)
{
  const gchar * const log_domains[] =
  {
    "Gimp",
    "Gimp-Actions",
    "Gimp-Base",
    "Gimp-Composite",
    "Gimp-Config",
    "Gimp-Core",
    "Gimp-Dialogs",
    "Gimp-Display",
    "Gimp-File",
    "Gimp-GUI",
    "Gimp-Menus",
    "Gimp-PDB",
    "Gimp-Paint",
    "Gimp-Paint-Funcs",
    "Gimp-Plug-In",
    "Gimp-Text",
    "Gimp-Tools",
    "Gimp-Vectors",
    "Gimp-Widgets",
    "Gimp-XCF"
  };
  gint i;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (_full_prog_name != NULL);
  g_return_if_fail (full_prog_name == NULL);

#ifdef GIMP_UNSTABLE
  g_printerr ("This is a development version of GIMP.  "
              "Debug messages may appear here.\n\n");
#endif /* GIMP_UNSTABLE */

  the_errors_gimp   = gimp;
  use_debug_handler = _use_debug_handler ? TRUE : FALSE;
  stack_trace_mode  = _stack_trace_mode;
  full_prog_name    = g_strdup (_full_prog_name);

  for (i = 0; i < G_N_ELEMENTS (log_domains); i++)
    g_log_set_handler (log_domains[i],
                       G_LOG_LEVEL_MESSAGE,
                       gimp_message_log_func, gimp);

  g_log_set_handler (NULL,
                     G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL,
                     gimp_error_log_func, gimp);
}

void
errors_exit (void)
{
  the_errors_gimp = NULL;
}

void
gimp_fatal_error (const gchar *fmt, ...)
{
  va_list  args;
  gchar   *message;

  va_start (args, fmt);
  message = g_strdup_vprintf (fmt, args);
  va_end (args);

  gimp_eek ("fatal error", message, TRUE);
}

void
gimp_terminate (const gchar *fmt, ...)
{
  va_list  args;
  gchar   *message;

  va_start (args, fmt);
  message = g_strdup_vprintf (fmt, args);
  va_end (args);

  gimp_eek ("terminated", message, use_debug_handler);
}


/*  private functions  */

static void
gimp_message_log_func (const gchar    *log_domain,
                       GLogLevelFlags  flags,
                       const gchar    *message,
                       gpointer        data)
{
  Gimp *gimp = data;

  if (gimp)
    {
      gimp_show_message (gimp, NULL, GIMP_MESSAGE_WARNING, NULL, message);
    }
  else
    {
      g_printerr ("%s: %s\n\n",
                  gimp_filename_to_utf8 (full_prog_name), message);
    }
}

static void
gimp_error_log_func (const gchar    *domain,
                     GLogLevelFlags  flags,
                     const gchar    *message,
                     gpointer        data)
{
  gimp_fatal_error (message);
}

static void
gimp_eek (const gchar *reason,
          const gchar *message,
          gboolean     use_handler)
{
#ifndef G_OS_WIN32
  g_printerr ("%s: %s: %s\n", gimp_filename_to_utf8 (full_prog_name),
              reason, message);

  if (use_handler)
    {
      switch (stack_trace_mode)
        {
        case GIMP_STACK_TRACE_NEVER:
          break;

        case GIMP_STACK_TRACE_QUERY:
          {
            sigset_t sigset;

            sigemptyset (&sigset);
            sigprocmask (SIG_SETMASK, &sigset, NULL);

            if (the_errors_gimp)
              gimp_gui_ungrab (the_errors_gimp);

            g_on_error_query (full_prog_name);
          }
          break;

        case GIMP_STACK_TRACE_ALWAYS:
          {
            sigset_t sigset;

            sigemptyset (&sigset);
            sigprocmask (SIG_SETMASK, &sigset, NULL);

            g_on_error_stack_trace (full_prog_name);
          }
          break;

        default:
          break;
        }
    }
#else

  /* g_on_error_* don't do anything reasonable on Win32. */

  MessageBox (NULL, g_strdup_printf ("%s: %s", reason, message),
              full_prog_name, MB_OK|MB_ICONERROR);

#endif /* ! G_OS_WIN32 */

  exit (EXIT_FAILURE);
}
