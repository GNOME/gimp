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

#ifndef __GIMPCOLORSPACE_H__
#define __GIMPCOLORSPACE_H__

/*  Color conversion routines  */

void    gimp_rgb_to_hsv		(gint    *red         /* returns hue        */,
				 gint    *green       /* returns saturation */,
				 gint    *blue        /* returns value      */);
void    gimp_hsv_to_rgb		(gint    *hue         /* returns red        */,
				 gint    *saturation  /* returns green      */,
				 gint    *value       /* returns blue       */);

void    gimp_rgb_to_hls		(gint    *red         /* returns hue        */,
				 gint    *green       /* returns lightness  */,
				 gint    *blue        /* returns saturation */);
gint    gimp_rgb_to_l		(gint     red,
				 gint     green,
				 gint     blue);
void    gimp_hls_to_rgb		(gint    *hue         /* returns red        */,
				 gint    *lightness   /* returns green      */,
				 gint    *saturation  /* returns blue       */);

void    gimp_rgb_to_hsv_double	(gdouble *red         /* returns hue        */,
				 gdouble *green       /* returns saturation */,
				 gdouble *blue        /* returns value      */);
void    gimp_hsv_to_rgb_double	(gdouble *hue         /* returns red        */,
				 gdouble *saturation, /* returns green      */
				 gdouble *value       /* returns blue       */);

void    gimp_rgb_to_hsv4        (guchar  *hsv,
				 gdouble *red,
				 gdouble *green,
				 gdouble *blue);
void    gimp_hsv_to_rgb4        (guchar  *rgb,
				 gdouble  hue,
				 gdouble  saturation,
				 gdouble  value);


/*  Map RGB to intensity  */

#define INTENSITY_RED   0.30
#define INTENSITY_GREEN 0.59
#define INTENSITY_BLUE  0.11
#define INTENSITY(r,g,b) ((r) * INTENSITY_RED   + \
			  (g) * INTENSITY_GREEN + \
			  (b) * INTENSITY_BLUE  + 0.001)

#endif  /* __GIMPCOLORSPACE_H__ */
