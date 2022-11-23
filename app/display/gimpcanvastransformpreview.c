/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvastransformpreview.c
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "display/display-types.h"

#include "gegl/ligma-gegl-nodes.h"
#include "gegl/ligma-gegl-utils.h"
#include "gegl/ligmatilehandlervalidate.h"

#include "core/ligma-transform-resize.h"
#include "core/ligma-transform-utils.h"
#include "core/ligma-utils.h"
#include "core/ligmachannel.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer.h"
#include "core/ligmapickable.h"

#include "ligmacanvas.h"
#include "ligmacanvastransformpreview.h"
#include "ligmadisplayshell.h"


enum
{
  PROP_0,
  PROP_PICKABLE,
  PROP_TRANSFORM,
  PROP_CLIP,
  PROP_X1,
  PROP_Y1,
  PROP_X2,
  PROP_Y2,
  PROP_OPACITY
};


typedef struct _LigmaCanvasTransformPreviewPrivate LigmaCanvasTransformPreviewPrivate;

struct _LigmaCanvasTransformPreviewPrivate
{
  LigmaPickable        *pickable;
  LigmaMatrix3          transform;
  LigmaTransformResize  clip;
  gdouble              x1, y1;
  gdouble              x2, y2;
  gdouble              opacity;

  GeglNode            *node;
  GeglNode            *source_node;
  GeglNode            *convert_format_node;
  GeglNode            *layer_mask_source_node;
  GeglNode            *layer_mask_opacity_node;
  GeglNode            *mask_source_node;
  GeglNode            *mask_translate_node;
  GeglNode            *mask_crop_node;
  GeglNode            *opacity_node;
  GeglNode            *cache_node;
  GeglNode            *transform_node;

  LigmaPickable        *node_pickable;
  LigmaDrawable        *node_layer_mask;
  LigmaDrawable        *node_mask;
  GeglRectangle        node_rect;
  gdouble              node_opacity;
  LigmaMatrix3          node_matrix;
  GeglNode            *node_output;
};

#define GET_PRIVATE(transform_preview) \
        ((LigmaCanvasTransformPreviewPrivate *) ligma_canvas_transform_preview_get_instance_private ((LigmaCanvasTransformPreview *) (transform_preview)))


/*  local function prototypes  */

static void             ligma_canvas_transform_preview_dispose       (GObject                    *object);
static void             ligma_canvas_transform_preview_set_property  (GObject                    *object,
                                                                     guint                       property_id,
                                                                     const GValue               *value,
                                                                     GParamSpec                 *pspec);
static void             ligma_canvas_transform_preview_get_property  (GObject                    *object,
                                                                     guint                       property_id,
                                                                     GValue                     *value,
                                                                     GParamSpec                 *pspec);

static void             ligma_canvas_transform_preview_draw          (LigmaCanvasItem             *item,
                                                                     cairo_t                    *cr);
static cairo_region_t * ligma_canvas_transform_preview_get_extents   (LigmaCanvasItem             *item);

static void             ligma_canvas_transform_preview_layer_changed (LigmaLayer                  *layer,
                                                                     LigmaCanvasTransformPreview *transform_preview);

static void             ligma_canvas_transform_preview_set_pickable  (LigmaCanvasTransformPreview *transform_preview,
                                                                     LigmaPickable               *pickable);
static void             ligma_canvas_transform_preview_sync_node     (LigmaCanvasTransformPreview *transform_preview);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaCanvasTransformPreview,
                            ligma_canvas_transform_preview,
                            LIGMA_TYPE_CANVAS_ITEM)

#define parent_class ligma_canvas_transform_preview_parent_class


/* private functions */


