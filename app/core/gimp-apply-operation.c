/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-apply-operation.c
 * Copyright (C) 2012 Øyvind Kolås <pippin@gimp.org>
 *                    Sven Neumann <sven@gimp.org>
 *                    Michael Natterer <mitch@gimp.org>
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

#include "gegl/gimp-gegl-utils.h"

#include "gimp-utils.h"
#include "gimp-apply-operation.h"
#include "gimpprogress.h"


void
gimp_apply_operation (GeglBuffer          *src_buffer,
                      GimpProgress        *progress,
                      const gchar         *undo_desc,
                      GeglNode            *operation,
                      GeglBuffer          *dest_buffer,
                      const GeglRectangle *dest_rect)
{
  GeglNode      *gegl;
  GeglNode      *src_node;
  GeglNode      *dest_node;
  GeglProcessor *processor;
  GeglRectangle  rect = { 0, };
  gdouble        value;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_NODE (operation));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  if (dest_rect)
    {
      rect = *dest_rect;
    }
  else
    {
      rect = *GIMP_GEGL_RECT (0, 0, gegl_buffer_get_width  (dest_buffer),
                                    gegl_buffer_get_height (dest_buffer));
    }

  gegl = gegl_node_new ();

  src_node = gegl_node_new_child (gegl,
                                  "operation", "gegl:buffer-source",
                                  "buffer",    src_buffer,
                                  NULL);
  dest_node = gegl_node_new_child (gegl,
                                   "operation", "gegl:write-buffer",
                                   "buffer",    dest_buffer,
                                   NULL);

  gegl_node_add_child (gegl, operation);

  gegl_node_link_many (src_node, operation, dest_node, NULL);

  processor = gegl_node_new_processor (dest_node, &rect);

  if (progress)
    {
      if (gimp_progress_is_active (progress))
        {
          if (undo_desc)
            gimp_progress_set_text (progress, undo_desc);
        }
      else
        {
          gimp_progress_start (progress, undo_desc, FALSE);
        }
    }

#ifdef GIMP_UNSTABLE
  GIMP_TIMER_START ();
#endif

  while (gegl_processor_work (processor, &value))
    if (progress)
      gimp_progress_set_value (progress, value);

#ifdef GIMP_UNSTABLE
  GIMP_TIMER_END (undo_desc ? undo_desc : "operation");
#endif

  g_object_unref (processor);

  g_object_unref (gegl);

  if (progress)
    gimp_progress_end (progress);
}
