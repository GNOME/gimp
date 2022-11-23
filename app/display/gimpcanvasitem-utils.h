/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvasitem-utils.h
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CANVAS_ITEM_UTILS_H__
#define __LIGMA_CANVAS_ITEM_UTILS_H__


gboolean   ligma_canvas_item_on_handle           (LigmaCanvasItem    *item,
                                                 gdouble            x,
                                                 gdouble            y,
                                                 LigmaHandleType     type,
                                                 gdouble            handle_x,
                                                 gdouble            handle_y,
                                                 gint               width,
                                                 gint               height,
                                                 LigmaHandleAnchor   anchor);

gboolean   ligma_canvas_item_on_vectors_handle   (LigmaCanvasItem    *item,
                                                 LigmaVectors       *vectors,
                                                 const LigmaCoords  *coord,
                                                 gint               width,
                                                 gint               height,
                                                 LigmaAnchorType     preferred,
                                                 gboolean           exclusive,
                                                 LigmaAnchor       **ret_anchor,
                                                 LigmaStroke       **ret_stroke);
gboolean   ligma_canvas_item_on_vectors_curve    (LigmaCanvasItem    *item,
                                                 LigmaVectors       *vectors,
                                                 const LigmaCoords  *coord,
                                                 gint               width,
                                                 gint               height,
                                                 LigmaCoords        *ret_coords,
                                                 gdouble           *ret_pos,
                                                 LigmaAnchor       **ret_segment_start,
                                                 LigmaAnchor       **ret_segment_end,
                                                 LigmaStroke       **ret_stroke);
gboolean   ligma_canvas_item_on_vectors          (LigmaCanvasItem    *item,
                                                 const LigmaCoords  *coords,
                                                 gint               width,
                                                 gint               height,
                                                 LigmaCoords        *ret_coords,
                                                 gdouble           *ret_pos,
                                                 LigmaAnchor       **ret_segment_start,
                                                 LigmaAnchor       **ret_segment_end,
                                                 LigmaStroke       **ret_stroke,
                                                 LigmaVectors      **ret_vectors);

void       ligma_canvas_item_shift_to_north_west (LigmaHandleAnchor   anchor,
                                                 gdouble            x,
                                                 gdouble            y,
                                                 gint               width,
                                                 gint               height,
                                                 gdouble           *shifted_x,
                                                 gdouble           *shifted_y);
void       ligma_canvas_item_shift_to_center     (LigmaHandleAnchor   anchor,
                                                 gdouble            x,
                                                 gdouble            y,
                                                 gint               width,
                                                 gint               height,
                                                 gdouble           *shifted_x,
                                                 gdouble           *shifted_y);


#endif /* __LIGMA_CANVAS_ITEM_UTILS_H__ */
