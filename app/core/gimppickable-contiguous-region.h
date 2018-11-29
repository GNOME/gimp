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

#ifndef __GIMP_PICKABLE_CONTIGUOUS_REGION_H__
#define __GIMP_PICKABLE_CONTIGUOUS_REGION_H__

typedef struct
{
  GeglBuffer *line_art;
  gfloat     *distmap;
} GimpPickableLineArtAsyncResult;

GeglBuffer * gimp_pickable_contiguous_region_prepare_line_art       (GimpPickable        *pickable,
                                                                     gboolean             select_transparent,
                                                                     gfloat               stroke_threshold,
                                                                     gint                 segment_max_length,
                                                                     gint                 spline_max_length,
                                                                     gfloat             **distmap);
GimpAsync  * gimp_pickable_contiguous_region_prepare_line_art_async (GimpPickable        *pickable,
                                                                     gboolean             select_transparent,
                                                                     gfloat               stroke_threshold,
                                                                     gint                 segment_max_length,
                                                                     gint                 spline_max_length,
                                                                     gint                 priority);

GeglBuffer * gimp_pickable_contiguous_region_by_seed                (GimpPickable        *pickable,
                                                                     GeglBuffer          *line_art,
                                                                     gfloat              *line_art_distmap,
                                                                     gboolean             antialias,
                                                                     gfloat               threshold,
                                                                     gboolean             select_transparent,
                                                                     GimpSelectCriterion  select_criterion,
                                                                     gboolean             diagonal_neighbors,
                                                                     gfloat               line_art_stroke_threshold,
                                                                     gint                 line_art_max_grow,
                                                                     gint                 line_art_segment_max_length,
                                                                     gint                 line_art_spline_max_length,
                                                                     gint                 x,
                                                                     gint                 y);

GeglBuffer * gimp_pickable_contiguous_region_by_color               (GimpPickable        *pickable,
                                                                     gboolean             antialias,
                                                                     gfloat               threshold,
                                                                     gboolean             select_transparent,
                                                                     GimpSelectCriterion  select_criterion,
                                                                     const GimpRGB       *color);


#endif  /*  __GIMP_PICKABLE_CONTIGUOUS_REGION_H__ */
