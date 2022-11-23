/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvasboundary.c
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

#include "core/ligma-cairo.h"
#include "core/ligma-transform-utils.h"
#include "core/ligmaboundary.h"
#include "core/ligmaparamspecs.h"

#include "ligmacanvasboundary.h"
#include "ligmadisplayshell.h"


enum
{
  PROP_0,
  PROP_SEGS,
  PROP_TRANSFORM,
  PROP_OFFSET_X,
  PROP_OFFSET_Y
};


typedef struct _LigmaCanvasBoundaryPrivate LigmaCanvasBoundaryPrivate;

struct _LigmaCanvasBoundaryPrivate
{
  LigmaBoundSeg *segs;
  gint          n_segs;
  LigmaMatrix3  *transform;
  gdouble       offset_x;
  gdouble       offset_y;
};

#define GET_PRIVATE(boundary) \
        ((LigmaCanvasBoundaryPrivate *) ligma_canvas_boundary_get_instance_private ((LigmaCanvasBoundary *) (boundary)))


/*  local function prototypes  */

static void             ligma_canvas_boundary_finalize     (GObject        *object);
static void             ligma_canvas_boundary_set_property (GObject        *object,
                                                           guint           property_id,
                                                           const GValue   *value,
                                                           GParamSpec     *pspec);
static void             ligma_canvas_boundary_get_property (GObject        *object,
                                                           guint           property_id,
                                                           GValue         *value,
                                                           GParamSpec     *pspec);
static void             ligma_canvas_boundary_draw         (LigmaCanvasItem *item,
                                                           cairo_t        *cr);
static cairo_region_t * ligma_canvas_boundary_get_extents  (LigmaCanvasItem *item);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaCanvasBoundary, ligma_canvas_boundary,
                            LIGMA_TYPE_CANVAS_ITEM)

#define parent_class ligma_canvas_boundary_parent_class


