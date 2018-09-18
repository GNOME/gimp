/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvassamplepoint.c
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

#include "gimpcanvas.h"
#include "gimpcanvas-style.h"
#include "gimpcanvassamplepoint.h"
#include "gimpdisplayshell.h"


#define GIMP_SAMPLE_POINT_DRAW_SIZE 14


enum
{
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_INDEX,
  PROP_SAMPLE_POINT_STYLE
};


typedef struct _GimpCanvasSamplePointPrivate GimpCanvasSamplePointPrivate;

struct _GimpCanvasSamplePointPrivate
{
  gint     x;
  gint     y;
  gint     index;
  gboolean sample_point_style;
};

#define GET_PRIVATE(sample_point) \
        ((GimpCanvasSamplePointPrivate *) gimp_canvas_sample_point_get_instance_private ((GimpCanvasSamplePoint *) (sample_point)))


/*  local function prototypes  */

static void             gimp_canvas_sample_point_set_property (GObject        *object,
                                                               guint           property_id,
                                                               const GValue   *value,
                                                               GParamSpec     *pspec);
static void             gimp_canvas_sample_point_get_property (GObject        *object,
                                                               guint           property_id,
                                                               GValue         *value,
                                                               GParamSpec     *pspec);
static void             gimp_canvas_sample_point_draw         (GimpCanvasItem *item,
                                                               cairo_t        *cr);
static cairo_region_t * gimp_canvas_sample_point_get_extents  (GimpCanvasItem *item);
static void             gimp_canvas_sample_point_stroke       (GimpCanvasItem *item,
                                                               cairo_t        *cr);
static void             gimp_canvas_sample_point_fill         (GimpCanvasItem *item,
                                                               cairo_t        *cr);


G_DEFINE_TYPE_WITH_PRIVATE (GimpCanvasSamplePoint, gimp_canvas_sample_point,
                            GIMP_TYPE_CANVAS_ITEM)

#define parent_class gimp_canvas_sample_point_parent_class


