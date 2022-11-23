/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmastroke-new.c
 * Copyright (C) 2006 Simon Budig  <simon@ligma.org>
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

#include "config.h"

#include <glib-object.h>

#include "vectors-types.h"

#include "ligmastroke-new.h"
#include "ligmabezierstroke.h"


LigmaStroke *
ligma_stroke_new_from_coords (LigmaVectorsStrokeType  type,
                             const LigmaCoords      *coords,
                             gint                   n_coords,
                             gboolean               closed)
{
  switch (type)
    {
    case LIGMA_VECTORS_STROKE_TYPE_BEZIER:
      return ligma_bezier_stroke_new_from_coords (coords, n_coords, closed);
      break;
    default:
      g_warning ("unknown type in ligma_stroke_new_from_coords(): %d", type);
      return NULL;
    }
}

