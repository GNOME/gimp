/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdio.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "xcf-read.h"

#include "gimp-intl.h"

/**
 * SECTION:xcf-read
 * @Short_description:XCF reading functions
 *
 * Low-level XCF reading functions
 */

/**
 * MAX_XCF_STRING_LEN:
<<<<<<< Upstream, based on origin/osx-build
 *
 * Maximum length of a string in an XCF file ((16L * 1024 * 1024) bytes)
=======
 * @MAX_XCF_STRING_LEN: maximum length of a string in an XCF file
 * ((16L * 1024 * 1024) bytes)
>>>>>>> 059857a API-doc minor fixes
 */
#define MAX_XCF_STRING_LEN (16L * 1024 * 1024)

/**
 * xcf_read_int32:
 * @fp:    input file stream
 * @data:  destination data array
 * @count: number of words to read
 *
 * Read @count 4-byte-words from @fp into @data.
 * The functions respects the machine specific byte order.
 *
 * Returns: number of read bytes (not words)
 */
guint
xcf_read_int32 (FILE    *fp,
                guint32 *data,
                gint     count)
{
  guint total = 0;

  if (count > 0)
    {
      total += xcf_read_int8 (fp, (guint8 *) data, count * 4);

      while (count--)
        {
          *data = g_ntohl (*data);
          data++;
        }
    }

  return total;
}

/**
 * xcf_read_float:
 * @fp:    input file stream
 * @data:  destination data array
 * @count: number of words to read
 *
 * Read @count float values from @fp into @data.
 *
 * Returns: number of read bytes
 */
guint
xcf_read_float (FILE   *fp,
                gfloat *data,
                gint    count)
{
  return xcf_read_int32 (fp, (guint32 *) ((void *) data), count);
}

/**
 * xcf_read_int8:
 * @fp:    input file stream
 * @data:  destination data array
 * @count: number of bytes to read
 *
 * Read @count bytes from @fp into @data.
 *
 * Returns: number of read bytes
 */
guint
xcf_read_int8 (FILE   *fp,
               guint8 *data,
               gint    count)
{
  guint total = 0;

  while (count > 0)
    {
      gint bytes = fread ((char *) data, sizeof (char), count, fp);

      if (bytes <= 0) /* something bad happened */
        break;

      total += bytes;

      count -= bytes;
      data += bytes;
    }

  return total;
}

/**
 * xcf_read_string:
 * @fp:    input file stream
 * @data:  destination data array
 * @count: number of strings to read
 *
 * Read @count bytes from @fp into @data
 * and convert them to UTF8.
 *
 * Returns: number of read bytes
 */
guint
xcf_read_string (FILE   *fp,
                 gchar **data,
                 gint    count)
{
  guint total = 0;
  gint  i;

  for (i = 0; i < count; i++)
    {
      guint32 tmp;

      total += xcf_read_int32 (fp, &tmp, 1);

      if (tmp > MAX_XCF_STRING_LEN)
        {
          g_warning ("Maximum string length (%ld bytes) exceeded. "
                     "Possibly corrupt XCF file.", MAX_XCF_STRING_LEN);
          data[i] = NULL;
        }
      else if (tmp > 0)
        {
          gchar *str;

          str = g_new (gchar, tmp);
          total += xcf_read_int8 (fp, (guint8*) str, tmp);

          if (str[tmp - 1] != '\0')
            str[tmp - 1] = '\0';

          data[i] = gimp_any_to_utf8 (str, -1,
                                      _("Invalid UTF-8 string in XCF file"));

          g_free (str);
        }
      else
        {
          data[i] = NULL;
        }
    }

  return total;
}
