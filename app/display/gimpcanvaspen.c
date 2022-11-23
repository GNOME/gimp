/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvaspen.c
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
#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"

#include "display-types.h"

#include "core/ligmacontext.h"
#include "core/ligmaparamspecs.h"

#include "ligmacanvas-style.h"
#include "ligmacanvaspen.h"
#include "ligmadisplayshell.h"


enum
{
  PROP_0,
  PROP_COLOR,
  PROP_WIDTH
};


typedef struct _LigmaCanvasPenPrivate LigmaCanvasPenPrivate;

struct _LigmaCanvasPenPrivate
{
  LigmaRGB color;
  gint    width;
};

#define GET_PRIVATE(pen) \
        ((LigmaCanvasPenPrivate *) ligma_canvas_pen_get_instance_private ((LigmaCanvasPen *) (pen)))


/*  local function prototypes  */

static void             ligma_canvas_pen_set_property (GObject        *object,
                                                      guint           property_id,
                                                      const GValue   *value,
                                                      GParamSpec     *pspec);
static void             ligma_canvas_pen_get_property (GObject        *object,
                                                      guint           property_id,
                                                      GValue         *value,
                                                      GParamSpec     *pspec);
static cairo_region_t * ligma_canvas_pen_get_extents  (LigmaCanvasItem *item);
static void             ligma_canvas_pen_stroke       (LigmaCanvasItem *item,
                                                      cairo_t        *cr);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaCanvasPen, ligma_canvas_pen,
                            LIGMA_TYPE_CANVAS_POLYGON)

#define parent_class ligma_canvas_pen_parent_class


static void
ligma_canvas_pen_class_init (LigmaCanvasPenClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaCanvasItemClass *item_class   = LIGMA_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = ligma_canvas_pen_set_property;
  object_class->get_property = ligma_canvas_pen_get_property;

  item_class->get_extents    = ligma_canvas_pen_get_extents;
  item_class->stroke         = ligma_canvas_pen_stroke;

  g_object_class_install_property (object_class, PROP_COLOR,
                                   ligma_param_spec_rgb ("color", NULL, NULL,
                                                        FALSE, NULL,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_WIDTH,
                                   g_param_spec_int ("width", NULL, NULL,
                                                     1, G_MAXINT, 1,
                                                     LIGMA_PARAM_READWRITE));
}

static void
ligma_canvas_pen_init (LigmaCanvasPen *pen)
{
}

static void
ligma_canvas_pen_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  LigmaCanvasPenPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_COLOR:
      ligma_value_get_rgb (value, &private->color);
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
ligma_canvas_pen_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  LigmaCanvasPenPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_COLOR:
      ligma_value_set_rgb (value, &private->color);
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
ligma_canvas_pen_get_extents (LigmaCanvasItem *item)
{
  LigmaCanvasPenPrivate *private = GET_PRIVATE (item);
  cairo_region_t       *region;

  region = LIGMA_CANVAS_ITEM_CLASS (parent_class)->get_extents (item);

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
ligma_canvas_pen_stroke (LigmaCanvasItem *item,
                        cairo_t        *cr)
{
  LigmaCanvasPenPrivate *private = GET_PRIVATE (item);

  ligma_canvas_set_pen_style (ligma_canvas_item_get_canvas (item), cr,
                             &private->color, private->width);
  cairo_stroke (cr);
}

LigmaCanvasItem *
ligma_canvas_pen_new (LigmaDisplayShell  *shell,
                     const LigmaVector2 *points,
                     gint               n_points,
                     LigmaContext       *context,
                     LigmaActiveColor    color,
                     gint               width)
{
  LigmaCanvasItem *item;
  LigmaArray      *array;
  LigmaRGB         rgb;

  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (points != NULL && n_points > 1, NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  array = ligma_array_new ((const guint8 *) points,
                          n_points * sizeof (LigmaVector2), TRUE);

  switch (color)
    {
    case LIGMA_ACTIVE_COLOR_FOREGROUND:
      ligma_context_get_foreground (context, &rgb);
      break;

    case LIGMA_ACTIVE_COLOR_BACKGROUND:
      ligma_context_get_background (context, &rgb);
      break;
    }

  item = g_object_new (LIGMA_TYPE_CANVAS_PEN,
                       "shell",  shell,
                       "points", array,
                       "color",  &rgb,
                       "width",  width,
                       NULL);

  ligma_array_free (array);

  return item;
}
