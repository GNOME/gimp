/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmachunkiterator.h
 * Copyright (C) 2019 Ell
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

#ifndef __LIGMA_CHUNK_ITEARTOR_H__
#define __LIGMA_CHUNK_ITEARTOR_H__


LigmaChunkIterator * ligma_chunk_iterator_new               (cairo_region_t      *region);

void                ligma_chunk_iterator_set_tile_rect     (LigmaChunkIterator   *iter,
                                                           const GeglRectangle *rect);

void                ligma_chunk_iterator_set_priority_rect (LigmaChunkIterator   *iter,
                                                           const GeglRectangle *rect);

void                ligma_chunk_iterator_set_interval      (LigmaChunkIterator   *iter,
                                                           gdouble              interval);

gboolean            ligma_chunk_iterator_next              (LigmaChunkIterator   *iter);
gboolean            ligma_chunk_iterator_get_rect          (LigmaChunkIterator   *iter,
                                                           GeglRectangle       *rect);

cairo_region_t    * ligma_chunk_iterator_stop              (LigmaChunkIterator   *iter,
                                                           gboolean             free_region);


#endif  /*  __LIGMA_CHUNK_ITEARTOR_H__  */
