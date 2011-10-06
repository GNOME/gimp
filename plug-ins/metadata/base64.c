/* base64.h - encode and decode base64 encoding according to RFC 2045
 *
 * Copyright (C) 2005, Raphaël Quinet <raphael@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/* Yet another implementation of base64 encoding and decoding.  I
 * ended up writing this because most of the implementations that I
 * found required a null-terminated buffer, some of the others did not
 * ignore whitespace (especially those written for HTTP usage) and the
 * rest were not compatible with the LGPL (some were GPL, not LGPL).
 * Or at least I haven't been able to find LGPL implementations.
 * Writing this according to RFC 2045 did not take long anyway.
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

/**
 * base64_decode:
 * @src_b64: input buffer containing base64-encoded data
 * @src_size: input buffer size (in bytes) or -1 if @src_b64 is nul-terminated
 * @dest: buffer in which the decoded data should be stored
 * @dest_size: size of the destination buffer
 * @ignore_errors: if #TRUE, skip all invalid characters (no data validation)
 *
 * Read base64-encoded data from the input buffer @src_b64 and write
 * the decoded data into @dest.
 *
 * The base64 encoding uses 4 bytes for every 3 bytes of input, so
 * @dest_size should be at least 3/4 of @src_size (or less if the
 * input contains whitespace characters).  The base64 encoding has no
 * reliable EOF marker, so this can cause additional data following
 * the base64-encoded block to be misinterpreted if @src_size is not
 * specified correctly.  The decoder will stop at the first nul byte
 * or at the first '=' (padding byte) so you should ensure that one of
 * these is present if you supply -1 for @src_size.  For more details
 * about the base64 encoding, see RFC 2045, chapter 6.8.
 *
 * Returns: the number of bytes stored in @dest, or -1 if invalid data was found.
 */
gssize
base64_decode (const gchar *src_b64,
               gsize        src_size,
               gchar       *dest,
               gsize        dest_size,
               gboolean     ignore_errors)
{
  gint32 decoded;
  gssize i;
  gint   n;

  g_return_val_if_fail (src_b64 != NULL, -1);
  g_return_val_if_fail (dest != NULL, -1);
  decoded = 0;
  n = 0;

  for (i = 0; (src_size != 0) && (i + 3 <= dest_size); src_b64++, src_size--)
    {
      gint bits;

      bits = base64_6bits[(int) *src_b64 & 0xff];
      if (bits < 0)
        {
          if (bits == -2)
            break;
          else if ((bits == -3) && !ignore_errors)
            return -1;
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

/**
 * base64_encode:
 * @src: input buffer
 * @src_size: input buffer size (in bytes) or -1 if @src is nul-terminated
 * @dest_b64: buffer in which the base64 encoded data should be stored
 * @dest_size: size of the destination buffer
 * @columns: if > 0, add line breaks in the output after this many columns
 *
 * Read binary data from the input buffer @src and write
 * base64-encoded data into @dest_b64.
 *
 * Since the base64 encoding uses 4 bytes for every 3 bytes of input,
 * @dest_size should be at least 4/3 of @src_size, plus optional line
 * breaks if @columns > 0 and up to two padding bytes at the end.  For
 * more details about the base64 encoding, see RFC 2045, chapter 6.8.
 * Note that RFC 2045 recommends setting @columns to 76.
 *
 * Returns: the number of bytes stored in @dest.
 */
gssize
base64_encode (const gchar *src,
               gsize        src_size,
               gchar       *dest_b64,
               gsize        dest_size,
               gint         columns)
{
  guint32 bits;
  gssize  i;
  gint    n;
  gint    c;

  g_return_val_if_fail (src != NULL, -1);
  g_return_val_if_fail (dest_b64 != NULL, -1);

  n = 0;
  bits = 0;
  c = 0;
  for (i = 0; (src_size != 0) && (i + 4 <= dest_size); src++, src_size--)
    {
      bits += *(guchar *)src;
      if (++n == 3)
        {
          dest_b64[i++] = base64_code[(bits >> 18) & 0x3f];
          dest_b64[i++] = base64_code[(bits >> 12) & 0x3f];
          dest_b64[i++] = base64_code[(bits >> 6)  & 0x3f];
          dest_b64[i++] = base64_code[bits         & 0x3f];
          bits = 0;
          n = 0;
          if (columns > 0)
            {
              c += 4;
              if ((c >= columns) && (i < dest_size))
                {
                  dest_b64[i++] = '\n';
                  c = 0;
                }
            }
        }
      else
        {
          bits <<= 8;
        }
    }
  if ((n != 0) && (i + 4 <= dest_size))
    {
      if (n == 1)
        {
          dest_b64[i++] = base64_code[(bits >> 10) & 0x3f];
          dest_b64[i++] = base64_code[(bits >> 4)  & 0x3f];
          dest_b64[i++] = '=';
          dest_b64[i++] = '=';
	}
      else
        {
          dest_b64[i++] = base64_code[(bits >> 18) & 0x3f];
          dest_b64[i++] = base64_code[(bits >> 12) & 0x3f];
          dest_b64[i++] = base64_code[(bits >> 6)  & 0x3f];
          dest_b64[i++] = '=';
	}
    }
  if ((columns > 0) && ((c != 0) || (n != 0)) && (i + 1 < dest_size))
    dest_b64[i++] = '\n';
  if (i < dest_size)
    dest_b64[i] = 0;
  return i;
}
