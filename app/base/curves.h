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

#ifndef __CURVES_H__
#define __CURVES_H__

#define CURVES_NUM_POINTS 17

struct _Curves
{
  GimpCurveType curve_type[5];
  gint          points[5][CURVES_NUM_POINTS][2];
  guchar        curve[5][256];
};


void    curves_init            (Curves               *curves);
void    curves_channel_reset   (Curves               *curves,
                                GimpHistogramChannel  channel);
void    curves_calculate_curve (Curves               *curves,
                                GimpHistogramChannel  channel);
gfloat  curves_lut_func        (Curves               *curves,
                                gint                  nchannels,
                                gint                  channel,
                                gfloat                value);


#endif  /*  __CURVES_H__  */
