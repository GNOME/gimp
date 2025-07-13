/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcoords.h
 * Copyright (C) 2002 Simon Budig  <simon@gimp.org>
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

#pragma once


void     gimp_coords_mix            (const gdouble     amul,
                                     const GimpCoords *a,
                                     const gdouble     bmul,
                                     const GimpCoords *b,
                                     GimpCoords       *ret_val);
void     gimp_coords_average        (const GimpCoords *a,
                                     const GimpCoords *b,
                                     GimpCoords       *ret_average);
void     gimp_coords_add            (const GimpCoords *a,
                                     const GimpCoords *b,
                                     GimpCoords       *ret_add);
void     gimp_coords_difference     (const GimpCoords *a,
                                     const GimpCoords *b,
                                     GimpCoords       *difference);
void     gimp_coords_scale          (const gdouble     f,
                                     const GimpCoords *a,
                                     GimpCoords       *ret_product);

gdouble  gimp_coords_scalarprod     (const GimpCoords *a,
                                     const GimpCoords *b);
gdouble  gimp_coords_length         (const GimpCoords *a);
gdouble  gimp_coords_length_squared (const GimpCoords *a);
gdouble  gimp_coords_manhattan_dist (const GimpCoords *a,
                                     const GimpCoords *b);

gboolean gimp_coords_equal          (const GimpCoords *a,
                                     const GimpCoords *b);

gdouble  gimp_coords_direction      (const GimpCoords *a,
                                     const GimpCoords *b);
