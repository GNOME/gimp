/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 * Copyright (C) 2000 Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>
#include <glib.h>

/**
 * gimp_strescape:
 * @source: A string to escape special characters in.
 * @exceptions: A string holding all characters that are to be escaped.
 * 
 * Escapes special characters in a string, by inserting a '\' before them. 
 * If the list of characters is NULL, a suitable set of exceptions is used. 
 *
 * If glib > 1.3 is installed this function is identical to 
 * g_strescape(). For systems using glib-1.2 this function provides the 
 * added functionality from glib-1.3.
 * 
 * Returns: A newly allocated copy of the string, with all special
 * characters escaped using a backslash.  
 */
#if !(defined (GLIB_CHECK_VERSION) && GLIB_CHECK_VERSION (1,3,1))
gchar* 
gimp_strescape (const gchar *source, 
		const gchar *exceptions)
{
  const guchar *p = (guchar *) source;
  /* Each source byte needs maximally four destination chars (\777) */
  gchar *dest = g_malloc (strlen (source) * 4 + 1);
  gchar *q = dest;
  guchar excmap[256];

  memset (excmap, 0, 256);
  if (exceptions)
    {
      guchar *e = (guchar *) exceptions;

      while (*e)
	{
	  excmap[*e] = 1;
	  e++;
	}
    }

  while (*p)
    {
      if (excmap[*p])
	*q++ = *p;
      else
	{
	  switch (*p)
	    {
	    case '\b':
	      *q++ = '\\';
	      *q++ = 'b';
	      break;
	    case '\f':
	      *q++ = '\\';
	      *q++ = 'f';
	      break;
	    case '\n':
	      *q++ = '\\';
	      *q++ = 'n';
	      break;
	    case '\r':
	      *q++ = '\\';
	      *q++ = 'r';
	      break;
	    case '\t':
	      *q++ = '\\';
	      *q++ = 't';
	      break;
	    case '\\':
	      *q++ = '\\';
	      *q++ = '\\';
	      break;
	    case '"':
	      *q++ = '\\';
	      *q++ = '"';
	      break;
	    default:
	      if ((*p < ' ') || (*p >= 0177))
		{
		  *q++ = '\\';
		  *q++ = '0' + (((*p) >> 6) & 07);
		  *q++ = '0' + (((*p) >> 3) & 07);
		  *q++ = '0' + ((*p) & 07);
		}
	      else
		*q++ = *p;
	      break;
	    }
	}
      p++;
    }
  *q = 0;
  return dest;
}
#endif  /* GLIB <= 1.3 */
