/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvaspen.c
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
#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "core/gimpcontext.h"
#include "core/gimpparamspecs.h"

#include "gimpcanvas-style.h"
#include "gimpcanvaspen.h"
#include "gimpdisplayshell.h"


enum
{
  PROP_0,
  PROP_COLOR,
  PROP_WIDTH
};


typedef struct _GimpCanvasPenPrivate GimpCanvasPenPrivate;

struct _GimpCanvasPenPrivate
{
  GimpRGB color;
  gint    width;
};

#define GET_PRIVATE(pen) \
        ((GimpCanvasPenPrivate *) gimp_canvas_pen_get_instance_private ((GimpCanvasPen *) (pen)))


/*  local function prototypes  */

static void             gimp_canvas_pen_set_property (GObject        *object,
                                                      guint           property_id,
                                                      const GValue   *value,
                                                      GParamSpec     *pspec);
static void             gimp_canvas_pen_get_property (GObject        *object,
                                                      guint           property_id,
                                                      GValue         *value,
                                                      GParamSpec     *pspec);
static cairo_region_t * gimp_canvas_pen_get_extents  (GimpCanvasItem *item);
static void             gimp_canvas_pen_stroke       (GimpCanvasItem *item,
                                                      cairo_t        *cr);


G_DEFINE_TYPE_WITH_PRIVATE (GimpCanvasPen, gimp_canvas_pen,
                            GIMP_TYPE_CANVAS_POLYGON)

#define parent_class gimp_canvas_pen_parent_class


static void
gimp_canvas_pen_class_init (GimpCanvasPenClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = gimp_canvas_pen_set_property;
  object_class->get_property = gimp_canvas_pen_get_property;

  item_class->get_extents    = gimp_canvas_pen_get_extents;
  item_class->stroke         = gimp_canvas_pen_stroke;

  g_object_class_install_property (object_class, PROP_COLOR,
                                   gimp_param_spec_rgb ("color", NULL, NULL,
                                                        FALSE, NULL,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_WIDTH,
                                   g_param_spec_int ("width", NULL, NULL,
                                                     1, G_MAXINT, 1,
                                                     GIMP_PARAM_READWRITE));
}

static void
gimp_canvas_pen_init (GimpCanvasPen *pen)
{
}

static void
gimp_canvas_pen_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpCanvasPenPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_COLOR:
      gimp_value_get_rgb (value, &private->color);
      break;
    case PROP_WIDTH:
      private->width = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_pen_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpCanvasPenPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_COLOR:
      gimp_value_set_rgb (value, &private->color);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, private->width);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static cairo_region_t *
gimp_canvas_pen_get_extents (GimpCanvasItem *item)
{
  GimpCanvasPenPrivate *private = GET_PRIVATE (item);
  cairo_region_t       *region;

  region = GIMP_CANVAS_ITEM_CLASS (parent_class)->get_extents (item);

  if (region)
    {
      cairo_rectangle_int_t rectangle;

      cairo_region_get_extents (region, &rectangle);

      rectangle.x      -= ceil (private->width / 2.0);
      rectangle.y      -= ceil (private->width / 2.0);
      rectangle.width  += private->width + 1;
      rectangle.height += private->width + 1;

      cairo_region_union_rectangle (region, &rectangle);
    }

  return region;
}

static void
gimp_canvas_pen_stroke (GimpCanvasItem *item,
                        cairo_t        *cr)
{
  GimpCanvasPenPrivate *private = GET_PRIVATE (item);

  gimp_canvas_set_pen_style (gimp_canvas_item_get_canvas (item), cr,
                             &private->color, private->width);
  cairo_stroke (cr);
}

GimpCanvasItem *
gimp_canvas_pen_new (GimpDisplayShell  *shell,
                     const GimpVector2 *points,
                     gint               n_points,
                     GimpContext       *context,
                     GimpActiveColor    color,
                     gint               width)
{
  GimpCanvasItem *item;
  GimpArray      *array;
  GimpRGB         rgb;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (points != NULL && n_points > 1, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  array = gimp_array_new ((const guint8 *) points,
                          n_points * sizeof (GimpVector2), TRUE);

  switch (color)
    {
    case GIMP_ACTIVE_COLOR_FOREGROUND:
      gimp_context_get_foreground (context, &rgb);
      break;

    case GIMP_ACTIVE_COLOR_BACKGROUND:
      gimp_context_get_background (context, &rgb);
      break;
    }

  item = g_object_new (GIMP_TYPE_CANVAS_PEN,
                       "shell",  shell,
                       "points", array,
                       "color",  &rgb,
                       "width",  width,
                       NULL);

  gimp_array_free (array);

  return item;
}
