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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <glib.h>
#include "app_procs.h"
#include "errors.h"

extern char *prog_name;

void
message (char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  printf ("%s: ", prog_name);
  vprintf (fmt, args);
  printf ("\n");
  va_end (args);
}

void
warning (char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  printf ("%s warning: ", prog_name);
  vprintf (fmt, args);
  printf ("\n");
  va_end (args);
}

void
fatal_error (char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  printf ("%s fatal error: ", prog_name);
  vprintf (fmt, args);
  printf ("\n");
  va_end (args);

  g_debug (prog_name);
  app_exit (1);
}
