/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawable-operation.c
 * Copyright (C) 2007 Øyvind Kolås <pippin@gimp.org>
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

#include "base/tile-manager.h"

#include "gimpdrawable.h"
#include "gimpdrawable-operation.h"
#include "gimpdrawable-shadow.h"
#include "gimpprogress.h"


/*  local function prototypes  */

static void   gimp_drawable_apply_operation_private (GimpDrawable       *drawable,
                                                     GimpProgress       *progress,
                                                     const gchar        *undo_desc,
                                                     GeglNode           *operation,
                                                     gboolean            linear,
                                                     TileManager         *dest_tiles,
                                                     const GeglRectangle *rect);


/*  public functions  */

void
gimp_drawable_apply_operation (GimpDrawable *drawable,
                               GimpProgress *progress,
                               const gchar  *undo_desc,
                               GeglNode     *operation,
                               gboolean      linear)
{
  GeglRectangle rect;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (GEGL_IS_NODE (operation));

  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable),
                                  &rect.x,     &rect.y,
                                  &rect.width, &rect.height))
    return;

  gimp_drawable_apply_operation_private (drawable,
                                         progress,
                                         undo_desc,
                                         operation,
                                         linear,
                                         gimp_drawable_get_shadow_tiles (drawable),
                                         &rect);

  gimp_drawable_merge_shadow_tiles (drawable, TRUE, undo_desc);
  gimp_drawable_free_shadow_tiles (drawable);

  gimp_drawable_update (drawable, rect.x, rect.y, rect.width, rect.height);

  if (progress)
    gimp_progress_end (progress);
}

void
gimp_drawable_apply_operation_to_tiles (GimpDrawable *drawable,
                                        GimpProgress *progress,
                                        const gchar  *undo_desc,
                                        GeglNode     *operation,
                                        gboolean      linear,
                                        TileManager  *new_tiles)
{
  GeglRectangle rect;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (GEGL_IS_NODE (operation));
  g_return_if_fail (new_tiles != NULL);

  rect.x      = 0;
  rect.y      = 0;
  rect.width  = tile_manager_width  (new_tiles);
  rect.height = tile_manager_height (new_tiles);

  gimp_drawable_apply_operation_private (drawable,
                                         progress,
                                         undo_desc,
                                         operation,
                                         linear,
                                         new_tiles,
                                         &rect);

  if (progress)
    gimp_progress_end (progress);
}


/*  private functions  */

static void
gimp_drawable_apply_operation_private (GimpDrawable       *drawable,
                                       GimpProgress       *progress,
                                       const gchar        *undo_desc,
                                       GeglNode           *operation,
                                       gboolean            linear,
                                       TileManager         *dest_tiles,
                                       const GeglRectangle *rect)
{
  GeglNode      *gegl;
  GeglNode      *input;
  GeglNode      *output;
  GeglProcessor *processor;
  gdouble        value;

  gegl = gegl_node_new ();

  /* Disable caching on all children of the node unless explicitly re-enabled.
   */
  g_object_set (gegl,
                "dont-cache", TRUE,
                NULL);

  input  = gegl_node_new_child (gegl,
                                "operation",    "gimp:tilemanager-source",
                                "tile-manager", gimp_drawable_get_tiles (drawable),
                                "linear",       linear,
                                NULL);
  output = gegl_node_new_child (gegl,
                                "operation",    "gimp:tilemanager-sink",
                                "tile-manager", dest_tiles,
                                "linear",       linear,
                                NULL);

  gegl_node_add_child (gegl, operation);

  gegl_node_link_many (input, operation, output, NULL);

  processor = gegl_node_new_processor (output, rect);

  if (progress)
    gimp_progress_start (progress, undo_desc, FALSE);

  while (gegl_processor_work (processor, &value))
    if (progress)
      gimp_progress_set_value (progress, value);

  g_object_unref (processor);

  g_object_unref (gegl);
}
