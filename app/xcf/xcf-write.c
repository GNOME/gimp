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
#include <string.h> /* strlen */
#include <errno.h>

#include <glib.h>

#include "xcf-write.h"


#include "gimp-intl.h"

/**
 * SECTION:xcf-write
 * @Short_description:XCF writing functions
 *
 * Low-level XCF writing functions
 */

/**
 * xcf_write_int32:
 * @fp:     output file stream
 * @data:   source data array
 * @count:  number of words to write
 * @error:  container for occurred errors
 *
 * Write @count 4-byte-words from @data to @fp.
 *
 * Returns: count (in numbers of bytes, not words)
 */
guint
xcf_write_int32 (FILE           *fp,
                 const guint32  *data,
                 gint            count,
                 GError        **error)
{
  GError  *tmp_error = NULL;
  gint     i;

  if (count > 0)
    {
      for (i = 0; i < count; i++)
        {
          guint32  tmp = g_htonl (data[i]);

          xcf_write_int8 (fp, (const guint8 *) &tmp, 4, &tmp_error);

          if (tmp_error)
            {
              g_propagate_error (error, tmp_error);

              return i * 4;
            }
        }
    }

  return count * 4;
}

/**
 * xcf_write_float:
 * @fp:     output file stream
 * @data:   source data array
 * @count:  number of words to write
 * @error:  container for occurred errors
 *
 * Write @count float values from @data to @fp.
 *
 * Returns: count (in numbers of bytes, not words)
 */
guint
xcf_write_float (FILE           *fp,
                 const gfloat   *data,
                 gint            count,
                 GError        **error)
{
  return xcf_write_int32 (fp,
                          (const guint32 *)((gconstpointer) data), count,
                          error);
}

/**
 * xcf_write_int8:
 * @fp:     output file stream
 * @data:   source data array
 * @count:  number of bytes to write
 * @error:  container for occurred errors
 *
 * Write @count bytes of unsigned integer from @data to @fp.
 *
 * Returns: @count
 */

/* TODO: shouldn't the return value (i.e. local variable total) mean the number of written bytes? */
guint
xcf_write_int8 (FILE           *fp,
                const guint8   *data,
                gint            count,
                GError        **error)
{
  guint total = count;

  while (count > 0)
    {
      gint bytes = fwrite ((const gchar*) data, sizeof (gchar), count, fp);

      if (bytes != count)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Error writing XCF: %s"), g_strerror (errno));

          return total;
        }

      count -= bytes;
      data += bytes;
    }

  return total;
}

/**
 * xcf_write_string:
 * @fp:     output file stream
 * @data:   source data array
 * @count:  number of strings to write
 * @error:  container for occurred errors
 *
 * Write @count bytes of unsigned integer from @data to @fp.
 *
 * Returns: number of written bytes
 */
guint
xcf_write_string (FILE     *fp,
                  gchar   **data,
                  gint      count,
                  GError  **error)
{
  GError  *tmp_error = NULL;
  guint    total     = 0;
  gint     i;

  for (i = 0; i < count; i++)
    {
      guint32 tmp;

      if (data[i])
        tmp = strlen (data[i]) + 1;
      else
        tmp = 0;

      xcf_write_int32 (fp, &tmp, 1, &tmp_error);

      if (tmp_error)
        {
          g_propagate_error (error, tmp_error);
          return total;
        }

      if (tmp > 0)
        xcf_write_int8 (fp, (const guint8 *) data[i], tmp, &tmp_error);

      if (tmp_error)
        {
          g_propagate_error (error, tmp_error);
          return total;
        }

      total += 4 + tmp;
    }

  return total;
}
