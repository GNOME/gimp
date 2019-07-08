/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvascorner.c
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

#include "gimpcanvascorner.h"
#include "gimpdisplayshell.h"


enum
{
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_ANCHOR,
  PROP_CORNER_WIDTH,
  PROP_CORNER_HEIGHT,
  PROP_OUTSIDE
};


typedef struct _GimpCanvasCornerPrivate GimpCanvasCornerPrivate;

struct _GimpCanvasCornerPrivate
{
  gdouble          x;
  gdouble          y;
  gdouble          width;
  gdouble          height;
  GimpHandleAnchor anchor;
  gint             corner_width;
  gint             corner_height;
  gboolean         outside;
};

#define GET_PRIVATE(corner) \
        ((GimpCanvasCornerPrivate *) gimp_canvas_corner_get_instance_private ((GimpCanvasCorner *) (corner)))


/*  local function prototypes  */

static void             gimp_canvas_corner_set_property (GObject        *object,
                                                         guint           property_id,
                                                         const GValue   *value,
                                                         GParamSpec     *pspec);
static void             gimp_canvas_corner_get_property (GObject        *object,
                                                         guint           property_id,
                                                         GValue         *value,
                                                         GParamSpec     *pspec);
static void             gimp_canvas_corner_draw         (GimpCanvasItem *item,
                                                         cairo_t        *cr);
static cairo_region_t * gimp_canvas_corner_get_extents  (GimpCanvasItem *item);


G_DEFINE_TYPE_WITH_PRIVATE (GimpCanvasCorner, gimp_canvas_corner,
                            GIMP_TYPE_CANVAS_ITEM)

#define parent_class gimp_canvas_corner_parent_class


