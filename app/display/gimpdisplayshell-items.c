/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdisplayshell-items.c
 * Copyright (C) 2010  Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "display-types.h"

#include "gimpcanvascursor.h"
#include "gimpcanvasgrid.h"
#include "gimpcanvaslayerboundary.h"
#include "gimpcanvaspassepartout.h"
#include "gimpcanvasproxygroup.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-expose.h"
#include "gimpdisplayshell-items.h"


/*  local function prototypes  */

static void   gimp_display_shell_item_update (GimpCanvasItem   *item,
                                              cairo_region_t   *region,
                                              GimpDisplayShell *shell);


/*  public functions  */

void
gimp_display_shell_items_init (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  shell->canvas_item = gimp_canvas_group_new (shell);

  shell->passe_partout = gimp_canvas_passe_partout_new (shell, 0, 0, 0, 0);
  gimp_canvas_item_set_visible (shell->passe_partout, FALSE);
  gimp_display_shell_add_item (shell, shell->passe_partout);
  g_object_unref (shell->passe_partout);

  shell->preview_items = gimp_canvas_group_new (shell);
  gimp_display_shell_add_item (shell, shell->preview_items);
  g_object_unref (shell->preview_items);

  shell->vectors = gimp_canvas_proxy_group_new (shell);
  gimp_display_shell_add_item (shell, shell->vectors);
  g_object_unref (shell->vectors);

  shell->grid = gimp_canvas_grid_new (shell, NULL);
  gimp_canvas_item_set_visible (shell->grid, FALSE);
  g_object_set (shell->grid, "grid-style", TRUE, NULL);
  gimp_display_shell_add_item (shell, shell->grid);
  g_object_unref (shell->grid);

  shell->guides = gimp_canvas_proxy_group_new (shell);
  gimp_display_shell_add_item (shell, shell->guides);
  g_object_unref (shell->guides);

  shell->sample_points = gimp_canvas_proxy_group_new (shell);
  gimp_display_shell_add_item (shell, shell->sample_points);
  g_object_unref (shell->sample_points);

  shell->layer_boundary = gimp_canvas_layer_boundary_new (shell);
  gimp_canvas_item_set_visible (shell->layer_boundary, FALSE);
  gimp_display_shell_add_item (shell, shell->layer_boundary);
  g_object_unref (shell->layer_boundary);

  shell->tool_items = gimp_canvas_group_new (shell);
  gimp_display_shell_add_item (shell, shell->tool_items);
  g_object_unref (shell->tool_items);

  shell->cursor = gimp_canvas_cursor_new (shell);
  gimp_canvas_item_set_visible (shell->cursor, FALSE);
  gimp_display_shell_add_item (shell, shell->cursor);
  g_object_unref (shell->cursor);

  g_signal_connect (shell->canvas_item, "update",
                    G_CALLBACK (gimp_display_shell_item_update),
                    shell);
}

void
gimp_display_shell_items_free (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->canvas_item)
    {
      g_signal_handlers_disconnect_by_func (shell->canvas_item,
                                            gimp_display_shell_item_update,
                                            shell);

      g_object_unref (shell->canvas_item);
      shell->canvas_item = NULL;

      shell->passe_partout  = NULL;
      shell->preview_items  = NULL;
      shell->vectors        = NULL;
      shell->grid           = NULL;
      shell->guides         = NULL;
      shell->sample_points  = NULL;
      shell->layer_boundary = NULL;
      shell->tool_items     = NULL;
      shell->cursor         = NULL;
    }
}

void
gimp_display_shell_add_item (GimpDisplayShell *shell,
                             GimpCanvasItem   *item)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  gimp_canvas_group_add_item (GIMP_CANVAS_GROUP (shell->canvas_item), item);
}

void
gimp_display_shell_remove_item (GimpDisplayShell *shell,
                                GimpCanvasItem   *item)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  gimp_canvas_group_remove_item (GIMP_CANVAS_GROUP (shell->canvas_item), item);
}

void
gimp_display_shell_add_preview_item (GimpDisplayShell *shell,
                                     GimpCanvasItem   *item)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  gimp_canvas_group_add_item (GIMP_CANVAS_GROUP (shell->preview_items), item);
}

void
gimp_display_shell_remove_preview_item (GimpDisplayShell *shell,
                                        GimpCanvasItem   *item)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  gimp_canvas_group_remove_item (GIMP_CANVAS_GROUP (shell->preview_items), item);
}

void
gimp_display_shell_add_tool_item (GimpDisplayShell *shell,
                                  GimpCanvasItem   *item)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  gimp_canvas_group_add_item (GIMP_CANVAS_GROUP (shell->tool_items), item);
}

void
gimp_display_shell_remove_tool_item (GimpDisplayShell *shell,
                                     GimpCanvasItem   *item)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  gimp_canvas_group_remove_item (GIMP_CANVAS_GROUP (shell->tool_items), item);
}


/*  private functions  */

static void
gimp_display_shell_item_update (GimpCanvasItem   *item,
                                cairo_region_t   *region,
                                GimpDisplayShell *shell)
{
  gimp_display_shell_expose_region (shell, region);
}
