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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core/core-types.h"

#include "gegl/gimp-babl.h"

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

/* calculates the (discrete) derivative of a sequence of pixels.  the input
 * sequence, given by `src`, shall be a derivative of order `from_order` of
 * some original sequence; the output sequence, written to `dest`, is the
 * derivative of order `to_order`, which shall be greater than or equal to
 * `from_order`, of the said original sequence.
 *
 * `dest` and `src` may be equal, but may not otherwise partially overlap.
 *
 * returns TRUE if successful, or FALSE otherwise.  upon failure, the contents
 * of `dest` are unspecified.
 */
gboolean
xcf_data_differentiate (void       *dest,
                        const void *src,
                        gint        n_pixels,
                        const Babl *format,
                        gint        from_order,
                        gint        to_order)
{
  gint bpp;
  gint n_components;
  gint o;
  gint i;
  gint c;

  g_return_val_if_fail (dest != NULL, FALSE);
  g_return_val_if_fail (src != NULL, FALSE);
  g_return_val_if_fail (n_pixels >= 0, FALSE);
  g_return_val_if_fail (format != NULL, FALSE);
  g_return_val_if_fail (from_order >= 0 && from_order <= to_order, FALSE);

  if (to_order == 0 && n_pixels == 0)
    return TRUE;
  else if (to_order >= n_pixels)
    return FALSE;

  bpp          = babl_format_get_bytes_per_pixel (format);
  n_components = babl_format_get_n_components    (format);

  if (from_order == to_order)
    {
      if (dest != src)
        memcpy (dest, src, n_pixels * bpp);

      return TRUE;
    }

  #define XCF_DATA_DIFFERENTIATE(type)                                         \
    do                                                                         \
      {                                                                        \
        type       *d = dest;                                                  \
        const type *s = src;                                                   \
                                                                               \
        if (dest != src)                                                       \
          memcpy (dest, src, (from_order + 1) * bpp);                          \
                                                                               \
        for (o = from_order; o < to_order; o++)                                \
          {                                                                    \
            for (i = n_pixels - 1; i > o; i--)                                 \
              {                                                                \
                for (c = 0; c < n_components; c++)                             \
                  {                                                            \
                    d[n_components * i + c] = s[n_components *  i      + c] -  \
                                              s[n_components * (i - 1) + c];   \
                  }                                                            \
              }                                                                \
                                                                               \
            s = d;                                                             \
          }                                                                    \
      }                                                                        \
    while (0)

  switch (gimp_babl_format_get_component_type (format))
    {
    case GIMP_COMPONENT_TYPE_U8:
      XCF_DATA_DIFFERENTIATE (guint8);
      break;

    case GIMP_COMPONENT_TYPE_U16:
    case GIMP_COMPONENT_TYPE_HALF:
      XCF_DATA_DIFFERENTIATE (guint16);
      break;

    case GIMP_COMPONENT_TYPE_U32:
    case GIMP_COMPONENT_TYPE_FLOAT:
      XCF_DATA_DIFFERENTIATE (guint32);
      break;

    case GIMP_COMPONENT_TYPE_DOUBLE:
      XCF_DATA_DIFFERENTIATE (guint64);
      break;

    default:
      return FALSE;
    }

  #undef XCF_DATA_DIFFERENTIATE

  return TRUE;
}

/* calculates the (discrete) integral of a sequence of pixels.  the input
 * sequence, given by `src`, shall be a derivative of order `from_order` of
 * some original sequence; the output sequence, written to `dest`, is the
 * derivative of order `to_order`, which shall be less than or equal to
 * `from_order`, of the said original sequence.
 *
 * `dest` and `src` may be equal, but may not otherwise partially overlap.
 *
 * returns TRUE if successful, or FALSE otherwise.  upon failure, the contents
 * of `dest` are unspecified.
 */
gboolean
xcf_data_integrate (void       *dest,
                    const void *src,
                    gint        n_pixels,
                    const Babl *format,
                    gint        from_order,
                    gint        to_order)
{
  gint bpp;
  gint n_components;
  gint o;
  gint i;
  gint c;

  g_return_val_if_fail (dest != NULL, FALSE);
  g_return_val_if_fail (src != NULL, FALSE);
  g_return_val_if_fail (n_pixels >= 0, FALSE);
  g_return_val_if_fail (format != NULL, FALSE);
  g_return_val_if_fail (to_order >= 0 && to_order <= from_order, FALSE);

  if (from_order == 0 && n_pixels == 0)
    return TRUE;
  else if (from_order >= n_pixels)
    return FALSE;

  bpp          = babl_format_get_bytes_per_pixel (format);
  n_components = babl_format_get_n_components    (format);

  if (from_order == to_order)
    {
      if (dest != src)
        memcpy (dest, src, n_pixels * bpp);

      return TRUE;
    }

  #define XCF_DATA_INTEGRATE(type)                                             \
    do                                                                         \
      {                                                                        \
        type       *d = dest;                                                  \
        const type *s = src;                                                   \
                                                                               \
        if (dest != src)                                                       \
          memcpy (dest, src, from_order * bpp);                                \
                                                                               \
        for (o = from_order; o > to_order; o--)                                \
          {                                                                    \
            for (i = o; i < n_pixels; i++)                                     \
              {                                                                \
                for (c = 0; c < n_components; c++)                             \
                  {                                                            \
                    d[n_components * i + c] = s[n_components *  i      + c] +  \
                                              d[n_components * (i - 1) + c];   \
                  }                                                            \
              }                                                                \
                                                                               \
            s = d;                                                             \
          }                                                                    \
      }                                                                        \
    while (0)

  switch (gimp_babl_format_get_component_type (format))
    {
    case GIMP_COMPONENT_TYPE_U8:
      XCF_DATA_INTEGRATE (guint8);
      break;

    case GIMP_COMPONENT_TYPE_U16:
    case GIMP_COMPONENT_TYPE_HALF:
      XCF_DATA_INTEGRATE (guint16);
      break;

    case GIMP_COMPONENT_TYPE_U32:
    case GIMP_COMPONENT_TYPE_FLOAT:
      XCF_DATA_INTEGRATE (guint32);
      break;

    case GIMP_COMPONENT_TYPE_DOUBLE:
      XCF_DATA_INTEGRATE (guint64);
      break;

    default:
      return FALSE;
    }

  #undef XCF_DATA_INTEGRATE

  return TRUE;
}
