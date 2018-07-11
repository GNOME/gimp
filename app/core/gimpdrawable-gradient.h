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

#ifndef  __GIMP_DRAWABLE_GRADIENT_H__
#define  __GIMP_DRAWABLE_GRADIENT_H__


void         gimp_drawable_gradient                    (GimpDrawable                *drawable,
                                                        GimpContext                 *context,
                                                        GimpGradient                *gradient,
                                                        GeglDistanceMetric           metric,
                                                        GimpLayerMode                paint_mode,
                                                        GimpGradientType             gradient_type,
                                                        gdouble                      opacity,
                                                        gdouble                      offset,
                                                        GimpRepeatMode               repeat,
                                                        gboolean                     reverse,
                                                        GimpGradientBlendColorSpace  blend_color_space,
                                                        gboolean                     supersample,
                                                        gint                         max_depth,
                                                        gdouble                      threshold,
                                                        gboolean                     dither,
                                                        gdouble                      startx,
                                                        gdouble                      starty,
                                                        gdouble                      endx,
                                                        gdouble                      endy,
                                                        GimpProgress                *progress);

GeglBuffer * gimp_drawable_gradient_shapeburst_distmap (GimpDrawable                *drawable,
                                                        GeglDistanceMetric           metric,
                                                        const GeglRectangle         *region,
                                                        GimpProgress                *progress);

void         gimp_drawable_gradient_adjust_coords      (GimpDrawable                *drawable,
                                                        GimpGradientType             gradient_type,
                                                        const GeglRectangle         *region,
                                                        gdouble                     *startx,
                                                        gdouble                     *starty,
                                                        gdouble                     *endx,
                                                        gdouble                     *endy);


#endif /* __GIMP_DRAWABLE_GRADIENT_H__ */
