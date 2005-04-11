/* base64.h - encode and decode base64 encoding according to RFC 1521
 *
 * Copyright (C) 2005, Raphaël Quinet <raphael@gimp.org>
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

/* Yet another implementation of base64 decoding.  I ended up writing this
 * because most of the implementations that I found required a null-terminated
 * buffer, some of the others did not ignore whitespace (especially those
 * written for HTTP usage) and the rest were not compatible with the LGPL.  Or
 * at least I haven't been able to find LGPL implementations.  Writing this
 * according to RFC 1521 did not take long anyway.
 */

#ifndef WITHOUT_GIMP
#  include "config.h"
#  include <string.h>
#  include "base64.h"
#  include "libgimp/stdplugins-intl.h"
#else
#  include <string.h>
#  include "base64.h"
#  define _(String) (String)
#  define N_(String) (String)
#endif

#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

static const gchar base64_code[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const gint base64_6bits[256] =
{
  -2, -3, -3, -3, -3, -3, -3, -3, -3, -1, -1, -1, -1, -1, -3, -3,
  -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
  -1, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 62, -3, -3, -3, 63,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -3, -3, -3, -2, -3, -3,
  -3,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -3, -3, -3, -3, -3,
  -3, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -3, -3, -3, -3, -3,
  -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
  -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
  -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
  -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
  -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
  -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
  -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
  -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3
}; /* -1: skip whitespace, -2: end of input, -3: error */

/*
 * FIXME: update this comment...
 * - dest_size should be at least 3/4 of strlen (src) minus all CRLF
 * - returns the number of bytes stored in dest
 */
gint
base64_decode (const gchar *src_b64,
               guint        src_size,
               gchar       *dest,
               guint        dest_size)
{
  gint32 decoded;
  gint   i;
  gint   n;
  gint   bits;

  g_return_val_if_fail (src_b64 != NULL, -1);
  decoded = 0;
  n = 0;
  bits = -3;
  for (i = 0; src_size && (i + 3 <= dest_size); src_b64++, src_size--)
    {
      bits = base64_6bits[(int) *src_b64 & 0xff];
      if (bits < 0)
        {
          if (bits < -1)
            break;
          else
            continue;
        }
      decoded <<= 6;
      decoded += bits;
      if (++n >= 4)
        {
          /* we have decoded 4 source chars => 24 bits of output (3 chars) */
          dest[i++] = decoded >> 16;
          dest[i++] = (decoded >> 8) & 0xff;
          dest[i++] = decoded & 0xff;
          decoded = 0;
          n = 0;
        }
    }
  if (bits < -2)
    return -1;
  if ((n == 3) && (i + 2 <= dest_size))
    {
      /* 3 source chars (+ 1 padding "=") => 16 bits of output (2 chars) */
      dest[i++] = decoded >> 10;
      dest[i++] = (decoded >> 2) & 0xff;
    }
  else if ((n == 2) && (i + 1 <= dest_size))
    {
      /* 2 source chars (+ 2 padding "=") => 8 bits of output (1 char) */
      dest[i++] = decoded >> 4;
    }
  if (i < dest_size)
    dest[i] = 0;
  return i;
}

gint
base64_encode (const gchar *src,
               guint        src_size,
               gchar       *dest_b64,
               guint        dest_size)
{
  /* FIXME: just index the base64_code string, 6 bits at a time */
  g_warning ("not written yet\n");
  return 1;
}
