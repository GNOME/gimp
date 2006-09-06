/* The GIMP -- an image manipulation program
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

#ifndef __SCALE_FUNCS_H__
#define __SCALE_FUNCS_H__


#define EPSILON          (0.0001)
#define LANCZOS_SPP      (1000)
#define LANCZOS_WIDTH    (4)
#define LANCZOS_WIDTH2   (1 + (LANCZOS_WIDTH * 2))
#define LANCZOS_SAMPLES  (LANCZOS_SPP * (LANCZOS_WIDTH + 1))


void  scale_region (PixelRegion           *srcPR,
                    PixelRegion           *destPR,
                    GimpInterpolationType  interpolation,
                    GimpProgressFunc       progress_callback,
                    gpointer               progress_data);


#endif  /*  __SCALE_FUNCS_H__  */

