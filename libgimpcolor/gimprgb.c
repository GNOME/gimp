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

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "gimpcolortypes.h"

#include "gimprgb.h"


/*  RGB type  */

static GimpRGB  * rgb_copy  (const GimpRGB *rgb);


GType
gimp_rgb_get_type (void)
{
  static GType rgb_type = 0;

  if (!rgb_type)
    rgb_type = g_boxed_type_register_static ("GimpRGB",
                                             (GBoxedCopyFunc) rgb_copy,
                                             (GBoxedFreeFunc) g_free);

  return rgb_type;
}

static GimpRGB *
rgb_copy (const GimpRGB *rgb)
{
  return g_memdup (rgb, sizeof (GimpRGB));
}


/*  RGB functions  */

/**
 * gimp_rgb_set:
 * @rgb: a #GimpRGB struct
 * @red:
 * @green:
 * @blue:
 *
 * Sets the red, green and blue components of @rgb and leaves the
 * alpha component unchanged. The color values should be between 0.0
 * and 1.0 but there is no check to enforce this and the values are
 * set exactly as they are passed in.
 **/
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

/**
 * gimp_rgb_set_alpha:
 * @rgb: a #GimpRGB struct
 * @alpha:
 *
 * Sets the alpha component of @rgb and leaves the RGB components unchanged.
 **/
void
gimp_rgb_set_alpha (GimpRGB *rgb,
		    gdouble  a)
{
  g_return_if_fail (rgb != NULL);

  rgb->a = a;
}

/**
 * gimp_rgb_set_uchar:
 * @rgb: a #GimpRGB struct
 * @red:
 * @green:
 * @blue:
 *
 * Sets the red, green and blue components of @rgb from 8bit values
 * (0 to 255) and leaves the alpha component unchanged.
 **/
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

  if (r) *r = ROUND (CLAMP (rgb->r, 0.0, 1.0) * 255.0);
  if (g) *g = ROUND (CLAMP (rgb->g, 0.0, 1.0) * 255.0);
  if (b) *b = ROUND (CLAMP (rgb->b, 0.0, 1.0) * 255.0);
}

void
gimp_rgb_add (GimpRGB       *rgb1,
	      const GimpRGB *rgb2)
{
  g_return_if_fail (rgb1 != NULL);
  g_return_if_fail (rgb2 != NULL);

  rgb1->r += rgb2->r;
  rgb1->g += rgb2->g;
  rgb1->b += rgb2->b;
}

void
gimp_rgb_subtract (GimpRGB       *rgb1,
		   const GimpRGB *rgb2)
{
  g_return_if_fail (rgb1 != NULL);
  g_return_if_fail (rgb2 != NULL);

  rgb1->r -= rgb2->r;
  rgb1->g -= rgb2->g;
  rgb1->b -= rgb2->b;
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

  if (rgb->r > rgb->g)
     return (rgb->r > rgb->b) ? rgb->r : rgb->b;
  return (rgb->g > rgb->b) ? rgb->g : rgb->b;
}

gdouble
gimp_rgb_min (const GimpRGB *rgb)
{
  g_return_val_if_fail (rgb != NULL, 0.0);

  if (rgb->r < rgb->g)
     return (rgb->r < rgb->b) ? rgb->r : rgb->b;
  return (rgb->g < rgb->b) ? rgb->g : rgb->b;
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
  gdouble intensity;

  g_return_val_if_fail (rgb != NULL, 0.0);

  intensity = GIMP_RGB_INTENSITY (rgb->r, rgb->g, rgb->b);

  return CLAMP (intensity, 0.0, 1.0);
}

guchar
gimp_rgb_intensity_uchar (const GimpRGB *rgb)
{
  g_return_val_if_fail (rgb != NULL, 0);

  return ROUND (gimp_rgb_intensity (rgb) * 255.0);
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
      if (color2->a == 1.0)
	{
	  *color1 = *color2;
	}
      else
	{
	  factor = color1->a * (1.0 - color2->a);
	  color1->r = color1->r * factor + color2->r * color2->a;
	  color1->g = color1->g * factor + color2->g * color2->a;
	  color1->b = color1->b * factor + color2->b * color2->a;
	  color1->a = factor + color2->a;
	}
      break;

    case GIMP_RGB_COMPOSITE_BEHIND:
      /*  put color2 below color1  */
      if (color1->a < 1.0)
	{
	  factor = color2->a * (1.0 - color1->a);
	  color1->r = color2->r * factor + color1->r * color1->a;
	  color1->g = color2->g * factor + color1->g * color1->a;
	  color1->b = color2->b * factor + color1->b * color1->a;
	  color1->a = factor + color1->a;
	}
      break;
    }
}

/*  RGBA functions  */

/**
 * gimp_rgba_set:
 * @rgba: a #GimpRGB struct
 * @red:
 * @green:
 * @blue:
 * @alpha:
 *
 * Sets the red, green, blue and alpha components of @rgb. The values
 * should be between 0.0 and 1.0 but there is no check to enforce this
 * and the values are set exactly as they are passed in.
 **/
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

/**
 * gimp_rgba_set_uchar:
 * @rgba: a #GimpRGB struct
 * @red:
 * @green:
 * @blue:
 * @alpha:
 *
 * Sets the red, green, blue and alpha components of @rgb from 8bit
 * values (0 to 255).
 **/
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

  if (r) *r = ROUND (CLAMP (rgba->r, 0.0, 1.0) * 255.0);
  if (g) *g = ROUND (CLAMP (rgba->g, 0.0, 1.0) * 255.0);
  if (b) *b = ROUND (CLAMP (rgba->b, 0.0, 1.0) * 255.0);
  if (a) *a = ROUND (CLAMP (rgba->a, 0.0, 1.0) * 255.0);
}

void
gimp_rgba_add (GimpRGB       *rgba1,
	       const GimpRGB *rgba2)
{
  g_return_if_fail (rgba1 != NULL);
  g_return_if_fail (rgba2 != NULL);

  rgba1->r += rgba2->r;
  rgba1->g += rgba2->g;
  rgba1->b += rgba2->b;
  rgba1->a += rgba2->a;
}

void
gimp_rgba_subtract (GimpRGB       *rgba1,
		    const GimpRGB *rgba2)
{
  g_return_if_fail (rgba1 != NULL);
  g_return_if_fail (rgba2 != NULL);

  rgba1->r -= rgba2->r;
  rgba1->g -= rgba2->g;
  rgba1->b -= rgba2->b;
  rgba1->a -= rgba2->a;
}

void
gimp_rgba_multiply (GimpRGB *rgba,
		    gdouble  factor)
{
  g_return_if_fail (rgba != NULL);

  rgba->r *= factor;
  rgba->g *= factor;
  rgba->b *= factor;
  rgba->a *= factor;
}

gdouble
gimp_rgba_distance (const GimpRGB *rgba1,
		    const GimpRGB *rgba2)
{
  g_return_val_if_fail (rgba1 != NULL, 0.0);
  g_return_val_if_fail (rgba2 != NULL, 0.0);

  return (fabs (rgba1->r - rgba2->r) +
          fabs (rgba1->g - rgba2->g) +
          fabs (rgba1->b - rgba2->b) +
          fabs (rgba1->a - rgba2->a));
}
