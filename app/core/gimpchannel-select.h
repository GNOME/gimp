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

#ifndef __GIMP_CHANNEL_SELECT_H__
#define __GIMP_CHANNEL_SELECT_H__


/*  basic selection functions  */

void   gimp_channel_select_rectangle    (GimpChannel         *channel,
                                         gint                 x,
                                         gint                 y,
                                         gint                 w,
                                         gint                 h,
                                         GimpChannelOps       op,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y,
                                         gboolean             push_undo);
void   gimp_channel_select_ellipse      (GimpChannel         *channel,
                                         gint                 x,
                                         gint                 y,
                                         gint                 w,
                                         gint                 h,
                                         GimpChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y,
                                         gboolean             push_undo);
void   gimp_channel_select_round_rect   (GimpChannel         *channel,
                                         gint                 x,
                                         gint                 y,
                                         gint                 w,
                                         gint                 h,
                                         gdouble              corner_radius_x,
                                         gdouble              corner_radius_y,
                                         GimpChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y,
                                         gboolean             push_undo);

/*  select by GimpScanConvert functions  */

void   gimp_channel_select_scan_convert (GimpChannel         *channel,
                                         const gchar         *undo_desc,
                                         GimpScanConvert     *scan_convert,
                                         gint                 offset_x,
                                         gint                 offset_y,
                                         GimpChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y,
                                         gboolean             push_undo);
void   gimp_channel_select_polygon      (GimpChannel         *channel,
                                         const gchar         *undo_desc,
                                         gint                 n_points,
                                         const GimpVector2   *points,
                                         GimpChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y,
                                         gboolean             push_undo);
void   gimp_channel_select_path         (GimpChannel         *channel,
                                         const gchar         *undo_desc,
                                         GimpPath            *vectors,
                                         GimpChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y,
                                         gboolean             push_undo);
void   gimp_channel_select_buffer       (GimpChannel         *channel,
                                         const gchar         *undo_desc,
                                         GeglBuffer          *add_on,
                                         gint                 offset_x,
                                         gint                 offset_y,
                                         GimpChannelOps       op,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);


/*  select by GimpChannel functions  */

void   gimp_channel_select_channel      (GimpChannel         *channel,
                                         const gchar         *undo_desc,
                                         GimpChannel         *add_on,
                                         gint                 offset_x,
                                         gint                 offset_y,
                                         GimpChannelOps       op,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);
void   gimp_channel_select_alpha        (GimpChannel         *channel,
                                         GimpDrawable        *drawable,
                                         GimpChannelOps       op,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);
void   gimp_channel_select_component    (GimpChannel         *channel,
                                         GimpChannelType      component,
                                         GimpChannelOps       op,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);
void   gimp_channel_select_fuzzy        (GimpChannel         *channel,
                                         GimpDrawable        *drawable,
                                         gboolean             sample_merged,
                                         gint                 x,
                                         gint                 y,
                                         gfloat               threshold,
                                         gboolean             select_transparent,
                                         GimpSelectCriterion  select_criterion,
                                         gboolean             diagonal_neighbors,
                                         GimpChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);
void   gimp_channel_select_by_color     (GimpChannel         *channel,
                                         GList               *drawables,
                                         gboolean             sample_merged,
                                         GeglColor           *color,
                                         gfloat               threshold,
                                         gboolean             select_transparent,
                                         GimpSelectCriterion  select_criterion,
                                         GimpChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);
void   gimp_channel_select_by_index     (GimpChannel         *channel,
                                         GimpDrawable        *drawable,
                                         gint                 index,
                                         GimpChannelOps       op,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);


#endif  /*  __GIMP_CHANNEL_SELECT_H__  */