static void
ligma_canvas_boundary_class_init (LigmaCanvasBoundaryClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaCanvasItemClass *item_class   = LIGMA_CANVAS_ITEM_CLASS (klass);

  object_class->finalize     = ligma_canvas_boundary_finalize;
  object_class->set_property = ligma_canvas_boundary_set_property;
  object_class->get_property = ligma_canvas_boundary_get_property;

  item_class->draw           = ligma_canvas_boundary_draw;
  item_class->get_extents    = ligma_canvas_boundary_get_extents;

  g_object_class_install_property (object_class, PROP_SEGS,
                                   ligma_param_spec_array ("segs", NULL, NULL,
                                                          LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_TRANSFORM,
                                   g_param_spec_pointer ("transform", NULL, NULL,
                                                         LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_OFFSET_X,
                                   g_param_spec_double ("offset-x", NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE, 0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_OFFSET_Y,
                                   g_param_spec_double ("offset-y", NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE, 0,
                                                        LIGMA_PARAM_READWRITE));
}

static void
ligma_canvas_boundary_init (LigmaCanvasBoundary *boundary)
{
  ligma_canvas_item_set_line_cap (LIGMA_CANVAS_ITEM (boundary),
                                 CAIRO_LINE_CAP_SQUARE);
}

static void
ligma_canvas_boundary_finalize (GObject *object)
{
  LigmaCanvasBoundaryPrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->segs, g_free);
  private->n_segs = 0;

  g_clear_pointer (&private->transform, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_canvas_boundary_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  LigmaCanvasBoundaryPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_SEGS:
      break;

    case PROP_TRANSFORM:
      {
        LigmaMatrix3 *transform = g_value_get_pointer (value);
        if (private->transform)
          g_free (private->transform);
        if (transform)
          private->transform = g_memdup2 (transform, sizeof (LigmaMatrix3));
        else
          private->transform = NULL;
      }
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
ligma_canvas_boundary_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  LigmaCanvasBoundaryPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_SEGS:
      break;

    case PROP_TRANSFORM:
      g_value_set_pointer (value, private->transform);
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
ligma_canvas_boundary_transform (LigmaCanvasItem *item,
                                LigmaSegment    *segs,
                                gint           *n_segs)
{
  LigmaCanvasBoundaryPrivate *private = GET_PRIVATE (item);
  gint                       i;

  if (private->transform)
    {
      gint n = 0;

      for (i = 0; i < private->n_segs; i++)
        {
          LigmaVector2 vertices[2];
          LigmaVector2 t_vertices[2];
          gint        n_t_vertices;

          vertices[0] = (LigmaVector2) { private->segs[i].x1, private->segs[i].y1 };
          vertices[1] = (LigmaVector2) { private->segs[i].x2, private->segs[i].y2 };

          ligma_transform_polygon (private->transform, vertices, 2, FALSE,
                                  t_vertices, &n_t_vertices);

          if (n_t_vertices == 2)
            {
              ligma_canvas_item_transform_xy (item,
                                             t_vertices[0].x + private->offset_x,
                                             t_vertices[0].y + private->offset_y,
                                             &segs[n].x1, &segs[n].y1);
              ligma_canvas_item_transform_xy (item,
                                             t_vertices[1].x + private->offset_x,
                                             t_vertices[1].y + private->offset_y,
                                             &segs[n].x2, &segs[n].y2);

              n++;
            }
        }

      *n_segs = n;
    }
  else
    {
      for (i = 0; i < private->n_segs; i++)
        {
          ligma_canvas_item_transform_xy (item,
                                         private->segs[i].x1 + private->offset_x,
                                         private->segs[i].y1 + private->offset_y,
                                         &segs[i].x1,
                                         &segs[i].y1);
          ligma_canvas_item_transform_xy (item,
                                         private->segs[i].x2 + private->offset_x,
                                         private->segs[i].y2 + private->offset_y,
                                         &segs[i].x2,
                                         &segs[i].y2);

          /*  If this segment is a closing segment && the segments lie inside
           *  the region, OR if this is an opening segment and the segments
           *  lie outside the region...
           *  we need to transform it by one display pixel
           */
          if (! private->segs[i].open)
            {
              /*  If it is vertical  */
              if (segs[i].x1 == segs[i].x2)
                {
                  segs[i].x1 -= 1;
                  segs[i].x2 -= 1;
                }
              else
                {
                  segs[i].y1 -= 1;
                  segs[i].y2 -= 1;
                }
            }
        }

      *n_segs = private->n_segs;
    }
}

static void
ligma_canvas_boundary_draw (LigmaCanvasItem *item,
                           cairo_t        *cr)
{
  LigmaCanvasBoundaryPrivate *private = GET_PRIVATE (item);
  LigmaSegment               *segs;
  gint                       n_segs;

  segs = g_new0 (LigmaSegment, private->n_segs);

  ligma_canvas_boundary_transform (item, segs, &n_segs);

  ligma_cairo_segments (cr, segs, n_segs);

  _ligma_canvas_item_stroke (item, cr);

  g_free (segs);
}

static cairo_region_t *
ligma_canvas_boundary_get_extents (LigmaCanvasItem *item)
{
  LigmaCanvasBoundaryPrivate *private = GET_PRIVATE (item);
  cairo_rectangle_int_t      rectangle;
  LigmaSegment               *segs;
  gint                       n_segs;
  gint                       x1, y1, x2, y2;
  gint                       i;

  segs = g_new0 (LigmaSegment, private->n_segs);

  ligma_canvas_boundary_transform (item, segs, &n_segs);

  if (n_segs == 0)
    {
      g_free (segs);

      return cairo_region_create ();
    }

  x1 = MIN (segs[0].x1, segs[0].x2);
  y1 = MIN (segs[0].y1, segs[0].y2);
  x2 = MAX (segs[0].x1, segs[0].x2);
  y2 = MAX (segs[0].y1, segs[0].y2);

  for (i = 1; i < n_segs; i++)
    {
      gint x3 = MIN (segs[i].x1, segs[i].x2);
      gint y3 = MIN (segs[i].y1, segs[i].y2);
      gint x4 = MAX (segs[i].x1, segs[i].x2);
      gint y4 = MAX (segs[i].y1, segs[i].y2);

      x1 = MIN (x1, x3);
      y1 = MIN (y1, y3);
      x2 = MAX (x2, x4);
      y2 = MAX (y2, y4);
    }

  g_free (segs);

  rectangle.x      = x1 - 2;
  rectangle.y      = y1 - 2;
  rectangle.width  = x2 - x1 + 4;
  rectangle.height = y2 - y1 + 4;

  return cairo_region_create_rectangle (&rectangle);
}

LigmaCanvasItem *
ligma_canvas_boundary_new (LigmaDisplayShell   *shell,
                          const LigmaBoundSeg *segs,
                          gint                n_segs,
                          LigmaMatrix3        *transform,
                          gdouble             offset_x,
                          gdouble             offset_y)
{
  LigmaCanvasItem            *item;
  LigmaCanvasBoundaryPrivate *private;

  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  item = g_object_new (LIGMA_TYPE_CANVAS_BOUNDARY,
                       "shell",     shell,
                       "transform", transform,
                       "offset-x",  offset_x,
                       "offset-y",  offset_y,
                       NULL);
  private = GET_PRIVATE (item);

  /* puke */
  private->segs   = g_memdup2 (segs, n_segs * sizeof (LigmaBoundSeg));
  private->n_segs = n_segs;

  return item;
}
