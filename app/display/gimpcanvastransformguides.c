/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvastransformguides.c
 * Copyright (C) 2011 Michael Natterer <mitch@ligma.org>
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

#include "core/ligma-transform-utils.h"
#include "core/ligma-utils.h"

#include "ligmacanvastransformguides.h"
#include "ligmadisplayshell.h"


#define SQRT5 2.236067977


enum
{
  PROP_0,
  PROP_TRANSFORM,
  PROP_X1,
  PROP_Y1,
  PROP_X2,
  PROP_Y2,
  PROP_TYPE,
  PROP_N_GUIDES,
  PROP_CLIP
};


typedef struct _LigmaCanvasTransformGuidesPrivate LigmaCanvasTransformGuidesPrivate;

struct _LigmaCanvasTransformGuidesPrivate
{
  LigmaMatrix3    transform;
  gdouble        x1, y1;
  gdouble        x2, y2;
  LigmaGuidesType type;
  gint           n_guides;
  gboolean       clip;
};

#define GET_PRIVATE(transform) \
        ((LigmaCanvasTransformGuidesPrivate *) ligma_canvas_transform_guides_get_instance_private ((LigmaCanvasTransformGuides *) (transform)))


/*  local function prototypes  */

static void             ligma_canvas_transform_guides_set_property (GObject        *object,
                                                                   guint           property_id,
                                                                   const GValue   *value,
                                                                   GParamSpec     *pspec);
static void             ligma_canvas_transform_guides_get_property (GObject        *object,
                                                                   guint           property_id,
                                                                   GValue         *value,
                                                                   GParamSpec     *pspec);
static void             ligma_canvas_transform_guides_draw         (LigmaCanvasItem *item,
                                                                   cairo_t        *cr);
static cairo_region_t * ligma_canvas_transform_guides_get_extents  (LigmaCanvasItem *item);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaCanvasTransformGuides,
                            ligma_canvas_transform_guides, LIGMA_TYPE_CANVAS_ITEM)

#define parent_class ligma_canvas_transform_guides_parent_class


