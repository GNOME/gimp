/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ParamSpecs for config objects
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "config-types.h"

#include "gimpconfig-types.h"


static GimpMatrix2 * matrix2_copy (const GimpMatrix2 *matrix);


GType
gimp_matrix2_get_type (void)
{
  static GType matrix_type = 0;

  if (!matrix_type)
    matrix_type = g_boxed_type_register_static ("GimpMatrix2",
					       (GBoxedCopyFunc) matrix2_copy,
					       (GBoxedFreeFunc) g_free);

  return matrix_type;
}

static GimpMatrix2 *
matrix2_copy (const GimpMatrix2 *matrix)
{
  return (GimpMatrix2 *) g_memdup (matrix, sizeof (GimpMatrix2));
}


GType
gimp_path_get_type (void)
{
  static GType path_type = 0;

  if (!path_type)
    {
      static const GTypeInfo type_info = { 0, };

      path_type = g_type_register_static (G_TYPE_STRING, "GimpPath",
                                          &type_info, 0);
    }

  return path_type;
}
