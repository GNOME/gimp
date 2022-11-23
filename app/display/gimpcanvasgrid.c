/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvasgrid.c
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
#include "libligmaconfig/ligmaconfig.h"

#include "display-types.h"

#include "core/ligmagrid.h"
#include "core/ligmaimage.h"

#include "ligmacanvas-style.h"
#include "ligmacanvasgrid.h"
#include "ligmacanvasitem-utils.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-scale.h"


enum
{
  PROP_0,
  PROP_GRID,
  PROP_GRID_STYLE
};


typedef struct _LigmaCanvasGridPrivate LigmaCanvasGridPrivate;

struct _LigmaCanvasGridPrivate
{
  LigmaGrid *grid;
  gboolean  grid_style;
};

#define GET_PRIVATE(grid) \
        ((LigmaCanvasGridPrivate *) ligma_canvas_grid_get_instance_private ((LigmaCanvasGrid *) (grid)))


/*  local function prototypes  */

static void             ligma_canvas_grid_finalize     (GObject        *object);
static void             ligma_canvas_grid_set_property (GObject        *object,
                                                       guint           property_id,
                                                       const GValue   *value,
                                                       GParamSpec     *pspec);
static void             ligma_canvas_grid_get_property (GObject        *object,
                                                       guint           property_id,
                                                       GValue         *value,
                                                       GParamSpec     *pspec);
static void             ligma_canvas_grid_draw         (LigmaCanvasItem *item,
                                                       cairo_t        *cr);
static cairo_region_t * ligma_canvas_grid_get_extents  (LigmaCanvasItem *item);
static void             ligma_canvas_grid_stroke       (LigmaCanvasItem *item,
                                                       cairo_t        *cr);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaCanvasGrid, ligma_canvas_grid,
                            LIGMA_TYPE_CANVAS_ITEM)

#define parent_class ligma_canvas_grid_parent_class


