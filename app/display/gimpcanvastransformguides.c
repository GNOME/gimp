/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvastransformguides.c
 * Copyright (C) 2011 Michael Natterer <mitch@gimp.org>
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

#include "core/gimp-transform-utils.h"
#include "core/gimp-utils.h"

#include "gimpcanvastransformguides.h"
#include "gimpdisplayshell.h"


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
  PROP_N_GUIDES
};


typedef struct _GimpCanvasTransformGuidesPrivate GimpCanvasTransformGuidesPrivate;

struct _GimpCanvasTransformGuidesPrivate
{
  GimpMatrix3    transform;
  gdouble        x1, y1;
  gdouble        x2, y2;
  GimpGuidesType type;
  gint           n_guides;
};

#define GET_PRIVATE(transform) \
        G_TYPE_INSTANCE_GET_PRIVATE (transform, \
                                     GIMP_TYPE_CANVAS_TRANSFORM_GUIDES, \
                                     GimpCanvasTransformGuidesPrivate)


/*  local function prototypes  */

static void             gimp_canvas_transform_guides_set_property (GObject        *object,
                                                                   guint           property_id,
                                                                   const GValue   *value,
                                                                   GParamSpec     *pspec);
static void             gimp_canvas_transform_guides_get_property (GObject        *object,
                                                                   guint           property_id,
                                                                   GValue         *value,
                                                                   GParamSpec     *pspec);
static void             gimp_canvas_transform_guides_draw         (GimpCanvasItem *item,
                                                                   cairo_t        *cr);
static cairo_region_t * gimp_canvas_transform_guides_get_extents  (GimpCanvasItem *item);


G_DEFINE_TYPE (GimpCanvasTransformGuides, gimp_canvas_transform_guides,
               GIMP_TYPE_CANVAS_ITEM)

#define parent_class gimp_canvas_transform_guides_parent_class


