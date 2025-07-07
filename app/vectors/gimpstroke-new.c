/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpstroke-new.c
 * Copyright (C) 2006 Simon Budig  <simon@gimp.org>
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

#include "path-types.h"

#include "gimpstroke-new.h"
#include "gimpbezierstroke.h"


GimpStroke *
gimp_stroke_new_from_coords (GimpPathStrokeType  type,
                             const GimpCoords   *coords,
                             gint                n_coords,
                             gboolean            closed)
{
  switch (type)
    {
    case GIMP_PATH_STROKE_TYPE_BEZIER:
      return gimp_bezier_stroke_new_from_coords (coords, n_coords, closed);
      break;
    default:
      g_warning ("unknown type in gimp_stroke_new_from_coords(): %d", type);
      return NULL;
    }
}

