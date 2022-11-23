/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabezierdesc.h
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_BEZIER_DESC_H__
#define __LIGMA_BEZIER_DESC_H__


#define          LIGMA_TYPE_BEZIER_DESC     (ligma_bezier_desc_get_type ())

GType            ligma_bezier_desc_get_type (void) G_GNUC_CONST;


/* takes ownership of "data" */
LigmaBezierDesc * ligma_bezier_desc_new                 (cairo_path_data_t    *data,
                                                       gint                  n_data);

/* expects sorted LigmaBoundSegs */
LigmaBezierDesc * ligma_bezier_desc_new_from_bound_segs (LigmaBoundSeg         *bound_segs,
                                                       gint                  n_bound_segs,
                                                       gint                  n_bound_groups);

void             ligma_bezier_desc_translate           (LigmaBezierDesc       *desc,
                                                       gdouble               offset_x,
                                                       gdouble               offset_y);

LigmaBezierDesc * ligma_bezier_desc_copy                (const LigmaBezierDesc *desc);
void             ligma_bezier_desc_free                (LigmaBezierDesc       *desc);


#endif /* __LIGMA_BEZIER_DESC_H__ */
