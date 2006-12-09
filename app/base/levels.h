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

#ifndef __LEVELS_H__
#define __LEVELS_H__


struct _Levels
{
  gdouble gamma[5];

  gint    low_input[5];
  gint    high_input[5];

  gint    low_output[5];
  gint    high_output[5];

  guchar  input[5][256]; /* this is used only by the gui */
};


void     levels_init                (Levels               *levels);
void     levels_channel_reset       (Levels               *levels,
                                     GimpHistogramChannel  channel);
void     levels_stretch             (Levels               *levels,
                                     GimpHistogram        *hist,
                                     gboolean              is_color);
void     levels_channel_stretch     (Levels               *levels,
                                     GimpHistogram        *hist,
                                     GimpHistogramChannel  channel);
void     levels_adjust_by_colors    (Levels               *levels,
                                     GimpHistogramChannel  channel,
                                     guchar               *black,
                                     guchar               *gray,
                                     guchar               *white);
void     levels_calculate_transfers (Levels               *levels);
gfloat   levels_lut_func            (Levels               *levels,
                                     gint                  n_channels,
                                     gint                  channel,
                                     gfloat                value);


#endif  /*  __LEVELS_H__  */
