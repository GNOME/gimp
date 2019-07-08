/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#ifndef __GIMP_TRANSFORM_RESIZE_H__
#define __GIMP_TRANSFORM_RESIZE_H__


void   gimp_transform_resize_boundary (const GimpMatrix3   *inv,
                                       GimpTransformResize  resize,
                                       gint                 u1,
                                       gint                 v1,
                                       gint                 u2,
                                       gint                 v2,
                                       gint                *x1,
                                       gint                *y1,
                                       gint                *x2,
                                       gint                *y2);


#endif  /*  __GIMP_TRANSFORM_RESIZE_H__  */
