/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabrush-mipmap.h
 * Copyright (C) 2020 Ell
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

#ifndef __LIGMA_BRUSH_MIPMAP_H__
#define __LIGMA_BRUSH_MIPMAP_H__


void                ligma_brush_mipmap_clear       (LigmaBrush *brush);

const LigmaTempBuf * ligma_brush_mipmap_get_mask    (LigmaBrush *brush,
                                                   gdouble   *scale_x,
                                                   gdouble   *scale_y);

const LigmaTempBuf * ligma_brush_mipmap_get_pixmap  (LigmaBrush *brush,
                                                   gdouble   *scale_x,
                                                   gdouble   *scale_y);

gsize               ligma_brush_mipmap_get_memsize (LigmaBrush *brush);


#endif  /*  __LIGMA_BRUSH_MIPMAP_H__  */
