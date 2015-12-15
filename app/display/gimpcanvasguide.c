/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasguide.c
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

#include "gimpcanvas-style.h"
#include "gimpcanvasguide.h"
#include "gimpdisplayshell.h"


enum
{
  PROP_0,
  PROP_ORIENTATION,
  PROP_POSITION,
  PROP_NORMAL_STYLE,
  PROP_ACTIVE_STYLE,
  PROP_LINE_WIDTH
};


typedef struct _GimpCanvasGuidePrivate GimpCanvasGuidePrivate;

struct _GimpCanvasGuidePrivate
{
  GimpOrientationType  orientation;
  gint                 position;

  cairo_pattern_t     *active_style;
  cairo_pattern_t     *normal_style;
  gdouble              line_width;
};

#define GET_PRIVATE(guide) \
        G_TYPE_INSTANCE_GET_PRIVATE (guide, \
                                     GIMP_TYPE_CANVAS_GUIDE, \
                                     GimpCanvasGuidePrivate)


/*  local function prototypes  */

static void             gimp_canvas_guide_finalize     (GObject        *object);
static void             gimp_canvas_guide_set_property (GObject        *object,
                                                        guint           property_id,
                                                        const GValue   *value,
                                                        GParamSpec     *pspec);
static void             gimp_canvas_guide_get_property (GObject        *object,
                                                        guint           property_id,
                                                        GValue         *value,
                                                        GParamSpec     *pspec);
static void             gimp_canvas_guide_draw         (GimpCanvasItem *item,
                                                        cairo_t        *cr);
static cairo_region_t * gimp_canvas_guide_get_extents  (GimpCanvasItem *item);
static void             gimp_canvas_guide_stroke       (GimpCanvasItem *item,
                                                        cairo_t        *cr);


G_DEFINE_TYPE (GimpCanvasGuide, gimp_canvas_guide, GIMP_TYPE_CANVAS_ITEM)

#define parent_class gimp_canvas_guide_parent_class


