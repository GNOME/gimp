/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacoords.h
 * Copyright (C) 2002 Simon Budig  <simon@ligma.org>
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

#ifndef __LIGMA_COORDS_H__
#define __LIGMA_COORDS_H__


void     ligma_coords_mix            (const gdouble     amul,
                                     const LigmaCoords *a,
                                     const gdouble     bmul,
                                     const LigmaCoords *b,
                                     LigmaCoords       *ret_val);
void     ligma_coords_average        (const LigmaCoords *a,
                                     const LigmaCoords *b,
                                     LigmaCoords       *ret_average);
void     ligma_coords_add            (const LigmaCoords *a,
                                     const LigmaCoords *b,
                                     LigmaCoords       *ret_add);
void     ligma_coords_difference     (const LigmaCoords *a,
                                     const LigmaCoords *b,
                                     LigmaCoords       *difference);
void     ligma_coords_scale          (const gdouble     f,
                                     const LigmaCoords *a,
                                     LigmaCoords       *ret_product);

gdouble  ligma_coords_scalarprod     (const LigmaCoords *a,
                                     const LigmaCoords *b);
gdouble  ligma_coords_length         (const LigmaCoords *a);
gdouble  ligma_coords_length_squared (const LigmaCoords *a);
gdouble  ligma_coords_manhattan_dist (const LigmaCoords *a,
                                     const LigmaCoords *b);

gboolean ligma_coords_equal          (const LigmaCoords *a,
                                     const LigmaCoords *b);

gdouble  ligma_coords_direction      (const LigmaCoords *a,
                                     const LigmaCoords *b);


#endif /* __LIGMA_COORDS_H__ */
