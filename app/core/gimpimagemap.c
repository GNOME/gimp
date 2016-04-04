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
 * previews, by connecting a specified GEGL operation to do the
 * processing. It uses drawable filters that allow for non-destructive
 * manupulation of drawable data, with live preview on screen.
 *
 * To create a tool that uses this, see /tools/gimpimagemaptool.c for
 * the interface and /tools/gimpcolorbalancetool.c for an example of
 * using that interface.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimpapplicator.h"

#include "gimpchannel.h"
#include "gimpdrawable-filter.h"
#include "gimpimage.h"
#include "gimpimagemap.h"
#include "gimpmarshal.h"
#include "gimpprogress.h"


enum
{
  FLUSH,
  LAST_SIGNAL
};


struct _GimpImageMap
{
  GimpObject            parent_instance;

  GimpDrawable         *drawable;
  gchar                *undo_desc;
  GeglNode             *operation;
  gchar                *icon_name;

  GimpImageMapRegion    region;
  gboolean              preview_enabled;
  GimpOrientationType   preview_orientation;
  gdouble               preview_percent;
  gdouble               opacity;
  GimpLayerModeEffects  paint_mode;
  gboolean              gamma_hack;

  GeglRectangle         filter_area;

  GimpFilter           *filter;
  GeglNode             *translate;
  GeglNode             *crop;
  GeglNode             *cast_before;
  GeglNode             *cast_after;
  GimpApplicator       *applicator;
};


static void       gimp_image_map_dispose         (GObject             *object);
static void       gimp_image_map_finalize        (GObject             *object);

static void       gimp_image_map_sync_region     (GimpImageMap        *image_map);
static void       gimp_image_map_sync_preview    (GimpImageMap        *image_map,
                                                  gboolean             old_enabled,
                                                  GimpOrientationType  old_orientation,
                                                  gdouble              old_percent);
static void       gimp_image_map_sync_mode       (GimpImageMap        *image_map);
static void       gimp_image_map_sync_affect     (GimpImageMap        *image_map);
static void       gimp_image_map_sync_gamma_hack (GimpImageMap        *image_map);

static gboolean   gimp_image_map_is_filtering    (GimpImageMap        *image_map);
static gboolean   gimp_image_map_add_filter      (GimpImageMap        *image_map);
static gboolean   gimp_image_map_remove_filter   (GimpImageMap        *image_map);

static void       gimp_image_map_update_drawable (GimpImageMap        *image_map,
                                                  const GeglRectangle *area);

static void       gimp_image_map_affect_changed  (GimpImage           *image,
                                                  GimpChannelType      channel,
                                                  GimpImageMap        *image_map);


G_DEFINE_TYPE (GimpImageMap, gimp_image_map, GIMP_TYPE_OBJECT)

#define parent_class gimp_image_map_parent_class

static guint image_map_signals[LAST_SIGNAL] = { 0, };


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
gimp_image_map_init (GimpImageMap *image_map)
{
  image_map->region              = GIMP_IMAGE_MAP_REGION_SELECTION;
  image_map->preview_orientation = GIMP_ORIENTATION_HORIZONTAL;
  image_map->preview_percent     = 1.0;
  image_map->opacity             = GIMP_OPACITY_OPAQUE;
  image_map->paint_mode          = GIMP_REPLACE_MODE;
}

static void
gimp_image_map_dispose (GObject *object)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (object);

  if (image_map->drawable)
    {
      gimp_image_map_remove_filter (image_map);
      gimp_viewable_preview_thaw (GIMP_VIEWABLE (image_map->drawable));
    }

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

  if (image_map->icon_name)
    {
      g_free (image_map->icon_name);
      image_map->icon_name = NULL;
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

GimpImageMap *
gimp_image_map_new (GimpDrawable *drawable,
                    const gchar  *undo_desc,
                    GeglNode     *operation,
                    const gchar  *icon_name)
{
  GimpImageMap *image_map;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GEGL_IS_NODE (operation), NULL);
  g_return_val_if_fail (gegl_node_has_pad (operation, "output"), NULL);

  image_map = g_object_new (GIMP_TYPE_IMAGE_MAP, NULL);

  image_map->drawable  = g_object_ref (drawable);
  image_map->undo_desc = g_strdup (undo_desc);

  image_map->operation = g_object_ref (operation);
  image_map->icon_name = g_strdup (icon_name);

  gimp_viewable_preview_freeze (GIMP_VIEWABLE (drawable));

  return image_map;
}

void
gimp_image_map_set_region (GimpImageMap       *image_map,
                           GimpImageMapRegion  region)
{
  g_return_if_fail (GIMP_IS_IMAGE_MAP (image_map));

  if (region != image_map->region)
    {
      image_map->region = region;

      gimp_image_map_sync_region (image_map);
    }
}

