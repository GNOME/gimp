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

#ifndef  __LIGMA_DRAWABLE_GRADIENT_H__
#define  __LIGMA_DRAWABLE_GRADIENT_H__


void         ligma_drawable_gradient                    (LigmaDrawable                *drawable,
                                                        LigmaContext                 *context,
                                                        LigmaGradient                *gradient,
                                                        GeglDistanceMetric           metric,
                                                        LigmaLayerMode                paint_mode,
                                                        LigmaGradientType             gradient_type,
                                                        gdouble                      opacity,
                                                        gdouble                      offset,
                                                        LigmaRepeatMode               repeat,
                                                        gboolean                     reverse,
                                                        LigmaGradientBlendColorSpace  blend_color_space,
                                                        gboolean                     supersample,
                                                        gint                         max_depth,
                                                        gdouble                      threshold,
                                                        gboolean                     dither,
                                                        gdouble                      startx,
                                                        gdouble                      starty,
                                                        gdouble                      endx,
                                                        gdouble                      endy,
                                                        LigmaProgress                *progress);

GeglBuffer * ligma_drawable_gradient_shapeburst_distmap (LigmaDrawable                *drawable,
                                                        GeglDistanceMetric           metric,
                                                        const GeglRectangle         *region,
                                                        LigmaProgress                *progress);

void         ligma_drawable_gradient_adjust_coords      (LigmaDrawable                *drawable,
                                                        LigmaGradientType             gradient_type,
                                                        const GeglRectangle         *region,
                                                        gdouble                     *startx,
                                                        gdouble                     *starty,
                                                        gdouble                     *endx,
                                                        gdouble                     *endy);


#endif /* __LIGMA_DRAWABLE_GRADIENT_H__ */
