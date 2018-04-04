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

#include "config.h"

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

extern "C"
{

#include "paint-types.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "core/gimptempbuf.h"

#include "operations/layer-modes/gimpoperationlayermode.h"

#include "gimppaintcore-loops.h"


void
combine_paint_mask_to_canvas_mask (const GimpTempBuf *paint_mask,
                                   gint               mask_x_offset,
                                   gint               mask_y_offset,
                                   GeglBuffer        *canvas_buffer,
                                   gint               x_offset,
                                   gint               y_offset,
                                   gfloat             opacity,
                                   gboolean           stipple)
{
  GeglRectangle       roi;
  GeglBufferIterator *iter;

  const gint   mask_stride       = gimp_temp_buf_get_width (paint_mask);
  const gint   mask_start_offset = mask_y_offset * mask_stride + mask_x_offset;
  const Babl  *mask_format       = gimp_temp_buf_get_format (paint_mask);
  gint         width;
  gint         height;

  width  = gimp_temp_buf_get_width (paint_mask);
  height = gimp_temp_buf_get_height (paint_mask);

  roi.x = x_offset;
  roi.y = y_offset;
  roi.width  = width - mask_x_offset;
  roi.height = height - mask_y_offset;

  iter = gegl_buffer_iterator_new (canvas_buffer, &roi, 0,
                                   babl_format ("Y float"),
                                   GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);

  if (stipple)
    {
      if (mask_format == babl_format ("Y u8"))
        {
          const guint8 *mask_data = (const guint8 *) gimp_temp_buf_get_data (paint_mask);
          mask_data += mask_start_offset;

          while (gegl_buffer_iterator_next (iter))
            {
              gfloat *out_pixel = (gfloat *)iter->data[0];
              int iy, ix;

              for (iy = 0; iy < iter->roi[0].height; iy++)
                {
                  int mask_offset = (iy + iter->roi[0].y - roi.y) * mask_stride + iter->roi[0].x - roi.x;
                  const guint8 *mask_pixel = &mask_data[mask_offset];

                  for (ix = 0; ix < iter->roi[0].width; ix++)
                    {
                      out_pixel[0] += (1.0 - out_pixel[0]) * (*mask_pixel / 255.0f) * opacity;

                      mask_pixel += 1;
                      out_pixel  += 1;
                    }
                }
            }
        }
      else if (mask_format == babl_format ("Y float"))
        {
          const gfloat *mask_data = (const gfloat *) gimp_temp_buf_get_data (paint_mask);
          mask_data += mask_start_offset;

          while (gegl_buffer_iterator_next (iter))
            {
              gfloat *out_pixel = (gfloat *)iter->data[0];
              int iy, ix;

              for (iy = 0; iy < iter->roi[0].height; iy++)
                {
                  int mask_offset = (iy + iter->roi[0].y - roi.y) * mask_stride + iter->roi[0].x - roi.x;
                  const gfloat *mask_pixel = &mask_data[mask_offset];

                  for (ix = 0; ix < iter->roi[0].width; ix++)
                    {
                      out_pixel[0] += (1.0 - out_pixel[0]) * (*mask_pixel) * opacity;

                      mask_pixel += 1;
                      out_pixel  += 1;
                    }
                }
            }
        }
      else
        {
          g_warning("Mask format not supported: %s", babl_get_name (mask_format));
        }
    }
  else
    {
      if (mask_format == babl_format ("Y u8"))
        {
          const guint8 *mask_data = (const guint8 *) gimp_temp_buf_get_data (paint_mask);
          mask_data += mask_start_offset;

          while (gegl_buffer_iterator_next (iter))
            {
              gfloat *out_pixel = (gfloat *)iter->data[0];
              int iy, ix;

              for (iy = 0; iy < iter->roi[0].height; iy++)
                {
                  int mask_offset = (iy + iter->roi[0].y - roi.y) * mask_stride + iter->roi[0].x - roi.x;
                  const guint8 *mask_pixel = &mask_data[mask_offset];

                  for (ix = 0; ix < iter->roi[0].width; ix++)
                    {
                      if (opacity > out_pixel[0])
                        out_pixel[0] += (opacity - out_pixel[0]) * (*mask_pixel / 255.0f) * opacity;

                      mask_pixel += 1;
                      out_pixel  += 1;
                    }
                }
            }
        }
      else if (mask_format == babl_format ("Y float"))
        {
          const gfloat *mask_data = (const gfloat *) gimp_temp_buf_get_data (paint_mask);
          mask_data += mask_start_offset;

          while (gegl_buffer_iterator_next (iter))
            {
              gfloat *out_pixel = (gfloat *)iter->data[0];
              int iy, ix;

              for (iy = 0; iy < iter->roi[0].height; iy++)
                {
                  int mask_offset = (iy + iter->roi[0].y - roi.y) * mask_stride + iter->roi[0].x - roi.x;
                  const gfloat *mask_pixel = &mask_data[mask_offset];

                  for (ix = 0; ix < iter->roi[0].width; ix++)
                    {
                      if (opacity > out_pixel[0])
                        out_pixel[0] += (opacity - out_pixel[0]) * (*mask_pixel) * opacity;

                      mask_pixel += 1;
                      out_pixel  += 1;
                    }
                }
            }
        }
      else
        {
          g_warning("Mask format not supported: %s", babl_get_name (mask_format));
        }
    }
}

void
canvas_buffer_to_paint_buf_alpha (GimpTempBuf  *paint_buf,
                                  GeglBuffer   *canvas_buffer,
                                  gint          x_offset,
                                  gint          y_offset)
{
  /* Copy the canvas buffer in rect to the paint buffer's alpha channel */
  GeglRectangle roi;
  GeglBufferIterator *iter;

  const guint paint_stride = gimp_temp_buf_get_width (paint_buf);
  gfloat *paint_data       = (gfloat *) gimp_temp_buf_get_data (paint_buf);

  roi.x = x_offset;
  roi.y = y_offset;
  roi.width  = gimp_temp_buf_get_width (paint_buf);
  roi.height = gimp_temp_buf_get_height (paint_buf);

  iter = gegl_buffer_iterator_new (canvas_buffer, &roi, 0,
                                   babl_format ("Y float"),
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *canvas_pixel = (gfloat *)iter->data[0];
      int iy, ix;

      for (iy = 0; iy < iter->roi[0].height; iy++)
        {
          int paint_offset = (iy + iter->roi[0].y - roi.y) * paint_stride + iter->roi[0].x - roi.x;
          float *paint_pixel = &paint_data[paint_offset * 4];

          for (ix = 0; ix < iter->roi[0].width; ix++)
            {
              paint_pixel[3] *= *canvas_pixel;

              canvas_pixel += 1;
              paint_pixel  += 4;
            }
        }
    }
}

void
paint_mask_to_paint_buffer (const GimpTempBuf  *paint_mask,
                            gint                mask_x_offset,
                            gint                mask_y_offset,
                            GimpTempBuf        *paint_buf,
                            gfloat              paint_opacity)
{
  gint width  = gimp_temp_buf_get_width (paint_buf);
  gint height = gimp_temp_buf_get_height (paint_buf);

  const gint mask_stride       = gimp_temp_buf_get_width (paint_mask);
  const gint mask_start_offset = mask_y_offset * mask_stride + mask_x_offset;
  const Babl *mask_format      = gimp_temp_buf_get_format (paint_mask);

  int iy, ix;
  gfloat *paint_pixel = (gfloat *)gimp_temp_buf_get_data (paint_buf);

  /* Validate that the paint buffer is withing the bounds of the paint mask */
  g_return_if_fail (width <= gimp_temp_buf_get_width (paint_mask) - mask_x_offset);
  g_return_if_fail (height <= gimp_temp_buf_get_height (paint_mask) - mask_y_offset);

  if (mask_format == babl_format ("Y u8"))
    {
      const guint8 *mask_data = (const guint8 *) gimp_temp_buf_get_data (paint_mask);
      mask_data += mask_start_offset;

      for (iy = 0; iy < height; iy++)
        {
          int mask_offset = iy * mask_stride;
          const guint8 *mask_pixel = &mask_data[mask_offset];

          for (ix = 0; ix < width; ix++)
            {
              paint_pixel[3] *= (((gfloat)*mask_pixel) / 255.0f) * paint_opacity;

              mask_pixel  += 1;
              paint_pixel += 4;
            }
        }
    }
  else if (mask_format == babl_format ("Y float"))
    {
      const gfloat *mask_data = (const gfloat *) gimp_temp_buf_get_data (paint_mask);
      mask_data += mask_start_offset;

      for (iy = 0; iy < height; iy++)
        {
          int mask_offset = iy * mask_stride;
          const gfloat *mask_pixel = &mask_data[mask_offset];

          for (ix = 0; ix < width; ix++)
            {
              paint_pixel[3] *= (*mask_pixel) * paint_opacity;

              mask_pixel  += 1;
              paint_pixel += 4;
            }
        }
    }
}

void
do_layer_blend (GeglBuffer    *src_buffer,
                GeglBuffer    *dst_buffer,
                GimpTempBuf   *paint_buf,
                GeglBuffer    *mask_buffer,
                gfloat         opacity,
                gint           x_offset,
                gint           y_offset,
                gint           mask_x_offset,
                gint           mask_y_offset,
                GimpLayerMode  paint_mode)
{
  GeglRectangle           roi;
  GeglRectangle           mask_roi;
  GeglRectangle           process_roi;
  const Babl             *iterator_format;
  GeglBufferIterator     *iter;
  guint                   paint_stride;
  gfloat                 *paint_data;
  GimpOperationLayerMode  layer_mode;

  paint_stride = gimp_temp_buf_get_width (paint_buf);
  paint_data   = (gfloat *) gimp_temp_buf_get_data (paint_buf);

  layer_mode.layer_mode          = paint_mode;
  layer_mode.opacity             = opacity;
  layer_mode.function            = gimp_layer_mode_get_function (paint_mode);
  layer_mode.blend_function      = gimp_layer_mode_get_blend_function (paint_mode);
  layer_mode.blend_space         = gimp_layer_mode_get_blend_space (paint_mode);
  layer_mode.composite_space     = gimp_layer_mode_get_composite_space (paint_mode);
  layer_mode.composite_mode      = gimp_layer_mode_get_paint_composite_mode (paint_mode);
  layer_mode.real_composite_mode = layer_mode.composite_mode;

  iterator_format = gimp_layer_mode_get_format (paint_mode,
                                                layer_mode.composite_space,
                                                layer_mode.blend_space,
                                                gimp_temp_buf_get_format (paint_buf));

  roi.x = x_offset;
  roi.y = y_offset;
  roi.width  = gimp_temp_buf_get_width (paint_buf);
  roi.height = gimp_temp_buf_get_height (paint_buf);

  mask_roi.x = roi.x - mask_x_offset;
  mask_roi.y = roi.y - mask_y_offset;
  mask_roi.width  = roi.width;
  mask_roi.height = roi.height;

  g_return_if_fail (gimp_temp_buf_get_format (paint_buf) == iterator_format);

  iter = gegl_buffer_iterator_new (dst_buffer, &roi, 0,
                                   iterator_format,
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, src_buffer, &roi, 0,
                            iterator_format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  if (mask_buffer)
    {
      gegl_buffer_iterator_add (iter, mask_buffer, &mask_roi, 0,
                                babl_format ("Y float"),
                                GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
    }

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *out_pixel  = (gfloat *) iter->data[0];
      gfloat *in_pixel   = (gfloat *) iter->data[1];
      gfloat *mask_pixel = NULL;
      gfloat *paint_pixel;
      gint    iy;

      paint_pixel = paint_data + ((iter->roi[0].y - roi.y) * paint_stride + iter->roi[0].x - roi.x) * 4;

      if (mask_buffer)
        mask_pixel  = (gfloat *)iter->data[2];

      process_roi.x = iter->roi[0].x;
      process_roi.width  = iter->roi[0].width;
      process_roi.height = 1;

      for (iy = 0; iy < iter->roi[0].height; iy++)
        {
          process_roi.y = iter->roi[0].y + iy;

          layer_mode.function ((GeglOperation*) &layer_mode,
                               in_pixel,
                               paint_pixel,
                               mask_pixel,
                               out_pixel,
                               iter->roi[0].width,
                               &process_roi,
                               0);

          in_pixel    += iter->roi[0].width * 4;
          out_pixel   += iter->roi[0].width * 4;
          if (mask_buffer)
            mask_pixel  += iter->roi[0].width;
          paint_pixel += paint_stride * 4;
        }
    }
}

void
mask_components_onto (GeglBuffer        *src_buffer,
                      GeglBuffer        *aux_buffer,
                      GeglBuffer        *dst_buffer,
                      GeglRectangle     *roi,
                      GimpComponentMask  mask,
                      gboolean           linear_mode)
{
  GeglBufferIterator *iter;
  const Babl         *iterator_format;

  if (linear_mode)
    iterator_format = babl_format ("RGBA float");
  else
    iterator_format = babl_format ("R'G'B'A float");

  iter = gegl_buffer_iterator_new (dst_buffer, roi, 0,
                                   iterator_format,
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, src_buffer, roi, 0,
                            iterator_format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, aux_buffer, roi, 0,
                            iterator_format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *dest    = (gfloat *)iter->data[0];
      gfloat *src     = (gfloat *)iter->data[1];
      gfloat *aux     = (gfloat *)iter->data[2];
      glong   samples = iter->length;

      while (samples--)
        {
          dest[RED]   = (mask & GIMP_COMPONENT_MASK_RED)   ? aux[RED]   : src[RED];
          dest[GREEN] = (mask & GIMP_COMPONENT_MASK_GREEN) ? aux[GREEN] : src[GREEN];
          dest[BLUE]  = (mask & GIMP_COMPONENT_MASK_BLUE)  ? aux[BLUE]  : src[BLUE];
          dest[ALPHA] = (mask & GIMP_COMPONENT_MASK_ALPHA) ? aux[ALPHA] : src[ALPHA];

          src  += 4;
          aux  += 4;
          dest += 4;
        }
    }
}

} /* extern "C" */
