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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gimpcolortypes.h"

#include "gimppixbuf.h"


/**
 * SECTION: gimppixbuf
 * @title: GimpPixbuf
 * @short_description: Definitions and Functions relating to GdkPixbuf.
 *
 * Definitions and Functions relating to GdkPixbuf.
 **/

/**
 * gimp_pixbuf_get_format:
 * @pixbuf: a #GdkPixbuf
 *
 * Returns the Babl format that corresponds to the @pixbuf's pixel format.
 *
 * Returns: the @pixbuf's pixel format
 *
 * Since: 2.10
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
 * Returns a #GeglBuffer that's either backed by the @pixbuf's pixels,
 * or a copy of them. This function tries to not copy the @pixbuf's
 * pixels. If the pixbuf's rowstride is a multiple of its bpp, a
 * simple reference to the @pixbuf's pixels is made and @pixbuf will
 * be kept around for as long as the buffer exists; otherwise the
 * pixels are copied.
 *
 * Returns: (transfer full): a new #GeglBuffer.
 *
 * Since: 2.10
 **/
GeglBuffer *
gimp_pixbuf_create_buffer (GdkPixbuf *pixbuf)
{
  gint width;
  gint height;
  gint rowstride;
  gint bpp;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  width     = gdk_pixbuf_get_width (pixbuf);
  height    = gdk_pixbuf_get_height (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  bpp       = gdk_pixbuf_get_n_channels (pixbuf);

  if ((rowstride % bpp) == 0)
    {
      return gegl_buffer_linear_new_from_data (gdk_pixbuf_get_pixels (pixbuf),
                                               gimp_pixbuf_get_format (pixbuf),
                                               GEGL_RECTANGLE (0, 0,
                                                               width, height),
                                               rowstride,
                                               (GDestroyNotify) g_object_unref,
                                               g_object_ref (pixbuf));
    }
  else
    {
      GeglBuffer *buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                            width, height),
                                            gimp_pixbuf_get_format (pixbuf));

      gegl_buffer_set (buffer, NULL, 0, NULL,
                       gdk_pixbuf_get_pixels (pixbuf),
                       gdk_pixbuf_get_rowstride (pixbuf));

      return buffer;
    }
}

/**
 * gimp_pixbuf_get_icc_profile:
 * @pixbuf: a #GdkPixbuf
 * @length: (out): return location for the ICC profile's length
 *
 * Returns the ICC profile attached to the @pixbuf, or %NULL if there
 * is none.
 *
 * Returns: (array length=length) (nullable): The ICC profile data, or %NULL.
 *          The value should be freed with g_free().
 *
 * Since: 2.10
 **/
guint8 *
gimp_pixbuf_get_icc_profile (GdkPixbuf *pixbuf,
                             gsize     *length)
{
  const gchar *icc_base64;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);
  g_return_val_if_fail (length != NULL, NULL);

  icc_base64 = gdk_pixbuf_get_option (pixbuf, "icc-profile");

  if (icc_base64)
    {
      guint8 *icc_data;

      icc_data = g_base64_decode (icc_base64, length);

      return icc_data;
    }

  return NULL;
}
