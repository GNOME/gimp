/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasgrid.c
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
#include "libgimpconfig/gimpconfig.h"

#include "display-types.h"

#include "core/gimpgrid.h"
#include "core/gimpimage.h"

#include "gimpcanvas-style.h"
#include "gimpcanvasgrid.h"
#include "gimpcanvasitem-utils.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-scale.h"


enum
{
  PROP_0,
  PROP_GRID,
  PROP_GRID_STYLE
};


typedef struct _GimpCanvasGridPrivate GimpCanvasGridPrivate;

struct _GimpCanvasGridPrivate
{
  GimpGrid *grid;
  gboolean  grid_style;
};

#define GET_PRIVATE(grid) \
        ((GimpCanvasGridPrivate *) gimp_canvas_grid_get_instance_private ((GimpCanvasGrid *) (grid)))


/*  local function prototypes  */

static void             gimp_canvas_grid_finalize     (GObject        *object);
static void             gimp_canvas_grid_set_property (GObject        *object,
                                                       guint           property_id,
                                                       const GValue   *value,
                                                       GParamSpec     *pspec);
static void             gimp_canvas_grid_get_property (GObject        *object,
                                                       guint           property_id,
                                                       GValue         *value,
                                                       GParamSpec     *pspec);
static void             gimp_canvas_grid_draw         (GimpCanvasItem *item,
                                                       cairo_t        *cr);
static cairo_region_t * gimp_canvas_grid_get_extents  (GimpCanvasItem *item);
static void             gimp_canvas_grid_stroke       (GimpCanvasItem *item,
                                                       cairo_t        *cr);


G_DEFINE_TYPE_WITH_PRIVATE (GimpCanvasGrid, gimp_canvas_grid,
                            GIMP_TYPE_CANVAS_ITEM)

#define parent_class gimp_canvas_grid_parent_class


