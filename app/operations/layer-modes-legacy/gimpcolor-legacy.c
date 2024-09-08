/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcolor-legacy.c
 * Copyright (C) 2024
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

#include "../operations-types.h"

#include "gimpcolor-legacy.h"


#define GIMP_HSL_UNDEFINED -1.0


void
gimp_rgb_to_hsv_legacy (gdouble *rgb,
                        gdouble *hsv)
{
  gdouble max, min, delta;

  g_return_if_fail (rgb != NULL);
  g_return_if_fail (hsv != NULL);

  max = MAX (rgb[0], MAX (rgb[1], rgb[2]));
  min = MIN (rgb[0], MIN (rgb[1], rgb[2]));

  hsv[2] = max;
  delta = max - min;

  if (delta > 0.0001)
    {
      hsv[1] = delta / max;

      if (rgb[0] == max)
        {
          hsv[0] = (rgb[1] - rgb[2]) / delta;
          if (hsv[0] < 0.0)
            hsv[0] += 6.0;
        }
      else if (rgb[1] == max)
        {
          hsv[0] = 2.0 + (rgb[2] - rgb[0]) / delta;
        }
      else
        {
          hsv[0] = 4.0 + (rgb[0] - rgb[1]) / delta;
        }

      hsv[0] /= 6.0;
    }
  else
    {
      hsv[1] = 0.0;
      hsv[0] = 0.0;
    }

  hsv[3] = rgb[3];
}

void
gimp_hsv_to_rgb_legacy (gdouble *hsv,
                        gdouble *rgb)
{
  gint    i;
  gdouble f, w, q, t;

  gdouble hue;

  g_return_if_fail (rgb != NULL);
  g_return_if_fail (hsv != NULL);

  if (hsv[1] == 0.0)
    {
      rgb[0] = hsv[2];
      rgb[1] = hsv[2];
      rgb[2] = hsv[2];
    }
  else
    {
      hue = hsv[0];

      if (hue == 1.0)
        hue = 0.0;

      hue *= 6.0;

      i = (gint) hue;
      f = hue - i;
      w = hsv[2] * (1.0 - hsv[1]);
      q = hsv[2] * (1.0 - (hsv[1] * f));
      t = hsv[2] * (1.0 - (hsv[1] * (1.0 - f)));

      switch (i)
        {
        case 0:
          rgb[0] = hsv[2];
          rgb[1] = t;
          rgb[2] = w;
          break;
        case 1:
          rgb[0] = q;
          rgb[1] = hsv[2];
          rgb[2] = w;
          break;
        case 2:
          rgb[0] = w;
          rgb[1] = hsv[2];
          rgb[2] = t;
          break;
        case 3:
          rgb[0] = w;
          rgb[1] = q;
          rgb[2] = hsv[2];
          break;
        case 4:
          rgb[0] = t;
          rgb[1] = w;
          rgb[2] = hsv[2];
          break;
        case 5:
          rgb[0] = hsv[2];
          rgb[1] = w;
          rgb[2] = q;
          break;
        }
    }

  rgb[3] = hsv[3];
}

static inline gdouble
gimp_hsl_value (gdouble n1,
                gdouble n2,
                gdouble hue)
{
  gdouble val;

  if (hue > 6.0)
    hue -= 6.0;
  else if (hue < 0.0)
    hue += 6.0;

  if (hue < 1.0)
    val = n1 + (n2 - n1) * hue;
  else if (hue < 3.0)
    val = n2;
  else if (hue < 4.0)
    val = n1 + (n2 - n1) * (4.0 - hue);
  else
    val = n1;

  return val;
}

void
gimp_rgb_to_hsl_legacy (gdouble *rgb,
                        gdouble *hsl)
{
  gdouble max, min, delta;

  g_return_if_fail (rgb != NULL);
  g_return_if_fail (hsl != NULL);

  max = MAX (rgb[0], MAX (rgb[1], rgb[2]));
  min = MIN (rgb[0], MIN (rgb[1], rgb[2]));

  hsl[2] = (max + min) / 2.0;

  if (max == min)
    {
      hsl[1] = 0.0;
      hsl[0] = GIMP_HSL_UNDEFINED;
    }
  else
    {
      if (hsl[2] <= 0.5)
        hsl[1] = (max - min) / (max + min);
      else
        hsl[1] = (max - min) / (2.0 - max - min);

      delta = max - min;

      if (delta == 0.0)
        delta = 1.0;

      if (rgb[0] == max)
        {
          hsl[0] = (rgb[1] - rgb[2]) / delta;
        }
      else if (rgb[1] == max)
        {
          hsl[0] = 2.0 + (rgb[2] - rgb[0]) / delta;
        }
      else
        {
          hsl[0] = 4.0 + (rgb[0] - rgb[1]) / delta;
        }

      hsl[0] /= 6.0;

      if (hsl[0] < 0.0)
        hsl[0] += 1.0;
    }

  hsl[3] = rgb[3];
}

void
gimp_hsl_to_rgb_legacy (gdouble *hsl,
                        gdouble *rgb)
{
  g_return_if_fail (hsl != NULL);
  g_return_if_fail (rgb != NULL);

  if (hsl[1] == 0)
    {
      /*  achromatic case  */
      rgb[0] = hsl[2];
      rgb[1] = hsl[2];
      rgb[2] = hsl[2];
    }
  else
    {
      gdouble m1, m2;

      if (hsl[2] <= 0.5)
        m2 = hsl[2] * (1.0 + hsl[1]);
      else
        m2 = hsl[2] + hsl[1] - hsl[2] * hsl[1];

      m1 = 2.0 * hsl[2] - m2;

      rgb[0] = gimp_hsl_value (m1, m2, hsl[0] * 6.0 + 2.0);
      rgb[1] = gimp_hsl_value (m1, m2, hsl[0] * 6.0);
      rgb[2] = gimp_hsl_value (m1, m2, hsl[0] * 6.0 - 2.0);
    }

  rgb[3] = hsl[3];
}
