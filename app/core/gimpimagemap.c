/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/* This file contains the code necessary for generating on canvas
 * previews, either by connecting a function to process the pixels or
 * by connecting a specified GEGL operation to do the processing. It
 * keeps an undo buffer to allow direct modification of the pixel data
 * (so that it will show up in the projection) and it will restore the
 * source in case the mapping procedure was cancelled.
 *
 * To create a tool that uses this, see /tools/gimpimagemaptool.c for
 * the interface and /tools/gimpcolorbalancetool.c for an example of
 * using that interface.
 *
 * Note that when talking about on canvas preview, we are speaking
 * about non destructive image editing where the operation is previewd
 * before being applied.
 */

#include "config.h"

#include <gegl.h>

#include "core-types.h"

#include "gegl/gimpapplicator.h"
#include "gegl/gimp-gegl-utils.h"

#include "gimpdrawable.h"
#include "gimpdrawable-filter.h"
#include "gimpfilter.h"
#include "gimpimage.h"
#include "gimpimagemap.h"
#include "gimpmarshal.h"
#include "gimppickable.h"
#include "gimpviewable.h"
#include "gimpchannel.h"
#include "gimpprogress.h"


enum
{
  FLUSH,
  LAST_SIGNAL
};


struct _GimpImageMap
{
  GimpObject      parent_instance;

  GimpDrawable   *drawable;
  gchar          *undo_desc;
  GeglNode       *operation;
  gchar          *stock_id;

  GimpFilter     *filter;
  GeglNode       *translate;
  GimpApplicator *applicator;
};


static void   gimp_image_map_pickable_iface_init (GimpPickableInterface *iface);

static void            gimp_image_map_dispose         (GObject             *object);
static void            gimp_image_map_finalize        (GObject             *object);

static GimpImage     * gimp_image_map_get_image       (GimpPickable        *pickable);
static const Babl    * gimp_image_map_get_format      (GimpPickable        *pickable);
static const Babl    * gimp_image_map_get_format_with_alpha
                                                      (GimpPickable        *pickable);
static GeglBuffer    * gimp_image_map_get_buffer      (GimpPickable        *pickable);
static gboolean        gimp_image_map_get_pixel_at    (GimpPickable        *pickable,
                                                       gint                 x,
                                                       gint                 y,
                                                       const Babl          *format,
                                                       gpointer             pixel);


G_DEFINE_TYPE_WITH_CODE (GimpImageMap, gimp_image_map, GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PICKABLE,
                                                gimp_image_map_pickable_iface_init))

#define parent_class gimp_image_map_parent_class

static guint image_map_signals[LAST_SIGNAL] = { 0 };


static void
gimp_image_map_class_init (GimpImageMapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  image_map_signals[FLUSH] =
    g_signal_new ("flush",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageMapClass, flush),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->dispose  = gimp_image_map_dispose;
  object_class->finalize = gimp_image_map_finalize;
}

static void
gimp_image_map_pickable_iface_init (GimpPickableInterface *iface)
{
  iface->get_image             = gimp_image_map_get_image;
  iface->get_format            = gimp_image_map_get_format;
  iface->get_format_with_alpha = gimp_image_map_get_format_with_alpha;
  iface->get_buffer            = gimp_image_map_get_buffer;
  iface->get_pixel_at          = gimp_image_map_get_pixel_at;
}

static void
gimp_image_map_init (GimpImageMap *image_map)
{
}

