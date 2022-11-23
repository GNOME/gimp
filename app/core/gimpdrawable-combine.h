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

#ifndef __LIGMA_DRAWABLE_COMBINE_H__
#define __LIGMA_DRAWABLE_COMBINE_H__


/*  virtual functions of LigmaDrawable, don't call directly  */

void   ligma_drawable_real_apply_buffer (LigmaDrawable           *drawable,
                                        GeglBuffer             *buffer,
                                        const GeglRectangle    *buffer_region,
                                        gboolean                push_undo,
                                        const gchar            *undo_desc,
                                        gdouble                 opacity,
                                        LigmaLayerMode           mode,
                                        LigmaLayerColorSpace     blend_space,
                                        LigmaLayerColorSpace     composite_space,
                                        LigmaLayerCompositeMode  composite_mode,
                                        GeglBuffer             *base_buffer,
                                        gint                    base_x,
                                        gint                    base_y);


#endif /* __LIGMA_DRAWABLE_COMBINE_H__ */
