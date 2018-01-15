/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvastransformpreview.c
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
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display/display-types.h"

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimp-transform-utils.h"
#include "core/gimp-utils.h"

#include "gegl/gimp-gegl-nodes.h"

#include "gimpcanvas.h"
#include "gimpcanvastransformpreview.h"
#include "gimpdisplayshell.h"


enum
{
  PROP_0,
  PROP_DRAWABLE,
  PROP_TRANSFORM,
  PROP_X1,
  PROP_Y1,
  PROP_X2,
  PROP_Y2,
  PROP_OPACITY
};


typedef struct _GimpCanvasTransformPreviewPrivate GimpCanvasTransformPreviewPrivate;

struct _GimpCanvasTransformPreviewPrivate
{
  GimpDrawable *drawable;
  GimpMatrix3   transform;
  gdouble       x1, y1;
  gdouble       x2, y2;
  gdouble       opacity;

  GeglNode     *node;
  GeglNode     *source_node;
  GeglNode     *convert_format_node;
  GeglNode     *mask_source_node;
  GeglNode     *opacity_node;
  GeglNode     *cache_node;
  GeglNode     *transform_node;
  GimpDrawable *node_drawable;
  GimpDrawable *node_mask;
  gdouble       node_opacity;
  GimpMatrix3   node_matrix;
};

#define GET_PRIVATE(transform_preview) \
        G_TYPE_INSTANCE_GET_PRIVATE (transform_preview, \
                                     GIMP_TYPE_CANVAS_TRANSFORM_PREVIEW, \
                                     GimpCanvasTransformPreviewPrivate)


/*  local function prototypes  */

static void             gimp_canvas_transform_preview_dispose      (GObject        *object);
static void             gimp_canvas_transform_preview_set_property (GObject        *object,
                                                                    guint           property_id,
                                                                    const GValue   *value,
                                                                    GParamSpec     *pspec);
static void             gimp_canvas_transform_preview_get_property (GObject        *object,
                                                                    guint           property_id,
                                                                    GValue         *value,
                                                                    GParamSpec     *pspec);

static void             gimp_canvas_transform_preview_draw         (GimpCanvasItem *item,
                                                                    cairo_t        *cr);
static cairo_region_t * gimp_canvas_transform_preview_get_extents  (GimpCanvasItem *item);

static void             gimp_canvas_transform_preview_sync_node    (GimpCanvasItem *item);


G_DEFINE_TYPE (GimpCanvasTransformPreview, gimp_canvas_transform_preview,
               GIMP_TYPE_CANVAS_ITEM)

#define parent_class gimp_canvas_transform_preview_parent_class


/* private functions */


