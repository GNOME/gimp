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

#ifndef __GIMPCOLORSPACE_H__
#define __GIMPCOLORSPACE_H__

/*  Color conversion routines  */

void	rgb_to_hsv		(int *, int *, int *);
void	hsv_to_rgb		(int *, int *, int *);
void	rgb_to_hls		(int *, int *, int *);
int	rgb_to_l		(int, int, int);
void	hls_to_rgb		(int *, int *, int *);
void	rgb_to_hsv_double	(double *, double *, double *);
void	hsv_to_rgb_double	(double *, double *, double *);
void	rgb_to_hsv4		(guchar *, double *, double *, double *);
void	hsv_to_rgb4		(guchar *, double, double, double);

#endif
