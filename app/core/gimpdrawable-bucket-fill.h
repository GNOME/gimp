/* LIGMA - The GNU Image Manipulation Program
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

#ifndef  __LIGMA_DRAWABLE_BUCKET_FILL_H__
#define  __LIGMA_DRAWABLE_BUCKET_FILL_H__


void         ligma_drawable_bucket_fill              (LigmaDrawable         *drawable,
                                                     LigmaFillOptions      *options,
                                                     gboolean              fill_transparent,
                                                     LigmaSelectCriterion   fill_criterion,
                                                     gdouble               threshold,
                                                     gboolean              sample_merged,
                                                     gboolean              diagonal_neighbors,
                                                     gdouble               x,
                                                     gdouble               y);
GeglBuffer * ligma_drawable_get_bucket_fill_buffer   (LigmaDrawable         *drawable,
                                                     LigmaFillOptions      *options,
                                                     gboolean              fill_transparent,
                                                     LigmaSelectCriterion   fill_criterion,
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

GeglBuffer * ligma_drawable_get_line_art_fill_buffer (LigmaDrawable         *drawable,
                                                     LigmaLineArt          *line_art,
                                                     LigmaFillOptions      *options,
                                                     gboolean              sample_merged,
                                                     gboolean              fill_color_as_line_art,
                                                     gdouble               fill_color_threshold,
                                                     gboolean              line_art_stroke,
                                                     LigmaStrokeOptions    *stroke_options,
                                                     gdouble               seed_x,
                                                     gdouble               seed_y,
                                                     GeglBuffer          **mask_buffer,
                                                     gdouble              *mask_x,
                                                     gdouble              *mask_y,
                                                     gint                 *mask_width,
                                                     gint                 *mask_height);

#endif  /*  __LIGMA_DRAWABLE_BUCKET_FILL_H__  */
