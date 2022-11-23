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

#ifndef __LIGMA_CHANNEL_SELECT_H__
#define __LIGMA_CHANNEL_SELECT_H__


/*  basic selection functions  */

void   ligma_channel_select_rectangle    (LigmaChannel         *channel,
                                         gint                 x,
                                         gint                 y,
                                         gint                 w,
                                         gint                 h,
                                         LigmaChannelOps       op,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y,
                                         gboolean             push_undo);
void   ligma_channel_select_ellipse      (LigmaChannel         *channel,
                                         gint                 x,
                                         gint                 y,
                                         gint                 w,
                                         gint                 h,
                                         LigmaChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y,
                                         gboolean             push_undo);
void   ligma_channel_select_round_rect   (LigmaChannel         *channel,
                                         gint                 x,
                                         gint                 y,
                                         gint                 w,
                                         gint                 h,
                                         gdouble              corner_radius_y,
                                         gdouble              corner_radius_x,
                                         LigmaChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y,
                                         gboolean             push_undo);

/*  select by LigmaScanConvert functions  */

void   ligma_channel_select_scan_convert (LigmaChannel         *channel,
                                         const gchar         *undo_desc,
                                         LigmaScanConvert     *scan_convert,
                                         gint                 offset_x,
                                         gint                 offset_y,
                                         LigmaChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y,
                                         gboolean             push_undo);
void   ligma_channel_select_polygon      (LigmaChannel         *channel,
                                         const gchar         *undo_desc,
                                         gint                 n_points,
                                         const LigmaVector2   *points,
                                         LigmaChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y,
                                         gboolean             push_undo);
void   ligma_channel_select_vectors      (LigmaChannel         *channel,
                                         const gchar         *undo_desc,
                                         LigmaVectors         *vectors,
                                         LigmaChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y,
                                         gboolean             push_undo);
void   ligma_channel_select_buffer       (LigmaChannel         *channel,
                                         const gchar         *undo_desc,
                                         GeglBuffer          *add_on,
                                         gint                 offset_x,
                                         gint                 offset_y,
                                         LigmaChannelOps       op,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);


/*  select by LigmaChannel functions  */

void   ligma_channel_select_channel      (LigmaChannel         *channel,
                                         const gchar         *undo_desc,
                                         LigmaChannel         *add_on,
                                         gint                 offset_x,
                                         gint                 offset_y,
                                         LigmaChannelOps       op,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);
void   ligma_channel_select_alpha        (LigmaChannel         *channel,
                                         LigmaDrawable        *drawable,
                                         LigmaChannelOps       op,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);
void   ligma_channel_select_component    (LigmaChannel         *channel,
                                         LigmaChannelType      component,
                                         LigmaChannelOps       op,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);
void   ligma_channel_select_fuzzy        (LigmaChannel         *channel,
                                         LigmaDrawable        *drawable,
                                         gboolean             sample_merged,
                                         gint                 x,
                                         gint                 y,
                                         gfloat               threshold,
                                         gboolean             select_transparent,
                                         LigmaSelectCriterion  select_criterion,
                                         gboolean             diagonal_neighbors,
                                         LigmaChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);
void   ligma_channel_select_by_color     (LigmaChannel         *channel,
                                         GList               *drawables,
                                         gboolean             sample_merged,
                                         const LigmaRGB       *color,
                                         gfloat               threshold,
                                         gboolean             select_transparent,
                                         LigmaSelectCriterion  select_criterion,
                                         LigmaChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);
void   ligma_channel_select_by_index     (LigmaChannel         *channel,
                                         LigmaDrawable        *drawable,
                                         gint                 index,
                                         LigmaChannelOps       op,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);


#endif  /*  __LIGMA_CHANNEL_SELECT_H__  */
