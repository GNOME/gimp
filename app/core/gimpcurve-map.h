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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_CURVE_MAP_H__
#define __GIMP_CURVE_MAP_H__


gdouble         gimp_curve_map_value         (GimpCurve     *curve,
                                              gdouble        value);
void            gimp_curve_map_pixels        (GimpCurve     *curve_colors,
                                              GimpCurve     *curve_red,
                                              GimpCurve     *curve_green,
                                              GimpCurve     *curve_blue,
                                              GimpCurve     *curve_alpha,
                                              gfloat        *src,
                                              gfloat        *dest,
                                              glong          samples);


#endif /* __GIMP_CURVE_MAP_H__ */