static void
ligma_canvas_transform_preview_class_init (LigmaCanvasTransformPreviewClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaCanvasItemClass *item_class   = LIGMA_CANVAS_ITEM_CLASS (klass);

  object_class->dispose      = ligma_canvas_transform_preview_dispose;
  object_class->set_property = ligma_canvas_transform_preview_set_property;
  object_class->get_property = ligma_canvas_transform_preview_get_property;

  item_class->draw           = ligma_canvas_transform_preview_draw;
  item_class->get_extents    = ligma_canvas_transform_preview_get_extents;

  g_object_class_install_property (object_class, PROP_PICKABLE,
                                   g_param_spec_object ("pickable",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_PICKABLE,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_TRANSFORM,
                                   ligma_param_spec_matrix3 ("transform",
                                                            NULL, NULL,
                                                            NULL,
                                                            LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_CLIP,
                                   g_param_spec_enum ("clip",
                                                      NULL, NULL,
                                                      LIGMA_TYPE_TRANSFORM_RESIZE,
                                                      LIGMA_TRANSFORM_RESIZE_ADJUST,
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

  g_object_class_install_property (object_class, PROP_OPACITY,
                                   g_param_spec_double ("opacity",
                                                        NULL, NULL,
                                                        0.0, 1.0, 1.0,
                                                        LIGMA_PARAM_READWRITE));
}

static void
ligma_canvas_transform_preview_init (LigmaCanvasTransformPreview *transform_preview)
{
  LigmaCanvasTransformPreviewPrivate *private = GET_PRIVATE (transform_preview);

  private->clip    = LIGMA_TRANSFORM_RESIZE_ADJUST;
  private->opacity = 1.0;
}

static void
ligma_canvas_transform_preview_dispose (GObject *object)
{
  LigmaCanvasTransformPreview        *transform_preview = LIGMA_CANVAS_TRANSFORM_PREVIEW (object);
  LigmaCanvasTransformPreviewPrivate *private           = GET_PRIVATE (object);

  g_clear_object (&private->node);

  ligma_canvas_transform_preview_set_pickable (transform_preview, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_canvas_transform_preview_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
  LigmaCanvasTransformPreview        *transform_preview = LIGMA_CANVAS_TRANSFORM_PREVIEW (object);
  LigmaCanvasTransformPreviewPrivate *private           = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_PICKABLE:
      ligma_canvas_transform_preview_set_pickable (transform_preview,
                                                  g_value_get_object (value));
      break;

    case PROP_TRANSFORM:
      {
        LigmaMatrix3 *transform = g_value_get_boxed (value);

        if (transform)
          private->transform = *transform;
        else
          ligma_matrix3_identity (&private->transform);
      }
      break;

    case PROP_CLIP:
      private->clip = g_value_get_enum (value);
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
ligma_canvas_transform_preview_get_property (GObject    *object,
                                            guint       property_id,
                                            GValue     *value,
                                            GParamSpec *pspec)
{
  LigmaCanvasTransformPreviewPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_PICKABLE:
      g_value_set_object (value, private->pickable);
      break;

    case PROP_TRANSFORM:
      g_value_set_boxed (value, &private->transform);
      break;

    case PROP_CLIP:
      g_value_set_enum (value, private->clip);
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
ligma_canvas_transform_preview_transform (LigmaCanvasItem        *item,
                                         cairo_rectangle_int_t *extents)
{
  LigmaCanvasTransformPreviewPrivate *private = GET_PRIVATE (item);
  gint                               x1,  y1;
  gint                               x2,  y2;
  gdouble                            tx1, ty1;
  gdouble                            tx2, ty2;

  if (! ligma_transform_resize_boundary (&private->transform,
                                        private->clip,
                                        private->x1, private->y1,
                                        private->x2, private->y2,
                                        &x1,         &y1,
                                        &x2,         &y2))
    {
      return FALSE;
    }

  ligma_canvas_item_transform_xy_f (item, x1, y1, &tx1, &ty1);
  ligma_canvas_item_transform_xy_f (item, x2, y2, &tx2, &ty2);

  extents->x      = (gint) floor (tx1);
  extents->y      = (gint) floor (ty1);
  extents->width  = (gint) ceil  (tx2) - extents->x;
  extents->height = (gint) ceil  (ty2) - extents->y;

  return TRUE;
}

static void
ligma_canvas_transform_preview_draw (LigmaCanvasItem *item,
                                    cairo_t        *cr)
{
  LigmaCanvasTransformPreview        *transform_preview = LIGMA_CANVAS_TRANSFORM_PREVIEW (item);
  LigmaCanvasTransformPreviewPrivate *private           = GET_PRIVATE (item);
  LigmaDisplayShell                  *shell             = ligma_canvas_item_get_shell (item);
  cairo_rectangle_int_t              extents;
  gdouble                            clip_x1, clip_y1;
  gdouble                            clip_x2, clip_y2;
  GeglRectangle                      bounds;
  cairo_surface_t                   *surface;
  guchar                            *surface_data;
  gint                               surface_stride;

  if (! ligma_canvas_transform_preview_transform (item, &extents))
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

  ligma_canvas_transform_preview_sync_node (transform_preview);

  gegl_node_blit (private->node_output, 1.0,
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
ligma_canvas_transform_preview_get_extents (LigmaCanvasItem *item)
{
  cairo_rectangle_int_t rectangle;

  if (ligma_canvas_transform_preview_transform (item, &rectangle))
    return cairo_region_create_rectangle (&rectangle);

  return NULL;
}

static void
ligma_canvas_transform_preview_layer_changed (LigmaLayer                  *layer,
                                             LigmaCanvasTransformPreview *transform_preview)
{
  LigmaCanvasItem *item = LIGMA_CANVAS_ITEM (transform_preview);

  ligma_canvas_item_begin_change (item);
  ligma_canvas_item_end_change   (item);
}

static void
ligma_canvas_transform_preview_set_pickable (LigmaCanvasTransformPreview *transform_preview,
                                            LigmaPickable               *pickable)
{
  LigmaCanvasTransformPreviewPrivate *private = GET_PRIVATE (transform_preview);

  if (private->pickable && LIGMA_IS_LAYER (private->pickable))
    {
      g_signal_handlers_disconnect_by_func (
        private->pickable,
        ligma_canvas_transform_preview_layer_changed,
        transform_preview);
    }

  g_set_object (&private->pickable, pickable);

  if (pickable && LIGMA_IS_LAYER (pickable))
    {
      g_signal_connect (pickable, "opacity-changed",
                        G_CALLBACK (ligma_canvas_transform_preview_layer_changed),
                        transform_preview);
      g_signal_connect (pickable, "mask-changed",
                        G_CALLBACK (ligma_canvas_transform_preview_layer_changed),
                        transform_preview);
      g_signal_connect (pickable, "apply-mask-changed",
                        G_CALLBACK (ligma_canvas_transform_preview_layer_changed),
                        transform_preview);
      g_signal_connect (pickable, "show-mask-changed",
                        G_CALLBACK (ligma_canvas_transform_preview_layer_changed),
                        transform_preview);
    }
}

static void
ligma_canvas_transform_preview_sync_node (LigmaCanvasTransformPreview *transform_preview)
{
  LigmaCanvasTransformPreviewPrivate *private    = GET_PRIVATE (transform_preview);
  LigmaCanvasItem                    *item       = LIGMA_CANVAS_ITEM (transform_preview);
  LigmaDisplayShell                  *shell      = ligma_canvas_item_get_shell (item);
  LigmaImage                         *image      = ligma_canvas_item_get_image (item);
  LigmaPickable                      *pickable   = private->pickable;
  LigmaDrawable                      *layer_mask = NULL;
  LigmaDrawable                      *mask       = NULL;
  gdouble                            opacity    = private->opacity;
  gint                               offset_x   = 0;
  gint                               offset_y   = 0;
  LigmaMatrix3                        matrix;

  if (! private->node)
    {
      private->node = gegl_node_new ();

      private->source_node =
        gegl_node_new_child (private->node,
                             "operation", "ligma:buffer-source-validate",
                             NULL);

      private->convert_format_node =
        gegl_node_new_child (private->node,
                             "operation", "gegl:convert-format",
                             NULL);

      private->layer_mask_source_node =
        gegl_node_new_child (private->node,
                             "operation", "ligma:buffer-source-validate",
                             NULL);

      private->layer_mask_opacity_node =
        gegl_node_new_child (private->node,
                             "operation", "gegl:opacity",
                             NULL);

      private->mask_source_node =
        gegl_node_new_child (private->node,
                             "operation", "ligma:buffer-source-validate",
                             NULL);

      private->mask_translate_node =
        gegl_node_new_child (private->node,
                             "operation", "gegl:translate",
                             NULL);

      private->mask_crop_node =
        gegl_node_new_child (private->node,
                             "operation", "gegl:crop",
                             "width",     0.0,
                             "height",    0.0,
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
                             "near-z",    LIGMA_TRANSFORM_NEAR_Z,
                             "sampler",   LIGMA_INTERPOLATION_NONE,
                             NULL);

      gegl_node_link_many (private->source_node,
                           private->convert_format_node,
                           private->transform_node,
                           NULL);

      gegl_node_connect_to (private->layer_mask_source_node,  "output",
                            private->layer_mask_opacity_node, "aux");

      gegl_node_link_many (private->mask_source_node,
                           private->mask_translate_node,
                           private->mask_crop_node,
                           NULL);

      private->node_pickable   = NULL;
      private->node_layer_mask = NULL;
      private->node_mask       = NULL;
      private->node_rect       = *GEGL_RECTANGLE (0, 0, 0, 0);
      private->node_opacity    = 1.0;
      ligma_matrix3_identity (&private->node_matrix);
      private->node_output     = private->transform_node;
    }

  if (LIGMA_IS_ITEM (pickable))
    {
      ligma_item_get_offset (LIGMA_ITEM (private->pickable),
                            &offset_x, &offset_y);

      if (ligma_item_mask_bounds (LIGMA_ITEM (pickable),
                                 NULL, NULL, NULL, NULL))
        {
          mask = LIGMA_DRAWABLE (ligma_image_get_mask (image));
        }

      if (LIGMA_IS_LAYER (pickable))
        {
          LigmaLayer *layer = LIGMA_LAYER (pickable);

          opacity *= ligma_layer_get_opacity (layer);

          layer_mask = LIGMA_DRAWABLE (ligma_layer_get_mask (layer));

          if (layer_mask)
            {
              if (ligma_layer_get_show_mask (layer) && ! mask)
                {
                  pickable   = LIGMA_PICKABLE (layer_mask);
                  layer_mask = NULL;
                }
              else if (! ligma_layer_get_apply_mask (layer))
                {
                  layer_mask = NULL;
                }
            }
        }
    }

  ligma_matrix3_identity (&matrix);
  ligma_matrix3_translate (&matrix, offset_x, offset_y);
  ligma_matrix3_mult (&private->transform, &matrix);
  ligma_matrix3_scale (&matrix, shell->scale_x, shell->scale_y);

  if (pickable != private->node_pickable)
    {
      GeglBuffer *buffer;

      ligma_pickable_flush (pickable);

      buffer = ligma_pickable_get_buffer (pickable);

      if (ligma_tile_handler_validate_get_assigned (buffer))
        buffer = ligma_gegl_buffer_dup (buffer);
      else
        buffer = g_object_ref (buffer);

      gegl_node_set (private->source_node,
                     "buffer", buffer,
                     NULL);
      gegl_node_set (private->convert_format_node,
                     "format", ligma_pickable_get_format_with_alpha (pickable),
                     NULL);

      g_object_unref (buffer);
    }

  if (layer_mask != private->node_layer_mask)
    {
      gegl_node_set (private->layer_mask_source_node,
                     "buffer", layer_mask ?
                                 ligma_drawable_get_buffer (layer_mask) :
                                 NULL,
                     NULL);
    }

  if (mask)
    {
      GeglRectangle  rect;

      rect.x      = offset_x;
      rect.y      = offset_y;
      rect.width  = ligma_item_get_width  (LIGMA_ITEM (private->pickable));
      rect.height = ligma_item_get_height (LIGMA_ITEM (private->pickable));

      if (mask != private->node_mask)
        {
          gegl_node_set (private->mask_source_node,
                         "buffer", ligma_drawable_get_buffer (mask),
                         NULL);
        }

      if (! gegl_rectangle_equal (&rect, &private->node_rect))
        {
          private->node_rect = rect;

          gegl_node_set (private->mask_translate_node,
                         "x", (gdouble) -rect.x,
                         "y", (gdouble) -rect.y,
                         NULL);

          gegl_node_set (private->mask_crop_node,
                         "width",  (gdouble) rect.width,
                         "height", (gdouble) rect.height,
                         NULL);
        }

      if (! private->node_mask)
        {
          gegl_node_connect_to (private->mask_crop_node, "output",
                                private->opacity_node,   "aux");
        }
    }
  else if (private->node_mask)
    {
      gegl_node_disconnect (private->opacity_node, "aux");
    }

  if (opacity != private->node_opacity)
    {
      gegl_node_set (private->opacity_node,
                     "value", opacity,
                     NULL);
    }

  if (layer_mask       != private->node_layer_mask ||
      mask             != private->node_mask       ||
      (opacity != 1.0) != (private->node_opacity != 1.0))
    {
      GeglNode *output = private->source_node;

      if (layer_mask && ! mask)
        {
          gegl_node_link (output, private->layer_mask_opacity_node);
          output = private->layer_mask_opacity_node;
        }
      else
        {
          gegl_node_disconnect (private->layer_mask_opacity_node, "input");
        }

      if (mask || (opacity != 1.0))
        {
          gegl_node_link (output, private->opacity_node);
          output = private->opacity_node;
        }
      else
        {
          gegl_node_disconnect (private->opacity_node, "input");
        }

      if (output == private->source_node)
        {
          gegl_node_disconnect (private->cache_node, "input");

          gegl_node_link (output, private->convert_format_node);
          output = private->convert_format_node;
        }
      else
        {
          gegl_node_disconnect (private->convert_format_node, "input");

          gegl_node_link (output, private->cache_node);
          output = private->cache_node;
        }

      gegl_node_link (output, private->transform_node);
      output = private->transform_node;

      if (layer_mask && mask)
        {
          gegl_node_link (output, private->layer_mask_opacity_node);
          output = private->layer_mask_opacity_node;
        }

      private->node_output = output;
    }

  if (memcmp (&matrix, &private->node_matrix, sizeof (matrix)))
    {
      private->node_matrix = matrix;

      ligma_gegl_node_set_matrix (private->transform_node, &matrix);
    }

  private->node_pickable   = pickable;
  private->node_layer_mask = layer_mask;
  private->node_mask       = mask;
  private->node_opacity    = opacity;
}


/* public functions */


LigmaCanvasItem *
ligma_canvas_transform_preview_new (LigmaDisplayShell  *shell,
                                   LigmaPickable      *pickable,
                                   const LigmaMatrix3 *transform,
                                   gdouble            x1,
                                   gdouble            y1,
                                   gdouble            x2,
                                   gdouble            y2)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (LIGMA_IS_PICKABLE (pickable), NULL);
  g_return_val_if_fail (transform != NULL, NULL);

  return g_object_new (LIGMA_TYPE_CANVAS_TRANSFORM_PREVIEW,
                       "shell",     shell,
                       "pickable",  pickable,
                       "transform", transform,
                       "x1",        x1,
                       "y1",        y1,
                       "x2",        x2,
                       "y2",        y2,
                       NULL);
}
