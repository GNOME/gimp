/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * General, non-jittered adaptive supersampling library
 * Copyright (C) 1997 Federico Mena Quintero
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
 *
 */


#ifndef __ASUPSAMPLE_H__
#define __ASUPSAMPLE_H__

#include "gimpprogress.h"

/***** Types *****/

typedef struct {
	double r, g, b, a; /* Range is [0, 1] */
} color_t;

typedef void (*render_func_t) (double x, double y, color_t *color, void *render_data);
typedef void (*put_pixel_func_t) (int x, int y, color_t color, void *put_pixel_data);


/***** Functions *****/

unsigned long adaptive_supersample_area(int x1, int y1, int x2, int y2, int max_depth, double threshold,
					render_func_t render_func, void *render_data,
					put_pixel_func_t put_pixel_func, void *put_pixel_data,
					progress_func_t progress_func, void *progress_data);


#endif
