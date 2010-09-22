/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvashandle.c
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

#include "gimpcanvashandle.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-transform.h"


enum
{
  PROP_0,
  PROP_TYPE,
  PROP_ANCHOR,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT
};


typedef struct _GimpCanvasHandlePrivate GimpCanvasHandlePrivate;

struct _GimpCanvasHandlePrivate
{
  GimpHandleType  type;
  GtkAnchorType   anchor;
  gdouble         x;
  gdouble         y;
  gint            width;
  gint            height;
};

#define GET_PRIVATE(handle) \
        G_TYPE_INSTANCE_GET_PRIVATE (handle, \
                                     GIMP_TYPE_CANVAS_HANDLE, \
                                     GimpCanvasHandlePrivate)


/*  local function prototypes  */

static void        gimp_canvas_handle_set_property (GObject          *object,
                                                    guint             property_id,
                                                    const GValue     *value,
                                                    GParamSpec       *pspec);
static void        gimp_canvas_handle_get_property (GObject          *object,
                                                    guint             property_id,
                                                    GValue           *value,
                                                    GParamSpec       *pspec);
static void        gimp_canvas_handle_draw         (GimpCanvasItem   *item,
                                                    GimpDisplayShell *shell,
                                                    cairo_t          *cr);
static GdkRegion * gimp_canvas_handle_get_extents  (GimpCanvasItem   *item,
                                                    GimpDisplayShell *shell);


G_DEFINE_TYPE (GimpCanvasHandle, gimp_canvas_handle,
               GIMP_TYPE_CANVAS_ITEM)

#define parent_class gimp_canvas_handle_parent_class


