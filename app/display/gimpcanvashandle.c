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

#include "widgets/gimpcairo.h"

#include "gimpcanvashandle.h"
#include "gimpcanvasitem-utils.h"
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
  PROP_HEIGHT,
  PROP_START_ANGLE,
  PROP_SLICE_ANGLE
};


typedef struct _GimpCanvasHandlePrivate GimpCanvasHandlePrivate;

struct _GimpCanvasHandlePrivate
{
  GimpHandleType   type;
  GimpHandleAnchor anchor;
  gdouble          x;
  gdouble          y;
  gint             width;
  gint             height;
  gdouble          start_angle;
  gdouble          slice_angle;;
};

#define GET_PRIVATE(handle) \
        G_TYPE_INSTANCE_GET_PRIVATE (handle, \
                                     GIMP_TYPE_CANVAS_HANDLE, \
                                     GimpCanvasHandlePrivate)


/*  local function prototypes  */

static void             gimp_canvas_handle_set_property (GObject          *object,
                                                         guint             property_id,
                                                         const GValue     *value,
                                                         GParamSpec       *pspec);
static void             gimp_canvas_handle_get_property (GObject          *object,
                                                         guint             property_id,
                                                         GValue           *value,
                                                         GParamSpec       *pspec);
static void             gimp_canvas_handle_draw         (GimpCanvasItem   *item,
                                                         GimpDisplayShell *shell,
                                                         cairo_t          *cr);
static cairo_region_t * gimp_canvas_handle_get_extents  (GimpCanvasItem   *item,
                                                         GimpDisplayShell *shell);
