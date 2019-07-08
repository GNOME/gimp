/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasrectangleguides.c
 * Copyright (C) 2011 Michael Natterer <mitch@gimp.org>
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

#include "gimpcanvasrectangleguides.h"
#include "gimpdisplayshell.h"


#define SQRT5 2.236067977


enum
{
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_TYPE,
  PROP_N_GUIDES
};


typedef struct _GimpCanvasRectangleGuidesPrivate GimpCanvasRectangleGuidesPrivate;

struct _GimpCanvasRectangleGuidesPrivate
{
  gdouble        x;
  gdouble        y;
  gdouble        width;
  gdouble        height;
  GimpGuidesType type;
  gint           n_guides;
};

#define GET_PRIVATE(rectangle) \
        ((GimpCanvasRectangleGuidesPrivate *) gimp_canvas_rectangle_guides_get_instance_private ((GimpCanvasRectangleGuides *) (rectangle)))


/*  local function prototypes  */

static void             gimp_canvas_rectangle_guides_set_property (GObject        *object,
                                                                   guint           property_id,
                                                                   const GValue   *value,
                                                                   GParamSpec     *pspec);
static void             gimp_canvas_rectangle_guides_get_property (GObject        *object,
                                                                   guint           property_id,
                                                                   GValue         *value,
                                                                   GParamSpec     *pspec);
static void             gimp_canvas_rectangle_guides_draw         (GimpCanvasItem *item,
                                                                   cairo_t        *cr);
static cairo_region_t * gimp_canvas_rectangle_guides_get_extents  (GimpCanvasItem *item);


G_DEFINE_TYPE_WITH_PRIVATE (GimpCanvasRectangleGuides,
                            gimp_canvas_rectangle_guides, GIMP_TYPE_CANVAS_ITEM)

#define parent_class gimp_canvas_rectangle_guides_parent_class


