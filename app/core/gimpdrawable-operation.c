/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadrawable-operation.c
 * Copyright (C) 2007 Øyvind Kolås <pippin@ligma.org>
 *                    Sven Neumann <sven@ligma.org>
 *                    Michael Natterer <mitch@ligma.org>
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "gegl/ligma-gegl-utils.h"

#include "operations/ligma-operation-config.h"
#include "operations/ligmaoperationsettings.h"

#include "ligmadrawable.h"
#include "ligmadrawable-operation.h"
#include "ligmadrawablefilter.h"
#include "ligmaprogress.h"
#include "ligmasettings.h"


/*  public functions  */

void
ligma_drawable_apply_operation (LigmaDrawable *drawable,
                               LigmaProgress *progress,
                               const gchar  *undo_desc,
                               GeglNode     *operation)
{
  ligma_drawable_apply_operation_with_config (drawable,
                                             progress, undo_desc,
                                             operation, NULL);
}

void
ligma_drawable_apply_operation_with_config (LigmaDrawable *drawable,
                                           LigmaProgress *progress,
                                           const gchar  *undo_desc,
                                           GeglNode     *operation,
                                           GObject      *config)
{
  LigmaDrawableFilter *filter;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)));
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (GEGL_IS_NODE (operation));
  g_return_if_fail (config == NULL || LIGMA_IS_OPERATION_SETTINGS (config));

  if (! ligma_item_mask_intersect (LIGMA_ITEM (drawable),
                                  NULL, NULL, NULL, NULL))
    {
      return;
    }

  filter = ligma_drawable_filter_new (drawable, undo_desc, operation, NULL);

  ligma_drawable_filter_set_add_alpha (filter,
                                      ligma_gegl_node_has_key (operation,
                                                              "needs-alpha"));

  if (config)
    {
      ligma_operation_config_sync_node (config, operation);

      ligma_operation_settings_sync_drawable_filter (
        LIGMA_OPERATION_SETTINGS (config), filter);
    }

  ligma_drawable_filter_apply  (filter, NULL);
  ligma_drawable_filter_commit (filter, progress, TRUE);

  g_object_unref (filter);

  if (progress)
    ligma_progress_end (progress);
}

void
ligma_drawable_apply_operation_by_name (LigmaDrawable *drawable,
                                       LigmaProgress *progress,
                                       const gchar  *undo_desc,
                                       const gchar  *operation_type,
                                       GObject      *config)
{
  GeglNode *node;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)));
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (operation_type != NULL);
  g_return_if_fail (config == NULL || LIGMA_IS_SETTINGS (config));

  node = g_object_new (GEGL_TYPE_NODE,
                       "operation", operation_type,
                       NULL);

  if (config)
    gegl_node_set (node,
                   "config", config,
                   NULL);

  ligma_drawable_apply_operation (drawable, progress, undo_desc, node);

  g_object_unref (node);
}
