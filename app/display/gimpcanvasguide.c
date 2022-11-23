/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvasguide.c
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
#include "libligmamath/ligmamath.h"

#include "display-types.h"

#include "ligmacanvas-style.h"
#include "ligmacanvasguide.h"
#include "ligmadisplayshell.h"


enum
{
  PROP_0,
  PROP_ORIENTATION,
  PROP_POSITION,
  PROP_STYLE
};


typedef struct _LigmaCanvasGuidePrivate LigmaCanvasGuidePrivate;

struct _LigmaCanvasGuidePrivate
{
  LigmaOrientationType  orientation;
  gint                 position;

  LigmaGuideStyle       style;
};

#define GET_PRIVATE(guide) \
        ((LigmaCanvasGuidePrivate *) ligma_canvas_guide_get_instance_private ((LigmaCanvasGuide *) (guide)))


/*  local function prototypes  */

static void             ligma_canvas_guide_set_property (GObject        *object,
                                                        guint           property_id,
                                                        const GValue   *value,
                                                        GParamSpec     *pspec);
static void             ligma_canvas_guide_get_property (GObject        *object,
                                                        guint           property_id,
                                                        GValue         *value,
                                                        GParamSpec     *pspec);
static void             ligma_canvas_guide_draw         (LigmaCanvasItem *item,
                                                        cairo_t        *cr);
static cairo_region_t * ligma_canvas_guide_get_extents  (LigmaCanvasItem *item);
static void             ligma_canvas_guide_stroke       (LigmaCanvasItem *item,
                                                        cairo_t        *cr);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaCanvasGuide, ligma_canvas_guide,
                            LIGMA_TYPE_CANVAS_ITEM)

#define parent_class ligma_canvas_guide_parent_class


static void
ligma_canvas_guide_class_init (LigmaCanvasGuideClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaCanvasItemClass *item_class   = LIGMA_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = ligma_canvas_guide_set_property;
  object_class->get_property = ligma_canvas_guide_get_property;

  item_class->draw           = ligma_canvas_guide_draw;
  item_class->get_extents    = ligma_canvas_guide_get_extents;
  item_class->stroke         = ligma_canvas_guide_stroke;

  g_object_class_install_property (object_class, PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation", NULL, NULL,
                                                      LIGMA_TYPE_ORIENTATION_TYPE,
                                                      LIGMA_ORIENTATION_HORIZONTAL,
                                                      LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_POSITION,
                                   g_param_spec_int ("position", NULL, NULL,
                                                     -LIGMA_MAX_IMAGE_SIZE,
                                                     LIGMA_MAX_IMAGE_SIZE, 0,
                                                     LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_STYLE,
                                   g_param_spec_enum ("style", NULL, NULL,
                                                      LIGMA_TYPE_GUIDE_STYLE,
                                                      LIGMA_GUIDE_STYLE_NONE,
                                                      LIGMA_PARAM_READWRITE));
}

static void
ligma_canvas_guide_init (LigmaCanvasGuide *guide)
{
}

static void
ligma_canvas_guide_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaCanvasGuidePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ORIENTATION:
      private->orientation = g_value_get_enum (value);
      break;
    case PROP_POSITION:
      private->position = g_value_get_int (value);
      break;
    case PROP_STYLE:
      private->style = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_canvas_guide_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaCanvasGuidePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, private->orientation);
      break;
    case PROP_POSITION:
      g_value_set_int (value, private->position);
      break;
    case PROP_STYLE:
      g_value_set_enum (value, private->style);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_canvas_guide_transform (LigmaCanvasItem *item,
                             gdouble        *x1,
                             gdouble        *y1,
                             gdouble        *x2,
                             gdouble        *y2)
{
  LigmaCanvasGuidePrivate *private = GET_PRIVATE (item);
  GtkWidget              *canvas  = ligma_canvas_item_get_canvas (item);
  GtkAllocation           allocation;
  gint                    max_outside;
  gint                    x, y;

  gtk_widget_get_allocation (canvas, &allocation);

  max_outside = allocation.width + allocation.height;

  *x1 = -max_outside;
  *y1 = -max_outside;
  *x2 = allocation.width  + max_outside;
  *y2 = allocation.height + max_outside;

  switch (private->orientation)
    {
    case LIGMA_ORIENTATION_HORIZONTAL:
      ligma_canvas_item_transform_xy (item, 0, private->position, &x, &y);
      *y1 = *y2 = y + 0.5;
      break;

    case LIGMA_ORIENTATION_VERTICAL:
      ligma_canvas_item_transform_xy (item, private->position, 0, &x, &y);
      *x1 = *x2 = x + 0.5;
      break;

    case LIGMA_ORIENTATION_UNKNOWN:
      return;
    }
}

static void
ligma_canvas_guide_draw (LigmaCanvasItem *item,
                        cairo_t        *cr)
{
  gdouble x1, y1;
  gdouble x2, y2;

  ligma_canvas_guide_transform (item, &x1, &y1, &x2, &y2);

  cairo_move_to (cr, x1, y1);
  cairo_line_to (cr, x2, y2);

  _ligma_canvas_item_stroke (item, cr);
}

static cairo_region_t *
ligma_canvas_guide_get_extents (LigmaCanvasItem *item)
{
  cairo_rectangle_int_t rectangle;
  gdouble               x1, y1;
  gdouble               x2, y2;

  ligma_canvas_guide_transform (item, &x1, &y1, &x2, &y2);

  rectangle.x      = MIN (x1, x2) - 1.5;
  rectangle.y      = MIN (y1, y2) - 1.5;
  rectangle.width  = ABS (x2 - x1) + 3.0;
  rectangle.height = ABS (y2 - y1) + 3.0;

  return cairo_region_create_rectangle (&rectangle);
}

static void
ligma_canvas_guide_stroke (LigmaCanvasItem *item,
                          cairo_t        *cr)
{
  LigmaCanvasGuidePrivate *private = GET_PRIVATE (item);

  if (private->style != LIGMA_GUIDE_STYLE_NONE)
    {
      LigmaDisplayShell *shell = ligma_canvas_item_get_shell (item);

      ligma_canvas_set_guide_style (ligma_canvas_item_get_canvas (item), cr,
                                   private->style,
                                   ligma_canvas_item_get_highlight (item),
                                   shell->offset_x, shell->offset_y);
      cairo_stroke (cr);
    }
  else
    {
      LIGMA_CANVAS_ITEM_CLASS (parent_class)->stroke (item, cr);
    }
}

LigmaCanvasItem *
ligma_canvas_guide_new (LigmaDisplayShell    *shell,
                       LigmaOrientationType  orientation,
                       gint                 position,
                       LigmaGuideStyle       style)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (LIGMA_TYPE_CANVAS_GUIDE,
                       "shell",        shell,
                       "orientation",  orientation,
                       "position",     position,
                       "style",        style,
                       NULL);
}

void
ligma_canvas_guide_set (LigmaCanvasItem      *guide,
                       LigmaOrientationType  orientation,
                       gint                 position)
{
  g_return_if_fail (LIGMA_IS_CANVAS_GUIDE (guide));

  ligma_canvas_item_begin_change (guide);

  g_object_set (guide,
                "orientation", orientation,
                "position",    position,
                NULL);

  ligma_canvas_item_end_change (guide);
}
