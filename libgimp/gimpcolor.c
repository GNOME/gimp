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

#include "config.h"

#include <glib.h>

#include "gimpcolor.h"
#include "gimpcolorspace.h"
#include "gimpmath.h"


/***************************
 *   channels operations   *
 ***************************/


/*  RGB functions  */

void
gimp_rgb_set (GimpRGB *rgb,
	      gdouble  r,
	      gdouble  g,
	      gdouble  b)
{
  g_return_if_fail (rgb != NULL);

  rgb->r = r;
  rgb->g = g;
  rgb->b = b;
}

void
gimp_rgb_set_alpha (GimpRGB *rgb,
		    gdouble  a)
{
  g_return_if_fail (rgb != NULL);

  rgb->a = a;
}

void
gimp_rgb_set_uchar (GimpRGB *rgb,
		    guchar   r,
		    guchar   g,
		    guchar   b)
{
  g_return_if_fail (rgb != NULL);

  rgb->r = (gdouble) r / 255.0;
  rgb->g = (gdouble) g / 255.0;
  rgb->b = (gdouble) b / 255.0;
}

void
gimp_rgb_get_uchar (const GimpRGB *rgb,
		    guchar        *r,
		    guchar        *g,
		    guchar        *b)
{
  g_return_if_fail (rgb != NULL);

  if (r) *r = CLAMP (rgb->r, 0.0, 1.0) * 255.999;
  if (g) *g = CLAMP (rgb->g, 0.0, 1.0) * 255.999;
  if (b) *b = CLAMP (rgb->b, 0.0, 1.0) * 255.999;
}

void
gimp_rgb_add (GimpRGB       *rgb1,
	      const GimpRGB *rgb2)
{
  g_return_if_fail (rgb1 != NULL);
  g_return_if_fail (rgb2 != NULL);

  rgb1->r = rgb1->r + rgb2->r;
  rgb1->g = rgb1->g + rgb2->g;
  rgb1->b = rgb1->b + rgb2->b;
}

void
gimp_rgb_subtract (GimpRGB       *rgb1,
		   const GimpRGB *rgb2)
{
  g_return_if_fail (rgb1 != NULL);
  g_return_if_fail (rgb2 != NULL);

  rgb1->r = rgb1->r - rgb2->r;
  rgb1->g = rgb1->g - rgb2->g;
  rgb1->b = rgb1->b - rgb2->b;
}

void
gimp_rgb_multiply (GimpRGB *rgb,
		   gdouble  factor)
{
  g_return_if_fail (rgb != NULL);

  rgb->r *= factor;
  rgb->g *= factor;
  rgb->b *= factor;
}

gdouble
gimp_rgb_distance (const GimpRGB *rgb1,
		   const GimpRGB *rgb2)
{
  g_return_val_if_fail (rgb1 != NULL, 0.0);
  g_return_val_if_fail (rgb2 != NULL, 0.0);

  return (fabs (rgb1->r - rgb2->r) +
	  fabs (rgb1->g - rgb2->g) +
	  fabs (rgb1->b - rgb2->b));
}

gdouble
gimp_rgb_max (const GimpRGB *rgb)
{
  g_return_val_if_fail (rgb != NULL, 0.0);

  return MAX (rgb->r, MAX (rgb->g, rgb->b));
}

gdouble
gimp_rgb_min (const GimpRGB *rgb)
{
  g_return_val_if_fail (rgb != NULL, 0.0);

  return MIN (rgb->r, MIN (rgb->g, rgb->b));
}

void
gimp_rgb_clamp (GimpRGB *rgb)
{
  g_return_if_fail (rgb != NULL);

  rgb->r = CLAMP (rgb->r, 0.0, 1.0);
  rgb->g = CLAMP (rgb->g, 0.0, 1.0);
  rgb->b = CLAMP (rgb->b, 0.0, 1.0);
  rgb->a = CLAMP (rgb->a, 0.0, 1.0);
}