static void
gimp_image_map_dispose (GObject *object)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (object);

  if (image_map->drawable)
    gimp_viewable_preview_thaw (GIMP_VIEWABLE (image_map->drawable));

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_image_map_finalize (GObject *object)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (object);

  if (image_map->undo_desc)
    {
      g_free (image_map->undo_desc);
      image_map->undo_desc = NULL;
    }

  if (image_map->operation)
    {
      g_object_unref (image_map->operation);
      image_map->operation = NULL;
    }

  if (image_map->stock_id)
    {
      g_free (image_map->stock_id);
      image_map->stock_id = NULL;
    }

  if (image_map->filter)
    {
      g_object_unref (image_map->filter);
      image_map->filter = NULL;
    }

  if (image_map->applicator)
    {
      g_object_unref (image_map->applicator);
      image_map->applicator = NULL;
    }

  if (image_map->drawable)
    {
      g_object_unref (image_map->drawable);
      image_map->drawable = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GimpImage *
gimp_image_map_get_image (GimpPickable *pickable)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (pickable);

  return gimp_pickable_get_image (GIMP_PICKABLE (image_map->drawable));
}

static const Babl *
gimp_image_map_get_format (GimpPickable *pickable)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (pickable);

  return gimp_pickable_get_format (GIMP_PICKABLE (image_map->drawable));
}

static const Babl *
gimp_image_map_get_format_with_alpha (GimpPickable *pickable)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (pickable);

  return gimp_pickable_get_format_with_alpha (GIMP_PICKABLE (image_map->drawable));
}

static GeglBuffer *
gimp_image_map_get_buffer (GimpPickable *pickable)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (pickable);

  return gimp_pickable_get_buffer (GIMP_PICKABLE (image_map->drawable));
}

static gboolean
gimp_image_map_get_pixel_at (GimpPickable *pickable,
                             gint          x,
                             gint          y,
                             const Babl   *format,
                             gpointer      pixel)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (pickable);

  return gimp_pickable_get_pixel_at (GIMP_PICKABLE (image_map->drawable),
                                     x, y, format, pixel);
}

GimpImageMap *
gimp_image_map_new (GimpDrawable *drawable,
                    const gchar  *undo_desc,
                    GeglNode     *operation,
                    const gchar  *stock_id)
{
  GimpImageMap *image_map;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GEGL_IS_NODE (operation), NULL);

  image_map = g_object_new (GIMP_TYPE_IMAGE_MAP, NULL);

  image_map->drawable  = g_object_ref (drawable);
  image_map->undo_desc = g_strdup (undo_desc);

  image_map->operation = g_object_ref (operation);
  image_map->stock_id  = g_strdup (stock_id);

  gimp_viewable_preview_freeze (GIMP_VIEWABLE (drawable));

  return image_map;
}

