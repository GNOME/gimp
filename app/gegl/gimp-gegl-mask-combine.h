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

#ifndef __LIGMA_GEGL_MASK_COMBINE_H__
#define __LIGMA_GEGL_MASK_COMBINE_H__


gboolean   ligma_gegl_mask_combine_rect         (GeglBuffer     *mask,
                                                LigmaChannelOps  op,
                                                gint            x,
                                                gint            y,
                                                gint            w,
                                                gint            h);
gboolean   ligma_gegl_mask_combine_ellipse      (GeglBuffer     *mask,
                                                LigmaChannelOps  op,
                                                gint            x,
                                                gint            y,
                                                gint            w,
                                                gint            h,
                                                gboolean        antialias);
gboolean   ligma_gegl_mask_combine_ellipse_rect (GeglBuffer     *mask,
                                                LigmaChannelOps  op,
                                                gint            x,
                                                gint            y,
                                                gint            w,
                                                gint            h,
                                                gdouble         rx,
                                                gdouble         ry,
                                                gboolean        antialias);
gboolean   ligma_gegl_mask_combine_buffer       (GeglBuffer     *mask,
                                                GeglBuffer     *add_on,
                                                LigmaChannelOps  op,
                                                gint            off_x,
                                                gint            off_y);


#endif /* __LIGMA_GEGL_MASK_COMBINE_H__ */
