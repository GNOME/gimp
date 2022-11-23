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

#ifndef __LIGMA_DRAWABLE_FILL_H__
#define __LIGMA_DRAWABLE_FILL_H__


/*  Lowlevel API that is used for initializing entire drawables and
 *  buffers before they are used in images, they don't push an undo.
 */

void       ligma_drawable_fill              (LigmaDrawable        *drawable,
                                            LigmaContext         *context,
                                            LigmaFillType         fill_type);
void       ligma_drawable_fill_buffer       (LigmaDrawable        *drawable,
                                            GeglBuffer          *buffer,
                                            const LigmaRGB       *color,
                                            LigmaPattern         *pattern,
                                            gint                 pattern_offset_x,
                                            gint                 pattern_offset_y);


/*  Proper API that is used for actual editing (not just initializing)
 */

void       ligma_drawable_fill_boundary     (LigmaDrawable        *drawable,
                                            LigmaFillOptions     *options,
                                            const LigmaBoundSeg  *bound_segs,
                                            gint                 n_bound_segs,
                                            gint                 offset_x,
                                            gint                 offset_y,
                                            gboolean             push_undo);

gboolean   ligma_drawable_fill_vectors      (LigmaDrawable        *drawable,
                                            LigmaFillOptions     *options,
                                            LigmaVectors         *vectors,
                                            gboolean             push_undo,
                                            GError             **error);

void       ligma_drawable_fill_scan_convert (LigmaDrawable        *drawable,
                                            LigmaFillOptions     *options,
                                            LigmaScanConvert     *scan_convert,
                                            gboolean             push_undo);


#endif /* __LIGMA_DRAWABLE_FILL_H__ */
