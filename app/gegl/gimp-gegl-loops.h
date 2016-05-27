/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-gegl-loops.h
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_GEGL_LOOPS_H__
#define __GIMP_GEGL_LOOPS_H__


/*  this is a pretty stupid port of concolve_region() that only works
 *  on a linear source buffer
 */
void   gimp_gegl_convolve              (GeglBuffer               *src_buffer,
                                        const GeglRectangle      *src_rect,
                                        GeglBuffer               *dest_buffer,
                                        const GeglRectangle      *dest_rect,
                                        const gfloat             *kernel,
                                        gint                      kernel_size,
                                        gdouble                   divisor,
                                        GimpConvolutionType       mode,
                                        gboolean                  alpha_weighting);

void   gimp_gegl_dodgeburn             (GeglBuffer               *src_buffer,
                                        const GeglRectangle      *src_rect,
                                        GeglBuffer               *dest_buffer,
                                        const GeglRectangle      *dest_rect,
                                        gdouble                   exposure,
                                        GimpDodgeBurnType         type,
                                        GimpTransferMode          mode);

void   gimp_gegl_smudge_blend          (GeglBuffer               *top_buffer,
                                        const GeglRectangle      *top_rect,
                                        GeglBuffer               *bottom_buffer,
                                        const GeglRectangle      *bottom_rect,
                                        GeglBuffer               *dest_buffer,
                                        const GeglRectangle      *dest_rect,
                                        gdouble                   blend);

void   gimp_gegl_apply_mask            (GeglBuffer               *mask_buffer,
                                        const GeglRectangle      *mask_rect,
                                        GeglBuffer               *dest_buffer,
                                        const GeglRectangle      *dest_rect,
                                        gdouble                   opacity);

void   gimp_gegl_combine_mask          (GeglBuffer               *mask_buffer,
                                        const GeglRectangle      *mask_rect,
                                        GeglBuffer               *dest_buffer,
                                        const GeglRectangle      *dest_rect,
                                        gdouble                   opacity);

void   gimp_gegl_combine_mask_weird    (GeglBuffer               *mask_buffer,
                                        const GeglRectangle      *mask_rect,
                                        GeglBuffer               *dest_buffer,
                                        const GeglRectangle      *dest_rect,
                                        gdouble                   opacity,
                                        gboolean                  stipple);

void   gimp_gegl_replace               (GeglBuffer               *top_buffer,
                                        const GeglRectangle      *top_rect,
                                        GeglBuffer               *bottom_buffer,
                                        const GeglRectangle      *bottom_rect,
                                        GeglBuffer               *mask_buffer,
                                        const GeglRectangle      *mask_rect,
                                        GeglBuffer               *dest_buffer,
                                        const GeglRectangle      *dest_rect,
                                        gdouble                   opacity,
                                        const gboolean           *affect);

void   gimp_gegl_index_to_mask         (GeglBuffer               *indexed_buffer,
                                        const GeglRectangle      *indexed_rect,
                                        const Babl               *indexed_format,
                                        GeglBuffer               *mask_buffer,
                                        const GeglRectangle      *mask_rect,
                                        gint                      index);

void   gimp_gegl_convert_color_profile (GeglBuffer               *src_buffer,
                                        const GeglRectangle      *src_rect,
                                        GimpColorProfile         *src_profile,
                                        GeglBuffer               *dest_buffer,
                                        const GeglRectangle      *dest_rect,
                                        GimpColorProfile         *dest_profile,
                                        GimpColorRenderingIntent  intent,
                                        gboolean                  bpc,
                                        GimpProgress             *progress);


#endif /* __GIMP_GEGL_LOOPS_H__ */
