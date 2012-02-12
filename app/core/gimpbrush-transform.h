/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrush-transform.h
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

#ifndef __GIMP_BRUSH_TRANSFORM_H__
#define __GIMP_BRUSH_TRANSFORM_H__


/*  virtual functions of GimpBrush, don't call directly  */

void      gimp_brush_real_transform_size   (GimpBrush   *brush,
                                            gdouble      scale,
                                            gdouble      aspect_ratio,
                                            gdouble      angle,
                                            gint        *scaled_width,
                                            gint        *scaled_height);
TempBuf * gimp_brush_real_transform_mask   (GimpBrush   *brush,
                                            gdouble      scale,
                                            gdouble      aspect_ratio,
                                            gdouble      angle,
                                            gdouble      hardness);
TempBuf * gimp_brush_real_transform_pixmap (GimpBrush   *brush,
                                            gdouble      scale,
                                            gdouble      aspect_ratio,
                                            gdouble      angle,
                                            gdouble      hardness);

void      gimp_brush_transform_matrix      (gdouble      width,
                                            gdouble      height,
                                            gdouble      scale,
                                            gdouble      aspect_ratio,
                                            gdouble      angle,
                                            GimpMatrix3 *matrix);


#endif  /*  __GIMP_BRUSH_TRANSFORM_H__  */
