/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <glib.h>

#include "gimpcolorspace.h"

/*********************************
 *   color conversion routines   *
 *********************************/

void
gimp_rgb_to_hsv (gint *red,
		 gint *green,
		 gint *blue)
{
  gint    r, g, b;
  gdouble h, s, v;
  gint    min, max;
  gint    delta;

  h = 0.0;

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

  v = max;

  if (max != 0)
    s = ((max - min) * 255) / (gdouble) max;
  else
    s = 0;

  if (s == 0)
    h = 0;
  else
    {
      delta = max - min;
      if (r == max)
	h = (g - b) / (gdouble) delta;
      else if (g == max)
	h = 2 + (b - r) / (gdouble) delta;
      else if (b == max)
	h = 4 + (r - g) / (gdouble) delta;
      h *= 42.5;

      if (h < 0)
	h += 255;
      if (h > 255)
	h -= 255;
    }

  *red   = h;
  *green = s;
  *blue  = v;
}

void
gimp_hsv_to_rgb (gint *hue,
		 gint *saturation,
		 gint *value)
{
  gdouble h, s, v;
  gdouble f, p, q, t;

  if (*saturation == 0)
    {
      *hue        = *value;
      *saturation = *value;
      *value      = *value;
    }
  else
    {
      h = *hue * 6.0  / 255.0;
      s = *saturation / 255.0;
      v = *value      / 255.0;

      f = h - (gint) h;
      p = v * (1.0 - s);
      q = v * (1.0 - (s * f));
      t = v * (1.0 - (s * (1.0 - f)));

      switch ((gint) h)
	{
	case 0:
	  *hue        = v * 255;
	  *saturation = t * 255;
	  *value      = p * 255;
	  break;

	case 1:
	  *hue        = q * 255;
	  *saturation = v * 255;
	  *value      = p * 255;
	  break;

	case 2:
	  *hue        = p * 255;
	  *saturation = v * 255;
	  *value      = t * 255;
	  break;

	case 3:
	  *hue        = p * 255;
	  *saturation = q * 255;
	  *value      = v * 255;
	  break;

	case 4:
	  *hue        = t * 255;
	  *saturation = p * 255;
	  *value      = v * 255;
	  break;

	case 5:
	  *hue        = v * 255;
	  *saturation = p * 255;
	  *value      = q * 255;
	  break;
	}
    }
}

void
gimp_rgb_to_hls (gint *red,
		 gint *green,
		 gint *blue)
{
  gint    r, g, b;
  gdouble h, l, s;
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

  *red   = h;
  *green = l;
  *blue  = s;
}

gint
gimp_rgb_to_l (gint red,
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

  return (max + min) / 2.0;
}

static gint
gimp_hls_value (gdouble n1,
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

  return (gint) (value * 255);
}

void
gimp_hls_to_rgb (gint *hue,
		 gint *lightness,
		 gint *saturation)
{
  gdouble h, l, s;
  gdouble m1, m2;

  h = *hue;
  l = *lightness;
  s = *saturation;

  if (s == 0)
    {
      /*  achromatic case  */
      *hue        = l;
      *lightness  = l;
      *saturation = l;
    }
  else
    {
      if (l < 128)
	m2 = (l * (255 + s)) / 65025.0;
      else
	m2 = (l + s - (l * s) / 255.0) / 255.0;

      m1 = (l / 127.5) - m2;

      /*  chromatic case  */
      *hue        = gimp_hls_value (m1, m2, h + 85);
      *lightness  = gimp_hls_value (m1, m2, h);
      *saturation = gimp_hls_value (m1, m2, h - 85);
    }
}

void
gimp_rgb_to_hsv_double (gdouble *red,
			gdouble *green,
			gdouble *blue)
{
  gdouble r, g, b;
  gdouble h, s, v;
  gdouble min, max;
  gdouble delta;

  r = *red;
  g = *green;
  b = *blue;

  h = 0.0; /* Shut up -Wall */

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

  v = max;

  if (max != 0.0)
    s = (max - min) / max;
  else
    s = 0.0;

  if (s == 0.0)
    {
      h = 0.0;
    }
  else
    {
      delta = max - min;

      if (r == max)
	h = (g - b) / delta;
      else if (g == max)
	h = 2 + (b - r) / delta;
      else if (b == max)
	h = 4 + (r - g) / delta;

      h /= 6.0;

      if (h < 0.0)
	h += 1.0;
      else if (h > 1.0)
	h -= 1.0;
    }

  *red   = h;
  *green = s;
  *blue  = v;
}

