/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __SCALE_REGION_H__
#define __SCALE_REGION_H__


#define LANCZOS_SPP      (4000)    /* number of data pts per unit x in lookup table */
#define LANCZOS_MIN      (1.0/LANCZOS_SPP)
#define LANCZOS_WIDTH    (3)       /* 3 for Lanczos3 code, for L4 prefer DUAL_LANCZOS below */
#define LANCZOS_WIDTH2   (1 + (LANCZOS_WIDTH * 2))
#define LANCZOS_SAMPLES  (LANCZOS_SPP * (LANCZOS_WIDTH + 1))


void     scale_region          (PixelRegion           *srcPR,
                                PixelRegion           *destPR,
                                GimpInterpolationType  interpolation,
                                GimpProgressFunc       progress_callback,
                                gpointer               progress_data);

gfloat * create_lanczos_lookup (void);


#endif  /*  __SCALE_REGION_H__  */

/*
 * Determining LANCZOS_SPP value
 *
 * 1000 points per unit will produce typically 1 bit of error per channel
 * on a Lanczos3 window. 4000 should not produce a detectable error caused
 * by lookup table size.
 * On 8b colours ie 24bit RGB this req 80kB of memory comparable to a small
 * 250x150 px image. Filling the array is a small part of  the time required
 * for the transform.
 * This will need reviewing when GIMP handles images with more bytes per pixel.
 */

