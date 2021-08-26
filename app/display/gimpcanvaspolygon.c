/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvaspolygon.c
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

#include "core/gimp-transform-utils.h"
#include "core/gimpparamspecs.h"

#include "gimpcanvaspolygon.h"
#include "gimpdisplayshell.h"


enum
{
  PROP_0,
  PROP_POINTS,
  PROP_TRANSFORM,
  PROP_FILLED
};


typedef struct _GimpCanvasPolygonPrivate GimpCanvasPolygonPrivate;

struct _GimpCanvasPolygonPrivate
{
  GimpVector2  *points;
  gint          n_points;
  GimpMatrix3  *transform;
  gboolean      filled;
};

#define GET_PRIVATE(polygon) \
        ((GimpCanvasPolygonPrivate *) gimp_canvas_polygon_get_instance_private ((GimpCanvasPolygon *) (polygon)))


/*  local function prototypes  */

static void             gimp_canvas_polygon_finalize     (GObject        *object);
static void             gimp_canvas_polygon_set_property (GObject        *object,
                                                          guint           property_id,
                                                          const GValue   *value,
                                                          GParamSpec     *pspec);
static void             gimp_canvas_polygon_get_property (GObject        *object,
                                                          guint           property_id,
                                                          GValue         *value,
                                                          GParamSpec     *pspec);
static void             gimp_canvas_polygon_draw         (GimpCanvasItem *item,
                                                          cairo_t        *cr);
static cairo_region_t * gimp_canvas_polygon_get_extents  (GimpCanvasItem *item);
static gboolean         gimp_canvas_polygon_hit          (GimpCanvasItem *item,
                                                          gdouble         x,
                                                          gdouble         y);


G_DEFINE_TYPE_WITH_PRIVATE (GimpCanvasPolygon, gimp_canvas_polygon,
                            GIMP_TYPE_CANVAS_ITEM)

#define parent_class gimp_canvas_polygon_parent_class


