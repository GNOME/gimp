/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawable-operation.c
 * Copyright (C) 2007 Øyvind Kolås <pippin@gimp.org>
 *                    Sven Neumann <sven@gimp.org>
 *                    Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>

#include "core-types.h"

#include "gimpdrawable.h"
#include "gimpdrawable-operation.h"
#include "gimpprogress.h"


void
gimp_drawable_apply_operation (GimpDrawable *drawable,
                               GeglNode     *operation,
                               gboolean      linear,
                               GimpProgress *progress,
                               const gchar  *undo_desc)
{
  GeglNode      *gegl;
  GeglNode      *input;
  GeglNode      *output;
  GeglProcessor *processor;
  GeglRectangle  rect;
  gdouble        value;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GEGL_IS_NODE (operation));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (undo_desc != NULL);

  if (! gimp_drawable_mask_intersect (drawable,
                                      &rect.x,     &rect.y,
                                      &rect.width, &rect.height))
    return;

  gegl   = gegl_node_new ();
  input  = gegl_node_new_child (gegl,
                                "operation",    "gimp-tilemanager-source",
                                "tile-manager", gimp_drawable_get_tiles (drawable),
                                "linear",       linear,
                                NULL);
  output = gegl_node_new_child (gegl,
                                "operation",    "gimp-tilemanager-sink",
                                "tile-manager", gimp_drawable_get_shadow_tiles (drawable),
                                "linear",       linear,
                                NULL);

  gegl_node_add_child (gegl, operation);

  gegl_node_link_many (input, operation, output, NULL);

  processor = gegl_node_new_processor (output, &rect);

  if (progress)
    gimp_progress_start (progress, undo_desc, FALSE);

  while (gegl_processor_work (processor, &value))
    if (progress)
      gimp_progress_set_value (progress, value);

  g_object_unref (processor);

  gimp_drawable_merge_shadow (drawable, TRUE, undo_desc);
  gimp_drawable_update (drawable, rect.x, rect.y, rect.width, rect.height);

  if (progress)
    gimp_progress_end (progress);

  g_object_unref (gegl);
}
