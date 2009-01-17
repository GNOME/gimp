/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcoords-interpolate.h
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

#ifndef __GIMP_COORDS_INTERPOLATE_H__
#define __GIMP_COORDS_INTERPOLATE_H__

void      gimp_coords_interpolate_bezier  (const GimpCoords bezier_pt1,
                                           const GimpCoords bezier_pt2,
                                           const GimpCoords bezier_pt3,
                                           const GimpCoords bezier_pt4,
                                           gdouble          precision,
                                           GArray           **ret_coords,
                                           GArray           **ret_params);

gboolean  gimp_coords_bezier_is_straight  (const GimpCoords bezier_pt1,
                                           const GimpCoords bezier_pt2,
                                           const GimpCoords bezier_pt3,
                                           const GimpCoords bezier_pt4,
                                           gdouble          precision);

void      gimp_coords_interpolate_catmull (const GimpCoords catmul_pt1,
                                           const GimpCoords catmul_pt2,
                                           const GimpCoords catmul_pt3,
                                           const GimpCoords catmul_pt4,
                                           gdouble          precision,
                                           GArray           **ret_coords,
                                           GArray           **ret_params);

#endif /* __GIMP_COORDS_INTERPOLATE_H__ */
