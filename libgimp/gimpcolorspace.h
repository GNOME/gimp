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

void	gimp_rgb_to_hsv		(int *, int *, int *);
void	gimp_hsv_to_rgb		(int *, int *, int *);
void	gimp_rgb_to_hls		(int *, int *, int *);
int	gimp_rgb_to_l		(int, int, int);
void	gimp_hls_to_rgb		(int *, int *, int *);
void	gimp_rgb_to_hsv_double	(double *, double *, double *);
void	gimp_hsv_to_rgb_double	(double *, double *, double *);
void	gimp_rgb_to_hsv4        (guchar *, double *, double *, double *);
void	gimp_hsv_to_rgb4        (guchar *, double, double, double);


/*  Map RGB to intensity  */

#define INTENSITY_RED   0.30
#define INTENSITY_GREEN 0.59
#define INTENSITY_BLUE  0.11
#define INTENSITY(r,g,b) ((r) * INTENSITY_RED   + \
			  (g) * INTENSITY_GREEN + \
			  (b) * INTENSITY_BLUE  + 0.001)
#endif
