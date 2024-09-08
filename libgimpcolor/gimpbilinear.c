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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
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


/**
 * gimp_bilinear:
 * @x:
 * @y:
 * @values: (array fixed-size=4):
 */
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

/**
 * gimp_bilinear_8:
 * @x:
 * @y:
 * @values: (array fixed-size=4):
 */
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

/**
 * gimp_bilinear_16:
 * @x:
 * @y:
 * @values: (array fixed-size=4):
 */
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

/**
 * gimp_bilinear_32:
 * @x:
 * @y:
 * @values: (array fixed-size=4):
 */
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

/**
 * gimp_bilinear_rgb:
 * @x:
 * @y:
 * @values:    (array fixed-size=16): Array of pixels in RGBA double format
 * @has_alpha: Whether @values has an alpha channel
 * @retvalues: (array fixed-size=4):  Resulting pixel
 */
void
gimp_bilinear_rgb (gdouble    x,
                   gdouble    y,
                   gdouble   *values,
                   gboolean   has_alpha,
                   gdouble   *retvalues)
{
  gdouble  m0;
  gdouble  m1;
  gdouble  ix;
  gdouble  iy;
  gdouble  a[4]  = { 1.0, 1.0, 1.0, 1.0 };
  gdouble  alpha = 1.0;

  for (gint i = 0; i < 3; i++)
    retvalues[i] = 0.0;
  retvalues[3] = 1.0;

  g_return_if_fail (values != NULL);

  x = fmod (x, 1.0);
  y = fmod (y, 1.0);

  if (x < 0)
    x += 1.0;
  if (y < 0)
    y += 1.0;

  ix = 1.0 - x;
  iy = 1.0 - y;

  if (has_alpha)
    {
      for (gint i = 0; i < 4; i++)
        a[i] = values[(i * 4) + 3];

      m0 = ix * a[0] + x * a[1];
      m1 = ix * a[2] + x * a[3];

      alpha = retvalues[3] = iy * m0 + y * m1;
    }

  if (alpha > 0)
    {
      for (gint i = 0; i < 3; i++)
        {
          m0 = ix * a[0] * values[0 + i] + x * a[1] * values[4 + i];
          m1 = ix * a[2] * values[8 + i] + x * a[3] * values[12 + i];

          retvalues[i] = (iy * m0 + y * m1) / alpha;
        }
    }
}
