/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasitem-utils.h
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_CANVAS_ITEM_UTILS_H__
#define __GIMP_CANVAS_ITEM_UTILS_H__


gboolean   gimp_canvas_item_on_handle           (GimpCanvasItem    *item,
                                                 gdouble            x,
                                                 gdouble            y,
                                                 GimpHandleType     type,
                                                 gdouble            handle_x,
                                                 gdouble            handle_y,
                                                 gint               width,
                                                 gint               height,
                                                 GimpHandleAnchor   anchor);

gboolean   gimp_canvas_item_on_vectors_handle   (GimpCanvasItem    *item,
                                                 GimpPath          *vectors,
                                                 const GimpCoords  *coord,
                                                 gint               width,
                                                 gint               height,
                                                 GimpAnchorType     preferred,
                                                 gboolean           exclusive,
                                                 GimpAnchor       **ret_anchor,
                                                 GimpStroke       **ret_stroke);
gboolean   gimp_canvas_item_on_vectors_curve    (GimpCanvasItem    *item,
                                                 GimpPath          *vectors,
                                                 const GimpCoords  *coord,
                                                 gint               width,
                                                 gint               height,
                                                 GimpCoords        *ret_coords,
                                                 gdouble           *ret_pos,
                                                 GimpAnchor       **ret_segment_start,
                                                 GimpAnchor       **ret_segment_end,
                                                 GimpStroke       **ret_stroke);
gboolean   gimp_canvas_item_on_path             (GimpCanvasItem    *item,
                                                 const GimpCoords  *coords,
                                                 gint               width,
                                                 gint               height,
                                                 GimpCoords        *ret_coords,
                                                 gdouble           *ret_pos,
                                                 GimpAnchor       **ret_segment_start,
                                                 GimpAnchor       **ret_segment_end,
                                                 GimpStroke       **ret_stroke,
                                                 GimpPath         **ret_path);

void       gimp_canvas_item_shift_to_north_west (GimpHandleAnchor   anchor,
                                                 gdouble            x,
                                                 gdouble            y,
                                                 gint               width,
                                                 gint               height,
                                                 gdouble           *shifted_x,
                                                 gdouble           *shifted_y);
void       gimp_canvas_item_shift_to_center     (GimpHandleAnchor   anchor,
                                                 gdouble            x,
                                                 gdouble            y,
                                                 gint               width,
                                                 gint               height,
                                                 gdouble           *shifted_x,
                                                 gdouble           *shifted_y);


#endif /* __GIMP_CANVAS_ITEM_UTILS_H__ */
