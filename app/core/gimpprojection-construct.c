/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "core-types.h"

#include "gimpprojection.h"
#include "gimpprojection-construct.h"


/*  public functions  */

void
gimp_projection_construct (GimpProjection *proj,
                           gint            x,
                           gint            y,
                           gint            w,
                           gint            h)
{
  GeglBuffer    *buffer;
  GeglRectangle  rect = { x, y, w, h };

  g_return_if_fail (GIMP_IS_PROJECTION (proj));

  /* GEGL should really do this for us... */
  gegl_node_get (gimp_projection_get_sink_node (proj),
                 "buffer", &buffer, NULL);
  gegl_buffer_clear (buffer, &rect);
  g_object_unref (buffer);

  if (! proj->processor)
    {
      GeglNode *sink = gimp_projection_get_sink_node (proj);

      proj->processor = gegl_node_new_processor (sink, &rect);
    }
  else
    {
      gegl_processor_set_rectangle (proj->processor, &rect);
    }

  while (gegl_processor_work (proj->processor, NULL));
}
