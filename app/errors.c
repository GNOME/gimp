/* The GIMP -- an image manipulation program
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

#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#include "core/core-types.h"

#include "widgets/gimpwidgets-utils.h"

#include "appenv.h"
#include "errorconsole.h"
#include "errors.h"

#ifdef G_OS_WIN32
#include <windows.h>
#endif


void
gimp_message_log_func (const gchar    *log_domain,
		       GLogLevelFlags  flags,
		       const gchar    *message,
		       gpointer        data)
{
  if (console_messages == FALSE)
    {
      switch (message_handler)
	{
	case MESSAGE_BOX:
	  gimp_message_box ((gchar *) message, NULL, NULL);
	  break;

	case ERROR_CONSOLE:
	  error_console_add ((gchar *) message);
	  break;

	default:
	  g_printerr ("%s: %s\n", prog_name, (gchar *) message);
	  break;
	}
    }
  else
    {
      g_printerr ("%s: %s\n", prog_name, (gchar *) message);
    }
}

void
gimp_error_log_func (const gchar    *domain,
		     GLogLevelFlags  flags,
		     const gchar    *message,
		     gpointer        data)
{
  gimp_fatal_error ("%s", message);
}

void
gimp_fatal_error (const gchar *fmt, ...)
{
  va_list  args;
  gchar   *message;

  va_start (args, fmt);
  message = g_strdup_vprintf (fmt, args);
  va_end (args);

#ifndef G_OS_WIN32

  g_printerr ("%s: fatal error: %s\n", prog_name, message);

  switch (stack_trace_mode)
    {
    case STACK_TRACE_NEVER:
      break;

    case STACK_TRACE_QUERY:
      {
	sigset_t sigset;

	sigemptyset (&sigset);
	sigprocmask (SIG_SETMASK, &sigset, NULL);
	g_on_error_query (prog_name);
      }
      break;

    case STACK_TRACE_ALWAYS:
      {
	sigset_t sigset;

	sigemptyset (&sigset);
	sigprocmask (SIG_SETMASK, &sigset, NULL);
	g_on_error_stack_trace (prog_name);
      }
      break;

    default:
      break;
    }

#else

  /* g_on_error_* don't do anything reasonable on Win32. */

  MessageBox (NULL, message, prog_name, MB_OK|MB_ICONERROR);

#endif /* ! G_OS_WIN32 */

  gdk_exit (1);
}

void
gimp_terminate (const gchar *fmt, ...)
{
  va_list  args;
  gchar   *message;

  va_start (args, fmt);
  message = g_strdup_vprintf (fmt, args);
  va_end (args);

#ifndef G_OS_WIN32

  g_printerr ("%s terminated: %s\n", prog_name, message);

  if (use_debug_handler)
    {
      switch (stack_trace_mode)
	{
	case STACK_TRACE_NEVER:
	  break;

	case STACK_TRACE_QUERY:
	  {
	    sigset_t sigset;

	    sigemptyset (&sigset);
	    sigprocmask (SIG_SETMASK, &sigset, NULL);
	    g_on_error_query (prog_name);
	  }
	  break;

	case STACK_TRACE_ALWAYS:
	  {
	    sigset_t sigset;

	    sigemptyset (&sigset);
	    sigprocmask (SIG_SETMASK, &sigset, NULL);
	    g_on_error_stack_trace (prog_name);
	  }
	  break;

	default:
	  break;
	}
    }

#else

  /* g_on_error_* don't do anything reasonable on Win32. */

  MessageBox (NULL, message, prog_name, MB_OK|MB_ICONERROR);

#endif /* ! G_OS_WIN32 */

  gdk_exit (1);
}
