/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2017 Sébastien Fourey & David Tchumperlé
 * Copyright (C) 2018 Jehan
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

#ifndef __GIMP_LINEART__
#define __GIMP_LINEART__


GeglBuffer * gimp_lineart_close (GeglBuffer  *buffer,
                                 gboolean     select_transparent,
                                 gfloat       stroke_threshold,
                                 gint         minimal_lineart_area,
                                 gint         normal_estimate_mask_size,
                                 gfloat       end_point_rate,
                                 gint         spline_max_length,
                                 gfloat       spline_max_angle,
                                 gint         end_point_connectivity,
                                 gfloat       spline_roundness,
                                 gboolean     allow_self_intersections,
                                 gint         created_regions_significant_area,
                                 gint         created_regions_minimum_area,
                                 gboolean     small_segments_from_spline_sources,
                                 gint         segments_max_length,
                                 gfloat     **lineart_distmap);


#endif /* __GIMP_LINEART__ */
