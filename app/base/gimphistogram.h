/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphistogram module Copyright (C) 1999 Jay Cox <jaycox@earthlink.net>
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

#ifndef __GIMP_HISTOGRAM_H__
#define __GIMP_HISTOGRAM_H__

#include "pixel_region.h"
#include "gimpdrawable.h"
#include "gimphistogramF.h"

#define HISTOGRAM_VALUE  0
#define HISTOGRAM_RED    1
#define HISTOGRAM_GREEN  2
#define HISTOGRAM_BLUE   3
#define HISTOGRAM_ALPHA  4


GimpHistogram *gimp_histogram_new         ();
void           gimp_histogram_free        (GimpHistogram *);
void           gimp_histogram_calculate   (GimpHistogram *, 
					   PixelRegion *region,
					   PixelRegion *mask);
void           gimp_histogram_calculate_drawable (GimpHistogram *, 
						  GimpDrawable *);
double         gimp_histogram_get_maximum (GimpHistogram *, int channel);
double         gimp_histogram_get_count   (GimpHistogram *,
					   int start, int end);
double         gimp_histogram_get_mean    (GimpHistogram *, int chan,
					   int start, int end);
int            gimp_histogram_get_median  (GimpHistogram *, int chan,
					   int start, int end);
double         gimp_histogram_get_std_dev (GimpHistogram *, int chan,
					   int start, int end);
double         gimp_histogram_get_value   (GimpHistogram *, int channel,
					   int bin);
double         gimp_histogram_get_channel (GimpHistogram *, int color,
					   int bin);
int            gimp_histogram_nchannels   (GimpHistogram *);


#endif /* __GIMP_HISTOGRAM_H__ */

