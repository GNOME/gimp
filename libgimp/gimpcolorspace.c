/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1999 Daniel Egger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <glib.h>

#include <libgimp/gimpcolorspace.h>

/*********************************
 *   color conversion routines   *
 *********************************/

void
rgb_to_hsv (int *r,
	    int *g,
	    int *b)
{
  int red, green, blue;
  float h, s, v;
  int min, max;
  int delta;

  h = 0.0;

  red = *r;
  green = *g;
  blue = *b;

  if (red > green)
    {
      if (red > blue)
	max = red;
      else
	max = blue;

      if (green < blue)
	min = green;
      else
	min = blue;
    }
  else
    {
      if (green > blue)
	max = green;
      else
	max = blue;

      if (red < blue)
	min = red;
      else
	min = blue;
    }

  v = max;

  if (max != 0)
    s = ((max - min) * 255) / (float) max;
  else
    s = 0;

  if (s == 0)
    h = 0;
  else
    {
      delta = max - min;
      if (red == max)
	h = (green - blue) / (float) delta;
      else if (green == max)
	h = 2 + (blue - red) / (float) delta;
      else if (blue == max)
	h = 4 + (red - green) / (float) delta;
      h *= 42.5;

      if (h < 0)
	h += 255;
      if (h > 255)
	h -= 255;
    }

  *r = h;
  *g = s;
  *b = v;
}

void
hsv_to_rgb (int *h,
	    int *s,
	    int *v)
{
  float hue, saturation, value;
  float f, p, q, t;

  if (*s == 0)
    {
      *h = *v;
      *s = *v;
      *v = *v;
    }
  else
    {
      hue = *h * 6.0 / 255.0;
      saturation = *s / 255.0;
      value = *v / 255.0;

      f = hue - (int) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - (saturation * f));
      t = value * (1.0 - (saturation * (1.0 - f)));

      switch ((int) hue)
	{
	case 0:
	  *h = value * 255;
	  *s = t * 255;
	  *v = p * 255;
	  break;
	case 1:
	  *h = q * 255;
	  *s = value * 255;
	  *v = p * 255;
	  break;
	case 2:
	  *h = p * 255;
	  *s = value * 255;
	  *v = t * 255;
	  break;
	case 3:
	  *h = p * 255;
	  *s = q * 255;
	  *v = value * 255;
	  break;
	case 4:
	  *h = t * 255;
	  *s = p * 255;
	  *v = value * 255;
	  break;
	case 5:
	  *h = value * 255;
	  *s = p * 255;
	  *v = q * 255;
	  break;
	}
    }
}

void
rgb_to_hls (int *r,
	    int *g,
	    int *b)
{
  int red, green, blue;
  float h, l, s;
  int min, max;
  int delta;

  red = *r;
  green = *g;
  blue = *b;

  if (red > green)
    {
      if (red > blue)
	max = red;
      else
	max = blue;

      if (green < blue)
	min = green;
      else
	min = blue;
    }
  else
    {
      if (green > blue)
	max = green;
      else
	max = blue;

      if (red < blue)
	min = red;
      else
	min = blue;
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
	s = 255 * (float) delta / (float) (max + min);
      else
	s = 255 * (float) delta / (float) (511 - max - min);

      if (red == max)
	h = (green - blue) / (float) delta;
      else if (green == max)
	h = 2 + (blue - red) / (float) delta;
      else
	h = 4 + (red - green) / (float) delta;

      h = h * 42.5;

      if (h < 0)
	h += 255;
      else if (h > 255)
	h -= 255;
    }

  *r = h;
  *g = l;
  *b = s;
}

int
rgb_to_l (int red,
	  int green,
	  int blue)
{
  int min, max;

  if (red > green)
    {
      if (red > blue)
	max = red;
      else
	max = blue;

      if (green < blue)
	min = green;
      else
	min = blue;
    }
  else
    {
      if (green > blue)
	max = green;
      else
	max = blue;

      if (red < blue)
	min = red;
      else
	min = blue;
    }

  return (max + min) / 2.0;
}

int
hls_value (float n1,
	   float n2,
	   float hue)
{
  float value;

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

  return (int) (value * 255);
}

