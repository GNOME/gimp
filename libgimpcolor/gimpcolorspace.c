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

#include <babl/babl.h>
#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "gimpcolortypes.h"

#include "gimpcolorspace.h"
#include "gimprgb.h"
#include "gimphsv.h"



/**
 * SECTION: gimpcolorspace
 * @title: GimpColorSpace
 * @short_description: Utility functions which convert colors between
 *                     different color models.
 *
 * When programming pixel data manipulation functions you will often
 * use algorithms operating on a color model different from the one
 * GIMP uses.  This file provides utility functions to convert colors
 * between different color spaces.
 **/


#define GIMP_HSV_UNDEFINED -1.0
#define GIMP_HSL_UNDEFINED -1.0

/*********************************
 *   color conversion routines   *
 *********************************/


/*  GimpRGB functions  */


/**
 * gimp_rgb_to_hsv:
 * @rgb: A color value in the RGB colorspace
 * @hsv: The value converted to the HSV colorspace
 *
 * Does a conversion from RGB to HSV (Hue, Saturation,
 * Value) colorspace.
 **/
void
gimp_rgb_to_hsv (const GimpRGB *rgb,
                 GimpHSV       *hsv)
{
  gdouble max, min, delta;

  g_return_if_fail (rgb != NULL);
  g_return_if_fail (hsv != NULL);

  max = gimp_rgb_max (rgb);
  min = gimp_rgb_min (rgb);

  hsv->v = max;
  delta = max - min;

  if (delta > 0.0001)
    {
      hsv->s = delta / max;

      if (rgb->r == max)
        {
          hsv->h = (rgb->g - rgb->b) / delta;
          if (hsv->h < 0.0)
            hsv->h += 6.0;
        }
      else if (rgb->g == max)
        {
          hsv->h = 2.0 + (rgb->b - rgb->r) / delta;
        }
      else
        {
          hsv->h = 4.0 + (rgb->r - rgb->g) / delta;
        }

      hsv->h /= 6.0;
    }
  else
    {
      hsv->s = 0.0;
      hsv->h = 0.0;
    }

  hsv->a = rgb->a;
}

/**
 * gimp_hsv_to_rgb:
 * @hsv: A color value in the HSV colorspace
 * @rgb: The returned RGB value.
 *
 * Converts a color value from HSV to RGB colorspace
 **/
void
gimp_hsv_to_rgb (const GimpHSV *hsv,
                 GimpRGB       *rgb)
{
  gint    i;
  gdouble f, w, q, t;

  gdouble hue;

  g_return_if_fail (rgb != NULL);
  g_return_if_fail (hsv != NULL);

  if (hsv->s == 0.0)
    {
      rgb->r = hsv->v;
      rgb->g = hsv->v;
      rgb->b = hsv->v;
    }
  else
    {
      hue = hsv->h;

      if (hue == 1.0)
        hue = 0.0;

      hue *= 6.0;

      i = (gint) hue;
      f = hue - i;
      w = hsv->v * (1.0 - hsv->s);
      q = hsv->v * (1.0 - (hsv->s * f));
      t = hsv->v * (1.0 - (hsv->s * (1.0 - f)));

      switch (i)
        {
        case 0:
          rgb->r = hsv->v;
          rgb->g = t;
          rgb->b = w;
          break;
        case 1:
          rgb->r = q;
          rgb->g = hsv->v;
          rgb->b = w;
          break;
        case 2:
          rgb->r = w;
          rgb->g = hsv->v;
          rgb->b = t;
          break;
        case 3:
          rgb->r = w;
          rgb->g = q;
          rgb->b = hsv->v;
          break;
        case 4:
          rgb->r = t;
          rgb->g = w;
          rgb->b = hsv->v;
          break;
        case 5:
          rgb->r = hsv->v;
          rgb->g = w;
          rgb->b = q;
          break;
        }
    }

  rgb->a = hsv->a;
}


/**
 * gimp_rgb_to_hsl:
 * @rgb: A color value in the RGB colorspace
 * @hsl: The value converted to HSL
 *
 * Convert an RGB color value to a HSL (Hue, Saturation, Lightness)
 * color value.
 **/