static void
gimp_canvas_sample_point_class_init (GimpCanvasSamplePointClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = gimp_canvas_sample_point_set_property;
  object_class->get_property = gimp_canvas_sample_point_get_property;

  item_class->draw           = gimp_canvas_sample_point_draw;
  item_class->get_extents    = gimp_canvas_sample_point_get_extents;
  item_class->stroke         = gimp_canvas_sample_point_stroke;
  item_class->fill           = gimp_canvas_sample_point_fill;

  g_object_class_install_property (object_class, PROP_X,
                                   g_param_spec_int ("x", NULL, NULL,
                                                     -GIMP_MAX_IMAGE_SIZE,
                                                     GIMP_MAX_IMAGE_SIZE, 0,
                                                     GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_Y,
                                   g_param_spec_int ("y", NULL, NULL,
                                                     -GIMP_MAX_IMAGE_SIZE,
                                                     GIMP_MAX_IMAGE_SIZE, 0,
                                                     GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_INDEX,
                                   g_param_spec_int ("index", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SAMPLE_POINT_STYLE,
                                   g_param_spec_boolean ("sample-point-style",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));
}

static void
gimp_canvas_sample_point_init (GimpCanvasSamplePoint *sample_point)
{
}

static void
gimp_canvas_sample_point_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GimpCanvasSamplePointPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_X:
      private->x = g_value_get_int (value);
      break;
    case PROP_Y:
      private->y = g_value_get_int (value);
      break;
    case PROP_INDEX:
      private->index = g_value_get_int (value);
      break;
    case PROP_SAMPLE_POINT_STYLE:
      private->sample_point_style = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_sample_point_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  GimpCanvasSamplePointPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_X:
      g_value_set_int (value, private->x);
      break;
    case PROP_Y:
      g_value_set_int (value, private->x);
      break;
    case PROP_INDEX:
      g_value_set_int (value, private->x);
      break;
    case PROP_SAMPLE_POINT_STYLE:
      g_value_set_boolean (value, private->sample_point_style);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_sample_point_transform (GimpCanvasItem *item,
                                    gdouble        *x,
                                    gdouble        *y)
{
  GimpCanvasSamplePointPrivate *private = GET_PRIVATE (item);

  gimp_canvas_item_transform_xy_f (item,
                                   private->x + 0.5,
                                   private->y + 0.5,
                                   x, y);

  *x = floor (*x) + 0.5;
  *y = floor (*y) + 0.5;
}

#define HALF_SIZE (GIMP_SAMPLE_POINT_DRAW_SIZE / 2)

static void
gimp_canvas_sample_point_draw (GimpCanvasItem *item,
                               cairo_t        *cr)
{
  GimpCanvasSamplePointPrivate *private = GET_PRIVATE (item);
  GtkWidget                    *canvas  = gimp_canvas_item_get_canvas (item);
  PangoLayout                  *layout;
  gdouble                       x, y;
  gint                          x1, x2, y1, y2;

  gimp_canvas_sample_point_transform (item, &x, &y);

  x1 = x - GIMP_SAMPLE_POINT_DRAW_SIZE;
  x2 = x + GIMP_SAMPLE_POINT_DRAW_SIZE;
  y1 = y - GIMP_SAMPLE_POINT_DRAW_SIZE;
  y2 = y + GIMP_SAMPLE_POINT_DRAW_SIZE;

  cairo_move_to (cr, x, y1);
  cairo_line_to (cr, x, y1 + HALF_SIZE);

  cairo_move_to (cr, x, y2);
  cairo_line_to (cr, x, y2 - HALF_SIZE);

  cairo_move_to (cr, x1,             y);
  cairo_line_to (cr, x1 + HALF_SIZE, y);

  cairo_move_to (cr, x2,             y);
  cairo_line_to (cr, x2 - HALF_SIZE, y);

  cairo_arc_negative (cr, x, y, HALF_SIZE, 0.0, 0.5 * G_PI);

  _gimp_canvas_item_stroke (item, cr);

  layout = gimp_canvas_get_layout (GIMP_CANVAS (canvas),
                                   "%d", private->index);

  cairo_move_to (cr, x + 3, y + 3);
  pango_cairo_show_layout (cr, layout);

  _gimp_canvas_item_stroke (item, cr);
}

static cairo_region_t *
gimp_canvas_sample_point_get_extents (GimpCanvasItem *item)
{
  GimpCanvasSamplePointPrivate *private = GET_PRIVATE (item);
  GtkWidget                    *canvas  = gimp_canvas_item_get_canvas (item);
  cairo_rectangle_int_t         rectangle;
  PangoLayout                  *layout;
  PangoRectangle                ink;
  gdouble                       x, y;
  gint                          x1, x2, y1, y2;

  gimp_canvas_sample_point_transform (item, &x, &y);

  x1 = floor (x - GIMP_SAMPLE_POINT_DRAW_SIZE);
  x2 = ceil  (x + GIMP_SAMPLE_POINT_DRAW_SIZE);
  y1 = floor (y - GIMP_SAMPLE_POINT_DRAW_SIZE);
  y2 = ceil  (y + GIMP_SAMPLE_POINT_DRAW_SIZE);

  layout = gimp_canvas_get_layout (GIMP_CANVAS (canvas),
                                   "%d", private->index);

  pango_layout_get_extents (layout, &ink, NULL);

  x2 = MAX (x2, 3 + ink.width);
  y2 = MAX (y2, 3 + ink.height);

  rectangle.x      = x1 - 1.5;
  rectangle.y      = y1 - 1.5;
  rectangle.width  = x2 - x1 + 3.0;
  rectangle.height = y2 - y1 + 3.0;

  return cairo_region_create_rectangle (&rectangle);
}

static void
gimp_canvas_sample_point_stroke (GimpCanvasItem *item,
                                 cairo_t        *cr)
{
  GimpCanvasSamplePointPrivate *private = GET_PRIVATE (item);

  if (private->sample_point_style)
    {
      gimp_canvas_set_tool_bg_style (gimp_canvas_item_get_canvas (item), cr);
      cairo_stroke_preserve (cr);

      gimp_canvas_set_sample_point_style (gimp_canvas_item_get_canvas (item), cr,
                                          gimp_canvas_item_get_highlight (item));
      cairo_stroke (cr);
    }
  else
    {
      GIMP_CANVAS_ITEM_CLASS (parent_class)->stroke (item, cr);
    }
}

static void
gimp_canvas_sample_point_fill (GimpCanvasItem *item,
                               cairo_t        *cr)
{
  GimpCanvasSamplePointPrivate *private = GET_PRIVATE (item);

  if (private->sample_point_style)
    {
      gimp_canvas_set_sample_point_style (gimp_canvas_item_get_canvas (item), cr,
                                          gimp_canvas_item_get_highlight (item));
      cairo_fill (cr);
    }
  else
    {
      GIMP_CANVAS_ITEM_CLASS (parent_class)->fill (item, cr);
    }
}

GimpCanvasItem *
gimp_canvas_sample_point_new (GimpDisplayShell *shell,
                              gint              x,
                              gint              y,
                              gint              index,
                              gboolean          sample_point_style)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_CANVAS_SAMPLE_POINT,
                       "shell",              shell,
                       "x",                  x,
                       "y",                  y,
                       "index",              index,
                       "sample-point-style", sample_point_style,
                       NULL);
}

void
gimp_canvas_sample_point_set (GimpCanvasItem *sample_point,
                              gint            x,
                              gint            y)
{
  g_return_if_fail (GIMP_IS_CANVAS_SAMPLE_POINT (sample_point));

  gimp_canvas_item_begin_change (sample_point);

  g_object_set (sample_point,
                "x", x,
                "y", y,
                NULL);

  gimp_canvas_item_end_change (sample_point);
}