static void
ligma_canvas_grid_class_init (LigmaCanvasGridClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaCanvasItemClass *item_class   = LIGMA_CANVAS_ITEM_CLASS (klass);

  object_class->finalize     = ligma_canvas_grid_finalize;
  object_class->set_property = ligma_canvas_grid_set_property;
  object_class->get_property = ligma_canvas_grid_get_property;

  item_class->draw           = ligma_canvas_grid_draw;
  item_class->get_extents    = ligma_canvas_grid_get_extents;
  item_class->stroke         = ligma_canvas_grid_stroke;

  g_object_class_install_property (object_class, PROP_GRID,
                                   g_param_spec_object ("grid", NULL, NULL,
                                                        LIGMA_TYPE_GRID,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_GRID_STYLE,
                                   g_param_spec_boolean ("grid-style",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));
}

static void
ligma_canvas_grid_init (LigmaCanvasGrid *grid)
{
  LigmaCanvasGridPrivate *private = GET_PRIVATE (grid);

  private->grid = g_object_new (LIGMA_TYPE_GRID, NULL);
}

static void
ligma_canvas_grid_finalize (GObject *object)
{
  LigmaCanvasGridPrivate *private = GET_PRIVATE (object);

  g_clear_object (&private->grid);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_canvas_grid_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaCanvasGridPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GRID:
      {
        LigmaGrid *grid = g_value_get_object (value);
        if (grid)
          ligma_config_sync (G_OBJECT (grid), G_OBJECT (private->grid), 0);
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
ligma_canvas_grid_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaCanvasGridPrivate *private = GET_PRIVATE (object);

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
ligma_canvas_grid_draw (LigmaCanvasItem *item,
                       cairo_t        *cr)
{
  LigmaCanvasGridPrivate *private = GET_PRIVATE (item);
  LigmaDisplayShell      *shell   = ligma_canvas_item_get_shell (item);
  gdouble                xspacing, yspacing;
  gdouble                xoffset, yoffset;
  gboolean               vert, horz;
  gdouble                dx1, dy1, dx2, dy2;
  gint                   x1, y1, x2, y2;
  gdouble                dx, dy;
  gint                   x, y;

#define CROSSHAIR 2

  ligma_grid_get_spacing (private->grid, &xspacing, &yspacing);
  ligma_grid_get_offset  (private->grid, &xoffset,  &yoffset);

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

  if (! ligma_display_shell_get_infinite_canvas (shell))
    {
      GeglRectangle bounds;

      ligma_display_shell_scale_get_image_unrotated_bounds (
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

  switch (ligma_grid_get_style (private->grid))
    {
    case LIGMA_GRID_INTERSECTIONS:
      x1 -= CROSSHAIR;
      y1 -= CROSSHAIR;
      x2 += CROSSHAIR;
      y2 += CROSSHAIR;
      break;

    case LIGMA_GRID_DOTS:
    case LIGMA_GRID_ON_OFF_DASH:
    case LIGMA_GRID_DOUBLE_DASH:
    case LIGMA_GRID_SOLID:
      break;
    }

  xoffset = fmod (xoffset - shell->offset_x - x1, xspacing);
  yoffset = fmod (yoffset - shell->offset_y - y1, yspacing);

  if (xoffset < 0.0)
    xoffset += xspacing;

  if (yoffset < 0.0)
    yoffset += yspacing;

  switch (ligma_grid_get_style (private->grid))
    {
    case LIGMA_GRID_DOTS:
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

    case LIGMA_GRID_INTERSECTIONS:
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

    case LIGMA_GRID_ON_OFF_DASH:
    case LIGMA_GRID_DOUBLE_DASH:
    case LIGMA_GRID_SOLID:
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

  _ligma_canvas_item_stroke (item, cr);
}

static cairo_region_t *
ligma_canvas_grid_get_extents (LigmaCanvasItem *item)
{
  LigmaDisplayShell      *shell = ligma_canvas_item_get_shell (item);
  LigmaImage             *image = ligma_canvas_item_get_image (item);
  cairo_rectangle_int_t  rectangle;

  if (! image)
    return NULL;

  if (! ligma_display_shell_get_infinite_canvas (shell))
    {
      
      gdouble x1, y1;
      gdouble x2, y2;
      gint    w, h;

      w = ligma_image_get_width  (image);
      h = ligma_image_get_height (image);

      ligma_canvas_item_transform_xy_f (item, 0, 0, &x1, &y1);
      ligma_canvas_item_transform_xy_f (item, w, h, &x2, &y2);

      rectangle.x      = floor (x1);
      rectangle.y      = floor (y1);
      rectangle.width  = ceil (x2) - rectangle.x;
      rectangle.height = ceil (y2) - rectangle.y;
    }
  else
    {
      ligma_canvas_item_untransform_viewport (item,
                                             &rectangle.x,
                                             &rectangle.y,
                                             &rectangle.width,
                                             &rectangle.height);
    }

  return cairo_region_create_rectangle (&rectangle);
}

static void
ligma_canvas_grid_stroke (LigmaCanvasItem *item,
                         cairo_t        *cr)
{
  LigmaCanvasGridPrivate *private = GET_PRIVATE (item);

  if (private->grid_style)
    {
      LigmaDisplayShell *shell = ligma_canvas_item_get_shell (item);

      ligma_canvas_set_grid_style (ligma_canvas_item_get_canvas (item), cr,
                                  private->grid,
                                  shell->offset_x, shell->offset_y);
      cairo_stroke (cr);
    }
  else
    {
      LIGMA_CANVAS_ITEM_CLASS (parent_class)->stroke (item, cr);
    }
}

LigmaCanvasItem *
ligma_canvas_grid_new (LigmaDisplayShell *shell,
                      LigmaGrid         *grid)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (grid == NULL || LIGMA_IS_GRID (grid), NULL);

  return g_object_new (LIGMA_TYPE_CANVAS_GRID,
                       "shell", shell,
                       "grid",  grid,
                       NULL);
}
