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

#include <string.h>

#include <gio/gio.h>

#include "xcf-write.h"


#include "gimp-intl.h"

guint
xcf_write_int32 (GOutputStream  *output,
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

          xcf_write_int8 (output, (const guint8 *) &tmp, 4, &tmp_error);

          if (tmp_error)
            {
              g_propagate_error (error, tmp_error);

              return i * 4;
            }
        }
    }

  return count * 4;
}

guint
xcf_write_offset (GOutputStream  *output,
                  const goffset  *data,
                  gint            count,
                  GError        **error)
{
  GError *tmp_error = NULL;
  gint    i;

  if (count > 0)
    {
      for (i = 0; i < count; i++)
        {
          guint32 tmp = g_htonl (data[i]);

          xcf_write_int8 (output, (const guint8 *) &tmp, 4, &tmp_error);

          if (tmp_error)
            {
              g_propagate_error (error, tmp_error);

              return i * 4;
            }
        }
    }

  return count * 4;
}

guint
xcf_write_zero_offset (GOutputStream  *output,
                       gint            count,
                       GError        **error)
{
  if (count > 0)
    {
      guint32 *tmp = g_alloca (count * 4);

      memset (tmp, 0, count * 4);

      return xcf_write_int8 (output, (const guint8 *) tmp, count * 4, error);
    }

  return 0;
}

guint
xcf_write_float (GOutputStream  *output,
                 const gfloat   *data,
                 gint            count,
                 GError        **error)
{
  return xcf_write_int32 (output,
                          (const guint32 *)((gconstpointer) data), count,
                          error);
}

guint
xcf_write_int8 (GOutputStream  *output,
                const guint8   *data,
                gint            count,
                GError        **error)
{
  GError *my_error = NULL;
  gsize   bytes_written;

  if (! g_output_stream_write_all (output, data, count,
                                   &bytes_written, NULL, &my_error))
    {
      g_propagate_prefixed_error (error, my_error,
                                  _("Error writing XCF: "));
    }

  return bytes_written;
}

guint
xcf_write_string (GOutputStream  *output,
                  gchar         **data,
                  gint            count,
                  GError        **error)
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

      xcf_write_int32 (output, &tmp, 1, &tmp_error);

      if (tmp_error)
        {
          g_propagate_error (error, tmp_error);
          return total;
        }

      if (tmp > 0)
        xcf_write_int8 (output, (const guint8 *) data[i], tmp, &tmp_error);

      if (tmp_error)
        {
          g_propagate_error (error, tmp_error);
          return total;
        }

      total += 4 + tmp;
    }

  return total;
}
