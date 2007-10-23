/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This code is a slightly modified copy of xdg-user-dir-lockup.c
 * as written by Alexander Larsson.
 *
 * See http://www.freedesktop.org/wiki/Software/xdg-user-dirs
 */

/*
  This file is not licenced under the GPL like the rest of the code.
  Its is under the MIT license, to encourage reuse by cut-and-paste.

  Copyright (c) 2007 Red Hat, inc

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "config.h"

#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "xdg-user-dir.h"


/**
 * xdg_user_dir_lookup_with_fallback:
 * @type: a string specifying the type of directory
 *
 * Looks up a XDG user directory of the specified type.
 * Example of types are "DESKTOP" and "DOWNLOAD".
 *
 * Return value: a newly allocated absolute pathname or %NULL
 **/
gchar *
_xdg_user_dir_lookup (const gchar *type)
{
  FILE        *file;
  const gchar *home_dir;
  const gchar *config_home;
  gchar        buffer[512];
  gchar       *filename;
  gchar       *user_dir = NULL;
  gchar       *p, *d;
  gint         len;
  gint         relative;

  home_dir = g_get_home_dir ();

  if (! home_dir)
    return NULL;

  config_home = g_getenv ("XDG_CONFIG_HOME");

  if (! config_home || ! *config_home)
    {
      filename = g_build_filename (home_dir, ".config", "user-dirs.dirs", NULL);
    }
  else
    {
      filename = g_build_filename (config_home, "user-dirs.dirs", NULL);
    }

  file = g_fopen (filename, "r");

  g_free (filename);

  if (! file)
    return NULL;

  while (fgets (buffer, sizeof (buffer), file))
    {
      /* Remove newline at end */
      len = strlen (buffer);
      if (len > 0 && buffer[len-1] == '\n')
	buffer[len-1] = 0;

      p = buffer;
      while (*p == ' ' || *p == '\t')
	p++;

      if (strncmp (p, "XDG_", 4) != 0)
	continue;
      p += 4;

      if (strncmp (p, type, strlen (type)) != 0)
	continue;
      p += strlen (type);

      if (strncmp (p, "_DIR", 4) != 0)
	continue;
      p += 4;

      while (*p == ' ' || *p == '\t')
	p++;

      if (*p != '=')
	continue;
      p++;

      while (*p == ' ' || *p == '\t')
	p++;

      if (*p != '"')
	continue;
      p++;

      relative = 0;
      if (strncmp (p, "$HOME/", 6) == 0)
	{
	  p += 6;
	  relative = 1;
	}
      else if (*p != '/')
	continue;

      if (relative)
	{
	  user_dir = g_malloc (strlen (home_dir) + 1 + strlen (p) + 1);
	  strcpy (user_dir, home_dir);
	  strcat (user_dir, "/");
	}
      else
	{
	  user_dir = g_malloc (strlen (p) + 1);
	  *user_dir = 0;
	}

      d = user_dir + strlen (user_dir);
      while (*p && *p != '"')
	{
	  if ((*p == '\\') && (*(p+1) != 0))
	    p++;
	  *d++ = *p++;
	}
      *d = 0;
    }

  fclose (file);

  return user_dir;
}
