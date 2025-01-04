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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "gegl/gimp-gegl-utils.h"

#include "operations/gimp-operation-config.h"
#include "operations/gimpoperationsettings.h"

#include "gimpcontainer.h"
#include "gimpdrawable-filters.h"
#include "gimpdrawable-operation.h"
#include "gimpdrawable.h"
#include "gimpdrawablefilter.h"
#include "gimpprogress.h"
#include "gimpsettings.h"


/*  public functions  */

void
gimp_drawable_apply_operation (GimpDrawable *drawable,
                               GimpProgress *progress,
                               const gchar  *undo_desc,
                               GeglNode     *operation)
{
  gimp_drawable_apply_operation_with_config (drawable,
                                             progress, undo_desc,
                                             operation, NULL);
}

void
gimp_drawable_apply_operation_with_config (GimpDrawable *drawable,
                                           GimpProgress *progress,
                                           const gchar  *undo_desc,
                                           GeglNode     *operation,
                                           GObject      *config)
{
  GimpDrawableFilter *filter;
  GimpContainer      *filter_stack;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (GEGL_IS_NODE (operation));
  g_return_if_fail (config == NULL || GIMP_IS_OPERATION_SETTINGS (config));

  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable),
                                  NULL, NULL, NULL, NULL))
    {
      return;
    }

  filter = gimp_drawable_filter_new (drawable, undo_desc, operation, NULL);

  gimp_drawable_filter_set_add_alpha (filter,
                                      gimp_gegl_node_has_key (operation,
                                                              "needs-alpha"));

  if (config)
    {
      gimp_operation_config_sync_node (config, operation);

      gimp_operation_settings_sync_drawable_filter (
        GIMP_OPERATION_SETTINGS (config), filter);
    }

  gimp_drawable_filter_apply  (filter, NULL);

  /* For destructive filters, we want them to apply directly on the
   * drawable rather than merge down onto existing NDE filters */
  filter_stack = gimp_drawable_get_filters (drawable);
  if (filter_stack)
    gimp_container_reorder (filter_stack, GIMP_OBJECT (filter),
                            gimp_container_get_n_children (filter_stack) - 1);

  gimp_drawable_filter_commit (filter, FALSE, progress, TRUE);

  g_object_unref (filter);

  if (progress)
    gimp_progress_end (progress);
}

void
gimp_drawable_apply_operation_by_name (GimpDrawable *drawable,
                                       GimpProgress *progress,
                                       const gchar  *undo_desc,
                                       const gchar  *operation_type,
                                       GObject      *config)
{
  GeglNode *node;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (operation_type != NULL);
  g_return_if_fail (config == NULL || GIMP_IS_SETTINGS (config));

  node = g_object_new (GEGL_TYPE_NODE,
                       "operation", operation_type,
                       NULL);

  if (config)
    gegl_node_set (node,
                   "config", config,
                   NULL);

  gimp_drawable_apply_operation (drawable, progress, undo_desc, node);

  g_object_unref (node);
}