void
gimp_image_map_apply (GimpImageMap *image_map)
{
  GimpImage     *image;
  GimpChannel   *mask;
  GeglRectangle  rect;

  g_return_if_fail (GIMP_IS_IMAGE_MAP (image_map));

  /*  Make sure the drawable is still valid  */
  if (! gimp_item_is_attached (GIMP_ITEM (image_map->drawable)))
    return;

  /*  The application should occur only within selection bounds  */
  if (! gimp_item_mask_intersect (GIMP_ITEM (image_map->drawable),
                                  &rect.x, &rect.y,
                                  &rect.width, &rect.height))
    return;

  if (! image_map->filter)
    {
      GeglNode *filter_node;
      GeglNode *filter_output;
      GeglNode *input;

      image_map->filter = gimp_filter_new (image_map->undo_desc);
      gimp_viewable_set_stock_id (GIMP_VIEWABLE (image_map->filter),
                                  image_map->stock_id);

      filter_node = gimp_filter_get_node (image_map->filter);

      gegl_node_add_child (filter_node, image_map->operation);

      image_map->applicator = gimp_applicator_new (filter_node);

      gimp_filter_set_applicator (image_map->filter,
                                  image_map->applicator);

      image_map->translate = gegl_node_new_child (filter_node,
                                                  "operation", "gegl:translate",
                                                  NULL);

      input  = gegl_node_get_input_proxy  (filter_node, "input");

      if (gegl_node_has_pad (image_map->operation, "input") &&
          gegl_node_has_pad (image_map->operation, "output"))
        {
          /*  if there are input and output pads we probably have a
           *  filter OP, connect it on both ends.
           */
          gegl_node_link_many (input,
                               image_map->translate,
                               image_map->operation,
                               NULL);

          filter_output = image_map->operation;
        }
      else if (gegl_node_has_pad (image_map->operation, "output"))
        {
          /*  if there is only an output pad we probably have a
           *  source OP, blend its result on top of the original
           *  pixels.
           */
          GeglNode *over = gegl_node_new_child (filter_node,
                                                "operation", "gegl:over",
                                                NULL);

          gegl_node_link_many (input,
                               image_map->translate,
                               over,
                               NULL);

          gegl_node_connect_to (image_map->operation, "output",
                                over,                 "aux");

          filter_output = over;
        }
      else
        {
          /* otherwise we just construct a silly nop pipleline
           */
          gegl_node_link_many (input,
                               image_map->translate,
                               NULL);

          filter_output = image_map->translate;
        }

      gegl_node_connect_to (filter_output, "output",
                            filter_node,   "aux");

      gimp_applicator_set_mode (image_map->applicator,
                                GIMP_OPACITY_OPAQUE,
                                GIMP_REPLACE_MODE);
    }

  if (! gimp_drawable_has_filter (image_map->drawable, image_map->filter))
    gimp_drawable_add_filter (image_map->drawable, image_map->filter);

  gegl_node_set (image_map->translate,
                 "x", (gdouble) -rect.x,
                 "y", (gdouble) -rect.y,
                 NULL);

  gimp_applicator_set_apply_offset (image_map->applicator,
                                    rect.x, rect.y);

  gimp_applicator_set_affect (image_map->applicator,
                              gimp_drawable_get_active_mask (image_map->drawable));

  image = gimp_item_get_image (GIMP_ITEM (image_map->drawable));
  mask  = gimp_image_get_mask (image);

  if (gimp_channel_is_empty (mask))
    {
      gimp_applicator_set_mask_buffer (image_map->applicator, NULL);
    }
  else
    {
      GeglBuffer *mask_buffer;
      gint        offset_x, offset_y;

      mask_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));
      gimp_item_get_offset (GIMP_ITEM (image_map->drawable),
                            &offset_x, &offset_y);

      gimp_applicator_set_mask_buffer (image_map->applicator, mask_buffer);
      gimp_applicator_set_mask_offset (image_map->applicator,
                                       offset_x, offset_y);
    }

  gimp_drawable_update (image_map->drawable,
                        rect.x, rect.y,
                        rect.width, rect.height);

  g_signal_emit (image_map, image_map_signals[FLUSH], 0);
}

void
gimp_image_map_commit (GimpImageMap *image_map,
                       GimpProgress *progress)
{
  g_return_if_fail (GIMP_IS_IMAGE_MAP (image_map));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  if (gimp_drawable_has_filter (image_map->drawable, image_map->filter))
    {
      gimp_drawable_remove_filter (image_map->drawable, image_map->filter);

      gimp_drawable_merge_filter (image_map->drawable, image_map->filter,
                                  progress,
                                  image_map->undo_desc);

      g_signal_emit (image_map, image_map_signals[FLUSH], 0);
    }
}

void
gimp_image_map_abort (GimpImageMap *image_map)
{
  g_return_if_fail (GIMP_IS_IMAGE_MAP (image_map));

  if (gimp_drawable_has_filter (image_map->drawable, image_map->filter))
    {
      GeglRectangle rect;

      gimp_drawable_remove_filter (image_map->drawable, image_map->filter);

      if (gimp_item_is_attached (GIMP_ITEM (image_map->drawable)) &&
          gimp_item_mask_intersect (GIMP_ITEM (image_map->drawable),
                                    &rect.x, &rect.y,
                                    &rect.width, &rect.height))
        {
          gimp_drawable_update (image_map->drawable,
                                rect.x, rect.y,
                                rect.width, rect.height);

          g_signal_emit (image_map, image_map_signals[FLUSH], 0);
        }
    }
}
