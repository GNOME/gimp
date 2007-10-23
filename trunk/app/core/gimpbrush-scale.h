/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrush-scale.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_BRUSH_SCALE_H__
#define __GIMP_BRUSH_SCALE_H__


/*  virtual functions of GimpBrush, don't call directly  */

void      gimp_brush_real_scale_size   (GimpBrush *brush,
                                        gdouble    scale,
                                        gint      *scaled_width,
                                        gint      *scaled_height);
TempBuf * gimp_brush_real_scale_mask   (GimpBrush *brush,
                                        gdouble    scale);
TempBuf * gimp_brush_real_scale_pixmap (GimpBrush *brush,
                                        gdouble    scale);


#endif  /*  __GIMP_BRUSH_SCALE_H__  */
