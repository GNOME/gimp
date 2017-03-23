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

#include "core/core-types.h"

#include "xcf-private.h"
#include "xcf-write.h"

#include "gimp-intl.h"


guint
xcf_write_int32 (XcfInfo        *info,
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

          xcf_write_int8 (info, (const guint8 *) &tmp, 4, &tmp_error);

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
xcf_write_offset (XcfInfo        *info,
                  const goffset  *data,
                  gint            count,
                  GError        **error)
{
  if (count > 0)
    {
      gint i;

      for (i = 0; i < count; i++)
        {
          GError *tmp_error = NULL;

          if (info->bytes_per_offset == 4)
            {
              guint32 tmp = g_htonl (data[i]);

              xcf_write_int8 (info, (const guint8 *) &tmp, 4, &tmp_error);
            }
          else
            {
              gint64 tmp = GINT64_TO_BE (data[i]);

              xcf_write_int8 (info, (const guint8 *) &tmp, 8, &tmp_error);
            }

          if (tmp_error)
            {
              g_propagate_error (error, tmp_error);

              return i * info->bytes_per_offset;
            }
        }
    }

  return count * info->bytes_per_offset;
}

guint
xcf_write_zero_offset (XcfInfo  *info,
                       gint      count,
                       GError  **error)
{
  if (count > 0)
    {
      guint8 *tmp = g_alloca (count * info->bytes_per_offset);

      memset (tmp, 0, count * info->bytes_per_offset);

      return xcf_write_int8 (info, (const guint8 *) tmp,
                             count * info->bytes_per_offset, error);
    }

  return 0;
}

guint
xcf_write_float (XcfInfo       *info,
                 const gfloat  *data,
                 gint           count,
                 GError       **error)
{
  return xcf_write_int32 (info,
                          (const guint32 *)((gconstpointer) data), count,
                          error);
}

guint
xcf_write_int8 (XcfInfo       *info,
                const guint8  *data,
                gint           count,
                GError       **error)
{
  GError *my_error = NULL;
  gsize   bytes_written;

  if (! g_output_stream_write_all (info->output, data, count,
                                   &bytes_written, NULL, &my_error))
    {
      g_propagate_prefixed_error (error, my_error,
                                  _("Error writing XCF: "));
    }

  info->cp += bytes_written;

  return bytes_written;
}

guint
xcf_write_string (XcfInfo  *info,
                  gchar   **data,
                  gint      count,
                  GError  **error)
{
  GError *tmp_error = NULL;
  guint   total     = 0;
  gint    i;

  for (i = 0; i < count; i++)
    {
      guint32 tmp;

      if (data[i])
        tmp = strlen (data[i]) + 1;
      else
        tmp = 0;

      xcf_write_int32 (info, &tmp, 1, &tmp_error);

      if (tmp_error)
        {
          g_propagate_error (error, tmp_error);
          return total;
        }

      if (tmp > 0)
        xcf_write_int8 (info, (const guint8 *) data[i], tmp, &tmp_error);

      if (tmp_error)
        {
          g_propagate_error (error, tmp_error);
          return total;
        }

      total += 4 + tmp;
    }

  return total;
}
