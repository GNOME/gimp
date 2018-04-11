/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 2013 Daniel Sabo
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_PAINT_CORE_LOOPS_H__
#define __GIMP_PAINT_CORE_LOOPS_H__


void combine_paint_mask_to_canvas_mask  (const GimpTempBuf *paint_mask,
                                         gint               mask_x_offset,
                                         gint               mask_y_offset,
                                         GeglBuffer        *canvas_buffer,
                                         gint               x_offset,
                                         gint               y_offset,
                                         gfloat             opacity,
                                         gboolean           stipple);

void canvas_buffer_to_paint_buf_alpha   (GimpTempBuf  *paint_buf,
                                         GeglBuffer   *canvas_buffer,
                                         gint          x_offset,
                                         gint          y_offset);

void paint_mask_to_paint_buffer         (const GimpTempBuf  *paint_mask,
                                         gint                mask_x_offset,
                                         gint                mask_y_offset,
                                         GimpTempBuf        *paint_buf,
                                         gfloat              paint_opacity);

void do_layer_blend                     (GeglBuffer    *src_buffer,
                                         GeglBuffer    *dst_buffer,
                                         GimpTempBuf   *paint_buf,
                                         GeglBuffer    *mask_buffer,
                                         gfloat         opacity,
                                         gint           x_offset,
                                         gint           y_offset,
                                         gint           mask_x_offset,
                                         gint           mask_y_offset,
                                         GimpLayerMode  paint_mode);

void mask_components_onto               (GeglBuffer        *src_buffer,
                                         GeglBuffer        *aux_buffer,
                                         GeglBuffer        *dst_buffer,
                                         GeglRectangle     *roi,
                                         GimpComponentMask  mask,
                                         gboolean           linear_mode);


#endif /* __GIMP_PAINT_CORE_LOOPS_H__ */