void
hls_to_rgb (int *h,
	    int *l,
	    int *s)
{
  float hue, lightness, saturation;
  float m1, m2;

  hue = *h;
  lightness = *l;
  saturation = *s;

  if (saturation == 0)
    {
      /*  achromatic case  */
      *h = lightness;
      *l = lightness;
      *s = lightness;
    }
  else
    {
      if (lightness < 128)
	m2 = (lightness * (255 + saturation)) / 65025.0;
      else
	m2 = (lightness + saturation - (lightness * saturation)/255.0) / 255.0;

      m1 = (lightness / 127.5) - m2;

      /*  chromatic case  */
      *h = hls_value (m1, m2, hue + 85);
      *l = hls_value (m1, m2, hue);
      *s = hls_value (m1, m2, hue - 85);
    }
}

void
rgb_to_hsv_double (double *r,
		   double *g,
		   double *b)
{
  double red, green, blue;
  double h, s, v;
  double min, max;
  double delta;

  red   = *r;
  green = *g;
  blue  = *b;

  h = 0.0; /* Shut up -Wall */

  if (red > green)
    {
      if (red > blue)
	max = red;
      else
	max = blue;

      if (green < blue)
	min = green;
      else
	min = blue;
    }
  else
    {
      if (green > blue)
	max = green;
      else
	max = blue;

      if (red < blue)
	min = red;
      else
	min = blue;
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

  *r = h;
  *g = s;
  *b = v;
}

void
hsv_to_rgb_double (double *h,
		   double *s,
		   double *v)
{
  double hue, saturation, value;
  double f, p, q, t;

  if (*s == 0.0)
    {
      *h = *v;
      *s = *v;
      *v = *v; /* heh */
    }
  else
    {
      hue        = *h * 6.0;
      saturation = *s;
      value      = *v;

      if (hue == 6.0)
	hue = 0.0;

      f = hue - (int) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - saturation * f);
      t = value * (1.0 - saturation * (1.0 - f));

      switch ((int) hue)
	{
	case 0:
	  *h = value;
	  *s = t;
	  *v = p;
	  break;

	case 1:
	  *h = q;
	  *s = value;
	  *v = p;
	  break;

	case 2:
	  *h = p;
	  *s = value;
	  *v = t;
	  break;

	case 3:
	  *h = p;
	  *s = q;
	  *v = value;
	  break;

	case 4:
	  *h = t;
	  *s = p;
	  *v = value;
	  break;

	case 5:
	  *h = value;
	  *s = p;
	  *v = q;
	  break;
	}
    }
}

void
rgb_to_hsv4 (guchar *rgb, 
	     double *hue, 
	     double *sat, 
	     double *val)
{
  double red, green, blue;
  double h, s, v;
  double min, max;
  double delta;

  red   = rgb[0] / 255.0;
  green = rgb[1] / 255.0;
  blue  = rgb[2] / 255.0;

  h = 0.0; /* Shut up -Wall */

  if (red > green)
    {
      if (red > blue)
	max = red;
      else
	max = blue;

      if (green < blue)
	min = green;
      else
	min = blue;
    }
  else
    {
      if (green > blue)
	max = green;
      else
	max = blue;

      if (red < blue)
	min = red;
      else
	min = blue;
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

  *hue = h;
  *sat = s;
  *val = v;
}

void
hsv_to_rgb4 (guchar *rgb, 
	     double  h, 
	     double  s, 
             double  v)
{
  double hue, saturation, value;
  double f, p, q, t;

  if (s == 0.0)
    {
      h = v;
      s = v;
      v = v; /* heh */
    }
  else
    {
      hue        = h * 6.0;
      saturation = s;
      value      = v;

      if (hue == 6.0)
	hue = 0.0;

      f = hue - (int) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - saturation * f);
      t = value * (1.0 - saturation * (1.0 - f));

      switch ((int) hue)
	{
	case 0:
	  h = value;
	  s = t;
	  v = p;
	  break;

	case 1:
	  h = q;
	  s = value;
	  v = p;
	  break;

	case 2:
	  h = p;
	  s = value;
	  v = t;
	  break;

	case 3:
	  h = p;
	  s = q;
	  v = value;
	  break;

	case 4:
	  h = t;
	  s = p;
	  v = value;
	  break;

	case 5:
	  h = value;
	  s = p;
	  v = q;
	  break;
	}
    }

  rgb[0] = h*255;
  rgb[1] = s*255;
  rgb[2] = v*255;
  
}