void
gimp_image_map_set_preview (GimpImageMap        *image_map,
                            gboolean             enabled,
                            GimpOrientationType  orientation,
                            gdouble              percent)
{
  g_return_if_fail (GIMP_IS_IMAGE_MAP (image_map));

  percent = CLAMP (percent, 0.0, 1.0);

  if (enabled     != image_map->preview_enabled     ||
      orientation != image_map->preview_orientation ||
      percent     != image_map->preview_percent)
    {
      gboolean            old_enabled     = image_map->preview_enabled;
      GimpOrientationType old_orientation = image_map->preview_orientation;
      gdouble             old_percent     = image_map->preview_percent;

      image_map->preview_enabled     = enabled;
      image_map->preview_orientation = orientation;
      image_map->preview_percent     = percent;

      gimp_image_map_sync_preview (image_map,
                                   old_enabled, old_orientation, old_percent);
    }
}

void
gimp_image_map_set_mode (GimpImageMap         *image_map,
                         gdouble               opacity,
                         GimpLayerModeEffects  paint_mode)
{
  g_return_if_fail (GIMP_IS_IMAGE_MAP (image_map));

  if (opacity    != image_map->opacity ||
      paint_mode != image_map->paint_mode)
    {
      image_map->opacity    = opacity;
      image_map->paint_mode = paint_mode;

      gimp_image_map_sync_mode (image_map);
    }
}

void
gimp_image_map_set_gamma_hack (GimpImageMap *image_map,
                               gboolean      gamma_hack)
{
  g_return_if_fail (GIMP_IS_IMAGE_MAP (image_map));

  if (gamma_hack != image_map->gamma_hack)
    {
      image_map->gamma_hack = gamma_hack;

      gimp_image_map_sync_gamma_hack (image_map);
    }
}

void
gimp_image_map_apply (GimpImageMap        *image_map,
                      const GeglRectangle *area)
{
  GimpImage     *image;
  GimpChannel   *mask;
  GeglRectangle  update_area;

  g_return_if_fail (GIMP_IS_IMAGE_MAP (image_map));

  /*  Make sure the drawable is still valid  */
  if (! gimp_item_is_attached (GIMP_ITEM (image_map->drawable)))
    {
      gimp_image_map_remove_filter (image_map);
      return;
    }

  /*  The application should occur only within selection bounds  */
  if (! gimp_item_mask_intersect (GIMP_ITEM (image_map->drawable),
                                  &image_map->filter_area.x,
                                  &image_map->filter_area.y,
                                  &image_map->filter_area.width,
                                  &image_map->filter_area.height))
    {
      gimp_image_map_remove_filter (image_map);
      return;
    }

  /* Only update "area" because only that has changed */
  if (! area)
    {
      update_area = image_map->filter_area;
    }
  else if (! gimp_rectangle_intersect (area->x,
                                       area->y,
                                       area->width,
                                       area->height,
                                       image_map->filter_area.x,
                                       image_map->filter_area.y,
                                       image_map->filter_area.width,
                                       image_map->filter_area.height,
                                       &update_area.x,
                                       &update_area.y,
                                       &update_area.width,
                                       &update_area.height))
    {
      /* Bail out, but don't remove the filter */
      return;
    }

  if (! image_map->filter)
    {
      GeglNode *filter_node;

      image_map->filter = gimp_filter_new (image_map->undo_desc);
      gimp_viewable_set_icon_name (GIMP_VIEWABLE (image_map->filter),
                                   image_map->icon_name);

      filter_node = gimp_filter_get_node (image_map->filter);

      gegl_node_add_child (filter_node, image_map->operation);

      image_map->applicator =
        gimp_applicator_new (filter_node,
                             gimp_drawable_get_linear (image_map->drawable),
                             TRUE, TRUE);

      gimp_filter_set_applicator (image_map->filter,
                                  image_map->applicator);

      image_map->translate = gegl_node_new_child (filter_node,
                                                  "operation", "gegl:translate",
                                                  NULL);
      image_map->crop = gegl_node_new_child (filter_node,
                                             "operation", "gegl:crop",
                                             NULL);

      image_map->cast_before = gegl_node_new_child (filter_node,
                                                    "operation", "gegl:nop",
                                                    NULL);
      image_map->cast_after = gegl_node_new_child (filter_node,
                                                   "operation", "gegl:nop",
                                                   NULL);

      gimp_image_map_sync_region (image_map);
      gimp_image_map_sync_preview (image_map,
                                   image_map->preview_enabled,
                                   image_map->preview_orientation,
                                   image_map->preview_percent);
      gimp_image_map_sync_mode (image_map);
      gimp_image_map_sync_gamma_hack (image_map);

      if (gegl_node_has_pad (image_map->operation, "input"))
        {
          GeglNode *input = gegl_node_get_input_proxy (filter_node, "input");

          gegl_node_link_many (input,
                               image_map->translate,
                               image_map->crop,
                               image_map->cast_before,
                               image_map->operation,
                               NULL);
        }

      gegl_node_link_many (image_map->operation,
                           image_map->cast_after,
                           NULL);

      gegl_node_connect_to (image_map->cast_after, "output",
                            filter_node,           "aux");
    }

  gimp_image_map_sync_affect (image_map);

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
                                       -offset_x, -offset_y);
    }

  gimp_image_map_add_filter (image_map);
  gimp_image_map_update_drawable (image_map, &update_area);
}