void
gimp_rgb_gamma (GimpRGB *rgb,
		gdouble  gamma)
{
  gdouble ig;

  g_return_if_fail (rgb != NULL);

  if (gamma != 0.0)
    ig = 1.0 / gamma;
  else
    ig = 0.0;

  rgb->r = pow (rgb->r, ig);
  rgb->g = pow (rgb->g, ig);
  rgb->b = pow (rgb->b, ig);
}

gdouble
gimp_rgb_intensity (const GimpRGB *rgb)
{
  g_return_val_if_fail (rgb != NULL, 0.0);

  return (INTENSITY_RED   * rgb->r + 
	  INTENSITY_GREEN * rgb->g + 
	  INTENSITY_BLUE  * rgb->b);
}

guchar
gimp_rgb_intensity_uchar (const GimpRGB *rgb)
{
  g_return_val_if_fail (rgb != NULL, 0);

  return (CLAMP (gimp_rgb_intensity (rgb) * 255.999, 0.0, 1.0));
}

void
gimp_rgb_composite (GimpRGB              *color1,
		    const GimpRGB        *color2,
		    GimpRGBCompositeMode  mode)
{
  gdouble factor;

  g_return_if_fail (color1 != NULL);
  g_return_if_fail (color2 != NULL);
  
  switch (mode)
    {
    case GIMP_RGB_COMPOSITE_NONE:
      break;

    case GIMP_RGB_COMPOSITE_NORMAL:
      /*  put color2 on top of color1  */
      factor = color1->a * (1.0 - color2->a);
      color1->r = color1->r * factor + color2->r * color2->a;
      color1->g = color1->g * factor + color2->g * color2->a;
      color1->b = color1->b * factor + color2->b * color2->a;
      color1->a = factor + color2->a;
      break;
      
    case GIMP_RGB_COMPOSITE_BEHIND:
      /*  put color2 below color1  */
      factor = color2->a * (1.0 - color1->a);
      color1->r = color2->r * factor + color1->r * color1->a;
      color1->g = color2->g * factor + color1->g * color1->a;
      color1->b = color2->b * factor + color1->b * color1->a;
      color1->a = factor + color1->a;
      break;
    }
}

/*  RGBA functions  */

void
gimp_rgba_set (GimpRGB *rgba,
	       gdouble  r,
	       gdouble  g,
	       gdouble  b,
	       gdouble  a)
{
  g_return_if_fail (rgba != NULL);

  rgba->r = r;
  rgba->g = g;
  rgba->b = b;
  rgba->a = a;
}

void
gimp_rgba_set_uchar (GimpRGB *rgba,
		     guchar   r,
		     guchar   g,
		     guchar   b,
		     guchar   a)
{
  g_return_if_fail (rgba != NULL);

  rgba->r = (gdouble) r / 255.0;
  rgba->g = (gdouble) g / 255.0;
  rgba->b = (gdouble) b / 255.0;
  rgba->a = (gdouble) a / 255.0;
}

void
gimp_rgba_get_uchar (const GimpRGB *rgba,
		     guchar        *r,
		     guchar        *g,
		     guchar        *b,
		     guchar        *a)
{
  g_return_if_fail (rgba != NULL);

  if (r) *r = CLAMP (rgba->r, 0.0, 1.0) * 255.999;
  if (g) *g = CLAMP (rgba->g, 0.0, 1.0) * 255.999;
  if (b) *b = CLAMP (rgba->b, 0.0, 1.0) * 255.999;
  if (a) *a = CLAMP (rgba->a, 0.0, 1.0) * 255.999;
}

void
gimp_rgba_add (GimpRGB       *rgba1,
	       const GimpRGB *rgba2)
{
  g_return_if_fail (rgba1 != NULL);
  g_return_if_fail (rgba2 != NULL);

  rgba1->r = rgba1->r + rgba2->r;
  rgba1->g = rgba1->g + rgba2->g;
  rgba1->b = rgba1->b + rgba2->b;
  rgba1->a = rgba1->a + rgba2->a;
}