static void
gimp_canvas_grid_class_init (GimpCanvasGridClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->finalize     = gimp_canvas_grid_finalize;
  object_class->set_property = gimp_canvas_grid_set_property;
  object_class->get_property = gimp_canvas_grid_get_property;

  item_class->draw           = gimp_canvas_grid_draw;
  item_class->get_extents    = gimp_canvas_grid_get_extents;
  item_class->stroke         = gimp_canvas_grid_stroke;

  g_object_class_install_property (object_class, PROP_GRID,
                                   g_param_spec_object ("grid", NULL, NULL,
                                                        GIMP_TYPE_GRID,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_GRID_STYLE,
                                   g_param_spec_boolean ("grid-style",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));
}

static void
gimp_canvas_grid_init (GimpCanvasGrid *grid)
{
  GimpCanvasGridPrivate *private = GET_PRIVATE (grid);

  private->grid = g_object_new (GIMP_TYPE_GRID, NULL);
}

static void
gimp_canvas_grid_finalize (GObject *object)
{
  GimpCanvasGridPrivate *private = GET_PRIVATE (object);

  g_clear_object (&private->grid);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_canvas_grid_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpCanvasGridPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GRID:
      {
        GimpGrid *grid = g_value_get_object (value);
        if (grid)
          gimp_config_sync (G_OBJECT (grid), G_OBJECT (private->grid), 0);
      }
      break;
    case PROP_GRID_STYLE:
      private->grid_style = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_grid_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpCanvasGridPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GRID:
      g_value_set_object (value, private->grid);
      break;
    case PROP_GRID_STYLE:
      g_value_set_boolean (value, private->grid_style);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_grid_draw (GimpCanvasItem *item,
                       cairo_t        *cr)
{
  GimpCanvasGridPrivate *private = GET_PRIVATE (item);
  GimpDisplayShell      *shell   = gimp_canvas_item_get_shell (item);
  gdouble                xspacing, yspacing;
  gdouble                xoffset, yoffset;
  gboolean               vert, horz;
  gdouble                dx1, dy1, dx2, dy2;
  gint                   x1, y1, x2, y2;
  gdouble                dx, dy;
  gint                   x, y;

#define CROSSHAIR 2

  gimp_grid_get_spacing (private->grid, &xspacing, &yspacing);
  gimp_grid_get_offset  (private->grid, &xoffset,  &yoffset);

  g_return_if_fail (xspacing >= 0.0 &&
                    yspacing >= 0.0);

  xspacing *= shell->scale_x;
  yspacing *= shell->scale_y;

  xoffset  *= shell->scale_x;
  yoffset  *= shell->scale_y;

  /*  skip grid drawing when the space between grid lines starts
   *  disappearing, see bug #599267.
   */
  vert = (xspacing >= 2.0);
  horz = (yspacing >= 2.0);

  if (! vert && ! horz)
    return;

  cairo_clip_extents (cr, &dx1, &dy1, &dx2, &dy2);

  x1 = floor (dx1) - 1;
  y1 = floor (dy1) - 1;
  x2 = ceil  (dx2) + 1;
  y2 = ceil  (dy2) + 1;

  if (! gimp_display_shell_get_infinite_canvas (shell))
    {
      GeglRectangle bounds;

      gimp_display_shell_scale_get_image_unrotated_bounds (
        shell,
        &bounds.x, &bounds.y, &bounds.width, &bounds.height);

      if (! gegl_rectangle_intersect (&bounds,
                                      &bounds,
                                      GEGL_RECTANGLE (x1,      y1,
                                                      x2 - x1, y2 - y1)))
        {
          return;
        }

      x1 = bounds.x;
      y1 = bounds.y;
      x2 = bounds.x + bounds.width;
      y2 = bounds.y + bounds.height;
    }

  switch (gimp_grid_get_style (private->grid))
    {
    case GIMP_GRID_INTERSECTIONS:
      x1 -= CROSSHAIR;
      y1 -= CROSSHAIR;
      x2 += CROSSHAIR;
      y2 += CROSSHAIR;
      break;

    case GIMP_GRID_DOTS:
    case GIMP_GRID_ON_OFF_DASH:
    case GIMP_GRID_DOUBLE_DASH:
    case GIMP_GRID_SOLID:
      break;
    }

  xoffset = fmod (xoffset - shell->offset_x - x1, xspacing);
  yoffset = fmod (yoffset - shell->offset_y - y1, yspacing);

  if (xoffset < 0.0)
    xoffset += xspacing;

  if (yoffset < 0.0)
    yoffset += yspacing;

  switch (gimp_grid_get_style (private->grid))
    {
    case GIMP_GRID_DOTS:
      if (vert && horz)
        {
          for (dx = x1 + xoffset; dx <= x2; dx += xspacing)
            {
              x = RINT (dx);

              for (dy = y1 + yoffset; dy <= y2; dy += yspacing)
                {
                  y = RINT (dy);

                  cairo_move_to (cr, x,       y + 0.5);
                  cairo_line_to (cr, x + 1.0, y + 0.5);
                }
            }
        }
      break;

    case GIMP_GRID_INTERSECTIONS:
      if (vert && horz)
        {
          for (dx = x1 + xoffset; dx <= x2; dx += xspacing)
            {
              x = RINT (dx);

              for (dy = y1 + yoffset; dy <= y2; dy += yspacing)
                {
                  y = RINT (dy);

                  cairo_move_to (cr, x + 0.5, y - CROSSHAIR);
                  cairo_line_to (cr, x + 0.5, y + CROSSHAIR + 1.0);

                  cairo_move_to (cr, x - CROSSHAIR, y + 0.5);
                  cairo_line_to (cr, x + CROSSHAIR + 1.0, y + 0.5);
                }
            }
        }
      break;

    case GIMP_GRID_ON_OFF_DASH:
    case GIMP_GRID_DOUBLE_DASH:
    case GIMP_GRID_SOLID:
      if (vert)
        {
          for (dx = x1 + xoffset; dx < x2; dx += xspacing)
            {
              x = RINT (dx);
              
              cairo_move_to (cr, x + 0.5, y1);
              cairo_line_to (cr, x + 0.5, y2);
            }
        }

      if (horz)
        {
          for (dy = y1 + yoffset; dy < y2; dy += yspacing)
            {
              y = RINT (dy);
              
              cairo_move_to (cr, x1, y + 0.5);
              cairo_line_to (cr, x2, y + 0.5);
            }
        }
      break;
    }

  _gimp_canvas_item_stroke (item, cr);
}

static cairo_region_t *
gimp_canvas_grid_get_extents (GimpCanvasItem *item)
{
  GimpDisplayShell      *shell = gimp_canvas_item_get_shell (item);
  GimpImage             *image = gimp_canvas_item_get_image (item);
  cairo_rectangle_int_t  rectangle;

  if (! image)
    return NULL;

  if (! gimp_display_shell_get_infinite_canvas (shell))
    {
      
      gdouble x1, y1;
      gdouble x2, y2;
      gint    w, h;

      w = gimp_image_get_width  (image);
      h = gimp_image_get_height (image);

      gimp_canvas_item_transform_xy_f (item, 0, 0, &x1, &y1);
      gimp_canvas_item_transform_xy_f (item, w, h, &x2, &y2);

      rectangle.x      = floor (x1);
      rectangle.y      = floor (y1);
      rectangle.width  = ceil (x2) - rectangle.x;
      rectangle.height = ceil (y2) - rectangle.y;
    }
  else
    {
      gimp_canvas_item_untransform_viewport (item,
                                             &rectangle.x,
                                             &rectangle.y,
                                             &rectangle.width,
                                             &rectangle.height);
    }

  return cairo_region_create_rectangle (&rectangle);
}

static void
gimp_canvas_grid_stroke (GimpCanvasItem *item,
                         cairo_t        *cr)
{
  GimpCanvasGridPrivate *private = GET_PRIVATE (item);

  if (private->grid_style)
    {
      GimpDisplayShell *shell = gimp_canvas_item_get_shell (item);

      gimp_canvas_set_grid_style (gimp_canvas_item_get_canvas (item), cr,
                                  private->grid,
                                  shell->offset_x, shell->offset_y);
      cairo_stroke (cr);
    }
  else
    {
      GIMP_CANVAS_ITEM_CLASS (parent_class)->stroke (item, cr);
    }
}

GimpCanvasItem *
gimp_canvas_grid_new (GimpDisplayShell *shell,
                      GimpGrid         *grid)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (grid == NULL || GIMP_IS_GRID (grid), NULL);

  return g_object_new (GIMP_TYPE_CANVAS_GRID,
                       "shell", shell,
                       "grid",  grid,
                       NULL);
}