gboolean
gimp_image_map_commit (GimpImageMap *image_map,
                       GimpProgress *progress,
                       gboolean      cancellable)
{
  gboolean success = TRUE;

  g_return_val_if_fail (GIMP_IS_IMAGE_MAP (image_map), FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);

  if (gimp_image_map_is_filtering (image_map))
    {
      success = gimp_drawable_merge_filter (image_map->drawable,
                                            image_map->filter,
                                            progress,
                                            image_map->undo_desc,
                                            cancellable);

      gimp_image_map_remove_filter (image_map);

      g_signal_emit (image_map, image_map_signals[FLUSH], 0);
    }

  return success;
}

void
gimp_image_map_abort (GimpImageMap *image_map)
{
  g_return_if_fail (GIMP_IS_IMAGE_MAP (image_map));

  if (gimp_image_map_remove_filter (image_map))
    {
      gimp_image_map_update_drawable (image_map, &image_map->filter_area);
    }
}


/*  private functions  */

static void
gimp_image_map_sync_region (GimpImageMap *image_map)
{
  if (image_map->applicator)
    {
      if (image_map->region == GIMP_IMAGE_MAP_REGION_SELECTION)
        {
          gegl_node_set (image_map->translate,
                         "x", (gdouble) -image_map->filter_area.x,
                         "y", (gdouble) -image_map->filter_area.y,
                         NULL);

          gegl_node_set (image_map->crop,
                         "width",  (gdouble) image_map->filter_area.width,
                         "height", (gdouble) image_map->filter_area.height,
                         NULL);

          gimp_applicator_set_apply_offset (image_map->applicator,
                                            image_map->filter_area.x,
                                            image_map->filter_area.y);
        }
      else
        {
          GimpItem *item   = GIMP_ITEM (image_map->drawable);
          gdouble   width  = gimp_item_get_width (item);
          gdouble   height = gimp_item_get_height (item);

          gegl_node_set (image_map->translate,
                         "x", (gdouble) 0.0,
                         "y", (gdouble) 0.0,
                         NULL);

          gegl_node_set (image_map->crop,
                         "width",  width,
                         "height", height,
                         NULL);

          gimp_applicator_set_apply_offset (image_map->applicator, 0, 0);
        }
    }
}

static void
gimp_image_map_get_preview_rect (GimpImageMap        *image_map,
                                 gboolean             enabled,
                                 GimpOrientationType  orientation,
                                 gdouble              percent,
                                 GeglRectangle       *rect)
{
  rect->x      = 0;
  rect->y      = 0;
  rect->width  = gimp_item_get_width  (GIMP_ITEM (image_map->drawable));
  rect->height = gimp_item_get_height (GIMP_ITEM (image_map->drawable));

  if (enabled)
    {
      if (orientation == GIMP_ORIENTATION_HORIZONTAL)
        rect->width *= percent;
      else
        rect->height *= percent;
    }
}

static void
gimp_image_map_sync_preview (GimpImageMap        *image_map,
                             gboolean             old_enabled,
                             GimpOrientationType  old_orientation,
                             gdouble              old_percent)
{
  if (image_map->applicator)
    {
      GeglRectangle old_rect;
      GeglRectangle new_rect;

      gimp_image_map_get_preview_rect (image_map,
                                       old_enabled,
                                       old_orientation,
                                       old_percent,
                                       &old_rect);

      gimp_image_map_get_preview_rect (image_map,
                                       image_map->preview_enabled,
                                       image_map->preview_orientation,
                                       image_map->preview_percent,
                                       &new_rect);

      gimp_applicator_set_preview (image_map->applicator,
                                   image_map->preview_enabled,
                                   &new_rect);

      if (old_rect.width  != new_rect.width ||
          old_rect.height != new_rect.height)
        {
          cairo_region_t *region;
          gint            n_rects;
          gint            i;

          region = cairo_region_create_rectangle ((cairo_rectangle_int_t *)
                                                  &old_rect);
          cairo_region_xor_rectangle (region,
                                      (cairo_rectangle_int_t *) &new_rect);

          n_rects = cairo_region_num_rectangles (region);

          for (i = 0; i < n_rects; i++)
            {
              cairo_rectangle_int_t rect;

              cairo_region_get_rectangle (region, i, &rect);

              gimp_image_map_update_drawable (image_map,
                                              (const GeglRectangle *) &rect);
            }

          cairo_region_destroy (region);
        }
    }
}