void
gimp_rgba_subtract (GimpRGB       *rgba1,
		    const GimpRGB *rgba2)
{
  g_return_if_fail (rgba1 != NULL);
  g_return_if_fail (rgba2 != NULL);

  rgba1->r = rgba1->r - rgba2->r;
  rgba1->g = rgba1->g - rgba2->g;
  rgba1->b = rgba1->b - rgba2->b;
  rgba1->a = rgba1->a - rgba2->a;
}

void
gimp_rgba_multiply (GimpRGB *rgba,
		    gdouble  factor)
{
  g_return_if_fail (rgba != NULL);

  rgba->r = rgba->r * factor;
  rgba->g = rgba->g * factor;
  rgba->b = rgba->b * factor;
  rgba->a = rgba->a * factor;
}

gdouble
gimp_rgba_distance (const GimpRGB *rgba1, 
		    const GimpRGB *rgba2)
{
  g_return_val_if_fail (rgba1 != NULL, 0.0);
  g_return_val_if_fail (rgba2 != NULL, 0.0);

  return (fabs (rgba1->r - rgba2->r) + fabs (rgba1->g - rgba2->g) +
          fabs (rgba1->b - rgba2->b) + fabs (rgba1->a - rgba2->a));
}


/*  HSV functions  */

void
gimp_hsv_set (GimpHSV *hsv,
	      gdouble  h,
	      gdouble  s,
	      gdouble  v)
{
  g_return_if_fail (hsv != NULL);

  hsv->h = h;
  hsv->s = s;
  hsv->v = v;
}

void
gimp_hsv_clamp (GimpHSV *hsv)
{
  g_return_if_fail (hsv != NULL);

  hsv->h -= (gint) hsv->h;

  if (hsv->h < 0)
    hsv->h += 1.0;

  hsv->s = CLAMP (hsv->s, 0.0, 1.0);
  hsv->v = CLAMP (hsv->v, 0.0, 1.0);
  hsv->a = CLAMP (hsv->a, 0.0, 1.0);
}

void
gimp_hsva_set (GimpHSV *hsva,
	       gdouble  h,
	       gdouble  s,
	       gdouble  v,
	       gdouble  a)
{
  g_return_if_fail (hsva != NULL);

  hsva->h = h;
  hsva->s = s;
  hsva->v = v;
  hsva->a = a;
}

/*  These functions will become the default one day  */

gboolean
gimp_palette_set_foreground_rgb (const GimpRGB *rgb)
{
  guchar r, g, b;

  g_return_val_if_fail (rgb != NULL, FALSE);

  gimp_rgb_get_uchar (rgb, &r, &g, &b);

  return gimp_palette_set_foreground (r, g, b);
}

gboolean
gimp_palette_get_foreground_rgb (GimpRGB *rgb)
{
  guchar r, g, b;
  
  g_return_val_if_fail (rgb != NULL, FALSE);

  if (gimp_palette_get_foreground (&r, &g, &b))
    {
      gimp_rgb_set_uchar (rgb, r, g, b);
      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_palette_set_background_rgb (const GimpRGB *rgb)
{
  guchar r, g, b;

  g_return_val_if_fail (rgb != NULL, FALSE);

  gimp_rgb_get_uchar (rgb, &r, &g, &b);

  return gimp_palette_set_background (r, g, b);
}

gboolean
gimp_palette_get_background_rgb (GimpRGB *rgb)
{
  guchar r, g, b;
  
  g_return_val_if_fail (rgb != NULL, FALSE);

  if (gimp_palette_get_background (&r, &g, &b))
    {
      gimp_rgb_set_uchar (rgb, r, g, b);
      return TRUE;
    }

  return FALSE;
}