static void
gimp_canvas_rectangle_guides_class_init (GimpCanvasRectangleGuidesClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = gimp_canvas_rectangle_guides_set_property;
  object_class->get_property = gimp_canvas_rectangle_guides_get_property;

  item_class->draw           = gimp_canvas_rectangle_guides_draw;
  item_class->get_extents    = gimp_canvas_rectangle_guides_get_extents;

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

  g_object_class_install_property (object_class, PROP_WIDTH,
                                   g_param_spec_double ("width", NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE, 0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_HEIGHT,
                                   g_param_spec_double ("height", NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE, 0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_TYPE,
                                   g_param_spec_enum ("type", NULL, NULL,
                                                      GIMP_TYPE_GUIDES_TYPE,
                                                      GIMP_GUIDES_NONE,
                                                      GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_N_GUIDES,
                                   g_param_spec_int ("n-guides", NULL, NULL,
                                                     1, 128, 4,
                                                     GIMP_PARAM_READWRITE));
}

static void
gimp_canvas_rectangle_guides_init (GimpCanvasRectangleGuides *rectangle)
{
}

static void
gimp_canvas_rectangle_guides_set_property (GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  GimpCanvasRectangleGuidesPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_X:
      private->x = g_value_get_double (value);
      break;
    case PROP_Y:
      private->y = g_value_get_double (value);
      break;
    case PROP_WIDTH:
      private->width = g_value_get_double (value);
      break;
    case PROP_HEIGHT:
      private->height = g_value_get_double (value);
      break;
    case PROP_TYPE:
      private->type = g_value_get_enum (value);
      break;
    case PROP_N_GUIDES:
      private->n_guides = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_rectangle_guides_get_property (GObject    *object,
                                           guint       property_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  GimpCanvasRectangleGuidesPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_X:
      g_value_set_double (value, private->x);
      break;
    case PROP_Y:
      g_value_set_double (value, private->y);
      break;
    case PROP_WIDTH:
      g_value_set_double (value, private->width);
      break;
    case PROP_HEIGHT:
      g_value_set_double (value, private->height);
      break;
    case PROP_TYPE:
      g_value_set_enum (value, private->type);
      break;
    case PROP_N_GUIDES:
      g_value_set_int (value, private->n_guides);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_rectangle_guides_transform (GimpCanvasItem *item,
                                        gdouble        *x1,
                                        gdouble        *y1,
                                        gdouble        *x2,
                                        gdouble        *y2)
{
  GimpCanvasRectangleGuidesPrivate *private = GET_PRIVATE (item);

  gimp_canvas_item_transform_xy_f (item,
                                   MIN (private->x,
                                        private->x + private->width),
                                   MIN (private->y,
                                        private->y + private->height),
                                   x1, y1);
  gimp_canvas_item_transform_xy_f (item,
                                   MAX (private->x,
                                        private->x + private->width),
                                   MAX (private->y,
                                        private->y + private->height),
                                   x2, y2);

  *x1 = floor (*x1) + 0.5;
  *y1 = floor (*y1) + 0.5;
  *x2 = ceil (*x2) - 0.5;
  *y2 = ceil (*y2) - 0.5;

  *x2 = MAX (*x1, *x2);
  *y2 = MAX (*y1, *y2);
}

static void
draw_hline (cairo_t *cr,
            gdouble  x1,
            gdouble  x2,
            gdouble  y)
{
  y = floor (y) + 0.5;

  cairo_move_to (cr, x1, y);
  cairo_line_to (cr, x2, y);
}

static void
draw_vline (cairo_t *cr,
            gdouble  y1,
            gdouble  y2,
            gdouble  x)
{
  x = floor (x) + 0.5;

  cairo_move_to (cr, x, y1);
  cairo_line_to (cr, x, y2);
}

static void
gimp_canvas_rectangle_guides_draw (GimpCanvasItem *item,
                                   cairo_t        *cr)
{
  GimpCanvasRectangleGuidesPrivate *private = GET_PRIVATE (item);
  gdouble                           x1, y1;
  gdouble                           x2, y2;
  gint                              i;

  gimp_canvas_rectangle_guides_transform (item, &x1, &y1, &x2, &y2);

  switch (private->type)
    {
    case GIMP_GUIDES_NONE:
      break;

    case GIMP_GUIDES_CENTER_LINES:
      draw_hline (cr, x1, x2, (y1 + y2) / 2);
      draw_vline (cr, y1, y2, (x1 + x2) / 2);
      break;

    case GIMP_GUIDES_THIRDS:
      draw_hline (cr, x1, x2, (2 * y1 +     y2) / 3);
      draw_hline (cr, x1, x2, (    y1 + 2 * y2) / 3);

      draw_vline (cr, y1, y2, (2 * x1 +     x2) / 3);
      draw_vline (cr, y1, y2, (    x1 + 2 * x2) / 3);
      break;

    case GIMP_GUIDES_FIFTHS:
      for (i = 0; i < 5; i++)
        {
          draw_hline (cr, x1, x2, y1 + i * (y2 - y1) / 5);
          draw_vline (cr, y1, y2, x1 + i * (x2 - x1) / 5);
        }
      break;

    case GIMP_GUIDES_GOLDEN:
      draw_hline (cr, x1, x2, (2 * y1 + (1 + SQRT5) * y2) / (3 + SQRT5));
      draw_hline (cr, x1, x2, ((1 + SQRT5) * y1 + 2 * y2) / (3 + SQRT5));

      draw_vline (cr, y1, y2, (2 * x1 + (1 + SQRT5) * x2) / (3 + SQRT5));
      draw_vline (cr, y1, y2, ((1 + SQRT5) * x1 + 2 * x2) / (3 + SQRT5));
      break;

    /* This code implements the method of diagonals discovered by
     * Edwin Westhoff - see http://www.diagonalmethod.info/
     */
    case GIMP_GUIDES_DIAGONALS:
      {
        /* the side of the largest square that can be
         * fitted in whole into the rectangle (x1, y1), (x2, y2)
         */
        const gdouble square_side = MIN (x2 - x1, y2 - y1);

        /* diagonal from the top-left edge */
        cairo_move_to (cr, x1, y1);
        cairo_line_to (cr, x1 + square_side, y1 + square_side);

        /* diagonal from the top-right edge */
        cairo_move_to (cr, x2, y1);
        cairo_line_to (cr, x2 - square_side, y1 + square_side);

        /* diagonal from the bottom-left edge */
        cairo_move_to (cr, x1, y2);
        cairo_line_to (cr, x1 + square_side, y2 - square_side);

        /* diagonal from the bottom-right edge */
        cairo_move_to (cr, x2, y2);
        cairo_line_to (cr, x2 - square_side, y2 - square_side);
      }
      break;

    case GIMP_GUIDES_N_LINES:
      for (i = 0; i < private->n_guides; i++)
        {
          draw_hline (cr, x1, x2, y1 + i * (y2 - y1) / private->n_guides);
          draw_vline (cr, y1, y2, x1 + i * (x2 - x1) / private->n_guides);
        }
      break;

    case GIMP_GUIDES_SPACING:
      break;
    }

  _gimp_canvas_item_stroke (item, cr);
}

static cairo_region_t *
gimp_canvas_rectangle_guides_get_extents (GimpCanvasItem *item)
{
  GimpCanvasRectangleGuidesPrivate *private = GET_PRIVATE (item);

  if (private->type != GIMP_GUIDES_NONE)
    {
      cairo_rectangle_int_t rectangle;
      gdouble               x1, y1;
      gdouble               x2, y2;

      gimp_canvas_rectangle_guides_transform (item, &x1, &y1, &x2, &y2);

      rectangle.x      = floor (x1 - 1.5);
      rectangle.y      = floor (y1 - 1.5);
      rectangle.width  = ceil (x2 - x1 + 3.0);
      rectangle.height = ceil (y2 - y1 + 3.0);

      return cairo_region_create_rectangle (&rectangle);
    }

  return NULL;
}

GimpCanvasItem *
gimp_canvas_rectangle_guides_new (GimpDisplayShell *shell,
                                  gdouble           x,
                                  gdouble           y,
                                  gdouble           width,
                                  gdouble           height,
                                  GimpGuidesType    type,
                                  gint              n_guides)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_CANVAS_RECTANGLE_GUIDES,
                       "shell",    shell,
                       "x",        x,
                       "y",        y,
                       "width",    width,
                       "height",   height,
                       "type",     type,
                       "n-guides", n_guides,
                       NULL);
}

void
gimp_canvas_rectangle_guides_set (GimpCanvasItem *rectangle,
                                  gdouble         x,
                                  gdouble         y,
                                  gdouble         width,
                                  gdouble         height,
                                  GimpGuidesType  type,
                                  gint            n_guides)
{
  g_return_if_fail (GIMP_IS_CANVAS_RECTANGLE_GUIDES (rectangle));

  gimp_canvas_item_begin_change (rectangle);

  g_object_set (rectangle,
                "x",        x,
                "y",        y,
                "width",    width,
                "height",   height,
                "type",     type,
                "n-guides", n_guides,
                NULL);

  gimp_canvas_item_end_change (rectangle);
}
