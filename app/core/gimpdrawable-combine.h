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

#ifndef __GIMP_DRAWABLE_COMBINE_H__
#define __GIMP_DRAWABLE_COMBINE_H__


/*  virtual functions of GimpDrawable, don't call directly  */

void   gimp_drawable_real_apply_buffer (GimpDrawable           *drawable,
                                        GeglBuffer             *buffer,
                                        const GeglRectangle    *buffer_region,
                                        gboolean                push_undo,
                                        const gchar            *undo_desc,
                                        gdouble                 opacity,
                                        GimpLayerMode           mode,
                                        GimpLayerColorSpace     blend_space,
                                        GimpLayerColorSpace     composite_space,
                                        GimpLayerCompositeMode  composite_mode,
                                        GeglBuffer             *base_buffer,
                                        gint                    base_x,
                                        gint                    base_y);


#endif /* __GIMP_DRAWABLE_COMBINE_H__ */