static void
ligma_canvas_transform_guides_class_init (LigmaCanvasTransformGuidesClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaCanvasItemClass *item_class   = LIGMA_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = ligma_canvas_transform_guides_set_property;
  object_class->get_property = ligma_canvas_transform_guides_get_property;

  item_class->draw           = ligma_canvas_transform_guides_draw;
  item_class->get_extents    = ligma_canvas_transform_guides_get_extents;

  g_object_class_install_property (object_class, PROP_TRANSFORM,
                                   ligma_param_spec_matrix3 ("transform",
                                                            NULL, NULL,
                                                            NULL,
                                                            LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_X1,
                                   g_param_spec_double ("x1",
                                                        NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_Y1,
                                   g_param_spec_double ("y1",
                                                        NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_X2,
                                   g_param_spec_double ("x2",
                                                        NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_Y2,
                                   g_param_spec_double ("y2",
                                                        NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_TYPE,
                                   g_param_spec_enum ("type", NULL, NULL,
                                                      LIGMA_TYPE_GUIDES_TYPE,
                                                      LIGMA_GUIDES_NONE,
                                                      LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_N_GUIDES,
                                   g_param_spec_int ("n-guides", NULL, NULL,
                                                     1, 128, 4,
                                                     LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_CLIP,
                                   g_param_spec_boolean ("clip", NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));
}

static void
ligma_canvas_transform_guides_init (LigmaCanvasTransformGuides *transform)
{
}

static void
ligma_canvas_transform_guides_set_property (GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  LigmaCanvasTransformGuidesPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_TRANSFORM:
      {
        LigmaMatrix3 *transform = g_value_get_boxed (value);

        if (transform)
          private->transform = *transform;
        else
          ligma_matrix3_identity (&private->transform);
      }
      break;

    case PROP_X1:
      private->x1 = g_value_get_double (value);
      break;

    case PROP_Y1:
      private->y1 = g_value_get_double (value);
      break;

    case PROP_X2:
      private->x2 = g_value_get_double (value);
      break;

    case PROP_Y2:
      private->y2 = g_value_get_double (value);
      break;

    case PROP_TYPE:
      private->type = g_value_get_enum (value);
      break;

    case PROP_N_GUIDES:
      private->n_guides = g_value_get_int (value);
      break;

    case PROP_CLIP:
      private->clip = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_canvas_transform_guides_get_property (GObject    *object,
                                           guint       property_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  LigmaCanvasTransformGuidesPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_TRANSFORM:
      g_value_set_boxed (value, &private->transform);
      break;

    case PROP_X1:
      g_value_set_double (value, private->x1);
      break;

    case PROP_Y1:
      g_value_set_double (value, private->y1);
      break;

    case PROP_X2:
      g_value_set_double (value, private->x2);
      break;

    case PROP_Y2:
      g_value_set_double (value, private->y2);
      break;

    case PROP_TYPE:
      g_value_set_enum (value, private->type);
      break;

    case PROP_N_GUIDES:
      g_value_set_int (value, private->n_guides);
      break;

    case PROP_CLIP:
      g_value_set_boolean (value, private->clip);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
ligma_canvas_transform_guides_transform (LigmaCanvasItem *item,
                                        LigmaVector2    *t_vertices,
                                        gint           *n_t_vertices)
{
  LigmaCanvasTransformGuidesPrivate *private = GET_PRIVATE (item);
  LigmaVector2                       vertices[4];

  vertices[0] = (LigmaVector2) { private->x1, private->y1 };
  vertices[1] = (LigmaVector2) { private->x2, private->y1 };
  vertices[2] = (LigmaVector2) { private->x2, private->y2 };
  vertices[3] = (LigmaVector2) { private->x1, private->y2 };

  if (private->clip)
    {
      ligma_transform_polygon (&private->transform, vertices, 4, TRUE,
                              t_vertices, n_t_vertices);

      return TRUE;
    }
  else
    {
      gint i;

      for (i = 0; i < 4; i++)
        {
          ligma_matrix3_transform_point (&private->transform,
                                        vertices[i].x,    vertices[i].y,
                                        &t_vertices[i].x, &t_vertices[i].y);
        }

      *n_t_vertices = 4;

      return ligma_transform_polygon_is_convex (t_vertices[0].x, t_vertices[0].y,
                                               t_vertices[1].x, t_vertices[1].y,
                                               t_vertices[3].x, t_vertices[3].y,
                                               t_vertices[2].x, t_vertices[2].y);
    }
}

static void
draw_line (cairo_t        *cr,
           LigmaCanvasItem *item,
           LigmaMatrix3    *transform,
           gdouble         x1,
           gdouble         y1,
           gdouble         x2,
           gdouble         y2)
{
  LigmaCanvasTransformGuidesPrivate *private = GET_PRIVATE (item);
  LigmaVector2                       vertices[2];
  LigmaVector2                       t_vertices[2];
  gint                              n_t_vertices;

  vertices[0] = (LigmaVector2) { x1, y1 };
  vertices[1] = (LigmaVector2) { x2, y2 };

  if (private->clip)
    {
      ligma_transform_polygon (transform, vertices, 2, FALSE,
                              t_vertices, &n_t_vertices);
    }
  else
    {
      gint i;

      for (i = 0; i < 2; i++)
        {
          ligma_matrix3_transform_point (transform,
                                        vertices[i].x,    vertices[i].y,
                                        &t_vertices[i].x, &t_vertices[i].y);
        }

      n_t_vertices = 2;
    }

  if (n_t_vertices == 2)
    {
      gint i;

      for (i = 0; i < 2; i++)
        {
          LigmaVector2 v;

          ligma_canvas_item_transform_xy_f (item,
                                           t_vertices[i].x, t_vertices[i].y,
                                           &v.x,            &v.y);

          v.x = floor (v.x) + 0.5;
          v.y = floor (v.y) + 0.5;

          if (i == 0)
            cairo_move_to (cr, v.x, v.y);
          else
            cairo_line_to (cr, v.x, v.y);
        }
    }
}

static void
draw_hline (cairo_t        *cr,
            LigmaCanvasItem *item,
            LigmaMatrix3    *transform,
            gdouble         x1,
            gdouble         x2,
            gdouble         y)
{
  draw_line (cr, item, transform, x1, y, x2, y);
}

static void
draw_vline (cairo_t        *cr,
            LigmaCanvasItem *item,
            LigmaMatrix3    *transform,
            gdouble         y1,
            gdouble         y2,
            gdouble         x)
{
  draw_line (cr, item, transform, x, y1, x, y2);
}

static void
ligma_canvas_transform_guides_draw (LigmaCanvasItem *item,
                                   cairo_t        *cr)
{
  LigmaCanvasTransformGuidesPrivate *private = GET_PRIVATE (item);
  LigmaVector2                       t_vertices[5];
  gint                              n_t_vertices;
  gboolean                          convex;
  gint                              i;

  convex = ligma_canvas_transform_guides_transform (item,
                                                   t_vertices, &n_t_vertices);

  if (n_t_vertices < 2)
    return;

  for (i = 0; i < n_t_vertices; i++)
    {
      LigmaVector2 v;

      ligma_canvas_item_transform_xy_f (item, t_vertices[i].x, t_vertices[i].y,
                                       &v.x, &v.y);

      v.x = floor (v.x) + 0.5;
      v.y = floor (v.y) + 0.5;

      if (i == 0)
        cairo_move_to (cr, v.x, v.y);
      else
        cairo_line_to (cr, v.x, v.y);
    }

  cairo_close_path (cr);

  if (! convex || n_t_vertices < 3)
    {
      _ligma_canvas_item_stroke (item, cr);
      return;
    }

  switch (private->type)
    {
    case LIGMA_GUIDES_NONE:
      break;

    case LIGMA_GUIDES_CENTER_LINES:
      draw_hline (cr, item, &private->transform,
                  private->x1, private->x2, (private->y1 + private->y2) / 2);
      draw_vline (cr, item, &private->transform,
                  private->y1, private->y2, (private->x1 + private->x2) / 2);
      break;

    case LIGMA_GUIDES_THIRDS:
      draw_hline (cr, item, &private->transform,
                  private->x1, private->x2, (2 * private->y1 + private->y2) / 3);
      draw_hline (cr, item, &private->transform,
                  private->x1, private->x2, (private->y1 + 2 * private->y2) / 3);

      draw_vline (cr, item, &private->transform,
                  private->y1, private->y2, (2 * private->x1 + private->x2) / 3);
      draw_vline (cr, item, &private->transform,
                  private->y1, private->y2, (private->x1 + 2 * private->x2) / 3);
      break;

    case LIGMA_GUIDES_FIFTHS:
      for (i = 0; i < 5; i++)
        {
          draw_hline (cr, item, &private->transform,
                      private->x1, private->x2,
                      private->y1 + i * (private->y2 - private->y1) / 5);
          draw_vline (cr, item, &private->transform,
                      private->y1, private->y2,
                      private->x1 + i * (private->x2 - private->x1) / 5);
        }
      break;

    case LIGMA_GUIDES_GOLDEN:
      draw_hline (cr, item, &private->transform,
                  private->x1, private->x2,
                  (2 * private->y1 + (1 + SQRT5) * private->y2) / (3 + SQRT5));
      draw_hline (cr, item, &private->transform,
                  private->x1, private->x2,
                  ((1 + SQRT5) * private->y1 + 2 * private->y2) / (3 + SQRT5));

      draw_vline (cr, item, &private->transform,
                  private->y1, private->y2,
                  (2 * private->x1 + (1 + SQRT5) * private->x2) / (3 + SQRT5));
      draw_vline (cr, item, &private->transform,
                  private->y1, private->y2,
                  ((1 + SQRT5) * private->x1 + 2 * private->x2) / (3 + SQRT5));
      break;

    /* This code implements the method of diagonals discovered by
     * Edwin Westhoff - see http://www.diagonalmethod.info/
     */
    case LIGMA_GUIDES_DIAGONALS:
      {
        /* the side of the largest square that can be
         * fitted in whole into the rectangle (x1, y1), (x2, y2)
         */
        const gdouble square_side = MIN (private->x2 - private->x1,
                                         private->y2 - private->y1);

        /* diagonal from the top-left edge */
        draw_line (cr, item, &private->transform,
                   private->x1, private->y1,
                   private->x1 + square_side,
                   private->y1 + square_side);

        /* diagonal from the top-right edge */
        draw_line (cr, item, &private->transform,
                   private->x2, private->y1,
                   private->x2 - square_side,
                   private->y1 + square_side);

        /* diagonal from the bottom-left edge */
        draw_line (cr, item, &private->transform,
                   private->x1, private->y2,
                   private->x1 + square_side,
                   private->y2 - square_side);

        /* diagonal from the bottom-right edge */
        draw_line (cr, item, &private->transform,
                   private->x2, private->y2,
                   private->x2 - square_side,
                   private->y2 - square_side);
      }
      break;

    case LIGMA_GUIDES_N_LINES:
    case LIGMA_GUIDES_SPACING:
      {
        gint width, height;
        gint ngx, ngy;

        width  = MAX (1, private->x2 - private->x1);
        height = MAX (1, private->y2 - private->y1);

        /*  the MIN() in the code below limits the grid to one line
         *  every 5 image pixels, see bug 772667.
         */

        if (private->type == LIGMA_GUIDES_N_LINES)
          {
            if (width <= height)
              {
                ngx = private->n_guides;
                ngx = MIN (ngx, width / 5);

                ngy = ngx * MAX (1, height / width);
                ngy = MIN (ngy, height / 5);
              }
            else
              {
                ngy = private->n_guides;
                ngy = MIN (ngy, height / 5);

                ngx = ngy * MAX (1, width / height);
                ngx = MIN (ngx, width / 5);
              }
          }
        else /* LIGMA_GUIDES_SPACING */
          {
            gint grid_size = MAX (2, private->n_guides);

            ngx = width  / grid_size;
            ngx = MIN (ngx, width / 5);

            ngy = height / grid_size;
            ngy = MIN (ngy, height / 5);
          }

        for (i = 1; i <= ngx; i++)
          {
            gdouble x = private->x1 + (((gdouble) i) / (ngx + 1) *
                                       (private->x2 - private->x1));

            draw_line (cr, item, &private->transform,
                       x, private->y1,
                       x, private->y2);
          }

        for (i = 1; i <= ngy; i++)
          {
            gdouble y = private->y1 + (((gdouble) i) / (ngy + 1) *
                                       (private->y2 - private->y1));

            draw_line (cr, item, &private->transform,
                       private->x1, y,
                       private->x2, y);
          }
      }
    }

  _ligma_canvas_item_stroke (item, cr);
}

static cairo_region_t *
ligma_canvas_transform_guides_get_extents (LigmaCanvasItem *item)
{
  LigmaVector2           t_vertices[5];
  gint                  n_t_vertices;
  LigmaVector2           top_left;
  LigmaVector2           bottom_right;
  cairo_rectangle_int_t extents;
  gint                  i;

  ligma_canvas_transform_guides_transform (item, t_vertices, &n_t_vertices);

  if (n_t_vertices < 2)
    return  cairo_region_create ();

  for (i = 0; i < n_t_vertices; i++)
    {
      LigmaVector2 v;

      ligma_canvas_item_transform_xy_f (item,
                                       t_vertices[i].x, t_vertices[i].y,
                                       &v.x,            &v.y);

      if (i == 0)
        {
          top_left = bottom_right = v;
        }
      else
        {
          top_left.x     = MIN (top_left.x,     v.x);
          top_left.y     = MIN (top_left.y,     v.y);

          bottom_right.x = MAX (bottom_right.x, v.x);
          bottom_right.y = MAX (bottom_right.y, v.y);
        }
    }

  extents.x      = (gint) floor (top_left.x     - 1.5);
  extents.y      = (gint) floor (top_left.y     - 1.5);
  extents.width  = (gint) ceil  (bottom_right.x + 1.5) - extents.x;
  extents.height = (gint) ceil  (bottom_right.y + 1.5) - extents.y;

  return cairo_region_create_rectangle (&extents);
}

LigmaCanvasItem *
ligma_canvas_transform_guides_new (LigmaDisplayShell  *shell,
                                  const LigmaMatrix3 *transform,
                                  gdouble            x1,
                                  gdouble            y1,
                                  gdouble            x2,
                                  gdouble            y2,
                                  LigmaGuidesType     type,
                                  gint               n_guides,
                                  gboolean           clip)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (LIGMA_TYPE_CANVAS_TRANSFORM_GUIDES,
                       "shell",     shell,
                       "transform", transform,
                       "x1",        x1,
                       "y1",        y1,
                       "x2",        x2,
                       "y2",        y2,
                       "type",      type,
                       "n-guides",  n_guides,
                       "clip",      clip,
                       NULL);
}

void
ligma_canvas_transform_guides_set (LigmaCanvasItem    *guides,
                                  const LigmaMatrix3 *transform,
                                  gdouble            x1,
                                  gdouble            y1,
                                  gdouble            x2,
                                  gdouble            y2,
                                  LigmaGuidesType     type,
                                  gint               n_guides,
                                  gboolean           clip)
{
  g_return_if_fail (LIGMA_IS_CANVAS_TRANSFORM_GUIDES (guides));

  ligma_canvas_item_begin_change (guides);

  g_object_set (guides,
                "transform", transform,
                "x1",        x1,
                "y1",        y1,
                "x2",        x2,
                "y2",        y2,
                "type",      type,
                "n-guides",  n_guides,
                "clip",      clip,
                NULL);

  ligma_canvas_item_end_change (guides);
}