static void
gimp_canvas_handle_class_init (GimpCanvasHandleClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = gimp_canvas_handle_set_property;
  object_class->get_property = gimp_canvas_handle_get_property;

  item_class->draw           = gimp_canvas_handle_draw;
  item_class->get_extents    = gimp_canvas_handle_get_extents;

  g_object_class_install_property (object_class, PROP_TYPE,
                                   g_param_spec_enum ("type", NULL, NULL,
                                                      GIMP_TYPE_HANDLE_TYPE,
                                                      GIMP_HANDLE_CROSS,
                                                      GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_ANCHOR,
                                   g_param_spec_enum ("anchor", NULL, NULL,
                                                      GTK_TYPE_ANCHOR_TYPE,
                                                      GTK_ANCHOR_CENTER,
                                                      GIMP_PARAM_READWRITE));

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
                                   g_param_spec_int ("width", NULL, NULL,
                                                     3, 1001, 7,
                                                     GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_HEIGHT,
                                   g_param_spec_int ("height", NULL, NULL,
                                                     3, 1001, 7,
                                                     GIMP_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (GimpCanvasHandlePrivate));
}

static void
gimp_canvas_handle_init (GimpCanvasHandle *handle)
{
}

static void
gimp_canvas_handle_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpCanvasHandlePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_TYPE:
      private->type = g_value_get_enum (value);
      break;
    case PROP_ANCHOR:
      private->anchor = g_value_get_enum (value);
      break;
    case PROP_X:
      private->x = g_value_get_double (value);
      break;
    case PROP_Y:
      private->y = g_value_get_double (value);
      break;
    case PROP_WIDTH:
      private->width = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      private->height = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_handle_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpCanvasHandlePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_TYPE:
      g_value_set_enum (value, private->type);
      break;
    case PROP_ANCHOR:
      g_value_set_enum (value, private->anchor);
      break;
    case PROP_X:
      g_value_set_double (value, private->x);
      break;
    case PROP_Y:
      g_value_set_double (value, private->y);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, private->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, private->height);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static inline void
gimp_canvas_handle_shift_to_center (GtkAnchorType  anchor,
                                    gdouble        x,
                                    gdouble        y,
                                    gint           width,
                                    gint           height,
                                    gdouble       *shifted_x,
                                    gdouble       *shifted_y)
{
  switch (anchor)
    {
    case GTK_ANCHOR_CENTER:
      /*  nothing, this is the default  */
      break;

    case GTK_ANCHOR_NORTH:
      y += height / 2;
      break;

    case GTK_ANCHOR_NORTH_WEST:
      x += width  / 2;
      y += height / 2;
      break;

    case GTK_ANCHOR_NORTH_EAST:
      x -= width  / 2;
      y += height / 2;
      break;

    case GTK_ANCHOR_SOUTH:
      y -= height / 2;
      break;

    case GTK_ANCHOR_SOUTH_WEST:
      x += width  / 2;
      y -= height / 2;
      break;

    case GTK_ANCHOR_SOUTH_EAST:
      x -= width  / 2;
      y -= height / 2;
      break;

    case GTK_ANCHOR_WEST:
      x += width / 2;
      break;

    case GTK_ANCHOR_EAST:
      x -= width / 2;
      break;

    default:
      break;
    }

  if (shifted_x)
    *shifted_x = x;

  if (shifted_y)
    *shifted_y = y;
}

static void
gimp_canvas_handle_transform (GimpCanvasItem   *item,
                              GimpDisplayShell *shell,
                              gdouble          *x,
                              gdouble          *y)
{
  GimpCanvasHandlePrivate *private = GET_PRIVATE (item);

  gimp_display_shell_transform_xy_f (shell,
                                     private->x, private->y,
                                     x, y,
                                     FALSE);

  switch (private->type)
    {
    case GIMP_HANDLE_SQUARE:
      break;

    case GIMP_HANDLE_FILLED_SQUARE:
      break;

    case GIMP_HANDLE_CIRCLE:
    case GIMP_HANDLE_FILLED_CIRCLE:
    case GIMP_HANDLE_CROSS:
      gimp_canvas_handle_shift_to_center (private->anchor,
                                          *x, *y,
                                          private->width,
                                          private->height,
                                          x, y);
      break;

    default:
      break;
    }

  *x = PROJ_ROUND (*x) + 0.5;
  *y = PROJ_ROUND (*y) + 0.5;
}

static void
gimp_canvas_handle_draw (GimpCanvasItem   *item,
                         GimpDisplayShell *shell,
                         cairo_t          *cr)
{
  GimpCanvasHandlePrivate *private = GET_PRIVATE (item);
  gdouble                  x, y;

  gimp_canvas_handle_transform (item, shell, &x, &y);

  switch (private->type)
    {
    case GIMP_HANDLE_SQUARE:
      break;

    case GIMP_HANDLE_FILLED_SQUARE:
      break;

    case GIMP_HANDLE_CIRCLE:
      cairo_arc (cr, x, y, private->width / 2, 0, 2 * G_PI);

      _gimp_canvas_item_stroke (item, shell, cr);
      break;

    case GIMP_HANDLE_FILLED_CIRCLE:
      cairo_arc (cr, x, y, (gdouble) private->width / 2.0, 0, 2 * G_PI);

      _gimp_canvas_item_fill (item, shell, cr);
      break;

    case GIMP_HANDLE_CROSS:
      cairo_move_to (cr, x - private->width / 2, y);
      cairo_line_to (cr, x + private->width / 2, y);

      cairo_move_to (cr, x, y - private->height / 2);
      cairo_line_to (cr, x, y + private->height / 2);

      _gimp_canvas_item_stroke (item, shell, cr);
      break;

    default:
      break;
    }
}

static GdkRegion *
gimp_canvas_handle_get_extents (GimpCanvasItem   *item,
                                GimpDisplayShell *shell)
{
  GimpCanvasHandlePrivate *private = GET_PRIVATE (item);
  GdkRectangle             rectangle;
  gdouble                  x, y;

  gimp_canvas_handle_transform (item, shell, &x, &y);

  switch (private->type)
    {
    case GIMP_HANDLE_SQUARE:
      break;

    case GIMP_HANDLE_FILLED_SQUARE:
      break;

    case GIMP_HANDLE_CIRCLE:
    case GIMP_HANDLE_FILLED_CIRCLE:
    case GIMP_HANDLE_CROSS:
      rectangle.x      = x - private->width  / 2 - 1.5;
      rectangle.y      = y - private->height / 2 - 1.5;
      rectangle.width  = private->width  + 3.0;
      rectangle.height = private->height + 3.0;
      break;

    default:
      break;
    }

  return gdk_region_rectangle (&rectangle);
}

GimpCanvasItem *
gimp_canvas_handle_new (GimpHandleType  type,
                        GtkAnchorType   anchor,
                        gdouble         x,
                        gdouble         y,
                        gint            width,
                        gint            height)
{
  return g_object_new (GIMP_TYPE_CANVAS_HANDLE,
                       "type",   type,
                       "anchor", anchor,
                       "x",      x,
                       "y",      y,
                       "width",  width,
                       "height", height,
                       NULL);
}
