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
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#include <sys/types.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>
#include "appenv.h"
#include "app_procs.h"
#include "errorconsole.h"
#include "errors.h"
#include "gimpui.h"

#include "libgimp/gimpintl.h"

#ifdef G_OS_WIN32
#include <windows.h>
#endif

extern gchar *prog_name;

void
gimp_message_func (gchar *str)
{
  if (console_messages == FALSE)
    switch (message_handler)
      {
        case MESSAGE_BOX:
          gimp_message_box (str, NULL, NULL);
          break;

        case ERROR_CONSOLE:
          error_console_add (str);
          break;

        default:
          fprintf (stderr, "%s: %s\n", prog_name, str);
	  break;
    }
  else
    fprintf (stderr, "%s: %s\n", prog_name, str);
}

void
gimp_fatal_error (gchar *fmt, ...)
{
#ifndef G_OS_WIN32
  va_list args;

  va_start (args, fmt);
  printf (_("%s: fatal error: %s\n"), prog_name, g_strdup_vprintf (fmt, args));
  va_end (args);
  g_on_error_query (prog_name);
#else
  /* g_on_error_query doesn't do anything reasonable on Win32. */
  va_list args;
  gchar *msg;

  va_start (args, fmt);
  msg = g_strdup_vprintf (fmt, args);
  va_end (args);

  MessageBox (NULL, msg, prog_name, MB_OK|MB_ICONERROR);
  /* I don't dare do anything more. */
  ExitProcess (1);
#endif
  app_exit (TRUE);
}

void
gimp_terminate (gchar *fmt, ...)
{
#ifndef G_OS_WIN32
  va_list args;

  va_start (args, fmt);
  printf ("%s terminated: ", prog_name);
  vprintf (fmt, args);
  printf ("\n");
  va_end (args);

  if (use_debug_handler)
    g_on_error_query (prog_name);
#else
  /* g_on_error_query doesn't do anything reasonable on Win32. */
  va_list args;
  gchar *msg;

  va_start (args, fmt);
  msg = g_strdup_vprintf (fmt, args);
  va_end (args);

  MessageBox (NULL, msg, prog_name, MB_OK|MB_ICONERROR);
#endif
  gdk_exit (1);
}
