/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpchunkiterator.h
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

#ifndef __GIMP_CHUNK_ITEARTOR_H__
#define __GIMP_CHUNK_ITEARTOR_H__


GimpChunkIterator * gimp_chunk_iterator_new               (cairo_region_t      *region);

void                gimp_chunk_iterator_set_tile_rect     (GimpChunkIterator   *iter,
                                                           const GeglRectangle *rect);

void                gimp_chunk_iterator_set_priority_rect (GimpChunkIterator   *iter,
                                                           const GeglRectangle *rect);

void                gimp_chunk_iterator_set_interval      (GimpChunkIterator   *iter,
                                                           gdouble              interval);

gboolean            gimp_chunk_iterator_next              (GimpChunkIterator   *iter);
gboolean            gimp_chunk_iterator_get_rect          (GimpChunkIterator   *iter,
                                                           GeglRectangle       *rect);

cairo_region_t    * gimp_chunk_iterator_stop              (GimpChunkIterator   *iter,
                                                           gboolean             free_region);


#endif  /*  __GIMP_CHUNK_ITEARTOR_H__  */
