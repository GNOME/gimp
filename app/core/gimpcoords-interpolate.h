/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacoords-interpolate.h
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

#ifndef __LIGMA_COORDS_INTERPOLATE_H__
#define __LIGMA_COORDS_INTERPOLATE_H__

void       ligma_coords_interpolate_bezier    (const LigmaCoords  bezier_pt[4],
                                              gdouble           precision,
                                              GArray           *ret_coords,
                                              GArray           *ret_params);

void       ligma_coords_interpolate_bezier_at (const LigmaCoords  bezier_pt[4],
                                              gdouble           t,
                                              LigmaCoords       *position,
                                              LigmaCoords       *velocity);

gboolean   ligma_coords_bezier_is_straight    (const LigmaCoords  bezier_pt[4],
                                              gdouble           precision);

void       ligma_coords_interpolate_catmull   (const LigmaCoords  catmull_pt[4],
                                              gdouble           precision,
                                              GArray           *ret_coords,
                                              GArray           *ret_params);

#endif /* __LIGMA_COORDS_INTERPOLATE_H__ */
