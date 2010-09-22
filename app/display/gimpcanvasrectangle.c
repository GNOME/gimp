/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasrectangle.c
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "gimpcanvasrectangle.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-transform.h"


enum
{
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_FILLED
};


typedef struct _GimpCanvasRectanglePrivate GimpCanvasRectanglePrivate;

struct _GimpCanvasRectanglePrivate
{
  gdouble  x;
  gdouble  y;
  gdouble  width;
  gdouble  height;
  gboolean filled;
};

#define GET_PRIVATE(rectangle) \
        G_TYPE_INSTANCE_GET_PRIVATE (rectangle, \
                                     GIMP_TYPE_CANVAS_RECTANGLE, \
                                     GimpCanvasRectanglePrivate)


/*  local function prototypes  */

static void        gimp_canvas_rectangle_set_property (GObject          *object,
                                                       guint             property_id,
                                                       const GValue     *value,
                                                       GParamSpec       *pspec);
static void        gimp_canvas_rectangle_get_property (GObject          *object,
                                                       guint             property_id,
                                                       GValue           *value,
                                                       GParamSpec       *pspec);
static void        gimp_canvas_rectangle_draw         (GimpCanvasItem   *item,
                                                       GimpDisplayShell *shell,
                                                       cairo_t          *cr);
static GdkRegion * gimp_canvas_rectangle_get_extents  (GimpCanvasItem   *item,
                                                       GimpDisplayShell *shell);


G_DEFINE_TYPE (GimpCanvasRectangle, gimp_canvas_rectangle,
               GIMP_TYPE_CANVAS_ITEM)

#define parent_class gimp_canvas_rectangle_parent_class


static void
gimp_canvas_rectangle_class_init (GimpCanvasRectangleClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = gimp_canvas_rectangle_set_property;
  object_class->get_property = gimp_canvas_rectangle_get_property;

  item_class->draw           = gimp_canvas_rectangle_draw;
  item_class->get_extents    = gimp_canvas_rectangle_get_extents;

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
                                                        0, GIMP_MAX_IMAGE_SIZE, 0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_HEIGHT,
                                   g_param_spec_double ("height", NULL, NULL,
                                                        0, GIMP_MAX_IMAGE_SIZE, 0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_FILLED,
                                   g_param_spec_boolean ("filled", NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (GimpCanvasRectanglePrivate));
}

static void
gimp_canvas_rectangle_init (GimpCanvasRectangle *rectangle)
{
}

static void
gimp_canvas_rectangle_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GimpCanvasRectanglePrivate *private = GET_PRIVATE (object);

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
    case PROP_FILLED:
      private->filled = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_rectangle_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpCanvasRectanglePrivate *private = GET_PRIVATE (object);

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
    case PROP_FILLED:
      g_value_set_boolean (value, private->filled);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_rectangle_transform (GimpCanvasItem   *item,
                                 GimpDisplayShell *shell,
                                 gdouble          *x,
                                 gdouble          *y,
                                 gdouble          *w,
                                 gdouble          *h)
{
  GimpCanvasRectanglePrivate *private = GET_PRIVATE (item);

  gimp_display_shell_transform_xy_f (shell,
                                     MIN (private->x,
                                          private->x + private->width),
                                     MIN (private->y,
                                          private->y + private->height),
                                     x, y,
                                     FALSE);
  gimp_display_shell_transform_xy_f (shell,
                                     MAX (private->x,
                                          private->x + private->width),
                                     MAX (private->y,
                                          private->y + private->height),
                                     w, h,
                                     FALSE);

  *w -= *x;
  *h -= *y;

  if (private->filled)
    {
      *x = floor (*x);
      *y = floor (*y);
      *w = ceil (*w);
      *h = ceil (*h);
    }
  else
    {
      *x = PROJ_ROUND (*x) + 0.5;
      *y = PROJ_ROUND (*y) + 0.5;
      *w = PROJ_ROUND (*w) - 1.0;
      *h = PROJ_ROUND (*h) - 1.0;
    }
}

static void
gimp_canvas_rectangle_draw (GimpCanvasItem   *item,
                            GimpDisplayShell *shell,
                            cairo_t          *cr)
{
  GimpCanvasRectanglePrivate *private = GET_PRIVATE (item);
  gdouble                     x, y;
  gdouble                     w, h;

  gimp_canvas_rectangle_transform (item, shell, &x, &y, &w, &h);

  cairo_rectangle (cr, x, y, w, h);

  if (private->filled)
    _gimp_canvas_item_fill (item, shell, cr);
  else
    _gimp_canvas_item_stroke (item, shell, cr);
}

static GdkRegion *
gimp_canvas_rectangle_get_extents (GimpCanvasItem   *item,
                                   GimpDisplayShell *shell)
{
  GimpCanvasRectanglePrivate *private = GET_PRIVATE (item);
  GdkRectangle                rectangle;
  gdouble                     x, y;
  gdouble                     w, h;

  gimp_canvas_rectangle_transform (item, shell, &x, &y, &w, &h);

  if (private->filled)
    {
      rectangle.x      = floor (x);
      rectangle.y      = floor (y);
      rectangle.width  = ceil (w);
      rectangle.height = ceil (h);
    }
  else
    {
      rectangle.x      = floor (x - 1.5);
      rectangle.y      = floor (y - 1.5);
      rectangle.width  = ceil (w + 3.0);
      rectangle.height = ceil (h + 3.0);
    }

  return gdk_region_rectangle (&rectangle);
}

GimpCanvasItem *
gimp_canvas_rectangle_new (gdouble  x,
                           gdouble  y,
                           gdouble  width,
                           gdouble  height,
                           gboolean filled)
{
  return g_object_new (GIMP_TYPE_CANVAS_RECTANGLE,
                       "x",      x,
                       "y",      y,
                       "width",  width,
                       "height", height,
                       "filled", filled,
                       NULL);
}
