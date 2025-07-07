/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvastext.c
 * Copyright (C) 2023 mr.fantastic <mrfantastic@firemail.cc>
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

#include "gimpcanvastext.h"
#include "gimpdisplayshell.h"


enum
{
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_FONT_SIZE,
  PROP_TEXT
};

typedef struct _GimpCanvasTextPrivate GimpCanvasTextPrivate;

struct _GimpCanvasTextPrivate
{
  gdouble  x;
  gdouble  y;
  gdouble  font_size;
  gchar    *text;
};

#define GET_PRIVATE(text_item) \
  ((GimpCanvasTextPrivate *) gimp_canvas_text_get_instance_private ((GimpCanvasText *) (text_item)))


/*  local function prototypes  */

static void             gimp_canvas_text_finalize     (GObject        *object);
static void             gimp_canvas_text_set_property (GObject        *object,
                                                       guint           property_id,
                                                       const GValue   *value,
                                                       GParamSpec     *pspec);
static void             gimp_canvas_text_get_property (GObject        *object,
                                                       guint           property_id,
                                                       GValue         *value,
                                                       GParamSpec     *pspec);
static void             gimp_canvas_text_draw         (GimpCanvasItem *item,
                                                       cairo_t        *cr);
static cairo_region_t * gimp_canvas_text_get_extents  (GimpCanvasItem *item);


G_DEFINE_TYPE_WITH_PRIVATE (GimpCanvasText, gimp_canvas_text,
                            GIMP_TYPE_CANVAS_ITEM)

#define parent_class gimp_canvas_text_parent_class


static void
gimp_canvas_text_class_init (GimpCanvasTextClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->finalize     = gimp_canvas_text_finalize;
  object_class->set_property = gimp_canvas_text_set_property;
  object_class->get_property = gimp_canvas_text_get_property;

  item_class->draw           = gimp_canvas_text_draw;
  item_class->get_extents    = gimp_canvas_text_get_extents;

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

  g_object_class_install_property (object_class, PROP_FONT_SIZE,
                                   g_param_spec_double ("font-size", NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE, 0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_TEXT,
                                   g_param_spec_string ("text", NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_canvas_text_init (GimpCanvasText *text)
{
}

static void
gimp_canvas_text_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpCanvasTextPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_X:
      private->x = g_value_get_double (value);
      break;
    case PROP_Y:
      private->y = g_value_get_double (value);
      break;
    case PROP_FONT_SIZE:
      private->font_size = g_value_get_double (value);
      break;
    case PROP_TEXT:
      private->text = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_text_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpCanvasTextPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_X:
      g_value_set_double (value, private->x);
      break;
    case PROP_Y:
      g_value_set_double (value, private->y);
      break;
    case PROP_FONT_SIZE:
      g_value_set_double (value, private->font_size);
      break;
    case PROP_TEXT:
      g_value_set_string (value, private->text);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_text_transform (GimpCanvasItem *item,
                            gdouble        *x1,
                            gdouble        *y1,
                            gdouble        *x2,
                            gdouble        *y2)
{
  GimpCanvasTextPrivate *private = GET_PRIVATE (item);
  cairo_text_extents_t   extents;
  cairo_surface_t       *dummy_surface;
  cairo_t               *cr;

  gimp_canvas_item_transform_xy_f (item,
                                   private->x, private->y,
                                   x1, y1);

  dummy_surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 0, 0);
  cr = cairo_create (dummy_surface);

  cairo_set_font_size (cr, private->font_size);
  cairo_text_extents (cr, private->text, &extents);

  cairo_surface_destroy (dummy_surface);
  cairo_destroy (cr);

  *x1 = floor (*x1) + 0.5;
  *y1 = floor (*y1) + 0.5;
  *x2 = extents.width;
  *y2 = extents.height;
}

static void
gimp_canvas_text_draw (GimpCanvasItem *item,
                       cairo_t        *cr)
{
  gdouble x, y, ignore_x2, ignore_y2;

  GimpCanvasTextPrivate *private = GET_PRIVATE (item);

  gimp_canvas_text_transform (item, &x, &y, &ignore_x2, &ignore_y2);

  cairo_set_font_size (cr, private->font_size);
  cairo_move_to (cr, x,y);
  cairo_text_path (cr, private->text);

  _gimp_canvas_item_fill (item, cr);
}

static cairo_region_t *
gimp_canvas_text_get_extents (GimpCanvasItem *item)
{
  cairo_rectangle_int_t rectangle;
  gdouble               x1, y1, x2, y2;

  gimp_canvas_text_transform (item, &x1, &y1, &x2, &y2);

  rectangle.x      = (gint) x1-1;
  rectangle.y      = (gint) (y1-y2-1);
  rectangle.width  = (gint) x2+3;
  rectangle.height = (gint) y2+3;

  return cairo_region_create_rectangle (&rectangle);
}

GimpCanvasItem *
gimp_canvas_text_new (GimpDisplayShell *shell,
                      gdouble           x,
                      gdouble           y,
                      gdouble           font_size,
                      gchar            *text)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_CANVAS_TEXT,
                       "shell",     shell,
                       "x",         x,
                       "y",         y,
                       "font-size", font_size,
                       "text",      text,
                       NULL);
}

static void
gimp_canvas_text_finalize (GObject *object)
{
  GimpCanvasTextPrivate *private = GET_PRIVATE (object);

  g_free (private->text);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}
