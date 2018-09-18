/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvastextcursor.c
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

#include "gimpcanvastextcursor.h"
#include "gimpdisplayshell.h"


enum
{
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_OVERWRITE,
  PROP_DIRECTION
};


typedef struct _GimpCanvasTextCursorPrivate GimpCanvasTextCursorPrivate;

struct _GimpCanvasTextCursorPrivate
{
  gint              x;
  gint              y;
  gint              width;
  gint              height;
  gboolean          overwrite;
  GimpTextDirection direction;
};

#define GET_PRIVATE(text_cursor) \
        ((GimpCanvasTextCursorPrivate *) gimp_canvas_text_cursor_get_instance_private ((GimpCanvasTextCursor *) (text_cursor)))


/*  local function prototypes  */

static void             gimp_canvas_text_cursor_set_property (GObject        *object,
                                                              guint           property_id,
                                                              const GValue   *value,
                                                              GParamSpec     *pspec);
static void             gimp_canvas_text_cursor_get_property (GObject        *object,
                                                              guint           property_id,
                                                              GValue         *value,
                                                              GParamSpec     *pspec);
static void             gimp_canvas_text_cursor_draw         (GimpCanvasItem *item,
                                                              cairo_t        *cr);
static cairo_region_t * gimp_canvas_text_cursor_get_extents  (GimpCanvasItem *item);


G_DEFINE_TYPE_WITH_PRIVATE (GimpCanvasTextCursor, gimp_canvas_text_cursor,
                            GIMP_TYPE_CANVAS_ITEM)

#define parent_class gimp_canvas_text_cursor_parent_class


