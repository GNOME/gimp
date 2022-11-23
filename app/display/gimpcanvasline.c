/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvasline.c
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "display-types.h"

#include "ligmacanvasline.h"
#include "ligmadisplayshell.h"


enum
{
  PROP_0,
  PROP_X1,
  PROP_Y1,
  PROP_X2,
  PROP_Y2
};


typedef struct _LigmaCanvasLinePrivate LigmaCanvasLinePrivate;

struct _LigmaCanvasLinePrivate
{
  gdouble  x1;
  gdouble  y1;
  gdouble  x2;
  gdouble  y2;
};

#define GET_PRIVATE(line) \
        ((LigmaCanvasLinePrivate *) ligma_canvas_line_get_instance_private ((LigmaCanvasLine *) (line)))


/*  local function prototypes  */

static void             ligma_canvas_line_set_property (GObject        *object,
                                                       guint           property_id,
                                                       const GValue   *value,
                                                       GParamSpec     *pspec);
static void             ligma_canvas_line_get_property (GObject        *object,
                                                       guint           property_id,
                                                       GValue         *value,
                                                       GParamSpec     *pspec);
static void             ligma_canvas_line_draw         (LigmaCanvasItem *item,
                                                       cairo_t        *cr);
static cairo_region_t * ligma_canvas_line_get_extents  (LigmaCanvasItem *item);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaCanvasLine, ligma_canvas_line,
                            LIGMA_TYPE_CANVAS_ITEM)

#define parent_class ligma_canvas_line_parent_class


static void
ligma_canvas_line_class_init (LigmaCanvasLineClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaCanvasItemClass *item_class   = LIGMA_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = ligma_canvas_line_set_property;
  object_class->get_property = ligma_canvas_line_get_property;

  item_class->draw           = ligma_canvas_line_draw;
  item_class->get_extents    = ligma_canvas_line_get_extents;

  g_object_class_install_property (object_class, PROP_X1,
                                   g_param_spec_double ("x1", NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE, 0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_Y1,
                                   g_param_spec_double ("y1", NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE, 0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_X2,
                                   g_param_spec_double ("x2", NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE, 0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_Y2,
                                   g_param_spec_double ("y2", NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE, 0,
                                                        LIGMA_PARAM_READWRITE));
}

static void
ligma_canvas_line_init (LigmaCanvasLine *line)
{
}

static void
ligma_canvas_line_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaCanvasLinePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_X1:
      private->x1 = g_value_get_double (value);
      break;
    case PROP_Y1:
      private->y1 = g_value_get_double (value);
      break;
    case PROP_X2:
      private->x2 = g_value_get_double (value);
      break;
    case PROP_Y2:
      private->y2 = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_canvas_line_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaCanvasLinePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_X1:
      g_value_set_double (value, private->x1);
      break;
    case PROP_Y1:
      g_value_set_double (value, private->y1);
      break;
    case PROP_X2:
      g_value_set_double (value, private->x2);
      break;
    case PROP_Y2:
      g_value_set_double (value, private->y2);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_canvas_line_transform (LigmaCanvasItem *item,
                            gdouble        *x1,
                            gdouble        *y1,
                            gdouble        *x2,
                            gdouble        *y2)
{
  LigmaCanvasLinePrivate *private = GET_PRIVATE (item);

  ligma_canvas_item_transform_xy_f (item,
                                   private->x1, private->y1,
                                   x1, y1);
  ligma_canvas_item_transform_xy_f (item,
                                   private->x2, private->y2,
                                   x2, y2);

  *x1 = floor (*x1) + 0.5;
  *y1 = floor (*y1) + 0.5;
  *x2 = floor (*x2) + 0.5;
  *y2 = floor (*y2) + 0.5;
}

static void
ligma_canvas_line_draw (LigmaCanvasItem *item,
                       cairo_t        *cr)
{
  gdouble x1, y1;
  gdouble x2, y2;

  ligma_canvas_line_transform (item, &x1, &y1, &x2, &y2);

  cairo_move_to (cr, x1, y1);
  cairo_line_to (cr, x2, y2);

  _ligma_canvas_item_stroke (item, cr);
}

static cairo_region_t *
ligma_canvas_line_get_extents (LigmaCanvasItem *item)
{
  cairo_rectangle_int_t rectangle;
  gdouble               x1, y1;
  gdouble               x2, y2;

  ligma_canvas_line_transform (item, &x1, &y1, &x2, &y2);

  if (x1 == x2 || y1 == y2)
    {
      rectangle.x      = MIN (x1, x2) - 1.5;
      rectangle.y      = MIN (y1, y2) - 1.5;
      rectangle.width  = ABS (x2 - x1) + 3.0;
      rectangle.height = ABS (y2 - y1) + 3.0;
    }
  else
    {
      rectangle.x      = floor (MIN (x1, x2) - 2.5);
      rectangle.y      = floor (MIN (y1, y2) - 2.5);
      rectangle.width  = ceil (ABS (x2 - x1) + 5.0);
      rectangle.height = ceil (ABS (y2 - y1) + 5.0);
    }

  return cairo_region_create_rectangle (&rectangle);
}

LigmaCanvasItem *
ligma_canvas_line_new (LigmaDisplayShell *shell,
                      gdouble           x1,
                      gdouble           y1,
                      gdouble           x2,
                      gdouble           y2)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (LIGMA_TYPE_CANVAS_LINE,
                       "shell", shell,
                       "x1",    x1,
                       "y1",    y1,
                       "x2",    x2,
                       "y2",    y2,
                       NULL);
}

void
ligma_canvas_line_set (LigmaCanvasItem *line,
                      gdouble         x1,
                      gdouble         y1,
                      gdouble         x2,
                      gdouble         y2)
{
  g_return_if_fail (LIGMA_IS_CANVAS_LINE (line));

  ligma_canvas_item_begin_change (line);

  g_object_set (line,
                "x1", x1,
                "y1", y1,
                "x2", x2,
                "y2", y2,
                NULL);

  ligma_canvas_item_end_change (line);
}
