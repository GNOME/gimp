/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasboundary.c
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

#include "base/boundary.h"

#include "core/gimpparamspecs.h"

#include "gimpcanvasboundary.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-transform.h"


enum
{
  PROP_0,
  PROP_SEGS,
  PROP_OFFSET_X,
  PROP_OFFSET_Y
};


typedef struct _GimpCanvasBoundaryPrivate GimpCanvasBoundaryPrivate;

struct _GimpCanvasBoundaryPrivate
{
  BoundSeg *segs;
  gint      n_segs;
  gdouble   offset_x;
  gdouble   offset_y;
};

#define GET_PRIVATE(boundary) \
        G_TYPE_INSTANCE_GET_PRIVATE (boundary, \
                                     GIMP_TYPE_CANVAS_BOUNDARY, \
                                     GimpCanvasBoundaryPrivate)


/*  local function prototypes  */

static void        gimp_canvas_boundary_finalize     (GObject          *object);
static void        gimp_canvas_boundary_set_property (GObject          *object,
                                                      guint             property_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);
static void        gimp_canvas_boundary_get_property (GObject          *object,
                                                      guint             property_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);
static void        gimp_canvas_boundary_draw         (GimpCanvasItem   *item,
                                                      GimpDisplayShell *shell,
                                                      cairo_t          *cr);
static GdkRegion * gimp_canvas_boundary_get_extents  (GimpCanvasItem   *item,
                                                      GimpDisplayShell *shell);


G_DEFINE_TYPE (GimpCanvasBoundary, gimp_canvas_boundary,
               GIMP_TYPE_CANVAS_ITEM)

#define parent_class gimp_canvas_boundary_parent_class