static void
gimp_image_map_sync_mode (GimpImageMap *image_map)
{
  if (image_map->applicator)
    gimp_applicator_set_mode (image_map->applicator,
                              image_map->opacity,
                              image_map->paint_mode);
}

static void
gimp_image_map_sync_affect (GimpImageMap *image_map)
{
  if (image_map->applicator)
    {
      GimpComponentMask active_mask;

      active_mask = gimp_drawable_get_active_mask (image_map->drawable);

      /*  don't let the filter affect the drawable projection's alpha,
       *  because it can't affect the drawable buffer's alpha either
       *  when finally merged (see bug #699279)
       */
      if (! gimp_drawable_has_alpha (image_map->drawable))
        active_mask &= ~GIMP_COMPONENT_MASK_ALPHA;

      gimp_applicator_set_affect (image_map->applicator, active_mask);
    }
}

static void
gimp_image_map_sync_gamma_hack (GimpImageMap *image_map)
{
  if (image_map->applicator)
    {
      if (image_map->gamma_hack)
        {
          const Babl *drawable_format;
          const Babl *cast_format;

          drawable_format =
            gimp_drawable_get_format_with_alpha (image_map->drawable);

          cast_format =
            gimp_babl_format (gimp_babl_format_get_base_type (drawable_format),
                              gimp_babl_precision (gimp_babl_format_get_component_type (drawable_format),
                                                   ! gimp_babl_format_get_linear (drawable_format)),
                              TRUE);

          gegl_node_set (image_map->cast_before,
                         "operation",     "gegl:cast-format",
                         "input-format",  drawable_format,
                         "output-format", cast_format,
                         NULL);

          gegl_node_set (image_map->cast_after,
                         "operation",     "gegl:cast-format",
                         "input-format",  cast_format,
                         "output-format", drawable_format,
                         NULL);
        }
      else
        {
          gegl_node_set (image_map->cast_before,
                         "operation", "gegl:nop",
                         NULL);

          gegl_node_set (image_map->cast_after,
                         "operation", "gegl:nop",
                         NULL);
        }
    }
}

static gboolean
gimp_image_map_is_filtering (GimpImageMap *image_map)
{
  if (image_map->filter &&
      gimp_drawable_has_filter (image_map->drawable, image_map->filter))
    {
      return TRUE;
    }

  return FALSE;
}

static gboolean
gimp_image_map_add_filter (GimpImageMap *image_map)
{
  if (! gimp_image_map_is_filtering (image_map))
    {
      if (image_map->filter)
        {
          GimpImage *image;

          gimp_drawable_add_filter (image_map->drawable, image_map->filter);

          image = gimp_item_get_image (GIMP_ITEM (image_map->drawable));

          g_signal_connect (image, "component-active-changed",
                            G_CALLBACK (gimp_image_map_affect_changed),
                            image_map);

          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
gimp_image_map_remove_filter (GimpImageMap *image_map)
{
  if (gimp_image_map_is_filtering (image_map))
    {
      GimpImage *image = gimp_item_get_image (GIMP_ITEM (image_map->drawable));

      g_signal_handlers_disconnect_by_func (image,
                                            gimp_image_map_affect_changed,
                                            image_map);

      gimp_drawable_remove_filter (image_map->drawable, image_map->filter);

      return TRUE;
    }

  return FALSE;
}

static void
gimp_image_map_update_drawable (GimpImageMap        *image_map,
                                const GeglRectangle *area)
{
  gimp_drawable_update (image_map->drawable,
                        area->x,
                        area->y,
                        area->width,
                        area->height);

  g_signal_emit (image_map, image_map_signals[FLUSH], 0);
}

static void
gimp_image_map_affect_changed (GimpImage       *image,
                               GimpChannelType  channel,
                               GimpImageMap    *image_map)
{
  gimp_image_map_sync_affect (image_map);
  gimp_image_map_update_drawable (image_map, &image_map->filter_area);
}
