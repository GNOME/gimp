/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasbufferpreview.c
 * Copyright (C) 2013 Marek Dvoroznak <dvoromar@gmail.com>
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
#include <cairo.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "display/display-types.h"

#include "core/gimpimage.h"

#include "gimpcanvas.h"
#include "gimpcanvasbufferpreview.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-scroll.h"


enum
{
  PROP_0,
  PROP_BUFFER
};


typedef struct _GimpCanvasBufferPreviewPrivate GimpCanvasBufferPreviewPrivate;

struct _GimpCanvasBufferPreviewPrivate
{
  GeglBuffer *buffer;
};


#define GET_PRIVATE(transform_preview) \
        ((GimpCanvasBufferPreviewPrivate *) gimp_canvas_buffer_preview_get_instance_private ((GimpCanvasBufferPreview *) (transform_preview)))


/*  local function prototypes  */

static void             gimp_canvas_buffer_preview_dispose        (GObject               *object);
static void             gimp_canvas_buffer_preview_set_property   (GObject               *object,
                                                                   guint                  property_id,
                                                                   const GValue          *value,
                                                                   GParamSpec            *pspec);
static void             gimp_canvas_buffer_preview_get_property   (GObject               *object,
                                                                   guint                  property_id,
                                                                   GValue                *value,
                                                                   GParamSpec            *pspec);

static void             gimp_canvas_buffer_preview_draw           (GimpCanvasItem        *item,
                                                                   cairo_t               *cr);
static cairo_region_t * gimp_canvas_buffer_preview_get_extents    (GimpCanvasItem        *item);
static void             gimp_canvas_buffer_preview_compute_bounds (GimpCanvasItem        *item,
                                                                   cairo_rectangle_int_t *bounds);


G_DEFINE_TYPE_WITH_PRIVATE (GimpCanvasBufferPreview, gimp_canvas_buffer_preview,
                            GIMP_TYPE_CANVAS_ITEM)

#define parent_class gimp_canvas_buffer_preview_parent_class


static void
gimp_canvas_buffer_preview_class_init (GimpCanvasBufferPreviewClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->dispose      = gimp_canvas_buffer_preview_dispose;
  object_class->set_property = gimp_canvas_buffer_preview_set_property;
  object_class->get_property = gimp_canvas_buffer_preview_get_property;

  item_class->draw           = gimp_canvas_buffer_preview_draw;
  item_class->get_extents    = gimp_canvas_buffer_preview_get_extents;

  g_object_class_install_property (object_class, PROP_BUFFER,
                                   g_param_spec_object ("buffer",
                                                        NULL, NULL,
                                                        GEGL_TYPE_BUFFER,
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_canvas_buffer_preview_init (GimpCanvasBufferPreview *transform_preview)
{
}

static void
gimp_canvas_buffer_preview_dispose (GObject *object)
{
  GimpCanvasBufferPreviewPrivate *private = GET_PRIVATE (object);

  g_clear_object (&private->buffer);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_canvas_buffer_preview_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  GimpCanvasBufferPreviewPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_BUFFER:
      g_set_object (&private->buffer, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_buffer_preview_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  GimpCanvasBufferPreviewPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_BUFFER:
      g_value_set_object (value, private->buffer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_buffer_preview_draw (GimpCanvasItem *item,
                                 cairo_t        *cr)
{
  GimpDisplayShell      *shell  = gimp_canvas_item_get_shell (item);
  GeglBuffer            *buffer = GET_PRIVATE (item)->buffer;
  cairo_surface_t       *area;
  guchar                *data;
  cairo_rectangle_int_t  rectangle;

  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  gimp_canvas_buffer_preview_compute_bounds (item, &rectangle);

  area = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                     rectangle.width,
                                     rectangle.height);

  data = cairo_image_surface_get_data (area);
  gegl_buffer_get (buffer,
                   GEGL_RECTANGLE (rectangle.x + shell->offset_x,
                                   rectangle.y + shell->offset_y,
                                   rectangle.width,
                                   rectangle.height),
                   shell->scale_x,
                   babl_format ("cairo-ARGB32"),
                   data,
                   cairo_image_surface_get_stride (area),
                   GEGL_ABYSS_NONE);

  cairo_surface_flush (area);
  cairo_surface_mark_dirty (area);

  cairo_set_source_surface (cr, area, rectangle.x, rectangle.y);
  cairo_rectangle (cr,
                   rectangle.x, rectangle.y,
                   rectangle.width, rectangle.height);
  cairo_fill (cr);

  cairo_surface_destroy (area);
}

static void
gimp_canvas_buffer_preview_compute_bounds (GimpCanvasItem        *item,
                                           cairo_rectangle_int_t *bounds)
{
  GimpDisplayShell *shell  = gimp_canvas_item_get_shell (item);
  GeglBuffer       *buffer = GET_PRIVATE (item)->buffer;
  GeglRectangle     extent;
  gdouble           x1, y1;
  gdouble           x2, y2;

  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  extent = *gegl_buffer_get_extent (buffer);

  gimp_canvas_item_transform_xy_f (item,
                                   extent.x,
                                   extent.y,
                                   &x1, &y1);
  gimp_canvas_item_transform_xy_f (item,
                                   extent.x + extent.width,
                                   extent.y + extent.height,
                                   &x2, &y2);

  extent.x      = floor (x1);
  extent.y      = floor (y1);
  extent.width  = ceil  (x2) - extent.x;
  extent.height = ceil  (y2) - extent.y;

  gegl_rectangle_intersect (&extent,
                            &extent,
                            GEGL_RECTANGLE (0,
                                            0,
                                            shell->disp_width,
                                            shell->disp_height));

  bounds->x      = extent.x;
  bounds->y      = extent.y;
  bounds->width  = extent.width;
  bounds->height = extent.height;
}

static cairo_region_t *
gimp_canvas_buffer_preview_get_extents (GimpCanvasItem *item)
{
  cairo_rectangle_int_t rectangle;

  gimp_canvas_buffer_preview_compute_bounds (item, &rectangle);

  return cairo_region_create_rectangle (&rectangle);
}

GimpCanvasItem *
gimp_canvas_buffer_preview_new (GimpDisplayShell  *shell,
                                GeglBuffer        *buffer)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  return g_object_new (GIMP_TYPE_CANVAS_BUFFER_PREVIEW,
                       "shell",       shell,
                       "buffer",      buffer,
                       NULL);
}