static void
gimp_canvas_text_cursor_class_init (GimpCanvasTextCursorClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = gimp_canvas_text_cursor_set_property;
  object_class->get_property = gimp_canvas_text_cursor_get_property;

  item_class->draw           = gimp_canvas_text_cursor_draw;
  item_class->get_extents    = gimp_canvas_text_cursor_get_extents;

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

  g_object_class_install_property (object_class, PROP_WIDTH,
                                   g_param_spec_int ("width", NULL, NULL,
                                                     -GIMP_MAX_IMAGE_SIZE,
                                                     GIMP_MAX_IMAGE_SIZE, 0,
                                                     GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_HEIGHT,
                                   g_param_spec_int ("height", NULL, NULL,
                                                     -GIMP_MAX_IMAGE_SIZE,
                                                     GIMP_MAX_IMAGE_SIZE, 0,
                                                     GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_OVERWRITE,
                                   g_param_spec_boolean ("overwrite", NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_DIRECTION,
                                   g_param_spec_enum ("direction", NULL, NULL,
                                                     gimp_text_direction_get_type(),
                                                     GIMP_TEXT_DIRECTION_LTR,
                                                     GIMP_PARAM_READWRITE));
}

static void
gimp_canvas_text_cursor_init (GimpCanvasTextCursor *text_cursor)
{
}

static void
gimp_canvas_text_cursor_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  GimpCanvasTextCursorPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_X:
      private->x = g_value_get_int (value);
      break;
    case PROP_Y:
      private->y = g_value_get_int (value);
      break;
    case PROP_WIDTH:
      private->width = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      private->height = g_value_get_int (value);
      break;
    case PROP_OVERWRITE:
      private->overwrite = g_value_get_boolean (value);
      break;
    case PROP_DIRECTION:
      private->direction = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_text_cursor_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  GimpCanvasTextCursorPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_X:
      g_value_set_int (value, private->x);
      break;
    case PROP_Y:
      g_value_set_int (value, private->y);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, private->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, private->height);
      break;
    case PROP_OVERWRITE:
      g_value_set_boolean (value, private->overwrite);
      break;
    case PROP_DIRECTION:
      g_value_set_enum (value, private->direction);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_text_cursor_transform (GimpCanvasItem *item,
                                   gdouble        *x,
                                   gdouble        *y,
                                   gdouble        *w,
                                   gdouble        *h)
{
  GimpCanvasTextCursorPrivate *private = GET_PRIVATE (item);

  gimp_canvas_item_transform_xy_f (item,
                                   MIN (private->x,
                                        private->x + private->width),
                                   MIN (private->y,
                                        private->y + private->height),
                                   x, y);
  gimp_canvas_item_transform_xy_f (item,
                                   MAX (private->x,
                                        private->x + private->width),
                                   MAX (private->y,
                                        private->y + private->height),
                                   w, h);

  *w -= *x;
  *h -= *y;

  *x = floor (*x) + 0.5;
  *y = floor (*y) + 0.5;
  switch (private->direction)
    {
    case GIMP_TEXT_DIRECTION_LTR:
    case GIMP_TEXT_DIRECTION_RTL:
      break;
    case GIMP_TEXT_DIRECTION_TTB_RTL:
    case GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
      *x = *x - *w;
      break;
    case GIMP_TEXT_DIRECTION_TTB_LTR:
    case GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
      *y = *y + *h;
      break;
    }

  if (private->overwrite)
    {
      *w = ceil (*w) - 1.0;
      *h = ceil (*h) - 1.0;
    }
  else
    {
      switch (private->direction)
        {
        case GIMP_TEXT_DIRECTION_LTR:
        case GIMP_TEXT_DIRECTION_RTL:
          *w = 0;
          *h = ceil (*h) - 1.0;
           break;
        case GIMP_TEXT_DIRECTION_TTB_RTL:
        case GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
          *w = ceil (*w) - 1.0;
          *h = 0;
          break;
        case GIMP_TEXT_DIRECTION_TTB_LTR:
        case GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
          *w = ceil (*w) - 1.0;
          *h = 0;
          break;
        }
    }
}

static void
gimp_canvas_text_cursor_draw (GimpCanvasItem *item,
                              cairo_t        *cr)
{
  GimpCanvasTextCursorPrivate *private = GET_PRIVATE (item);
  gdouble                      x, y;
  gdouble                      w, h;

  gimp_canvas_text_cursor_transform (item, &x, &y, &w, &h);

  if (private->overwrite)
    {
      cairo_rectangle (cr, x, y, w, h);
    }
  else
    {
      switch (private->direction)
        {
        case GIMP_TEXT_DIRECTION_LTR:
        case GIMP_TEXT_DIRECTION_RTL:
          cairo_move_to (cr, x, y);
          cairo_line_to (cr, x, y + h);

          cairo_move_to (cr, x - 3.0, y);
          cairo_line_to (cr, x + 3.0, y);

          cairo_move_to (cr, x - 3.0, y + h);
          cairo_line_to (cr, x + 3.0, y + h);
          break;
        case GIMP_TEXT_DIRECTION_TTB_RTL:
        case GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
        case GIMP_TEXT_DIRECTION_TTB_LTR:
        case GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
          cairo_move_to (cr, x, y);
          cairo_line_to (cr, x + w, y);

          cairo_move_to (cr, x, y - 3.0);
          cairo_line_to (cr, x, y + 3.0);

          cairo_move_to (cr, x + w, y - 3.0);
          cairo_line_to (cr, x + w, y + 3.0);
          break;
        }
    }

  _gimp_canvas_item_stroke (item, cr);
}

static cairo_region_t *
gimp_canvas_text_cursor_get_extents (GimpCanvasItem *item)
{
  GimpCanvasTextCursorPrivate *private = GET_PRIVATE (item);
  cairo_rectangle_int_t        rectangle;
  gdouble                      x, y;
  gdouble                      w, h;

  gimp_canvas_text_cursor_transform (item, &x, &y, &w, &h);

  if (private->overwrite)
    {
      rectangle.x      = floor (x - 1.5);
      rectangle.y      = floor (y - 1.5);
      rectangle.width  = ceil (w + 3.0);
      rectangle.height = ceil (h + 3.0);
    }
  else
    {
      switch (private->direction)
        {
        case GIMP_TEXT_DIRECTION_LTR:
        case GIMP_TEXT_DIRECTION_RTL:
          rectangle.x      = floor (x - 4.5);
          rectangle.y      = floor (y - 1.5);
          rectangle.width  = ceil (9.0);
          rectangle.height = ceil (h + 3.0);
          break;
        case GIMP_TEXT_DIRECTION_TTB_RTL:
        case GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
        case GIMP_TEXT_DIRECTION_TTB_LTR:
        case GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
          rectangle.x      = floor (x - 1.5);
          rectangle.y      = floor (y - 4.5);
          rectangle.width  = ceil (w + 3.0);
          rectangle.height = ceil (9.0);
          break;
        }
    }

  return cairo_region_create_rectangle (&rectangle);
}

GimpCanvasItem *
gimp_canvas_text_cursor_new (GimpDisplayShell *shell,
                             PangoRectangle   *cursor,
                             gboolean          overwrite,
                             GimpTextDirection direction)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (cursor != NULL, NULL);

  return g_object_new (GIMP_TYPE_CANVAS_TEXT_CURSOR,
                       "shell",     shell,
                       "x",         cursor->x,
                       "y",         cursor->y,
                       "width",     cursor->width,
                       "height",    cursor->height,
                       "overwrite", overwrite,
                       "direction", direction,
                       NULL);
}
