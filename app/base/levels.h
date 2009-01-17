/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
};


void     levels_init     (Levels *levels);
gfloat   levels_lut_func (Levels *levels,
                          gint    n_channels,
                          gint    channel,
                          gfloat  value);


#endif  /*  __LEVELS_H__  */
