/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbezierdesc.h
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_BEZIER_DESC_H__
#define __GIMP_BEZIER_DESC_H__


#define          GIMP_TYPE_BEZIER_DESC     (gimp_bezier_desc_get_type ())

GType            gimp_bezier_desc_get_type (void) G_GNUC_CONST;


/* takes ownership of "data" */
GimpBezierDesc * gimp_bezier_desc_new                 (cairo_path_data_t    *data,
                                                       gint                  n_data);

/* expects sorted GimpBoundSegs */
GimpBezierDesc * gimp_bezier_desc_new_from_bound_segs (GimpBoundSeg         *bound_segs,
                                                       gint                  n_bound_segs,
                                                       gint                  n_bound_groups);

void             gimp_bezier_desc_translate           (GimpBezierDesc       *desc,
                                                       gdouble               offset_x,
                                                       gdouble               offset_y);

GimpBezierDesc * gimp_bezier_desc_copy                (const GimpBezierDesc *desc);
void             gimp_bezier_desc_free                (GimpBezierDesc       *desc);


#endif /* __GIMP_BEZIER_DESC_H__ */
