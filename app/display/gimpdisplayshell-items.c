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

#include "gimpcanvasgroup.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-expose.h"
#include "gimpdisplayshell-items.h"


void
gimp_display_shell_add_item (GimpDisplayShell *shell,
                             GimpCanvasItem   *item)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  gimp_canvas_group_add_item (GIMP_CANVAS_GROUP (shell->canvas_item), item);

  gimp_display_shell_expose_item (shell, item);
}

void
gimp_display_shell_remove_item (GimpDisplayShell *shell,
                                GimpCanvasItem   *item)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  gimp_display_shell_expose_item (shell, item);

  gimp_canvas_group_remove_item (GIMP_CANVAS_GROUP (shell->canvas_item), item);
}
