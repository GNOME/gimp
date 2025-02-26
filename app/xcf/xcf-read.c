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

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"

#include "core/core-types.h"

#include "xcf-private.h"
#include "xcf-read.h"

#include "gimp-intl.h"


#define MAX_XCF_STRING_LEN (16L * 1024 * 1024)


guint
xcf_read_int8 (XcfInfo *info,
               guint8  *data,
               gint     count)
{
  gsize bytes_read = 0;

  /* we allow for 'data == NULL && count == 0', which g_input_stream_read_all()
   * rejects.
   */
  if (count > 0)
    {
      g_input_stream_read_all (info->input, data, count,
                               &bytes_read, NULL, NULL);

      info->cp += bytes_read;
    }

  return bytes_read;
}

guint
xcf_read_int16 (XcfInfo *info,
                guint16 *data,
                gint     count)
{
  guint total = 0;

  if (count > 0)
    {
      total += xcf_read_int8 (info, (guint8 *) data, count * 2);

      while (count--)
        {
          *data = g_ntohs (*data);
          data++;
        }
    }

  return total;
}

guint
xcf_read_int32 (XcfInfo *info,
                guint32 *data,
                gint     count)
{
  guint total = 0;

  if (count > 0)
    {
      total += xcf_read_int8 (info, (guint8 *) data, count * 4);

      while (count--)
        {
          *data = g_ntohl (*data);
          data++;
        }
    }

  return total;
}

guint
xcf_read_int64 (XcfInfo *info,
                guint64 *data,
                gint     count)
{
  guint total = 0;

  if (count > 0)
    {
      total += xcf_read_int8 (info, (guint8 *) data, count * 8);

      while (count--)
        {
          *data = GINT64_FROM_BE (*data);
          data++;
        }
    }

  return total;
}

guint
xcf_read_offset (XcfInfo *info,
                 goffset *data,
                 gint     count)
{
  guint total = 0;

  *data = 0;
  if (count > 0)
    {
      if (info->bytes_per_offset == 4)
        {
          gint32 *int_offsets = g_alloca (count * sizeof (gint32));

          total += xcf_read_int8 (info, (guint8 *) int_offsets, count * 4);

          while (count--)
            {
              *data = g_ntohl (*int_offsets);
              int_offsets++;
              data++;
            }
        }
      else
        {
          total += xcf_read_int8 (info, (guint8 *) data, count * 8);

          while (count--)
            {
              *data = GINT64_FROM_BE (*data);
              data++;
            }
        }
    }

  return total;
}

guint
xcf_read_float (XcfInfo *info,
                gfloat  *data,
                gint     count)
{
  return xcf_read_int32 (info, (guint32 *) ((void *) data), count);
}

guint
xcf_read_string (XcfInfo  *info,
                 gchar   **data,
                 gint      count)
{
  guint total = 0;
  gint  i;

  for (i = 0; i < count; i++)
    {
      guint32 tmp;

      total += xcf_read_int32 (info, &tmp, 1);

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
          total += xcf_read_int8 (info, (guint8*) str, tmp);

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

guint
xcf_read_component (XcfInfo *info,
                    gint     bpc,
                    guint8  *data,
                    gint     count)
{
  switch (bpc)
    {
    case 1:
      return xcf_read_int8 (info, data, count);

    case 2:
      return xcf_read_int16 (info, (guint16 *) data, count);

    case 4:
      return xcf_read_int32 (info, (guint32 *) data, count);

    case 8:
      return xcf_read_int64 (info, (guint64 *) data, count);

    default:
      break;
    }

  return 0;
}

void
xcf_read_from_be (gint    bpc,
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
          d[i] = g_ntohs (d[i]);
      }
      break;

    case 4:
      {
        guint32 *d = (guint32 *) data;

        for (i = 0; i < count; i++)
          d[i] = g_ntohl (d[i]);
      }
      break;

    case 8:
      {
        guint64 *d = (guint64 *) data;

        for (i = 0; i < count; i++)
          d[i] = GINT64_FROM_BE (d[i]);
      }
      break;
    }
}