static void
gimp_canvas_boundary_class_init (GimpCanvasBoundaryClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->finalize     = gimp_canvas_boundary_finalize;
  object_class->set_property = gimp_canvas_boundary_set_property;
  object_class->get_property = gimp_canvas_boundary_get_property;

  item_class->draw           = gimp_canvas_boundary_draw;
  item_class->get_extents    = gimp_canvas_boundary_get_extents;

  g_object_class_install_property (object_class, PROP_SEGS,
                                   gimp_param_spec_array ("segs", NULL, NULL,
                                                          GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_OFFSET_X,
                                   g_param_spec_double ("offset-x", NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE, 0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_OFFSET_Y,
                                   g_param_spec_double ("offset-y", NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE, 0,
                                                        GIMP_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (GimpCanvasBoundaryPrivate));
}

static void
gimp_canvas_boundary_init (GimpCanvasBoundary *boundary)
{
}

static void
gimp_canvas_boundary_finalize (GObject *object)
{
  GimpCanvasBoundaryPrivate *private = GET_PRIVATE (object);

  if (private->segs)
    {
      g_free (private->segs);
      private->segs = NULL;
      private->n_segs = 0;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_canvas_boundary_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpCanvasBoundaryPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_SEGS:
      break;
    case PROP_OFFSET_X:
      private->offset_x = g_value_get_double (value);
      break;
    case PROP_OFFSET_Y:
      private->offset_y = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_boundary_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpCanvasBoundaryPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_SEGS:
      break;
    case PROP_OFFSET_X:
      g_value_set_double (value, private->offset_x);
      break;
    case PROP_OFFSET_Y:
      g_value_set_double (value, private->offset_y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_boundary_transform (GimpCanvasItem   *item,
                                GimpDisplayShell *shell,
                                BoundSeg         *segs)
{
  GimpCanvasBoundaryPrivate *private = GET_PRIVATE (item);
  gint                       i;

  for (i = 0; i < private->n_segs; i++)
    {
      if (private->segs[i].x1 == -1 &&
          private->segs[i].y1 == -1 &&
          private->segs[i].x2 == -1 &&
          private->segs[i].y2 == -1)
        {
          segs[i] = private->segs[i];
        }
      else
        {
          gimp_display_shell_transform_xy (shell,
                                           private->segs[i].x1 +
                                           private->offset_x,
                                           private->segs[i].y1 +
                                           private->offset_y,
                                           &segs[i].x1,
                                           &segs[i].y1);
          gimp_display_shell_transform_xy (shell,
                                           private->segs[i].x2 +
                                           private->offset_x,
                                           private->segs[i].y2 +
                                           private->offset_y,
                                           &segs[i].x2,
                                           &segs[i].y2);
        }
    }
}

static void
gimp_canvas_boundary_draw (GimpCanvasItem   *item,
                           GimpDisplayShell *shell,
                           cairo_t          *cr)
{
  GimpCanvasBoundaryPrivate *private = GET_PRIVATE (item);
  BoundSeg                  *segs;
  gint                       i;

  segs = g_new0 (BoundSeg, private->n_segs);

  gimp_canvas_boundary_transform (item, shell, segs);

  cairo_move_to (cr, segs[0].x1 + 0.5, segs[0].y1 + 0.5);

  for (i = 1; i < private->n_segs; i++)
    {
      if (segs[i].x1 == -1 &&
          segs[i].y1 == -1 &&
          segs[i].x2 == -1 &&
          segs[i].y2 == -1)
        {
          cairo_close_path (cr);
          _gimp_canvas_item_stroke (item, shell, cr);

          i++;
          if (i == private->n_segs)
            break;

          cairo_move_to (cr, segs[i].x1 + 0.5, segs[i].y1 + 0.5);
        }
      else
        {
          cairo_line_to (cr, segs[i].x1 + 0.5, segs[i].y1 + 0.5);
        }
    }

  cairo_close_path (cr);
  _gimp_canvas_item_stroke (item, shell, cr);

  g_free (segs);
}

static GdkRegion *
gimp_canvas_boundary_get_extents (GimpCanvasItem   *item,
                                  GimpDisplayShell *shell)
{
  GimpCanvasBoundaryPrivate *private = GET_PRIVATE (item);
  GdkRectangle               rectangle;
  BoundSeg                  *segs;
  gint                       x1, y1, x2, y2;
  gint                       i;

  segs = g_new0 (BoundSeg, private->n_segs);

  gimp_canvas_boundary_transform (item, shell, segs);

  x1 = segs[0].x1 - 1;
  y1 = segs[0].y1 - 1;
  x2 = x1 + 3;
  y2 = y1 + 3;

  for (i = 1; i < private->n_segs; i++)
    {
      if (segs[i].x1 != -1 ||
          segs[i].y1 != -1 ||
          segs[i].x2 != -1 ||
          segs[i].y2 != -1)
        {
          gint x3 = segs[i].x1 - 1;
          gint y3 = segs[i].y1 - 1;
          gint x4 = x3 + 3;
          gint y4 = y3 + 3;

          x1 = MIN (x1, x3);
          y1 = MIN (y1, y3);
          x2 = MAX (x2, x4);
          y2 = MAX (y2, y4);
        }
    }

  g_free (segs);

  rectangle.x      = x1;
  rectangle.y      = y1;
  rectangle.width  = x2 - x1;
  rectangle.height = y2 - y1;

  return gdk_region_rectangle (&rectangle);
}

GimpCanvasItem *
gimp_canvas_boundary_new (const BoundSeg *segs,
                          gint            n_segs,
                          gdouble         offset_x,
                          gdouble         offset_y)
{
  GimpCanvasItem            *item;
  GimpCanvasBoundaryPrivate *private;

  item = g_object_new (GIMP_TYPE_CANVAS_BOUNDARY,
                       "offset-x", offset_x,
                       "offset-y", offset_y,
                       NULL);
  private = GET_PRIVATE (item);

  /* puke */
  private->segs   = g_memdup (segs, n_segs * sizeof (BoundSeg));
  private->n_segs = n_segs;

  return item;
}
