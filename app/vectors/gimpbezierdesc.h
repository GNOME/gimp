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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_BEZIER_DESC_H__
#define __GIMP_BEZIER_DESC_H__


/* takes ownership of "data" */
GimpBezierDesc    * gimp_bezier_desc_new  (cairo_path_data_t    *data,
                                           gint                  n_data);
GimpBezierDesc    * gimp_bezier_desc_copy (const GimpBezierDesc *desc);
cairo_path_data_t * gimp_bezier_desc_free (GimpBezierDesc       *desc,
                                           gboolean              free_data);


#endif /* __GIMP_BEZIER_DESC_H__ */
