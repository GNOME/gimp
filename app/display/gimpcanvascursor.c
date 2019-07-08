/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvascursor.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "core/gimp-cairo.h"

#include "gimpcanvascursor.h"
#include "gimpdisplayshell.h"


#define GIMP_CURSOR_SIZE 7


enum
{
  PROP_0,
  PROP_X,
  PROP_Y
};


typedef struct _GimpCanvasCursorPrivate GimpCanvasCursorPrivate;

struct _GimpCanvasCursorPrivate
{
  gdouble x;
  gdouble y;
};

#define GET_PRIVATE(cursor) \
        ((GimpCanvasCursorPrivate *) gimp_canvas_cursor_get_instance_private ((GimpCanvasCursor *) (cursor)))


/*  local function prototypes  */

static void             gimp_canvas_cursor_set_property (GObject        *object,
                                                         guint           property_id,
                                                         const GValue   *value,
                                                         GParamSpec     *pspec);
static void             gimp_canvas_cursor_get_property (GObject        *object,
                                                         guint           property_id,
                                                         GValue         *value,
                                                         GParamSpec     *pspec);
static void             gimp_canvas_cursor_draw         (GimpCanvasItem *item,
                                                         cairo_t        *cr);
static cairo_region_t * gimp_canvas_cursor_get_extents  (GimpCanvasItem *item);


G_DEFINE_TYPE_WITH_PRIVATE (GimpCanvasCursor, gimp_canvas_cursor,
                            GIMP_TYPE_CANVAS_ITEM)

#define parent_class gimp_canvas_cursor_parent_class


static void
gimp_canvas_cursor_class_init (GimpCanvasCursorClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = gimp_canvas_cursor_set_property;
  object_class->get_property = gimp_canvas_cursor_get_property;

  item_class->draw           = gimp_canvas_cursor_draw;
  item_class->get_extents    = gimp_canvas_cursor_get_extents;

  g_object_class_install_property (object_class, PROP_X,
                                   g_param_spec_double ("x", NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE, 0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_Y,
                                   g_param_spec_double ("y", NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE, 0,
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_canvas_cursor_init (GimpCanvasCursor *cursor)
{
  gimp_canvas_item_set_line_cap (GIMP_CANVAS_ITEM (cursor),
                                 CAIRO_LINE_CAP_SQUARE);
}

static void
gimp_canvas_cursor_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpCanvasCursorPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_X:
      private->x = g_value_get_double (value);
      break;
    case PROP_Y:
      private->y = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_cursor_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpCanvasCursorPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_X:
      g_value_set_double (value, private->x);
      break;
    case PROP_Y:
      g_value_set_double (value, private->y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_cursor_draw (GimpCanvasItem *item,
                         cairo_t        *cr)
{
  GimpCanvasCursorPrivate *private = GET_PRIVATE (item);
  gdouble                  x, y;

  x = floor (private->x) + 0.5;
  y = floor (private->y) + 0.5;

  cairo_move_to (cr, x - GIMP_CURSOR_SIZE, y);
  cairo_line_to (cr, x + GIMP_CURSOR_SIZE, y);

  cairo_move_to (cr, x, y - GIMP_CURSOR_SIZE);
  cairo_line_to (cr, x, y + GIMP_CURSOR_SIZE);

  _gimp_canvas_item_stroke (item, cr);
}

static cairo_region_t *
gimp_canvas_cursor_get_extents (GimpCanvasItem *item)
{
  GimpCanvasCursorPrivate *private = GET_PRIVATE (item);
  cairo_rectangle_int_t    rectangle;
  gdouble                  x, y;

  x = floor (private->x) + 0.5;
  y = floor (private->y) + 0.5;

  rectangle.x      = floor (x - GIMP_CURSOR_SIZE - 1.5);
  rectangle.y      = floor (y - GIMP_CURSOR_SIZE - 1.5);
  rectangle.width  = ceil (x + GIMP_CURSOR_SIZE + 1.5) - rectangle.x;
  rectangle.height = ceil (y + GIMP_CURSOR_SIZE + 1.5) - rectangle.y;

  return cairo_region_create_rectangle (&rectangle);
}

GimpCanvasItem *
gimp_canvas_cursor_new (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_CANVAS_CURSOR,
                       "shell", shell,
                       NULL);
}

void
gimp_canvas_cursor_set (GimpCanvasItem *cursor,
                        gdouble         x,
                        gdouble         y)
{
  GimpCanvasCursorPrivate *private;

  g_return_if_fail (GIMP_IS_CANVAS_CURSOR (cursor));

  private = GET_PRIVATE (cursor);

  if (private->x != x || private->y != y)
    {
      gimp_canvas_item_begin_change (cursor);

      g_object_set (cursor,
                    "x", x,
                    "y", y,
                    NULL);

      gimp_canvas_item_end_change (cursor);
    }
}
