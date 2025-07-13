/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattisbvf
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


typedef struct _GimpImageSnapData
{
  GimpAlignmentType  snapped_side_horizontal; /* if GIMP_ARRANGE_HFILL then no snapping happen */
  GimpLayer         *snapped_layer_horizontal;

  GimpAlignmentType  snapped_side_vertical; /* if GIMP_ARRANGE_HFILL then no snapping happen */
  GimpLayer         *snapped_layer_vertical;

  GimpAlignmentType  equidistance_side_horizontal;
  GimpLayer         *near_layer_horizontal1;
  GimpLayer         *near_layer_horizontal2;

  GimpAlignmentType  equidistance_side_vertical;
  GimpLayer         *near_layer_vertical1;
  GimpLayer         *near_layer_vertical2;
} GimpImageSnapData;


gboolean    gimp_image_snap_x         (GimpImage         *image,
                                       GimpImageSnapData *snapping_data,
                                       gdouble            x,
                                       gdouble           *tx,
                                       gdouble            epsilon_x,
                                       gboolean           snap_to_guides,
                                       gboolean           snap_to_grid,
                                       gboolean           snap_to_canvas,
                                       gboolean           snap_to_bbox,
                                       gboolean           snap_to_equidistance,
                                       GimpAlignmentType  alignment_side);
gboolean    gimp_image_snap_y         (GimpImage         *image,
                                       GimpImageSnapData *snapping_data,
                                       gdouble            y,
                                       gdouble           *ty,
                                       gdouble            epsilon_y,
                                       gboolean           snap_to_guides,
                                       gboolean           snap_to_grid,
                                       gboolean           snap_to_canvas,
                                       gboolean           snap_to_bbox,
                                       gboolean           snap_to_equidistance,
                                       GimpAlignmentType  alignment_side);
gboolean    gimp_image_snap_point     (GimpImage         *image,
                                       gdouble            x,
                                       gdouble            y,
                                       gdouble           *tx,
                                       gdouble           *ty,
                                       gdouble            epsilon_x,
                                       gdouble            epsilon_y,
                                       gboolean           snap_to_guides,
                                       gboolean           snap_to_grid,
                                       gboolean           snap_to_canvas,
                                       gboolean           snap_to_path,
                                       gboolean           snap_to_bbox,
                                       gboolean           show_all);
gboolean   gimp_image_snap_rectangle  (GimpImage         *image,
                                       GimpImageSnapData *snapping_data,
                                       gdouble            x1,
                                       gdouble            y1,
                                       gdouble            x2,
                                       gdouble            y2,
                                       gdouble           *tx1,
                                       gdouble           *ty1,
                                       gdouble            epsilon_x,
                                       gdouble            epsilon_y,
                                       gboolean           snap_to_guides,
                                       gboolean           snap_to_grid,
                                       gboolean           snap_to_canvas,
                                       gboolean           snap_to_path,
                                       gboolean           snap_to_bbox,
                                       gboolean           snap_to_equidistance);
