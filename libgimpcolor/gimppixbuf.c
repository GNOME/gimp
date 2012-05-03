/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppixbuf.c
 * Copyright (C) 2012  Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gimpcolortypes.h"

#include "gimppixbuf.h"


/**
 * gimp_pixbuf_get_format:
 * @pixbuf: a #GdkPixbuf
 *
 * Returns the Babl format that corresponds to the @pixbuf's pixel format.
 *
 * Return value: the @pixbuf's pixel format
 *
 * Since: GIMP 2.10
 **/
const Babl *
gimp_pixbuf_get_format (GdkPixbuf *pixbuf)
{
  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  switch (gdk_pixbuf_get_n_channels (pixbuf))
    {
    case 3: return babl_format ("R'G'B' u8");
    case 4: return babl_format ("R'G'B'A u8");
    }

  g_return_val_if_reached (NULL);
}

/**
 * gimp_pixbuf_create_buffer:
 * @pixbuf: a #GdkPixbuf
 *
 * Returns a #GeglBuffer that's backed by the @pixbuf's pixels, without
 * copying them. This function refs the pixbuf, so it will be kept
 * around for as long as te buffer exists.
 *
 * Return value: a new #GeglBuffer as a wrapper around @pixbuf.
 *
 * Since: GIMP 2.10
 **/
GeglBuffer *
gimp_pixbuf_create_buffer (GdkPixbuf *pixbuf)
{
  gint width;
  gint height;
  gint rowstride;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  width     = gdk_pixbuf_get_width (pixbuf);
  height    = gdk_pixbuf_get_height (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  return gegl_buffer_linear_new_from_data (gdk_pixbuf_get_pixels (pixbuf),
                                           gimp_pixbuf_get_format (pixbuf),
                                           GEGL_RECTANGLE (0, 0, width, height),
                                           rowstride,
                                           (GDestroyNotify) g_object_unref,
                                           g_object_ref (pixbuf));
}