static gboolean         gimp_canvas_handle_hit          (GimpCanvasItem   *item,
                                                         GimpDisplayShell *shell,
                                                         gdouble           x,
                                                         gdouble           y);


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
  item_class->hit            = gimp_canvas_handle_hit;

  g_object_class_install_property (object_class, PROP_TYPE,
                                   g_param_spec_enum ("type", NULL, NULL,
                                                      GIMP_TYPE_HANDLE_TYPE,
                                                      GIMP_HANDLE_CROSS,
                                                      GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_ANCHOR,
                                   g_param_spec_enum ("anchor", NULL, NULL,
                                                      GIMP_TYPE_HANDLE_ANCHOR,
                                                      GIMP_HANDLE_ANCHOR_CENTER,
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

  g_object_class_install_property (object_class, PROP_START_ANGLE,
                                   g_param_spec_double ("start-angle", NULL, NULL,
                                                        -1000, 1000, 0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SLICE_ANGLE,
                                   g_param_spec_double ("slice-angle", NULL, NULL,
                                                        -1000, 1000, 2 * G_PI,
                                                        GIMP_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (GimpCanvasHandlePrivate));
}

static void
gimp_canvas_handle_init (GimpCanvasHandle *handle)
{
  GimpCanvasHandlePrivate *private = GET_PRIVATE (handle);

  gimp_canvas_item_set_line_cap (GIMP_CANVAS_ITEM (handle),
                                 CAIRO_LINE_CAP_SQUARE);

  private->start_angle = 0.0;
  private->slice_angle = 2.0 * G_PI;
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
    case PROP_START_ANGLE:
      private->start_angle = g_value_get_double (value);
      break;
    case PROP_SLICE_ANGLE:
      private->slice_angle = g_value_get_double (value);
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
    case PROP_START_ANGLE:
      g_value_set_double (value, private->start_angle);
      break;
    case PROP_SLICE_ANGLE:
      g_value_set_double (value, private->slice_angle);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
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
                                     x, y);

  switch (private->type)
    {
    case GIMP_HANDLE_SQUARE:
    case GIMP_HANDLE_FILLED_SQUARE:
      gimp_canvas_item_shift_to_north_west (private->anchor,
                                            *x, *y,
                                            private->width,
                                            private->height,
                                            x, y);
      break;

    case GIMP_HANDLE_CIRCLE:
    case GIMP_HANDLE_FILLED_CIRCLE:
    case GIMP_HANDLE_CROSS:
      gimp_canvas_item_shift_to_center (private->anchor,
                                        *x, *y,
                                        private->width,
                                        private->height,
                                        x, y);
      break;

    default:
      break;
    }

  *x = floor (*x) + 0.5;
  *y = floor (*y) + 0.5;
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
      cairo_rectangle (cr, x, y, private->width - 1.0, private->height - 1.0);

      _gimp_canvas_item_stroke (item, cr);
      break;

    case GIMP_HANDLE_FILLED_SQUARE:
      cairo_rectangle (cr, x - 0.5, y - 0.5, private->width, private->height);

      _gimp_canvas_item_fill (item, cr);
      break;

    case GIMP_HANDLE_CIRCLE:
      gimp_cairo_add_arc (cr, x, y, private->width / 2,
                          private->start_angle,
                          private->slice_angle);

      _gimp_canvas_item_stroke (item, cr);
      break;

    case GIMP_HANDLE_FILLED_CIRCLE:
      cairo_move_to (cr, x, y);

      gimp_cairo_add_arc (cr, x, y, (gdouble) private->width / 2.0,
                          private->start_angle,
                          private->slice_angle);

      _gimp_canvas_item_fill (item, cr);
      break;

    case GIMP_HANDLE_CROSS:
      cairo_move_to (cr, x - private->width / 2, y);
      cairo_line_to (cr, x + private->width / 2, y);

      cairo_move_to (cr, x, y - private->height / 2);
      cairo_line_to (cr, x, y + private->height / 2);

      _gimp_canvas_item_stroke (item, cr);
      break;

    default:
      break;
    }
}

static cairo_region_t *
gimp_canvas_handle_get_extents (GimpCanvasItem   *item,
                                GimpDisplayShell *shell)
{
  GimpCanvasHandlePrivate *private = GET_PRIVATE (item);
  cairo_rectangle_int_t    rectangle;
  gdouble                  x, y;

  gimp_canvas_handle_transform (item, shell, &x, &y);

  switch (private->type)
    {
    case GIMP_HANDLE_SQUARE:
    case GIMP_HANDLE_FILLED_SQUARE:
      rectangle.x      = x - 1.5;
      rectangle.y      = y - 1.5;
      rectangle.width  = private->width  + 3.0;
      rectangle.height = private->height + 3.0;
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

  return cairo_region_create_rectangle (&rectangle);
}

static gboolean
gimp_canvas_handle_hit (GimpCanvasItem   *item,
                        GimpDisplayShell *shell,
                        gdouble           x,
                        gdouble           y)
{
  GimpCanvasHandlePrivate *private = GET_PRIVATE (item);
  gdouble                  handle_tx, handle_ty;
  gdouble                  tx, ty;

  gimp_canvas_handle_transform (item, shell, &handle_tx, &handle_ty);

  gimp_display_shell_transform_xy_f (shell,
                                     x, y,
                                     &tx, &ty);

  switch (private->type)
    {
    case GIMP_HANDLE_SQUARE:
    case GIMP_HANDLE_FILLED_SQUARE:
     return (tx == CLAMP (tx, handle_tx, handle_tx + private->width) &&
              ty == CLAMP (ty, handle_ty, handle_ty + private->height));

    case GIMP_HANDLE_CIRCLE:
    case GIMP_HANDLE_FILLED_CIRCLE:
    case GIMP_HANDLE_CROSS:
      {
        gint width = private->width;

        if (width != private->height)
          width = (width + private->height) / 2;

        width /= 2;

        return ((SQR (handle_tx - tx) + SQR (handle_ty - ty)) < SQR (width));
      }

    default:
      break;
    }

  return FALSE;
}

GimpCanvasItem *
gimp_canvas_handle_new (GimpDisplayShell *shell,
                        GimpHandleType    type,
                        GimpHandleAnchor  anchor,
                        gdouble           x,
                        gdouble           y,
                        gint              width,
                        gint              height)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_CANVAS_HANDLE,
                       "shell",  shell,
                       "type",   type,
                       "anchor", anchor,
                       "x",      x,
                       "y",      y,
                       "width",  width,
                       "height", height,
                       NULL);
}

void
gimp_canvas_handle_get_position (GimpCanvasItem *handle,
                                 gdouble        *x,
                                 gdouble        *y)
{
  g_return_if_fail (GIMP_IS_CANVAS_HANDLE (handle));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);

  g_object_get (handle,
                "x", x,
                "y", y,
                NULL);
}

void
gimp_canvas_handle_set_position (GimpCanvasItem *handle,
                                 gdouble         x,
                                 gdouble         y)
{
  g_return_if_fail (GIMP_IS_CANVAS_HANDLE (handle));

  gimp_canvas_item_begin_change (handle);

  g_object_set (handle,
                "x", x,
                "y", y,
                NULL);

  gimp_canvas_item_end_change (handle);
}

void
gimp_canvas_handle_set_angles (GimpCanvasItem *handle,
                               gdouble         start_angle,
                               gdouble         slice_angle)
{
  g_return_if_fail (GIMP_IS_CANVAS_HANDLE (handle));

  gimp_canvas_item_begin_change (handle);

  g_object_set (handle,
                "start-angle", start_angle,
                "slice-angle", slice_angle,
                NULL);

  gimp_canvas_item_end_change (handle);
}
