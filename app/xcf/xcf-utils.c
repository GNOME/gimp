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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "xcf-utils.h"


gboolean
xcf_data_is_zero (const void *data,
                  gint        size)
{
  const guint8  *data8;
  const guint64 *data64;

  for (data8 = data; size > 0 && (guintptr) data8 % 8 != 0; data8++, size--)
    {
      if (*data8)
        return FALSE;
    }

  for (data64 = (gpointer) data8; size >= 8; data64++, size -= 8)
    {
      if (*data64)
        return FALSE;
    }

  for (data8 = (gpointer) data64; size > 0; data8++, size--)
    {
      if (*data8)
        return FALSE;
    }

  return TRUE;
}
