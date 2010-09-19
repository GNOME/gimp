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
#include <gtk/gtk.h>

#include "display-types.h"

#include "gimpcanvasitem.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-style.h"


/*  local function prototypes  */

static void   gimp_canvas_item_real_draw       (GimpCanvasItem   *item,
                                                GimpDisplayShell *shell,
                                                cairo_t          *cr);
static void   gimp_canvas_item_real_invalidate (GimpCanvasItem   *item,
                                                GimpDisplayShell *shell);


G_DEFINE_TYPE (GimpCanvasItem, gimp_canvas_item,
               GIMP_TYPE_OBJECT)

#define parent_class gimp_canvas_item_parent_class


static void
gimp_canvas_item_class_init (GimpCanvasItemClass *klass)
{
  klass->draw       = gimp_canvas_item_real_draw;
  klass->invalidate = gimp_canvas_item_real_invalidate;
}

static void
gimp_canvas_item_init (GimpCanvasItem *item)
{
}

static void
gimp_canvas_item_real_draw (GimpCanvasItem   *item,
                            GimpDisplayShell *shell,
                            cairo_t          *cr)
{
  g_warn_if_reached ();
}

static void
gimp_canvas_item_real_invalidate (GimpCanvasItem   *item,
                                  GimpDisplayShell *shell)
{
  g_warn_if_reached ();
}


/*  public functions  */

void
gimp_canvas_item_draw (GimpCanvasItem   *item,
                       GimpDisplayShell *shell,
                       cairo_t          *cr)
{
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);

  cairo_save (cr);

  GIMP_CANVAS_ITEM_GET_CLASS (item)->draw (item, shell, cr);

  cairo_restore (cr);
}

void
gimp_canvas_item_invalidate (GimpCanvasItem   *item,
                             GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  GIMP_CANVAS_ITEM_GET_CLASS (item)->invalidate (item, shell);
}

void
_gimp_canvas_item_stroke (GimpCanvasItem   *item,
                          GimpDisplayShell *shell,
                          cairo_t          *cr)
{
  gimp_display_shell_set_tool_bg_style (shell, cr);
  cairo_stroke_preserve (cr);

  gimp_display_shell_set_tool_fg_style (shell, cr);
  cairo_stroke (cr);
}

void
_gimp_canvas_item_fill (GimpCanvasItem   *item,
                        GimpDisplayShell *shell,
                        cairo_t          *cr)
{
  gimp_display_shell_set_tool_fg_style (shell, cr);
  cairo_fill (cr);
}