static void
gimp_canvas_transform_guides_class_init (GimpCanvasTransformGuidesClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = gimp_canvas_transform_guides_set_property;
  object_class->get_property = gimp_canvas_transform_guides_get_property;

  item_class->draw           = gimp_canvas_transform_guides_draw;
  item_class->get_extents    = gimp_canvas_transform_guides_get_extents;

  g_object_class_install_property (object_class, PROP_TRANSFORM,
                                   gimp_param_spec_matrix3 ("transform",
                                                            NULL, NULL,
                                                            NULL,
                                                            GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_X1,
                                   g_param_spec_double ("x1",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_Y1,
                                   g_param_spec_double ("y1",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_X2,
                                   g_param_spec_double ("x2",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_Y2,
                                   g_param_spec_double ("y2",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_TYPE,
                                   g_param_spec_enum ("type", NULL, NULL,
                                                      GIMP_TYPE_GUIDES_TYPE,
                                                      GIMP_GUIDES_NONE,
                                                      GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_N_GUIDES,
                                   g_param_spec_int ("n-guides", NULL, NULL,
                                                     1, 128, 4,
                                                     GIMP_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (GimpCanvasTransformGuidesPrivate));
}

static void
gimp_canvas_transform_guides_init (GimpCanvasTransformGuides *transform)
{
}

static void
gimp_canvas_transform_guides_set_property (GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  GimpCanvasTransformGuidesPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_TRANSFORM:
      {
        GimpMatrix3 *transform = g_value_get_boxed (value);

        if (transform)
          private->transform = *transform;
        else
          gimp_matrix3_identity (&private->transform);
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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_transform_guides_get_property (GObject    *object,
                                           guint       property_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  GimpCanvasTransformGuidesPrivate *private = GET_PRIVATE (object);

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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_canvas_transform_guides_transform (GimpCanvasItem *item,
                                        gdouble        *tx1,
                                        gdouble        *ty1,
                                        gdouble        *tx2,
                                        gdouble        *ty2,
                                        gdouble        *tx3,
                                        gdouble        *ty3,
                                        gdouble        *tx4,
                                        gdouble        *ty4)
{
  GimpCanvasTransformGuidesPrivate *private = GET_PRIVATE (item);

  gimp_matrix3_transform_point (&private->transform,
                                private->x1, private->y1,
                                tx1, ty1);
  gimp_matrix3_transform_point (&private->transform,
                                private->x2, private->y1,
                                tx2, ty2);
  gimp_matrix3_transform_point (&private->transform,
                                private->x1, private->y2,
                                tx3, ty3);
  gimp_matrix3_transform_point (&private->transform,
                                private->x2, private->y2,
                                tx4, ty4);

  return gimp_transform_polygon_is_convex (*tx1, *ty1,
                                           *tx2, *ty2,
                                           *tx3, *ty3,
                                           *tx4, *ty4);
}

static void
draw_line (cairo_t        *cr,
           GimpCanvasItem *item,
           GimpMatrix3    *transform,
           gdouble         x1,
           gdouble         y1,
           gdouble         x2,
           gdouble         y2)
{
  gimp_matrix3_transform_point (transform, x1, y1, &x1, &y1);
  gimp_matrix3_transform_point (transform, x2, y2, &x2, &y2);

  gimp_canvas_item_transform_xy_f (item, x1, y1, &x1, &y1);
  gimp_canvas_item_transform_xy_f (item, x2, y2, &x2, &y2);

  x1 = floor (x1) + 0.5;
  y1 = floor (y1) + 0.5;
  x2 = floor (x2) + 0.5;
  y2 = floor (y2) + 0.5;

  cairo_move_to (cr, x1, y1);
  cairo_line_to (cr, x2, y2);
}

static void
draw_hline (cairo_t        *cr,
            GimpCanvasItem *item,
            GimpMatrix3    *transform,
            gdouble         x1,
            gdouble         x2,
            gdouble         y)
{
  draw_line (cr, item, transform, x1, y, x2, y);
}

static void
draw_vline (cairo_t        *cr,
            GimpCanvasItem *item,
            GimpMatrix3    *transform,
            gdouble         y1,
            gdouble         y2,
            gdouble         x)
{
  draw_line (cr, item, transform, x, y1, x, y2);
}

static void
gimp_canvas_transform_guides_draw (GimpCanvasItem *item,
                                   cairo_t        *cr)
{
  GimpCanvasTransformGuidesPrivate *private = GET_PRIVATE (item);
  gdouble                           x1, y1;
  gdouble                           x2, y2;
  gdouble                           x3, y3;
  gdouble                           x4, y4;
  gboolean                          convex;
  gint                              i;

  convex = gimp_canvas_transform_guides_transform (item,
                                                   &x1, &y1,
                                                   &x2, &y2,
                                                   &x3, &y3,
                                                   &x4, &y4);

  gimp_canvas_item_transform_xy_f (item, x1, y1, &x1, &y1);
  gimp_canvas_item_transform_xy_f (item, x2, y2, &x2, &y2);
  gimp_canvas_item_transform_xy_f (item, x3, y3, &x3, &y3);
  gimp_canvas_item_transform_xy_f (item, x4, y4, &x4, &y4);

  x1 = floor (x1) + 0.5;
  y1 = floor (y1) + 0.5;
  x2 = floor (x2) + 0.5;
  y2 = floor (y2) + 0.5;
  x3 = floor (x3) + 0.5;
  y3 = floor (y3) + 0.5;
  x4 = floor (x4) + 0.5;
  y4 = floor (y4) + 0.5;

  cairo_move_to (cr, x1, y1);
  cairo_line_to (cr, x2, y2);
  cairo_line_to (cr, x4, y4);
  cairo_line_to (cr, x3, y3);
  cairo_line_to (cr, x1, y1);

  if (! convex)
    {
      _gimp_canvas_item_stroke (item, cr);
      return;
    }

  switch (private->type)
    {
    case GIMP_GUIDES_NONE:
      break;

    case GIMP_GUIDES_CENTER_LINES:
      draw_hline (cr, item, &private->transform,
                  private->x1, private->x2, (private->y1 + private->y2) / 2);
      draw_vline (cr, item, &private->transform,
                  private->y1, private->y2, (private->x1 + private->x2) / 2);
      break;

    case GIMP_GUIDES_THIRDS:
      draw_hline (cr, item, &private->transform,
                  private->x1, private->x2, (2 * private->y1 + private->y2) / 3);
      draw_hline (cr, item, &private->transform,
                  private->x1, private->x2, (private->y1 + 2 * private->y2) / 3);

      draw_vline (cr, item, &private->transform,
                  private->y1, private->y2, (2 * private->x1 + private->x2) / 3);
      draw_vline (cr, item, &private->transform,
                  private->y1, private->y2, (private->x1 + 2 * private->x2) / 3);
      break;

    case GIMP_GUIDES_FIFTHS:
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

    case GIMP_GUIDES_GOLDEN:
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
    case GIMP_GUIDES_DIAGONALS:
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

    case GIMP_GUIDES_N_LINES:
    case GIMP_GUIDES_SPACING:
      {
        gint width, height;
        gint ngx, ngy;

        width  = MAX (1, private->x2 - private->x1);
        height = MAX (1, private->y2 - private->y1);

        /*  the MIN() in the code below limits the grid to one line
         *  every 5 image pixels, see bug 772667.
         */

        if (private->type == GIMP_GUIDES_N_LINES)
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
        else /* GIMP_GUIDES_SPACING */
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

  _gimp_canvas_item_stroke (item, cr);
}

static cairo_region_t *
gimp_canvas_transform_guides_get_extents (GimpCanvasItem *item)
{
  gdouble               x1, y1;
  gdouble               x2, y2;
  gdouble               x3, y3;
  gdouble               x4, y4;
  cairo_rectangle_int_t extents;

  gimp_canvas_transform_guides_transform (item,
                                          &x1, &y1,
                                          &x2, &y2,
                                          &x3, &y3,
                                          &x4, &y4);

  gimp_canvas_item_transform_xy_f (item, x1, y1, &x1, &y1);
  gimp_canvas_item_transform_xy_f (item, x2, y2, &x2, &y2);
  gimp_canvas_item_transform_xy_f (item, x3, y3, &x3, &y3);
  gimp_canvas_item_transform_xy_f (item, x4, y4, &x4, &y4);

  extents.x      = (gint) floor (MIN4 (x1, x2, x3, x4) - 1.5);
  extents.y      = (gint) floor (MIN4 (y1, y2, y3, y4) - 1.5);
  extents.width  = (gint) ceil  (MAX4 (x1, x2, x3, x4) + 1.5);
  extents.height = (gint) ceil  (MAX4 (y1, y2, y3, y4) + 1.5);

  extents.width  -= extents.x;
  extents.height -= extents.y;

  return cairo_region_create_rectangle (&extents);
}

GimpCanvasItem *
gimp_canvas_transform_guides_new (GimpDisplayShell  *shell,
                                  const GimpMatrix3 *transform,
                                  gdouble            x1,
                                  gdouble            y1,
                                  gdouble            x2,
                                  gdouble            y2,
                                  GimpGuidesType     type,
                                  gint               n_guides)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_CANVAS_TRANSFORM_GUIDES,
                       "shell",     shell,
                       "transform", transform,
                       "x1",        x1,
                       "y1",        y1,
                       "x2",        x2,
                       "y2",        y2,
                       "type",      type,
                       "n-guides",  n_guides,
                       NULL);
}

void
gimp_canvas_transform_guides_set (GimpCanvasItem    *guides,
                                  const GimpMatrix3 *transform,
                                  GimpGuidesType     type,
                                  gint               n_guides)
{
  g_return_if_fail (GIMP_IS_CANVAS_TRANSFORM_GUIDES (guides));

  gimp_canvas_item_begin_change (guides);

  g_object_set (guides,
                "transform", transform,
                "type",      type,
                "n-guides",  n_guides,
                NULL);

  gimp_canvas_item_end_change (guides);
}
