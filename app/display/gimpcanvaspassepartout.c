/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvaspassepartout.c
 * Copyright (C) 2010 Sven Neumann <sven@gimp.org>
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

#include "gimpcanvas.h"
#include "gimpcanvaspassepartout.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-draw.h"
#include "gimpdisplayshell-style.h"


/*  local function prototypes  */

static void             gimp_canvas_passe_partout_draw        (GimpCanvasItem   *item,
                                                               GimpDisplayShell *shell,
                                                               cairo_t          *cr);
static cairo_region_t * gimp_canvas_passe_partout_get_extents (GimpCanvasItem   *item,
                                                               GimpDisplayShell *shell);
static void             gimp_canvas_passe_partout_fill        (GimpCanvasItem   *item,
                                                               GimpDisplayShell *shell,
                                                               cairo_t          *cr);


G_DEFINE_TYPE (GimpCanvasPassePartout, gimp_canvas_passe_partout,
               GIMP_TYPE_CANVAS_RECTANGLE)

#define parent_class gimp_canvas_passe_partout_parent_class


static void
gimp_canvas_passe_partout_class_init (GimpCanvasPassePartoutClass *klass)
{
  GimpCanvasItemClass *item_class = GIMP_CANVAS_ITEM_CLASS (klass);

  item_class->draw        = gimp_canvas_passe_partout_draw;
  item_class->get_extents = gimp_canvas_passe_partout_get_extents;
  item_class->fill        = gimp_canvas_passe_partout_fill;
}

static void
gimp_canvas_passe_partout_init (GimpCanvasPassePartout *passe_partout)
{
}

static void
gimp_canvas_passe_partout_draw (GimpCanvasItem   *item,
                                GimpDisplayShell *shell,
                                cairo_t          *cr)
{
  gint w, h;

  gimp_display_shell_draw_get_scaled_image_size (shell, &w, &h);

  cairo_rectangle (cr, - shell->offset_x, - shell->offset_y, w, h);

  GIMP_CANVAS_ITEM_CLASS (parent_class)->draw (item, shell, cr);
}

static cairo_region_t *
gimp_canvas_passe_partout_get_extents (GimpCanvasItem   *item,
                                       GimpDisplayShell *shell)
{
  cairo_rectangle_int_t  rectangle;
  cairo_region_t        *inner;
  cairo_region_t        *outer;

  rectangle.x = - shell->offset_x;
  rectangle.y = - shell->offset_y;
  gimp_display_shell_draw_get_scaled_image_size (shell,
                                                 &rectangle.width,
                                                 &rectangle.height);

  outer = cairo_region_create_rectangle (&rectangle);

  inner = GIMP_CANVAS_ITEM_CLASS (parent_class)->get_extents (item, shell);

  cairo_region_xor (outer, inner);

  return outer;
}

static void
gimp_canvas_passe_partout_fill (GimpCanvasItem   *item,
                                GimpDisplayShell *shell,
                                cairo_t          *cr)
{
  cairo_translate (cr, -shell->offset_x, -shell->offset_y);

  cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
  cairo_clip (cr);

  gimp_display_shell_set_passe_partout_style (shell, cr);
  cairo_paint (cr);
}

GimpCanvasItem *
gimp_canvas_passe_partout_new (GimpDisplayShell *shell,
                               gdouble           x,
                               gdouble           y,
                               gdouble           width,
                               gdouble           height)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_CANVAS_PASSE_PARTOUT,
                       "shell", shell,
                       "x",      x,
                       "y",      y,
                       "width",  width,
                       "height", height,
                       "filled", TRUE,
                       NULL);
}
