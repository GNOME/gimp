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

#ifndef __LUT_FUNCS_H__
#define __LUT_FUNCS_H__

#include "gimplutF.h"
#include "gimphistogramF.h"

typedef enum /*< chop=_LUT >*/
{
  VALUE_LUT,    /*< nick=VALUE/GRAY >*/
  RED_LUT,
  GREEN_LUT,
  BLUE_LUT,
  GRAY_LUT = 0  /*< skip >*/
} ChannelLutType;

/* brightness contrast */
void     brightness_contrast_lut_setup (GimpLut *lut,
					double brightness, double contrast,
					int nchannels);
GimpLut *brightness_contrast_lut_new   (double brightness, double contrast,
					int nchannels);

/* invert */
void     invert_lut_setup (GimpLut *lut, int nchannels);
GimpLut *invert_lut_new   (int nchannels);

/* add (or subtract) */
void     add_lut_setup (GimpLut *lut, double ammount, int nchannels);
GimpLut *add_lut_new   (double ammount, int nchannels);

/* intersect (MINIMUM(pixel, value)) */
void     intersect_lut_setup (GimpLut *lut, double value, int nchannels);
GimpLut *intersect_lut_new   (double value, int nchannels);

/* threshold */
void     threshold_lut_setup (GimpLut *lut, double value, int nchannels);
GimpLut *threshold_lut_new   (double value, int nchannels);

/* levels */
void     levels_lut_setup (GimpLut *lut, double *gamma,
			   int *low_input, int *high_input,
			   int *low_output, int *high_output, int nchannels);
GimpLut *levels_lut_new   (double *gamma, int *low_input, int *high_input,
			   int *low_output, int *high_output, int nchannels);
/* posterize */
void     posterize_lut_setup (GimpLut *lut, int levels, int nchannels);
GimpLut *posterize_lut_new   (int levels, int nchannels);

/* equalize histogram */
void     eq_histogram_lut_setup (GimpLut *lut, GimpHistogram *hist, int bytes);
GimpLut *eq_histogram_lut_new   (GimpHistogram *h, int nchannels);

#endif /* __LUT_FUNCS_H__ */
