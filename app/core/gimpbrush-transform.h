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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once


/*  virtual functions of GimpBrush, don't call directly  */

void          gimp_brush_real_transform_size   (GimpBrush   *brush,
                                                gdouble      scale,
                                                gdouble      aspect_ratio,
                                                gdouble      angle,
                                                gboolean     reflect,
                                                gint        *scaled_width,
                                                gint        *scaled_height);
GimpTempBuf * gimp_brush_real_transform_mask   (GimpBrush   *brush,
                                                gdouble      scale,
                                                gdouble      aspect_ratio,
                                                gdouble      angle,
                                                gboolean     reflect,
                                                gdouble      hardness);
GimpTempBuf * gimp_brush_real_transform_pixmap (GimpBrush   *brush,
                                                gdouble      scale,
                                                gdouble      aspect_ratio,
                                                gdouble      angle,
                                                gboolean     reflect,
                                                gdouble      hardness);

void          gimp_brush_transform_get_scale   (gdouble      scale,
                                                gdouble      aspect_ratio,
                                                gdouble     *scale_x,
                                                gdouble     *scale_y);
void          gimp_brush_transform_matrix      (gdouble      width,
                                                gdouble      height,
                                                gdouble      scale_x,
                                                gdouble      scale_y,
                                                gdouble      angle,
                                                gboolean     reflect,
                                                GimpMatrix3 *matrix);
