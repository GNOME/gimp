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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_PAINT_CORE_LOOPS_H__
#define __GIMP_PAINT_CORE_LOOPS_H__


typedef enum
{
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_NONE                                = 0,

  GIMP_PAINT_CORE_LOOPS_ALGORITHM_COMBINE_PAINT_MASK_TO_CANVAS_BUFFER = 1 << 0,
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_PAINT_BUF_ALPHA    = 1 << 1,
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_PAINT_BUF_ALPHA       = 1 << 2,
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_COMP_MASK          = 1 << 3,
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_COMP_MASK             = 1 << 4,
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_DO_LAYER_BLEND                      = 1 << 5,
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_MASK_COMPONENTS                     = 1 << 6
} GimpPaintCoreLoopsAlgorithm;


typedef struct
{
  GeglBuffer        *canvas_buffer;

  GimpTempBuf       *paint_buf;
  gint               paint_buf_offset_x;
  gint               paint_buf_offset_y;

  const GimpTempBuf *paint_mask;
  gint               paint_mask_offset_x;
  gint               paint_mask_offset_y;

  gboolean           stipple;

  GeglBuffer        *src_buffer;
  GeglBuffer        *dest_buffer;

  GeglBuffer        *mask_buffer;
  gint               mask_offset_x;
  gint               mask_offset_y;

  gdouble            paint_opacity;
  gdouble            image_opacity;

  GimpLayerMode      paint_mode;

  GimpComponentMask  affect;
} GimpPaintCoreLoopsParams;


void   gimp_paint_core_loops_process (const GimpPaintCoreLoopsParams *params,
                                      GimpPaintCoreLoopsAlgorithm     algorithms);


#endif /* __GIMP_PAINT_CORE_LOOPS_H__ */
