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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gio/gio.h>

#include "core/core-types.h"

#include "xcf-private.h"
#include "xcf-write.h"

#include "gimp-intl.h"


guint
xcf_write_int8 (XcfInfo       *info,
                const guint8  *data,
                gint           count,
                GError       **error)
{
  GError *my_error      = NULL;
  gsize   bytes_written = 0;

  /* we allow for 'data == NULL && count == 0', which
   * g_output_stream_write_all() rejects.
   */
  if (count > 0)
    {
      if (! g_output_stream_write_all (info->output, data, count,
                                       &bytes_written, NULL, &my_error))
        {
          g_propagate_prefixed_error (error, my_error,
                                      _("Error writing XCF: "));
        }

      info->cp += bytes_written;
    }

  return bytes_written;
}

guint
xcf_write_int16 (XcfInfo        *info,
                 const guint16  *data,
                 gint            count,
                 GError        **error)
{
  GError *tmp_error = NULL;
  gint    i;

  if (count > 0)
    {
      for (i = 0; i < count; i++)
        {
          guint16 tmp = g_htons (data[i]);

          xcf_write_int8 (info, (const guint8 *) &tmp, 2, &tmp_error);

          if (tmp_error)
            {
              g_propagate_error (error, tmp_error);

              return i * 2;
            }
        }
    }

  return count * 2;
}

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
xcf_write_int64 (XcfInfo        *info,
                 const guint64  *data,
                 gint            count,
                 GError        **error)
{
  GError *tmp_error = NULL;
  gint    i;

  if (count > 0)
    {
      for (i = 0; i < count; i++)
        {
          guint64 tmp = GINT64_TO_BE (data[i]);

          xcf_write_int8 (info, (const guint8 *) &tmp, 8, &tmp_error);

          if (tmp_error)
            {
              g_propagate_error (error, tmp_error);

              return i * 8;
            }
        }
    }

  return count * 8;
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
      guint8 *tmp;
      guint   bytes_written = 0;

      tmp = g_try_malloc (count * info->bytes_per_offset);
      if (! tmp)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Error writing XCF: failed to allocate %d bytes of memory."),
                       count * info->bytes_per_offset);
        }
      else
        {
          memset (tmp, 0, count * info->bytes_per_offset);

          bytes_written = xcf_write_int8 (info, (const guint8 *) tmp,
                                          count * info->bytes_per_offset, error);
          g_free (tmp);
        }

      return bytes_written;
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

guint
xcf_write_component (XcfInfo       *info,
                     gint           bpc,
                     const guint8  *data,
                     gint           count,
                     GError       **error)
{
  switch (bpc)
    {
    case 1:
      return xcf_write_int8 (info, data, count, error);

    case 2:
      return xcf_write_int16 (info, (const guint16 *) data, count, error);

    case 4:
      return xcf_write_int32 (info, (const guint32 *) data, count, error);

    case 8:
      return xcf_write_int64 (info, (const guint64 *) data, count, error);

    default:
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error writing XCF: unsupported BPC when writing pixel: %d"),
                   bpc);
    }

  return 0;
}

void
xcf_write_to_be (gint    bpc,
                 guint8 *data,
                 gint    count)
{
  gint i;

  switch (bpc)
    {
    case 1:
      break;

    case 2:
      {
        guint16 *d = (guint16 *) data;

        for (i = 0; i < count; i++)
          d[i] = g_htons (d[i]);
      }
      break;

    case 4:
      {
        guint32 *d = (guint32 *) data;

        for (i = 0; i < count; i++)
          d[i] = g_htonl (d[i]);
      }
      break;

    case 8:
      {
        guint64 *d = (guint64 *) data;

        for (i = 0; i < count; i++)
          d[i] = GINT64_TO_BE (d[i]);
      }
      break;
    }
}
