/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaTextLayer
 * Copyright (C) 2002-2003  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_TEXT_LAYER_TRANSFORM_H__
#define __LIGMA_TEXT_LAYER_TRANSFORM_H__


void  ligma_text_layer_scale     (LigmaItem               *item,
                                 gint                    new_width,
                                 gint                    new_height,
                                 gint                    new_offset_x,
                                 gint                    new_offset_y,
                                 LigmaInterpolationType   interpolation_type,
                                 LigmaProgress           *progress);
void  ligma_text_layer_flip      (LigmaItem               *item,
                                 LigmaContext            *context,
                                 LigmaOrientationType     flip_type,
                                 gdouble                 axis,
                                 gboolean                clip_result);
void  ligma_text_layer_rotate    (LigmaItem               *item,
                                 LigmaContext            *context,
                                 LigmaRotationType        rotate_type,
                                 gdouble                 center_x,
                                 gdouble                 center_y,
                                 gboolean                clip_result);
void  ligma_text_layer_transform (LigmaItem               *item,
                                 LigmaContext            *context,
                                 const LigmaMatrix3      *matrix,
                                 LigmaTransformDirection  direction,
                                 LigmaInterpolationType   interpolation_type,
                                 gboolean                supersample,
                                 LigmaTransformResize     clip_result,
                                 LigmaProgress           *progress);


#endif /* __LIGMA_TEXT_LAYER_TRANSFORM_H__ */
