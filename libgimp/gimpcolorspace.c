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

#include <libgimp/gimpcolorspace.h>

/*********************************
 *   color conversion routines   *
 *********************************/

void
gimp_rgb_to_hsv (gint *r,
		 gint *g,
		 gint *b)
{
  gint    red, green, blue;
  gdouble h, s, v;
  gint    min, max;
  gint    delta;

  h = 0.0;

  red   = *r;
  green = *g;
  blue  = *b;

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

  if (max != 0)
    s = ((max - min) * 255) / (gdouble) max;
  else
    s = 0;

  if (s == 0)
    h = 0;
  else
    {
      delta = max - min;
      if (red == max)
	h = (green - blue) / (gdouble) delta;
      else if (green == max)
	h = 2 + (blue - red) / (gdouble) delta;
      else if (blue == max)
	h = 4 + (red - green) / (gdouble) delta;
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
gimp_hsv_to_rgb (gint *h,
		 gint *s,
		 gint *v)
{
  gdouble hue, saturation, value;
  gdouble f, p, q, t;

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
	  *s = t     * 255;
	  *v = p     * 255;
	  break;

	case 1:
	  *h = q     * 255;
	  *s = value * 255;
	  *v = p     * 255;
	  break;

	case 2:
	  *h = p     * 255;
	  *s = value * 255;
	  *v = t     * 255;
	  break;

	case 3:
	  *h = p     * 255;
	  *s = q     * 255;
	  *v = value * 255;
	  break;

	case 4:
	  *h = t     * 255;
	  *s = p     * 255;
	  *v = value * 255;
	  break;

	case 5:
	  *h = value * 255;
	  *s = p     * 255;
	  *v = q     * 255;
	  break;
	}
    }
}

void
gimp_rgb_to_hls (gint *r,
		 gint *g,
		 gint *b)
{
  gint    red, green, blue;
  gdouble h, l, s;
  gint    min, max;
  gint    delta;

  red   = *r;
  green = *g;
  blue  = *b;

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

      if (red == max)
	h = (green - blue) / (gdouble) delta;
      else if (green == max)
	h = 2 + (blue - red) / (gdouble) delta;
      else
	h = 4 + (red - green) / (gdouble) delta;

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
gimp_hls_to_rgb (gint *h,
		 gint *l,
		 gint *s)
{
  gdouble hue, lightness, saturation;
  gdouble m1, m2;

  hue        = *h;
  lightness  = *l;
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
      *h = gimp_hls_value (m1, m2, hue + 85);
      *l = gimp_hls_value (m1, m2, hue);
      *s = gimp_hls_value (m1, m2, hue - 85);
    }
}

void
gimp_rgb_to_hsv_double (gdouble *r,
			gdouble *g,
			gdouble *b)
{
  gdouble red, green, blue;
  gdouble h, s, v;
  gdouble min, max;
  gdouble delta;

  red   = *r;
  green = *g;
  blue  = *b;

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
gimp_hsv_to_rgb_double (gdouble *h,
			gdouble *s,
			gdouble *v)
{
  gdouble hue, saturation, value;
  gdouble f, p, q, t;

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

      f = hue - (gint) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - saturation * f);
      t = value * (1.0 - saturation * (1.0 - f));

      switch ((gint) hue)
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
gimp_rgb_to_hsv4 (guchar  *rgb, 
		  gdouble *hue, 
		  gdouble *sat, 
		  gdouble *val)
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
gimp_hsv_to_rgb4 (guchar  *rgb, 
		  gdouble  h, 
		  gdouble  s, 
		  gdouble  v)
{
  gdouble hue, saturation, value;
  gdouble f, p, q, t;

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

  rgb[0] = h * 255;
  rgb[1] = s * 255;
  rgb[2] = v * 255;
  
}