static void
gimp_canvas_guide_class_init (GimpCanvasGuideClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->finalize     = gimp_canvas_guide_finalize;
  object_class->set_property = gimp_canvas_guide_set_property;
  object_class->get_property = gimp_canvas_guide_get_property;

  item_class->draw           = gimp_canvas_guide_draw;
  item_class->get_extents    = gimp_canvas_guide_get_extents;
  item_class->stroke         = gimp_canvas_guide_stroke;

  g_object_class_install_property (object_class, PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation", NULL, NULL,
                                                      GIMP_TYPE_ORIENTATION_TYPE,
                                                      GIMP_ORIENTATION_HORIZONTAL,
                                                      GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_POSITION,
                                   g_param_spec_int ("position", NULL, NULL,
                                                     -GIMP_MAX_IMAGE_SIZE,
                                                     GIMP_MAX_IMAGE_SIZE, 0,
                                                     GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_NORMAL_STYLE,
                                   g_param_spec_pointer ("normal-style", NULL, NULL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_ACTIVE_STYLE,
                                   g_param_spec_pointer ("active-style", NULL, NULL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_LINE_WIDTH,
                                   g_param_spec_double ("line-width", NULL, NULL,
                                                        0, GIMP_MAX_IMAGE_SIZE,
                                                        1.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (GimpCanvasGuidePrivate));
}

static void
gimp_canvas_guide_init (GimpCanvasGuide *guide)
{
}

static void
gimp_canvas_guide_finalize (GObject *object)
{
  GimpCanvasGuidePrivate *private = GET_PRIVATE (object);

  cairo_pattern_destroy (private->normal_style);
  cairo_pattern_destroy (private->active_style);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_canvas_guide_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpCanvasGuidePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ORIENTATION:
      private->orientation = g_value_get_enum (value);
      break;
    case PROP_POSITION:
      private->position = g_value_get_int (value);
      break;
    case PROP_NORMAL_STYLE:
      if (private->normal_style)
        cairo_pattern_destroy (private->normal_style);
      private->normal_style = g_value_get_pointer (value);
      break;
    case PROP_ACTIVE_STYLE:
      if (private->active_style)
        cairo_pattern_destroy (private->active_style);
      private->active_style = g_value_get_pointer (value);
      break;
    case PROP_LINE_WIDTH:
      private->line_width = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_guide_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpCanvasGuidePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, private->orientation);
      break;
    case PROP_POSITION:
      g_value_set_int (value, private->position);
      break;
    case PROP_NORMAL_STYLE:
      g_value_set_pointer (value, private->normal_style);
      break;
    case PROP_ACTIVE_STYLE:
      g_value_set_pointer (value, private->active_style);
      break;
    case PROP_LINE_WIDTH:
      g_value_set_double (value, private->line_width);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_guide_transform (GimpCanvasItem *item,
                             gdouble        *x1,
                             gdouble        *y1,
                             gdouble        *x2,
                             gdouble        *y2)
{
  GimpCanvasGuidePrivate *private = GET_PRIVATE (item);
  GtkWidget              *canvas  = gimp_canvas_item_get_canvas (item);
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
    case GIMP_ORIENTATION_HORIZONTAL:
      gimp_canvas_item_transform_xy (item, 0, private->position, &x, &y);
      *y1 = *y2 = y + 0.5;
      break;

    case GIMP_ORIENTATION_VERTICAL:
      gimp_canvas_item_transform_xy (item, private->position, 0, &x, &y);
      *x1 = *x2 = x + 0.5;
      break;

    case GIMP_ORIENTATION_UNKNOWN:
      return;
    }
}

static void
gimp_canvas_guide_draw (GimpCanvasItem *item,
                        cairo_t        *cr)
{
  gdouble x1, y1;
  gdouble x2, y2;

  gimp_canvas_guide_transform (item, &x1, &y1, &x2, &y2);

  cairo_move_to (cr, x1, y1);
  cairo_line_to (cr, x2, y2);

  _gimp_canvas_item_stroke (item, cr);
}

static cairo_region_t *
gimp_canvas_guide_get_extents (GimpCanvasItem *item)
{
  cairo_rectangle_int_t rectangle;
  gdouble               x1, y1;
  gdouble               x2, y2;

  gimp_canvas_guide_transform (item, &x1, &y1, &x2, &y2);

  rectangle.x      = MIN (x1, x2) - 1.5;
  rectangle.y      = MIN (y1, y2) - 1.5;
  rectangle.width  = ABS (x2 - x1) + 3.0;
  rectangle.height = ABS (y2 - y1) + 3.0;

  return cairo_region_create_rectangle (&rectangle);
}

static void
gimp_canvas_guide_stroke (GimpCanvasItem *item,
                          cairo_t        *cr)
{
  GimpCanvasGuidePrivate *private = GET_PRIVATE (item);

  if (private->active_style &&
      gimp_canvas_item_get_highlight (item))
    {
      cairo_set_line_width (cr, private->line_width);
      cairo_set_source (cr, private->active_style);
      cairo_stroke (cr);
    }
  else if (private->normal_style &&
           ! gimp_canvas_item_get_highlight (item))
    {
      cairo_set_line_width (cr, private->line_width);
      cairo_set_source (cr, private->normal_style);
      cairo_stroke (cr);
    }
  else
    {
      GIMP_CANVAS_ITEM_CLASS (parent_class)->stroke (item, cr);
    }
}

GimpCanvasItem *
gimp_canvas_guide_new (GimpDisplayShell    *shell,
                       GimpOrientationType  orientation,
                       gint                 position,
                       cairo_pattern_t     *normal_style,
                       cairo_pattern_t     *active_style,
                       gdouble              line_width)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_CANVAS_GUIDE,
                       "shell",        shell,
                       "orientation",  orientation,
                       "position",     position,
                       "normal-style", normal_style,
                       "active-style", active_style,
                       "line-width",   line_width,
                       NULL);
}

void
gimp_canvas_guide_set (GimpCanvasItem      *guide,
                       GimpOrientationType  orientation,
                       gint                 position)
{
  g_return_if_fail (GIMP_IS_CANVAS_GUIDE (guide));

  gimp_canvas_item_begin_change (guide);

  g_object_set (guide,
                "orientation", orientation,
                "position",    position,
                NULL);

  gimp_canvas_item_end_change (guide);
}