static void
gimp_canvas_corner_class_init (GimpCanvasCornerClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = gimp_canvas_corner_set_property;
  object_class->get_property = gimp_canvas_corner_get_property;

  item_class->draw           = gimp_canvas_corner_draw;
  item_class->get_extents    = gimp_canvas_corner_get_extents;

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

  g_object_class_install_property (object_class, PROP_ANCHOR,
                                   g_param_spec_enum ("anchor", NULL, NULL,
                                                      GIMP_TYPE_HANDLE_ANCHOR,
                                                      GIMP_HANDLE_ANCHOR_CENTER,
                                                      GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_CORNER_WIDTH,
                                   g_param_spec_int ("corner-width", NULL, NULL,
                                                     3, GIMP_MAX_IMAGE_SIZE, 3,
                                                     GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_CORNER_HEIGHT,
                                   g_param_spec_int ("corner-height", NULL, NULL,
                                                     3, GIMP_MAX_IMAGE_SIZE, 3,
                                                     GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_OUTSIDE,
                                   g_param_spec_boolean ("outside", NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));
}

static void
gimp_canvas_corner_init (GimpCanvasCorner *corner)
{
}

static void
gimp_canvas_corner_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpCanvasCornerPrivate *private = GET_PRIVATE (object);

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
    case PROP_ANCHOR:
      private->anchor = g_value_get_enum (value);
      break;
    case PROP_CORNER_WIDTH:
      private->corner_width = g_value_get_int (value);
      break;
    case PROP_CORNER_HEIGHT:
      private->corner_height = g_value_get_int (value);
      break;
   case PROP_OUTSIDE:
      private->outside = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_corner_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpCanvasCornerPrivate *private = GET_PRIVATE (object);

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
    case PROP_ANCHOR:
      g_value_set_enum (value, private->anchor);
      break;
    case PROP_CORNER_WIDTH:
      g_value_set_int (value, private->corner_width);
      break;
    case PROP_CORNER_HEIGHT:
      g_value_set_int (value, private->corner_height);
      break;
    case PROP_OUTSIDE:
      g_value_set_boolean (value, private->outside);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_corner_transform (GimpCanvasItem *item,
                              gdouble        *x,
                              gdouble        *y,
                              gdouble        *w,
                              gdouble        *h)
{
  GimpCanvasCornerPrivate *private = GET_PRIVATE (item);
  gdouble                  rx, ry;
  gdouble                  rw, rh;
  gint                     top_and_bottom_handle_x_offset;
  gint                     left_and_right_handle_y_offset;

  gimp_canvas_item_transform_xy_f (item,
                                   MIN (private->x,
                                        private->x + private->width),
                                   MIN (private->y,
                                        private->y + private->height),
                                   &rx, &ry);
  gimp_canvas_item_transform_xy_f (item,
                                   MAX (private->x,
                                        private->x + private->width),
                                   MAX (private->y,
                                        private->y + private->height),
                                   &rw, &rh);

  rw -= rx;
  rh -= ry;

  rx = floor (rx) + 0.5;
  ry = floor (ry) + 0.5;
  rw = ceil (rw) - 1.0;
  rh = ceil (rh) - 1.0;

  top_and_bottom_handle_x_offset = (rw - private->corner_width)  / 2;
  left_and_right_handle_y_offset = (rh - private->corner_height) / 2;

  *w = private->corner_width;
  *h = private->corner_height;

  switch (private->anchor)
    {
    case GIMP_HANDLE_ANCHOR_CENTER:
      break;

    case GIMP_HANDLE_ANCHOR_NORTH_WEST:
      if (private->outside)
        {
          *x = rx - private->corner_width;
          *y = ry - private->corner_height;
        }
      else
        {
          *x = rx;
          *y = ry;
        }
      break;

    case GIMP_HANDLE_ANCHOR_NORTH_EAST:
      if (private->outside)
        {
          *x = rx + rw;
          *y = ry - private->corner_height;
        }
      else
        {
          *x = rx + rw - private->corner_width;
          *y = ry;
        }
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH_WEST:
      if (private->outside)
        {
          *x = rx - private->corner_width;
          *y = ry + rh;
        }
      else
        {
          *x = rx;
          *y = ry + rh - private->corner_height;
        }
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH_EAST:
      if (private->outside)
        {
          *x = rx + rw;
          *y = ry + rh;
        }
      else
        {
          *x = rx + rw - private->corner_width;
          *y = ry + rh - private->corner_height;
        }
      break;

    case GIMP_HANDLE_ANCHOR_NORTH:
      if (private->outside)
        {
          *x = rx;
          *y = ry - private->corner_height;
          *w = rw;
        }
      else
        {
          *x = rx + top_and_bottom_handle_x_offset;
          *y = ry;
        }
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH:
      if (private->outside)
        {
          *x = rx;
          *y = ry + rh;
          *w = rw;
        }
      else
        {
          *x = rx + top_and_bottom_handle_x_offset;
          *y = ry + rh - private->corner_height;
        }
      break;

    case GIMP_HANDLE_ANCHOR_WEST:
      if (private->outside)
        {
          *x = rx - private->corner_width;
          *y = ry;
          *h = rh;
        }
      else
        {
          *x = rx;
          *y = ry + left_and_right_handle_y_offset;
        }
      break;

    case GIMP_HANDLE_ANCHOR_EAST:
      if (private->outside)
        {
          *x = rx + rw;
          *y = ry;
          *h = rh;
        }
      else
        {
          *x = rx + rw - private->corner_width;
          *y = ry + left_and_right_handle_y_offset;
        }
      break;
    }
}

static void
gimp_canvas_corner_draw (GimpCanvasItem *item,
                         cairo_t        *cr)
{
  gdouble x, y;
  gdouble w, h;

  gimp_canvas_corner_transform (item, &x, &y, &w, &h);

  cairo_rectangle (cr, x, y, w, h);

  _gimp_canvas_item_stroke (item, cr);
}

static cairo_region_t *
gimp_canvas_corner_get_extents (GimpCanvasItem *item)
{
  cairo_rectangle_int_t rectangle;
  gdouble               x, y;
  gdouble               w, h;

  gimp_canvas_corner_transform (item, &x, &y, &w, &h);

  rectangle.x      = floor (x - 1.5);
  rectangle.y      = floor (y - 1.5);
  rectangle.width  = ceil (w + 3.0);
  rectangle.height = ceil (h + 3.0);

  return cairo_region_create_rectangle (&rectangle);
}

GimpCanvasItem *
gimp_canvas_corner_new (GimpDisplayShell *shell,
                        gdouble           x,
                        gdouble           y,
                        gdouble           width,
                        gdouble           height,
                        GimpHandleAnchor  anchor,
                        gint              corner_width,
                        gint              corner_height,
                        gboolean          outside)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_CANVAS_CORNER,
                       "shell",         shell,
                       "x",             x,
                       "y",             y,
                       "width",         width,
                       "height",        height,
                       "anchor",        anchor,
                       "corner-width",  corner_width,
                       "corner-height", corner_height,
                       "outside",       outside,
                       NULL);
}

void
gimp_canvas_corner_set (GimpCanvasItem *corner,
                        gdouble         x,
                        gdouble         y,
                        gdouble         width,
                        gdouble         height,
                        gint            corner_width,
                        gint            corner_height,
                        gboolean        outside)
{
  g_return_if_fail (GIMP_IS_CANVAS_CORNER (corner));

  gimp_canvas_item_begin_change (corner);
  g_object_set (corner,
                "x",             x,
                "y",             y,
                "width",         width,
                "height",        height,
                "corner-width",  corner_width,
                "corner-height", corner_height,
                "outside",       outside,
                NULL);
  gimp_canvas_item_end_change (corner);
}