static void
gimp_canvas_transform_preview_class_init (GimpCanvasTransformPreviewClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpCanvasItemClass *item_class   = GIMP_CANVAS_ITEM_CLASS (klass);

  object_class->dispose      = gimp_canvas_transform_preview_dispose;
  object_class->set_property = gimp_canvas_transform_preview_set_property;
  object_class->get_property = gimp_canvas_transform_preview_get_property;

  item_class->draw           = gimp_canvas_transform_preview_draw;
  item_class->get_extents    = gimp_canvas_transform_preview_get_extents;

  g_object_class_install_property (object_class, PROP_DRAWABLE,
                                   g_param_spec_object ("drawable",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DRAWABLE,
                                                        GIMP_PARAM_READWRITE));

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

  g_object_class_install_property (object_class, PROP_OPACITY,
                                   g_param_spec_double ("opacity",
                                                        NULL, NULL,
                                                        0.0, 1.0, 1.0,
                                                        GIMP_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (GimpCanvasTransformPreviewPrivate));
}

static void
gimp_canvas_transform_preview_init (GimpCanvasTransformPreview *transform_preview)
{
}

static void
gimp_canvas_transform_preview_dispose (GObject *object)
{
  GimpCanvasTransformPreviewPrivate *private = GET_PRIVATE (object);

  g_clear_object (&private->node);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_canvas_transform_preview_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
  GimpCanvasTransformPreviewPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      private->drawable = g_value_get_object (value); /* don't ref */
      break;

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

    case PROP_OPACITY:
      private->opacity = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_canvas_transform_preview_get_property (GObject    *object,
                                            guint       property_id,
                                            GValue     *value,
                                            GParamSpec *pspec)
{
  GimpCanvasTransformPreviewPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      g_value_set_object (value, private->drawable);
      break;

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

    case PROP_OPACITY:
      g_value_set_double (value, private->opacity);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_canvas_transform_preview_transform (GimpCanvasItem        *item,
                                         cairo_rectangle_int_t *extents)
{
  GimpCanvasTransformPreviewPrivate *private = GET_PRIVATE (item);
  gdouble                            tx1, ty1;
  gdouble                            tx2, ty2;
  gdouble                            tx3, ty3;
  gdouble                            tx4, ty4;

  gimp_matrix3_transform_point (&private->transform,
                                private->x1, private->y1,
                                &tx1, &ty1);
  gimp_matrix3_transform_point (&private->transform,
                                private->x2, private->y1,
                                &tx2, &ty2);
  gimp_matrix3_transform_point (&private->transform,
                                private->x1, private->y2,
                                &tx3, &ty3);
  gimp_matrix3_transform_point (&private->transform,
                                private->x2, private->y2,
                                &tx4, &ty4);

  if (! gimp_transform_polygon_is_convex (tx1, ty1,
                                          tx2, ty2,
                                          tx3, ty3,
                                          tx4, ty4))
    return FALSE;

  if (extents)
    {
      gdouble dx1, dy1;
      gdouble dx2, dy2;
      gdouble dx3, dy3;
      gdouble dx4, dy4;

      gimp_canvas_item_transform_xy_f (item,
                                       tx1, ty1,
                                       &dx1, &dy1);
      gimp_canvas_item_transform_xy_f (item,
                                       tx2, ty2,
                                       &dx2, &dy2);
      gimp_canvas_item_transform_xy_f (item,
                                       tx3, ty3,
                                       &dx3, &dy3);
      gimp_canvas_item_transform_xy_f (item,
                                       tx4, ty4,
                                       &dx4, &dy4);

      extents->x      = (gint) floor (MIN4 (dx1, dx2, dx3, dx4));
      extents->y      = (gint) floor (MIN4 (dy1, dy2, dy3, dy4));
      extents->width  = (gint) ceil (MAX4 (dx1, dx2, dx3, dx4));
      extents->height = (gint) ceil (MAX4 (dy1, dy2, dy3, dy4));

      extents->width  -= extents->x;
      extents->height -= extents->y;
    }

  return TRUE;
}

static void
gimp_canvas_transform_preview_draw (GimpCanvasItem *item,
                                    cairo_t        *cr)
{
  GimpCanvasTransformPreviewPrivate *private = GET_PRIVATE (item);
  GimpDisplayShell                  *shell   = gimp_canvas_item_get_shell (item);
  cairo_rectangle_int_t              extents;
  gdouble                            clip_x1, clip_y1;
  gdouble                            clip_x2, clip_y2;
  GeglRectangle                      bounds;
  cairo_surface_t                   *surface;
  guchar                            *surface_data;
  gint                               surface_stride;

  /* only draw convex polygons */
  if (! gimp_canvas_transform_preview_transform (item, &extents))
    return;

  cairo_clip_extents (cr, &clip_x1, &clip_y1, &clip_x2, &clip_y2);

  clip_x1 = floor (clip_x1);
  clip_y1 = floor (clip_y1);
  clip_x2 = ceil  (clip_x2);
  clip_y2 = ceil  (clip_y2);

  if (! gegl_rectangle_intersect (&bounds,
                                  GEGL_RECTANGLE (extents.x,
                                                  extents.y,
                                                  extents.width,
                                                  extents.height),
                                  GEGL_RECTANGLE (clip_x1,
                                                  clip_y1,
                                                  clip_x2 - clip_x1,
                                                  clip_y2 - clip_y1)))
    {
      return;
    }

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        bounds.width, bounds.height);

  g_return_if_fail (surface != NULL);

  surface_data   = cairo_image_surface_get_data (surface);
  surface_stride = cairo_image_surface_get_stride (surface);

  gimp_canvas_transform_preview_sync_node (item);

  gegl_node_blit (private->transform_node, 1.0,
                  GEGL_RECTANGLE (bounds.x + shell->offset_x,
                                  bounds.y + shell->offset_y,
                                  bounds.width,
                                  bounds.height),
                  babl_format ("cairo-ARGB32"), surface_data, surface_stride,
                  GEGL_BLIT_CACHE);

  cairo_surface_mark_dirty (surface);

  cairo_set_source_surface (cr, surface, bounds.x, bounds.y);
  cairo_rectangle (cr, bounds.x, bounds.y, bounds.width, bounds.height);
  cairo_fill (cr);

  cairo_surface_destroy (surface);
}

static cairo_region_t *
gimp_canvas_transform_preview_get_extents (GimpCanvasItem *item)
{
  cairo_rectangle_int_t rectangle;

  if (gimp_canvas_transform_preview_transform (item, &rectangle))
    return cairo_region_create_rectangle (&rectangle);

  return NULL;
}

static void
gimp_canvas_transform_preview_sync_node (GimpCanvasItem *item)
{
  GimpCanvasTransformPreviewPrivate *private = GET_PRIVATE (item);
  GimpDisplayShell                  *shell   = gimp_canvas_item_get_shell (item);
  GimpImage                         *image   = gimp_canvas_item_get_image (item);
  gint                               offset_x, offset_y;
  GimpMatrix3                        matrix;
  gboolean                           has_mask;

  if (! private->node)
    {
      private->node = gegl_node_new ();

      private->source_node =
        gegl_node_new_child (private->node,
                             "operation", "gimp:buffer-source-validate",
                             NULL);

      private->convert_format_node =
        gegl_node_new_child (private->node,
                             "operation", "gegl:convert-format",
                             NULL);

      private->mask_source_node =
        gegl_node_new_child (private->node,
                             "operation", "gimp:buffer-source-validate",
                             NULL);

      private->opacity_node =
        gegl_node_new_child (private->node,
                             "operation", "gegl:opacity",
                             NULL);

      private->cache_node =
        gegl_node_new_child (private->node,
                             "operation", "gegl:cache",
                             NULL);

      private->transform_node =
        gegl_node_new_child (private->node,
                             "operation", "gegl:transform",
                             "sampler",   GIMP_INTERPOLATION_NONE,
                             NULL);

      gegl_node_link_many (private->source_node,
                           private->convert_format_node,
                           private->transform_node,
                           NULL);

      private->node_drawable = NULL;
      private->node_mask     = NULL;
      private->node_opacity  = 1.0;
      gimp_matrix3_identity (&private->node_matrix);
    }

  gimp_item_get_offset (GIMP_ITEM (private->drawable), &offset_x, &offset_y);

  gimp_matrix3_identity (&matrix);
  gimp_matrix3_translate (&matrix, offset_x, offset_y);
  gimp_matrix3_mult (&private->transform, &matrix);
  gimp_matrix3_scale (&matrix, shell->scale_x, shell->scale_y);

  has_mask = gimp_item_mask_bounds (GIMP_ITEM (private->drawable),
                                    NULL, NULL, NULL, NULL);

  if (private->drawable != private->node_drawable)
    {
      private->node_drawable = private->drawable;

      gegl_node_set (private->source_node,
                     "buffer", gimp_drawable_get_buffer (private->drawable),
                     NULL);
      gegl_node_set (private->convert_format_node,
                     "format", gimp_drawable_get_format_with_alpha (private->drawable),
                     NULL);
    }

  if ((has_mask           || private->opacity      != 1.0) !=
      (private->node_mask || private->node_opacity != 1.0))
    {
      if (has_mask || private->opacity != 1.0)
        {
          gegl_node_disconnect (private->convert_format_node, "input");

          gegl_node_link_many (private->source_node,
                               private->opacity_node,
                               private->cache_node,
                               private->transform_node,
                               NULL);
        }
      else
        {
          gegl_node_disconnect (private->opacity_node, "input");

          gegl_node_link_many (private->source_node,
                               private->convert_format_node,
                               private->transform_node,
                               NULL);
        }
    }

  if (has_mask)
    {
      GimpDrawable *mask = GIMP_DRAWABLE (gimp_image_get_mask (image));

      if (mask != private->node_mask)
        {
          private->node_mask = mask;

          gegl_node_set (private->mask_source_node,
                         "buffer", gimp_drawable_get_buffer (mask),
                         NULL);

          gegl_node_connect_to (private->mask_source_node, "output",
                                private->opacity_node,     "aux");
        }
    }
  else if (private->node_mask)
    {
      private->node_mask = NULL;

      gegl_node_disconnect (private->opacity_node, "aux");
    }

  if (private->opacity != private->node_opacity)
    {
      private->node_opacity = private->opacity;

      gegl_node_set (private->opacity_node,
                     "value", private->opacity,
                     NULL);
    }

  if (memcmp (&matrix, &private->node_matrix, sizeof (matrix)))
    {
      private->node_matrix = matrix;

      gimp_gegl_node_set_matrix (private->transform_node, &matrix);
    }
}


/* public functions */


GimpCanvasItem *
gimp_canvas_transform_preview_new (GimpDisplayShell  *shell,
                                   GimpDrawable      *drawable,
                                   const GimpMatrix3 *transform,
                                   gdouble            x1,
                                   gdouble            y1,
                                   gdouble            x2,
                                   gdouble            y2)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (transform != NULL, NULL);

  return g_object_new (GIMP_TYPE_CANVAS_TRANSFORM_PREVIEW,
                       "shell",     shell,
                       "drawable",  drawable,
                       "transform", transform,
                       "x1",        x1,
                       "y1",        y1,
                       "x2",        x2,
                       "y2",        y2,
                       NULL);
}
