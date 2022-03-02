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

#ifndef  __GIMP_DRAWABLE_BUCKET_FILL_H__
#define  __GIMP_DRAWABLE_BUCKET_FILL_H__


void         gimp_drawable_bucket_fill              (GimpDrawable         *drawable,
                                                     GimpFillOptions      *options,
                                                     gboolean              fill_transparent,
                                                     GimpSelectCriterion   fill_criterion,
                                                     gdouble               threshold,
                                                     gboolean              sample_merged,
                                                     gboolean              diagonal_neighbors,
                                                     gdouble               x,
                                                     gdouble               y);
GeglBuffer * gimp_drawable_get_bucket_fill_buffer   (GimpDrawable         *drawable,
                                                     GimpFillOptions      *options,
                                                     gboolean              fill_transparent,
                                                     GimpSelectCriterion   fill_criterion,
                                                     gdouble               threshold,
                                                     gboolean              show_all,
                                                     gboolean              sample_merged,
                                                     gboolean              diagonal_neighbors,
                                                     gdouble               seed_x,
                                                     gdouble               seed_y,
                                                     GeglBuffer          **mask_buffer,
                                                     gdouble              *mask_x,
                                                     gdouble              *mask_y,
                                                     gint                 *mask_width,
                                                     gint                 *mask_height);

GeglBuffer * gimp_drawable_get_line_art_fill_buffer (GimpDrawable         *drawable,
                                                     GimpLineArt          *line_art,
                                                     GimpFillOptions      *options,
                                                     gboolean              sample_merged,
                                                     gboolean              fill_color_as_line_art,
                                                     gdouble               fill_color_threshold,
                                                     gboolean              line_art_stroke,
                                                     GimpStrokeOptions    *stroke_options,
                                                     gdouble               seed_x,
                                                     gdouble               seed_y,
                                                     GeglBuffer          **mask_buffer,
                                                     gdouble              *mask_x,
                                                     gdouble              *mask_y,
                                                     gint                 *mask_width,
                                                     gint                 *mask_height);

#endif  /*  __GIMP_DRAWABLE_BUCKET_FILL_H__  */