void
gimp_rgb_to_hsl (const GimpRGB *rgb,
                 GimpHSL       *hsl)
{
  gdouble max, min, delta;

  g_return_if_fail (rgb != NULL);
  g_return_if_fail (hsl != NULL);

  max = gimp_rgb_max (rgb);
  min = gimp_rgb_min (rgb);

  hsl->l = (max + min) / 2.0;

  if (max == min)
    {
      hsl->s = 0.0;
      hsl->h = GIMP_HSL_UNDEFINED;
    }
  else
    {
      if (hsl->l <= 0.5)
        hsl->s = (max - min) / (max + min);
      else
        hsl->s = (max - min) / (2.0 - max - min);

      delta = max - min;

      if (delta == 0.0)
        delta = 1.0;

      if (rgb->r == max)
        {
          hsl->h = (rgb->g - rgb->b) / delta;
        }
      else if (rgb->g == max)
        {
          hsl->h = 2.0 + (rgb->b - rgb->r) / delta;
        }
      else
        {
          hsl->h = 4.0 + (rgb->r - rgb->g) / delta;
        }

      hsl->h /= 6.0;

      if (hsl->h < 0.0)
        hsl->h += 1.0;
    }

  hsl->a = rgb->a;
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


/**
 * gimp_hsl_to_rgb:
 * @hsl: A color value in the HSL colorspace
 * @rgb: The value converted to a value in the RGB colorspace
 *
 * Convert a HSL color value to an RGB color value.
 **/
void
gimp_hsl_to_rgb (const GimpHSL *hsl,
                 GimpRGB       *rgb)
{
  g_return_if_fail (hsl != NULL);
  g_return_if_fail (rgb != NULL);

  if (hsl->s == 0)
    {
      /*  achromatic case  */
      rgb->r = hsl->l;
      rgb->g = hsl->l;
      rgb->b = hsl->l;
    }
  else
    {
      gdouble m1, m2;

      if (hsl->l <= 0.5)
        m2 = hsl->l * (1.0 + hsl->s);
      else
        m2 = hsl->l + hsl->s - hsl->l * hsl->s;

      m1 = 2.0 * hsl->l - m2;

      rgb->r = gimp_hsl_value (m1, m2, hsl->h * 6.0 + 2.0);
      rgb->g = gimp_hsl_value (m1, m2, hsl->h * 6.0);
      rgb->b = gimp_hsl_value (m1, m2, hsl->h * 6.0 - 2.0);
    }

  rgb->a = hsl->a;
}


/**
 * gimp_rgb_to_cmyk:
 * @rgb: A value in the RGB colorspace
 * @pullout: A scaling value (0-1) indicating how much black should be
 *           pulled out
 * @cmyk: The input value naively converted to the CMYK colorspace
 *
 * Does a naive conversion from RGB to CMYK colorspace. A simple
 * formula that doesn't take any color-profiles into account is used.
 * The amount of black pullout how can be controlled via the @pullout
 * parameter. A @pullout value of 0 makes this a conversion to CMY.
 * A value of 1 causes the maximum amount of black to be pulled out.
 **/
void
gimp_rgb_to_cmyk (const GimpRGB  *rgb,
                  gdouble         pullout,
                  GimpCMYK       *cmyk)
{
  gdouble c, m, y, k;

  g_return_if_fail (rgb != NULL);
  g_return_if_fail (cmyk != NULL);

  c = 1.0 - rgb->r;
  m = 1.0 - rgb->g;
  y = 1.0 - rgb->b;

  k = 1.0;
  if (c < k)  k = c;
  if (m < k)  k = m;
  if (y < k)  k = y;

  k *= pullout;

  if (k < 1.0)
    {
      cmyk->c = (c - k) / (1.0 - k);
      cmyk->m = (m - k) / (1.0 - k);
      cmyk->y = (y - k) / (1.0 - k);
    }
  else
    {
      cmyk->c = 0.0;
      cmyk->m = 0.0;
      cmyk->y = 0.0;
    }

  cmyk->k = k;
  cmyk->a = rgb->a;
}

/**
 * gimp_cmyk_to_rgb:
 * @cmyk: A color value in the CMYK colorspace
 * @rgb: The value converted to the RGB colorspace
 *
 * Does a simple transformation from the CMYK colorspace to the RGB
 * colorspace, without taking color profiles into account.
 **/
void
gimp_cmyk_to_rgb (const GimpCMYK *cmyk,
                  GimpRGB        *rgb)
{
  gdouble c, m, y, k;

  g_return_if_fail (cmyk != NULL);
  g_return_if_fail (rgb != NULL);

  k = cmyk->k;

  if (k < 1.0)
    {
      c = cmyk->c * (1.0 - k) + k;
      m = cmyk->m * (1.0 - k) + k;
      y = cmyk->y * (1.0 - k) + k;
    }
  else
    {
      c = 1.0;
      m = 1.0;
      y = 1.0;
    }

  rgb->r = 1.0 - c;
  rgb->g = 1.0 - m;
  rgb->b = 1.0 - y;
  rgb->a = cmyk->a;
}


#define GIMP_RETURN_RGB(x, y, z) { rgb->r = x; rgb->g = y; rgb->b = z; return; }

/****************************************************************************
 * Theoretically, hue 0 (pure red) is identical to hue 6 in these transforms.
 * Pure red always maps to 6 in this implementation. Therefore UNDEFINED can
 * be defined as 0 in situations where only unsigned numbers are desired.
 ****************************************************************************/


/*  gint functions  */

/**
 * gimp_rgb_to_hsv_int:
 * @red: The red channel value, returns the Hue channel
 * @green: The green channel value, returns the Saturation channel
 * @blue: The blue channel value, returns the Value channel
 *
 * The arguments are pointers to int representing channel values in
 * the RGB colorspace, and the values pointed to are all in the range
 * [0, 255].
 *
 * The function changes the arguments to point to the HSV value
 * corresponding, with the returned values in the following
 * ranges: H [0, 359], S [0, 255], V [0, 255].
 **/
void
gimp_rgb_to_hsv_int (gint *red,
                     gint *green,
                     gint *blue)
{
  gdouble  r, g, b;
  gdouble  h, s, v;
  gint     min;
  gdouble  delta;

  r = *red;
  g = *green;
  b = *blue;

  if (r > g)
    {
      v = MAX (r, b);
      min = MIN (g, b);
    }
  else
    {
      v = MAX (g, b);
      min = MIN (r, b);
    }

  delta = v - min;

  if (v == 0.0)
    s = 0.0;
  else
    s = delta / v;

  if (s == 0.0)
    {
      h = 0.0;
    }
  else
    {
      if (r == v)
        h = 60.0 * (g - b) / delta;
      else if (g == v)
        h = 120 + 60.0 * (b - r) / delta;
      else
        h = 240 + 60.0 * (r - g) / delta;

      if (h < 0.0)
        h += 360.0;

      if (h > 360.0)
        h -= 360.0;
    }

  *red   = ROUND (h);
  *green = ROUND (s * 255.0);
  *blue  = ROUND (v);

  /* avoid the ambiguity of returning different values for the same color */
  if (*red == 360)
    *red = 0;
}

/**
 * gimp_hsv_to_rgb_int:
 * @hue: The hue channel, returns the red channel
 * @saturation: The saturation channel, returns the green channel
 * @value: The value channel, returns the blue channel
 *
 * The arguments are pointers to int, with the values pointed to in the
 * following ranges:  H [0, 360], S [0, 255], V [0, 255].
 *
 * The function changes the arguments to point to the RGB value
 * corresponding, with the returned values all in the range [0, 255].
 **/
void
gimp_hsv_to_rgb_int (gint *hue,
                     gint *saturation,
                     gint *value)
{
  gdouble h, s, v, h_temp;
  gdouble f, p, q, t;
  gint i;

  if (*saturation == 0)
    {
      *hue        = *value;
      *saturation = *value;
      *value      = *value;
    }
  else
    {
      h = *hue;
      s = *saturation / 255.0;
      v = *value      / 255.0;

      if (h == 360)
        h_temp = 0;
      else
        h_temp = h;

      h_temp = h_temp / 60.0;
      i = floor (h_temp);
      f = h_temp - i;
      p = v * (1.0 - s);
      q = v * (1.0 - (s * f));
      t = v * (1.0 - (s * (1.0 - f)));

      switch (i)
        {
        case 0:
          *hue        = ROUND (v * 255.0);
          *saturation = ROUND (t * 255.0);
          *value      = ROUND (p * 255.0);
          break;

        case 1:
          *hue        = ROUND (q * 255.0);
          *saturation = ROUND (v * 255.0);
          *value      = ROUND (p * 255.0);
          break;

        case 2:
          *hue        = ROUND (p * 255.0);
          *saturation = ROUND (v * 255.0);
          *value      = ROUND (t * 255.0);
          break;

        case 3:
          *hue        = ROUND (p * 255.0);
          *saturation = ROUND (q * 255.0);
          *value      = ROUND (v * 255.0);
          break;

        case 4:
          *hue        = ROUND (t * 255.0);
          *saturation = ROUND (p * 255.0);
          *value      = ROUND (v * 255.0);
          break;

        case 5:
          *hue        = ROUND (v * 255.0);
          *saturation = ROUND (p * 255.0);
          *value      = ROUND (q * 255.0);
          break;
        }
    }
}

/**
 * gimp_rgb_to_hsl_int:
 * @red: Red channel, returns Hue channel
 * @green: Green channel, returns Lightness channel
 * @blue: Blue channel, returns Saturation channel
 *
 * The arguments are pointers to int representing channel values in the
 * RGB colorspace, and the values pointed to are all in the range [0, 255].
 *
 * The function changes the arguments to point to the corresponding HLS
 * value with the values pointed to in the following ranges:  H [0, 360],
 * L [0, 255], S [0, 255].
 **/
void
gimp_rgb_to_hsl_int (gint *red,
                     gint *green,
                     gint *blue)
{
  gint    r, g, b;
  gdouble h, s, l;
  gint    min, max;
  gint    delta;

  r = *red;
  g = *green;
  b = *blue;

  if (r > g)
    {
      max = MAX (r, b);
      min = MIN (g, b);
    }
  else
    {
      max = MAX (g, b);
      min = MIN (r, b);
    }

  l = (max + min) / 2.0;

  if (max == min)
    {
      s = 0.0;
      h = 0.0;
    }
  else
    {
      delta = (max - min);

      if (l < 128)
        s = 255 * (gdouble) delta / (gdouble) (max + min);
      else
        s = 255 * (gdouble) delta / (gdouble) (511 - max - min);

      if (r == max)
        h = (g - b) / (gdouble) delta;
      else if (g == max)
        h = 2 + (b - r) / (gdouble) delta;
      else
        h = 4 + (r - g) / (gdouble) delta;

      h = h * 42.5;

      if (h < 0)
        h += 255;
      else if (h > 255)
        h -= 255;
    }

  *red   = ROUND (h);
  *green = ROUND (s);
  *blue  = ROUND (l);
}

/**
 * gimp_rgb_to_l_int:
 * @red: Red channel
 * @green: Green channel
 * @blue: Blue channel
 *
 * Calculates the lightness value of an RGB triplet with the formula
 * L = (max(R, G, B) + min (R, G, B)) / 2
 *
 * Return value: Luminance value corresponding to the input RGB value
 **/
gint
gimp_rgb_to_l_int (gint red,
                   gint green,
                   gint blue)
{
  gint min, max;

  if (red > green)
    {
      max = MAX (red,   blue);
      min = MIN (green, blue);
    }
  else
    {
      max = MAX (green, blue);
      min = MIN (red,   blue);
    }

  return ROUND ((max + min) / 2.0);
}

static inline gint
gimp_hsl_value_int (gdouble n1,
                    gdouble n2,
                    gdouble hue)
{
  gdouble value;

  if (hue > 255)
    hue -= 255;
  else if (hue < 0)
    hue += 255;

  if (hue < 42.5)
    value = n1 + (n2 - n1) * (hue / 42.5);
  else if (hue < 127.5)
    value = n2;
  else if (hue < 170)
    value = n1 + (n2 - n1) * ((170 - hue) / 42.5);
  else
    value = n1;

  return ROUND (value * 255.0);
}

/**
 * gimp_hsl_to_rgb_int:
 * @hue: Hue channel, returns Red channel
 * @saturation: Saturation channel, returns Green channel
 * @lightness: Lightness channel, returns Blue channel
 *
 * The arguments are pointers to int, with the values pointed to in the
 * following ranges:  H [0, 360], L [0, 255], S [0, 255].
 *
 * The function changes the arguments to point to the RGB value
 * corresponding, with the returned values all in the range [0, 255].
 **/
void
gimp_hsl_to_rgb_int (gint *hue,
                     gint *saturation,
                     gint *lightness)
{
  gdouble h, s, l;

  h = *hue;
  s = *saturation;
  l = *lightness;

  if (s == 0)
    {
      /*  achromatic case  */
      *hue        = l;
      *lightness  = l;
      *saturation = l;
    }
  else
    {
      gdouble m1, m2;

      if (l < 128)
        m2 = (l * (255 + s)) / 65025.0;
      else
        m2 = (l + s - (l * s) / 255.0) / 255.0;

      m1 = (l / 127.5) - m2;

      /*  chromatic case  */
      *hue        = gimp_hsl_value_int (m1, m2, h + 85);
      *saturation = gimp_hsl_value_int (m1, m2, h);
      *lightness  = gimp_hsl_value_int (m1, m2, h - 85);
    }
}