static void
gimp_canvas_polygon_class_init (GimpCanvasPolygonClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->finalize     = gimp_canvas_polygon_finalize;
  object_class->set_property = gimp_canvas_polygon_set_property;
  object_class->get_property = gimp_canvas_polygon_get_property;

  item_class->draw           = gimp_canvas_polygon_draw;
  item_class->get_extents    = gimp_canvas_polygon_get_extents;
  item_class->hit            = gimp_canvas_polygon_hit;

  g_object_class_install_property (object_class, PROP_POINTS,
                                   gimp_param_spec_array ("points", NULL, NULL,
                                                          GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_TRANSFORM,
                                   g_param_spec_pointer ("transform", NULL, NULL,
                                                         GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_FILLED,
                                   g_param_spec_boolean ("filled", NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));
}

static void
gimp_canvas_polygon_init (GimpCanvasPolygon *polygon)
{
}

static void
gimp_canvas_polygon_finalize (GObject *object)
{
  GimpCanvasPolygonPrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->points, g_free);
  private->n_points = 0;

  g_clear_pointer (&private->transform, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_canvas_polygon_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpCanvasPolygonPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_POINTS:
      {
        GimpArray *array = g_value_get_boxed (value);

        g_clear_pointer (&private->points, g_free);
        private->n_points = 0;

        if (array)
          {
            private->points = g_memdup2 (array->data, array->length);
            private->n_points = array->length / sizeof (GimpVector2);
          }
      }
      break;

    case PROP_TRANSFORM:
      {
        GimpMatrix3 *transform = g_value_get_pointer (value);
        if (private->transform)
          g_free (private->transform);
        if (transform)
          private->transform = g_memdup2 (transform, sizeof (GimpMatrix3));
        else
          private->transform = NULL;
      }
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
gimp_canvas_polygon_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpCanvasPolygonPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_POINTS:
      if (private->points)
        {
          GimpArray *array;

          array = gimp_array_new ((const guint8 *) private->points,
                                  private->n_points * sizeof (GimpVector2),
                                  FALSE);
          g_value_take_boxed (value, array);
        }
      else
        {
          g_value_set_boxed (value, NULL);
        }
      break;

    case PROP_TRANSFORM:
      g_value_set_pointer (value, private->transform);
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
gimp_canvas_polygon_transform (GimpCanvasItem *item,
                               GimpVector2    *points,
                               gint           *n_points)
{
  GimpCanvasPolygonPrivate *private = GET_PRIVATE (item);
  gint                      i;

  if (private->transform)
    {
      gimp_transform_polygon (private->transform,
                              private->points, private->n_points, FALSE,
                              points, n_points);

      for (i = 0; i < *n_points; i++)
        {
          gimp_canvas_item_transform_xy_f (item,
                                           points[i].x,
                                           points[i].y,
                                           &points[i].x,
                                           &points[i].y);

          points[i].x = floor (points[i].x) + 0.5;
          points[i].y = floor (points[i].y) + 0.5;
        }
    }
  else
    {
      for (i = 0; i < private->n_points; i++)
        {
          gimp_canvas_item_transform_xy_f (item,
                                           private->points[i].x,
                                           private->points[i].y,
                                           &points[i].x,
                                           &points[i].y);

          points[i].x = floor (points[i].x) + 0.5;
          points[i].y = floor (points[i].y) + 0.5;
        }

      *n_points = private->n_points;
    }
}

static void
gimp_canvas_polygon_draw (GimpCanvasItem *item,
                          cairo_t        *cr)
{
  GimpCanvasPolygonPrivate *private = GET_PRIVATE (item);
  GimpVector2              *points;
  gint                      n_points;
  gint                      i;

  if (! private->points)
    return;

  n_points = private->n_points;

  if (private->transform)
    n_points = 3 * n_points / 2;

  points = g_new0 (GimpVector2, n_points);

  gimp_canvas_polygon_transform (item, points, &n_points);

  if (n_points < 2)
    {
      g_free (points);

      return;
    }

  cairo_move_to (cr, points[0].x, points[0].y);

  for (i = 1; i < n_points; i++)
    {
      cairo_line_to (cr, points[i].x, points[i].y);
    }

  if (private->filled)
    _gimp_canvas_item_fill (item, cr);
  else
    _gimp_canvas_item_stroke (item, cr);

  g_free (points);
}

static cairo_region_t *
gimp_canvas_polygon_get_extents (GimpCanvasItem *item)
{
  GimpCanvasPolygonPrivate *private = GET_PRIVATE (item);
  cairo_rectangle_int_t     rectangle;
  GimpVector2              *points;
  gint                      n_points;
  gint                      x1, y1, x2, y2;
  gint                      i;

  if (! private->points)
    return NULL;

  n_points = private->n_points;

  if (private->transform)
    n_points = 3 * n_points / 2;

  points = g_new0 (GimpVector2, n_points);

  gimp_canvas_polygon_transform (item, points, &n_points);

  if (n_points < 2)
    {
      g_free (points);

      return NULL;
    }

  x1 = floor (points[0].x - 1.5);
  y1 = floor (points[0].y - 1.5);
  x2 = x1 + 3;
  y2 = y1 + 3;

  for (i = 1; i < n_points; i++)
    {
      gint x3 = floor (points[i].x - 1.5);
      gint y3 = floor (points[i].y - 1.5);
      gint x4 = x3 + 3;
      gint y4 = y3 + 3;

      x1 = MIN (x1, x3);
      y1 = MIN (y1, y3);
      x2 = MAX (x2, x4);
      y2 = MAX (y2, y4);
    }

  g_free (points);

  rectangle.x      = x1;
  rectangle.y      = y1;
  rectangle.width  = x2 - x1;
  rectangle.height = y2 - y1;

  return cairo_region_create_rectangle (&rectangle);
}

static gboolean
gimp_canvas_polygon_hit (GimpCanvasItem *item,
                         gdouble         x,
                         gdouble         y)
{
  GimpCanvasPolygonPrivate *private = GET_PRIVATE (item);
  GimpVector2              *points;
  gint                      n_points;
  gdouble                   tx, ty;
  cairo_surface_t          *surface;
  cairo_t                  *cr;
  gboolean                  hit;
  gint                      i;

  if (! private->points)
    return FALSE;

  gimp_canvas_item_transform_xy_f (item, x, y, &tx, &ty);

  n_points = private->n_points;

  if (private->transform)
    n_points = 3 * n_points / 2;

  points = g_new0 (GimpVector2, n_points);

  gimp_canvas_polygon_transform (item, points, &n_points);

  if (n_points < 2)
    {
      g_free (points);

      return FALSE;
    }

  surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 1, 1);
  cr = cairo_create (surface);
  cairo_surface_destroy (surface);

  cairo_move_to (cr, points[0].x, points[0].y);

  for (i = 1; i < private->n_points; i++)
    {
      cairo_line_to (cr, points[i].x, points[i].y);
    }

  g_free (points);

  hit = cairo_in_fill (cr, tx, ty);

  cairo_destroy (cr);

  return hit;
}

GimpCanvasItem *
gimp_canvas_polygon_new (GimpDisplayShell  *shell,
                         const GimpVector2 *points,
                         gint               n_points,
                         GimpMatrix3       *transform,
                         gboolean           filled)
{
  GimpCanvasItem *item;
  GimpArray      *array;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (points == NULL || n_points > 0, NULL);

  array = gimp_array_new ((const guint8 *) points,
                          n_points * sizeof (GimpVector2), TRUE);

  item = g_object_new (GIMP_TYPE_CANVAS_POLYGON,
                       "shell",     shell,
                       "transform", transform,
                       "filled",    filled,
                       "points",    array,
                       NULL);

  gimp_array_free (array);

  return item;
}

GimpCanvasItem *
gimp_canvas_polygon_new_from_coords (GimpDisplayShell *shell,
                                     const GimpCoords *coords,
                                     gint              n_coords,
                                     GimpMatrix3      *transform,
                                     gboolean          filled)
{
  GimpCanvasItem *item;
  GimpVector2    *points;
  GimpArray      *array;
  gint            i;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (coords == NULL || n_coords > 0, NULL);

  points = g_new (GimpVector2, n_coords);

  for (i = 0; i < n_coords; i++)
    {
      points[i].x = coords[i].x;
      points[i].y = coords[i].y;
    }

  array = gimp_array_new ((const guint8 *) points,
                          n_coords * sizeof (GimpVector2), TRUE);

  item = g_object_new (GIMP_TYPE_CANVAS_POLYGON,
                       "shell",     shell,
                       "transform", transform,
                       "filled",    filled,
                       "points",    array,
                       NULL);

  gimp_array_free (array);
  g_free (points);

  return item;
}

void
gimp_canvas_polygon_set_points (GimpCanvasItem    *polygon,
                                const GimpVector2 *points,
                                gint               n_points)
{
  GimpArray *array;

  g_return_if_fail (GIMP_IS_CANVAS_POLYGON (polygon));
  g_return_if_fail (points == NULL || n_points > 0);

  array = gimp_array_new ((const guint8 *) points,
                          n_points * sizeof (GimpVector2), TRUE);

  gimp_canvas_item_begin_change (polygon);
  g_object_set (polygon,
                "points", array,
                NULL);
  gimp_canvas_item_end_change (polygon);

  gimp_array_free (array);
}