void
gimp_hsv_to_rgb_double (gdouble *hue,
			gdouble *saturation,
			gdouble *value)
{
  gdouble h, s, v;
  gdouble f, p, q, t;

  if (*saturation == 0.0)
    {
      *hue        = *value;
      *saturation = *value;
      *value      = *value;
    }
  else
    {
      h = *hue * 6.0;
      s = *saturation;
      v = *value;

      if (h == 6.0)
	h = 0.0;

      f = h - (gint) h;
      p = v * (1.0 - s);
      q = v * (1.0 - s * f);
      t = v * (1.0 - s * (1.0 - f));

      switch ((gint) h)
	{
	case 0:
	  *hue        = v;
	  *saturation = t;
	  *value      = p;
	  break;

	case 1:
	  *hue        = q;
	  *saturation = v;
	  *value      = p;
	  break;

	case 2:
	  *hue        = p;
	  *saturation = v;
	  *value      = t;
	  break;

	case 3:
	  *hue        = p;
	  *saturation = q;
	  *value      = v;
	  break;

	case 4:
	  *hue        = t;
	  *saturation = p;
	  *value      = v;
	  break;

	case 5:
	  *hue        = v;
	  *saturation = p;
	  *value      = q;
	  break;
	}
    }
}

void
gimp_rgb_to_hsv4 (guchar  *rgb, 
		  gdouble *hue, 
		  gdouble *saturation, 
		  gdouble *value)
{
  gdouble red, green, blue;
  gdouble h, s, v;
  gdouble min, max;
  gdouble delta;

  red   = rgb[0] / 255.0;
  green = rgb[1] / 255.0;
  blue  = rgb[2] / 255.0;

  h = 0.0; /* Shut up -Wall */

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

  v = max;

  if (max != 0.0)
    s = (max - min) / max;
  else
    s = 0.0;

  if (s == 0.0)
    h = 0.0;
  else
    {
      delta = max - min;

      if (red == max)
	h = (green - blue) / delta;
      else if (green == max)
	h = 2 + (blue - red) / delta;
      else if (blue == max)
	h = 4 + (red - green) / delta;

      h /= 6.0;

      if (h < 0.0)
	h += 1.0;
      else if (h > 1.0)
	h -= 1.0;
    }

  *hue        = h;
  *saturation = s;
  *value      = v;
}

void
gimp_hsv_to_rgb4 (guchar  *rgb, 
		  gdouble  hue,
		  gdouble  saturation,
		  gdouble  value)
{
  gdouble h, s, v;
  gdouble f, p, q, t;

  if (saturation == 0.0)
    {
      hue        = value;
      saturation = value;
      value      = value;
    }
  else
    {
      h = hue * 6.0;
      s = saturation;
      v = value;

      if (h == 6.0)
	h = 0.0;

      f = h - (gint) h;
      p = v * (1.0 - s);
      q = v * (1.0 - s * f);
      t = v * (1.0 - s * (1.0 - f));

      switch ((int) h)
	{
	case 0:
	  hue        = v;
	  saturation = t;
	  value      = p;
	  break;

	case 1:
	  hue        = q;
	  saturation = v;
	  value      = p;
	  break;

	case 2:
	  hue        = p;
	  saturation = v;
	  value      = t;
	  break;

	case 3:
	  hue        = p;
	  saturation = q;
	  value      = v;
	  break;

	case 4:
	  hue        = t;
	  saturation = p;
	  value      = v;
	  break;

	case 5:
	  hue        = v;
	  saturation = p;
	  value      = q;
	  break;
	}
    }

  rgb[0] = hue        * 255;
  rgb[1] = saturation * 255;
  rgb[2] = value      * 255;
}
