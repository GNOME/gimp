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

#ifndef __GIMP_DRAWABLE_COLOR_BALANCE_H__
#define __GIMP_DRAWABLE_COLOR_BALANCE_H__


void   gimp_drawable_color_balance (GimpDrawable     *drawable,
                                    GimpProgress     *progress,
                                    GimpTransferMode  range,
                                    gdouble           cyan_red,
                                    gdouble           magenta_green,
                                    gdouble           yellow_blue,
                                    gboolean          preserve_luminosity);


#endif  /*  __GIMP_DRAWABLE_COLOR_BALANCE_H__  */
