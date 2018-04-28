/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "gimpcolortypes.h"

#include "gimpbilinear.h"


/**
 * SECTION: gimpbilinear
 * @title: GimpBilinear
 * @short_description: Utility functions for bilinear interpolation.
 *
 * Utility functions for bilinear interpolation.
 **/


gdouble
gimp_bilinear (gdouble  x,
               gdouble  y,
               gdouble *values)
{
  gdouble m0, m1;

  g_return_val_if_fail (values != NULL, 0.0);

  x = fmod (x, 1.0);
  y = fmod (y, 1.0);

  if (x < 0.0)
    x += 1.0;
  if (y < 0.0)
    y += 1.0;

  m0 = (1.0 - x) * values[0] + x * values[1];
  m1 = (1.0 - x) * values[2] + x * values[3];

  return (1.0 - y) * m0 + y * m1;
}

guchar
gimp_bilinear_8 (gdouble x,
                 gdouble y,
                 guchar *values)
{
  gdouble m0, m1;

  g_return_val_if_fail (values != NULL, 0);

  x = fmod (x, 1.0);
  y = fmod (y, 1.0);

  if (x < 0.0)
    x += 1.0;
  if (y < 0.0)
    y += 1.0;

  m0 = (1.0 - x) * values[0] + x * values[1];
  m1 = (1.0 - x) * values[2] + x * values[3];

  return (guchar) ((1.0 - y) * m0 + y * m1);
}

guint16
gimp_bilinear_16 (gdouble  x,
                  gdouble  y,
                  guint16 *values)
{
  gdouble m0, m1;

  g_return_val_if_fail (values != NULL, 0);

  x = fmod (x, 1.0);
  y = fmod (y, 1.0);

  if (x < 0.0)
    x += 1.0;
  if (y < 0.0)
    y += 1.0;

  m0 = (1.0 - x) * values[0] + x * values[1];
  m1 = (1.0 - x) * values[2] + x * values[3];

  return (guint16) ((1.0 - y) * m0 + y * m1);
}

guint32
gimp_bilinear_32 (gdouble  x,
                  gdouble  y,
                  guint32 *values)
{
  gdouble m0, m1;

  g_return_val_if_fail (values != NULL, 0);

  x = fmod (x, 1.0);
  y = fmod (y, 1.0);

  if (x < 0.0)
    x += 1.0;
  if (y < 0.0)
    y += 1.0;

  m0 = (1.0 - x) * values[0] + x * values[1];
  m1 = (1.0 - x) * values[2] + x * values[3];

  return (guint32) ((1.0 - y) * m0 + y * m1);
}

GimpRGB
gimp_bilinear_rgb (gdouble  x,
                   gdouble  y,
                   GimpRGB *values)
{
  gdouble m0, m1;
  gdouble ix, iy;
  GimpRGB v = { 0, };

  g_return_val_if_fail (values != NULL, v);

  x = fmod(x, 1.0);
  y = fmod(y, 1.0);

  if (x < 0)
    x += 1.0;
  if (y < 0)
    y += 1.0;

  ix = 1.0 - x;
  iy = 1.0 - y;

  /* Red */

  m0 = ix * values[0].r + x * values[1].r;
  m1 = ix * values[2].r + x * values[3].r;

  v.r = iy * m0 + y * m1;

  /* Green */

  m0 = ix * values[0].g + x * values[1].g;
  m1 = ix * values[2].g + x * values[3].g;

  v.g = iy * m0 + y * m1;

  /* Blue */

  m0 = ix * values[0].b + x * values[1].b;
  m1 = ix * values[2].b + x * values[3].b;

  v.b = iy * m0 + y * m1;

  return v;
}

GimpRGB
gimp_bilinear_rgba (gdouble  x,
                    gdouble  y,
                    GimpRGB *values)
{
  gdouble m0, m1;
  gdouble ix, iy;
  gdouble a0, a1, a2, a3, alpha;
  GimpRGB v = { 0, };

  g_return_val_if_fail (values != NULL, v);

  x = fmod (x, 1.0);
  y = fmod (y, 1.0);

  if (x < 0)
    x += 1.0;
  if (y < 0)
    y += 1.0;

  ix = 1.0 - x;
  iy = 1.0 - y;

  a0 = values[0].a;
  a1 = values[1].a;
  a2 = values[2].a;
  a3 = values[3].a;

  /* Alpha */

  m0 = ix * a0 + x * a1;
  m1 = ix * a2 + x * a3;

  alpha = v.a = iy * m0 + y * m1;

  if (alpha > 0)
    {
      /* Red */

      m0 = ix * a0 * values[0].r + x * a1 * values[1].r;
      m1 = ix * a2 * values[2].r + x * a3 * values[3].r;

      v.r = (iy * m0 + y * m1)/alpha;

      /* Green */

      m0 = ix * a0 * values[0].g + x * a1 * values[1].g;
      m1 = ix * a2 * values[2].g + x * a3 * values[3].g;

      v.g = (iy * m0 + y * m1)/alpha;

      /* Blue */

      m0 = ix * a0 * values[0].b + x * a1 * values[1].b;
      m1 = ix * a2 * values[2].b + x * a3 * values[3].b;

      v.b = (iy * m0 + y * m1)/alpha;
    }

  return v;
}
