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
gimp_rgb_sub (GimpRGB       *rgb1,
	      const GimpRGB *rgb2)
{
  g_return_if_fail (rgb1 != NULL);
  g_return_if_fail (rgb2 != NULL);

  rgb1->r = rgb1->r - rgb2->r;
  rgb1->g = rgb1->g - rgb2->g;
  rgb1->b = rgb1->b - rgb2->b;
}

void
gimp_rgb_mul (GimpRGB *rgb,
	      gdouble  factor)
{
  g_return_if_fail (rgb != NULL);

  rgb->r *= factor;
  rgb->g *= factor;
  rgb->b *= factor;
}

gdouble
gimp_rgb_dist (const GimpRGB *rgb1,
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
gimp_rgba_sub (GimpRGB       *rgba1,
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
gimp_rgba_mul (GimpRGB *rgba,
	       gdouble  factor)
{
  g_return_if_fail (rgba != NULL);

  rgba->r = rgba->r * factor;
  rgba->g = rgba->g * factor;
  rgba->b = rgba->b * factor;
  rgba->a = rgba->a * factor;
}

gdouble
gimp_rgba_dist (const GimpRGB *rgba1, 
		const GimpRGB *rgba2)
{
  g_return_val_if_fail (rgba1 != NULL, 0.0);
  g_return_val_if_fail (rgba2 != NULL, 0.0);

  return (fabs (rgba1->r - rgba2->r) + fabs (rgba1->g - rgba2->g) +
          fabs (rgba1->b - rgba2->b) + fabs (rgba1->a - rgba2->a));
}

/* These two are probably not needed */

gdouble
gimp_rgba_max (const GimpRGB *rgba)
{
  g_return_val_if_fail (rgba != NULL, 0.0);

  return MAX (rgba->r, MAX (rgba->g, MAX (rgba->b, rgba->a)));
}

gdouble
gimp_rgba_min (const GimpRGB *rgba)
{
  g_return_val_if_fail (rgba != NULL, 0.0);

  return MIN (rgba->r, MIN (rgba->g, MIN (rgba->b, rgba->a)));
}

void
gimp_rgba_clamp (GimpRGB *rgba)
{
  g_return_if_fail (rgba != NULL);

  rgba->r = CLAMP (rgba->r, 0.0, 1.0);
  rgba->g = CLAMP (rgba->g, 0.0, 1.0);
  rgba->b = CLAMP (rgba->b, 0.0, 1.0);
  rgba->a = CLAMP (rgba->a, 0.0, 1.0);
}

void
gimp_rgba_gamma (GimpRGB *rgba,
		 gdouble  gamma)
{
  gdouble ig;

  g_return_if_fail (rgba != NULL);

  if (gamma != 0.0)
    ig = 1.0 / gamma;
  else
    (ig = 0.0);

  rgba->r = pow (rgba->r, ig);
  rgba->g = pow (rgba->g, ig);
  rgba->b = pow (rgba->b, ig);
  rgba->a = pow (rgba->a, ig);
}
