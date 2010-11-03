/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbezierdesc.c
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

#include "config.h"

#include <gegl.h>
#include <cairo.h>

#include "vectors-types.h"

#include "gimpbezierdesc.h"


GType
gimp_bezier_desc_get_type (void)
{
  static GType type = 0;

  if (! type)
    type = g_boxed_type_register_static ("GimpBezierDesc",
                                         (GBoxedCopyFunc) gimp_bezier_desc_copy,
                                         (GBoxedFreeFunc) gimp_bezier_desc_free);

  return type;
}

GimpBezierDesc *
gimp_bezier_desc_new (cairo_path_data_t *data,
                      gint               n_data)
{
  GimpBezierDesc *desc;

  g_return_val_if_fail (n_data == 0 || data != NULL, NULL);

  desc = g_slice_new (GimpBezierDesc);

  desc->status   = CAIRO_STATUS_SUCCESS;
  desc->num_data = n_data;
  desc->data     = data;

  return desc;
}

GimpBezierDesc *
gimp_bezier_desc_copy (const GimpBezierDesc *desc)
{
  g_return_val_if_fail (desc != NULL, NULL);

  return gimp_bezier_desc_new (g_memdup (desc->data,
                                         desc->num_data * sizeof (cairo_path_data_t)),
                               desc->num_data);
}

void
gimp_bezier_desc_free (GimpBezierDesc *desc)
{
  g_return_if_fail (desc != NULL);

  g_free (desc->data);
  g_slice_free (GimpBezierDesc, desc);
}
