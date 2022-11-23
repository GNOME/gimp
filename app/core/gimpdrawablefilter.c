/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* This file contains the code necessary for generating on canvas
 * previews, by connecting a specified GEGL operation to do the
 * processing. It uses drawable filters that allow for non-destructive
 * manipulation of drawable data, with live preview on screen.
 *
 * To create a tool that uses this, see app/tools/ligmafiltertool.c for
 * the interface and e.g. app/tools/ligmacolorbalancetool.c for an
 * example of using that interface.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"

#include "core-types.h"

#include "gegl/ligma-babl.h"
#include "gegl/ligmaapplicator.h"
#include "gegl/ligma-gegl-utils.h"

#include "ligmachannel.h"
#include "ligmadrawable-filters.h"
#include "ligmadrawablefilter.h"
#include "ligmaimage.h"
#include "ligmalayer.h"
#include "ligmaprogress.h"


enum
{
  FLUSH,
  LAST_SIGNAL
};


struct _LigmaDrawableFilter
{
  LigmaFilter              parent_instance;

  LigmaDrawable           *drawable;
  GeglNode               *operation;

  gboolean                has_input;

  gboolean                clip;
  LigmaFilterRegion        region;
  gboolean                crop_enabled;
  GeglRectangle           crop_rect;
  gboolean                preview_enabled;
  gboolean                preview_split_enabled;
  LigmaAlignmentType       preview_split_alignment;
  gint                    preview_split_position;
  gdouble                 opacity;
  LigmaLayerMode           paint_mode;
  LigmaLayerColorSpace     blend_space;
  LigmaLayerColorSpace     composite_space;
  LigmaLayerCompositeMode  composite_mode;
  gboolean                add_alpha;
  gboolean                color_managed;
  gboolean                gamma_hack;

  gboolean                override_constraints;

  GeglRectangle           filter_area;
  gboolean                filter_clip;

  GeglNode               *translate;
  GeglNode               *crop_before;
  GeglNode               *cast_before;
  GeglNode               *cast_after;
  GeglNode               *crop_after;
  LigmaApplicator         *applicator;
};


static void       ligma_drawable_filter_dispose               (GObject             *object);
static void       ligma_drawable_filter_finalize              (GObject             *object);

static void       ligma_drawable_filter_sync_active           (LigmaDrawableFilter  *filter);
static void       ligma_drawable_filter_sync_clip             (LigmaDrawableFilter  *filter,
                                                              gboolean             sync_region);
static void       ligma_drawable_filter_sync_region           (LigmaDrawableFilter  *filter);
static void       ligma_drawable_filter_sync_crop             (LigmaDrawableFilter  *filter,
                                                              gboolean             old_crop_enabled,
                                                              const GeglRectangle *old_crop_rect,
                                                              gboolean             old_preview_split_enabled,
                                                              LigmaAlignmentType    old_preview_split_alignment,
                                                              gint                 old_preview_split_position,
                                                              gboolean             update);
static void       ligma_drawable_filter_sync_opacity          (LigmaDrawableFilter  *filter);
static void       ligma_drawable_filter_sync_mode             (LigmaDrawableFilter  *filter);
static void       ligma_drawable_filter_sync_affect           (LigmaDrawableFilter  *filter);
static void       ligma_drawable_filter_sync_format           (LigmaDrawableFilter  *filter);
static void       ligma_drawable_filter_sync_mask             (LigmaDrawableFilter  *filter);
static void       ligma_drawable_filter_sync_gamma_hack       (LigmaDrawableFilter  *filter);

static gboolean   ligma_drawable_filter_is_added              (LigmaDrawableFilter  *filter);
static gboolean   ligma_drawable_filter_is_active             (LigmaDrawableFilter  *filter);
static gboolean   ligma_drawable_filter_add_filter            (LigmaDrawableFilter  *filter);
static gboolean   ligma_drawable_filter_remove_filter         (LigmaDrawableFilter  *filter);

static void       ligma_drawable_filter_update_drawable       (LigmaDrawableFilter  *filter,
                                                              const GeglRectangle *area);

static void       ligma_drawable_filter_affect_changed        (LigmaImage           *image,
                                                              LigmaChannelType      channel,
                                                              LigmaDrawableFilter  *filter);
static void       ligma_drawable_filter_mask_changed          (LigmaImage           *image,
                                                              LigmaDrawableFilter  *filter);
static void       ligma_drawable_filter_lock_position_changed (LigmaDrawable        *drawable,
                                                              LigmaDrawableFilter  *filter);
static void       ligma_drawable_filter_format_changed        (LigmaDrawable        *drawable,
                                                              LigmaDrawableFilter  *filter);
static void       ligma_drawable_filter_drawable_removed      (LigmaDrawable        *drawable,
                                                              LigmaDrawableFilter  *filter);
static void       ligma_drawable_filter_lock_alpha_changed    (LigmaLayer           *layer,
                                                              LigmaDrawableFilter  *filter);


G_DEFINE_TYPE (LigmaDrawableFilter, ligma_drawable_filter, LIGMA_TYPE_FILTER)

#define parent_class ligma_drawable_filter_parent_class

static guint drawable_filter_signals[LAST_SIGNAL] = { 0, };


static void
ligma_drawable_filter_class_init (LigmaDrawableFilterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  drawable_filter_signals[FLUSH] =
    g_signal_new ("flush",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDrawableFilterClass, flush),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->dispose  = ligma_drawable_filter_dispose;
  object_class->finalize = ligma_drawable_filter_finalize;
}

static void
ligma_drawable_filter_init (LigmaDrawableFilter *drawable_filter)
{
  drawable_filter->clip                    = TRUE;
  drawable_filter->region                  = LIGMA_FILTER_REGION_SELECTION;
  drawable_filter->preview_enabled         = TRUE;
  drawable_filter->preview_split_enabled   = FALSE;
  drawable_filter->preview_split_alignment = LIGMA_ALIGN_LEFT;
  drawable_filter->preview_split_position  = 0;
  drawable_filter->opacity                 = LIGMA_OPACITY_OPAQUE;
  drawable_filter->paint_mode              = LIGMA_LAYER_MODE_REPLACE;
  drawable_filter->blend_space             = LIGMA_LAYER_COLOR_SPACE_AUTO;
  drawable_filter->composite_space         = LIGMA_LAYER_COLOR_SPACE_AUTO;
  drawable_filter->composite_mode          = LIGMA_LAYER_COMPOSITE_AUTO;
}

static void
ligma_drawable_filter_dispose (GObject *object)
{
  LigmaDrawableFilter *drawable_filter = LIGMA_DRAWABLE_FILTER (object);

  if (drawable_filter->drawable)
    ligma_drawable_filter_remove_filter (drawable_filter);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_drawable_filter_finalize (GObject *object)
{
  LigmaDrawableFilter *drawable_filter = LIGMA_DRAWABLE_FILTER (object);

  g_clear_object (&drawable_filter->operation);
  g_clear_object (&drawable_filter->applicator);
  g_clear_object (&drawable_filter->drawable);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

LigmaDrawableFilter *
ligma_drawable_filter_new (LigmaDrawable *drawable,
                          const gchar  *undo_desc,
                          GeglNode     *operation,
                          const gchar  *icon_name)
{
  LigmaDrawableFilter *filter;
  GeglNode           *node;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)), NULL);
  g_return_val_if_fail (GEGL_IS_NODE (operation), NULL);
  g_return_val_if_fail (gegl_node_has_pad (operation, "output"), NULL);

  filter = g_object_new (LIGMA_TYPE_DRAWABLE_FILTER,
                         "name",      undo_desc,
                         "icon-name", icon_name,
                         NULL);

  filter->drawable  = g_object_ref (drawable);
  filter->operation = g_object_ref (operation);

  node = ligma_filter_get_node (LIGMA_FILTER (filter));

  gegl_node_add_child (node, operation);
  ligma_gegl_node_set_underlying_operation (node, operation);

  filter->applicator = ligma_applicator_new (node);

  ligma_filter_set_applicator (LIGMA_FILTER (filter), filter->applicator);

  ligma_applicator_set_cache (filter->applicator, TRUE);

  filter->has_input = gegl_node_has_pad (filter->operation, "input");

  if (filter->has_input)
    {
      GeglNode *input;

      input = gegl_node_get_input_proxy (node, "input");

      filter->translate = gegl_node_new_child (node,
                                               "operation", "gegl:translate",
                                               NULL);

      filter->crop_before = gegl_node_new_child (node,
                                                 "operation", "gegl:crop",
                                                 NULL);

      filter->cast_before = gegl_node_new_child (node,
                                                 "operation", "gegl:nop",
                                                 NULL);

      gegl_node_link_many (input,
                           filter->translate,
                           filter->crop_before,
                           filter->cast_before,
                           filter->operation,
                           NULL);
    }

  filter->cast_after = gegl_node_new_child (node,
                                            "operation", "gegl:nop",
                                            NULL);

  filter->crop_after = gegl_node_new_child (node,
                                            "operation", "gegl:crop",
                                            NULL);

  gegl_node_link_many (filter->operation,
                       filter->cast_after,
                       filter->crop_after,
                       NULL);

  gegl_node_connect_to (filter->crop_after, "output",
                        node,               "aux");

  return filter;
}

LigmaDrawable *
ligma_drawable_filter_get_drawable (LigmaDrawableFilter *filter)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE_FILTER (filter), NULL);

  return filter->drawable;
}

GeglNode *
ligma_drawable_filter_get_operation (LigmaDrawableFilter *filter)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE_FILTER (filter), NULL);

  return filter->operation;
}

void
ligma_drawable_filter_set_clip (LigmaDrawableFilter *filter,
                               gboolean            clip)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE_FILTER (filter));

  if (clip != filter->clip)
    {
      filter->clip = clip;

      ligma_drawable_filter_sync_clip (filter, TRUE);
    }
}

void
ligma_drawable_filter_set_region (LigmaDrawableFilter *filter,
                                 LigmaFilterRegion    region)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE_FILTER (filter));

  if (region != filter->region)
    {
      filter->region = region;

      ligma_drawable_filter_sync_region (filter);

      if (ligma_drawable_filter_is_active (filter))
        ligma_drawable_filter_update_drawable (filter, NULL);
    }
}

void
ligma_drawable_filter_set_crop (LigmaDrawableFilter  *filter,
                               const GeglRectangle *rect,
                               gboolean             update)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE_FILTER (filter));

  if ((rect != NULL) != filter->crop_enabled ||
      (rect && ! gegl_rectangle_equal (rect, &filter->crop_rect)))
    {
      gboolean      old_enabled = filter->crop_enabled;
      GeglRectangle old_rect    = filter->crop_rect;

      if (rect)
        {
          filter->crop_enabled = TRUE;
          filter->crop_rect    = *rect;
        }
      else
        {
          filter->crop_enabled = FALSE;
        }

      ligma_drawable_filter_sync_crop (filter,
                                      old_enabled,
                                      &old_rect,
                                      filter->preview_split_enabled,
                                      filter->preview_split_alignment,
                                      filter->preview_split_position,
                                      update);
    }
}

void
ligma_drawable_filter_set_preview (LigmaDrawableFilter *filter,
                                  gboolean            enabled)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE_FILTER (filter));

  if (enabled != filter->preview_enabled)
    {
      filter->preview_enabled = enabled;

      ligma_drawable_filter_sync_active (filter);

      if (ligma_drawable_filter_is_added (filter))
        {
          ligma_drawable_update_bounding_box (filter->drawable);

          ligma_drawable_filter_update_drawable (filter, NULL);
        }
    }
}

void
ligma_drawable_filter_set_preview_split (LigmaDrawableFilter  *filter,
                                        gboolean             enabled,
                                        LigmaAlignmentType    alignment,
                                        gint                 position)
{
  LigmaItem *item;

  g_return_if_fail (LIGMA_IS_DRAWABLE_FILTER (filter));
  g_return_if_fail (alignment == LIGMA_ALIGN_LEFT  ||
                    alignment == LIGMA_ALIGN_RIGHT ||
                    alignment == LIGMA_ALIGN_TOP   ||
                    alignment == LIGMA_ALIGN_BOTTOM);

  item = LIGMA_ITEM (filter->drawable);

  switch (alignment)
    {
    case LIGMA_ALIGN_LEFT:
    case LIGMA_ALIGN_RIGHT:
      position = CLAMP (position, 0, ligma_item_get_width (item));
      break;

    case LIGMA_ALIGN_TOP:
    case LIGMA_ALIGN_BOTTOM:
      position = CLAMP (position, 0, ligma_item_get_height (item));
      break;

    default:
      g_return_if_reached ();
    }

  if (enabled   != filter->preview_split_enabled   ||
      alignment != filter->preview_split_alignment ||
      position  != filter->preview_split_position)
    {
      gboolean          old_enabled   = filter->preview_split_enabled;
      LigmaAlignmentType old_alignment = filter->preview_split_alignment;
      gint              old_position  = filter->preview_split_position;

      filter->preview_split_enabled   = enabled;
      filter->preview_split_alignment = alignment;
      filter->preview_split_position  = position;

      ligma_drawable_filter_sync_crop (filter,
                                      filter->crop_enabled,
                                      &filter->crop_rect,
                                      old_enabled,
                                      old_alignment,
                                      old_position,
                                      TRUE);
    }
}

void
ligma_drawable_filter_set_opacity (LigmaDrawableFilter *filter,
                                  gdouble             opacity)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE_FILTER (filter));

  if (opacity != filter->opacity)
    {
      filter->opacity = opacity;

      ligma_drawable_filter_sync_opacity (filter);

      if (ligma_drawable_filter_is_active (filter))
        ligma_drawable_filter_update_drawable (filter, NULL);
    }
}

void
ligma_drawable_filter_set_mode (LigmaDrawableFilter     *filter,
                               LigmaLayerMode           paint_mode,
                               LigmaLayerColorSpace     blend_space,
                               LigmaLayerColorSpace     composite_space,
                               LigmaLayerCompositeMode  composite_mode)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE_FILTER (filter));

  if (paint_mode      != filter->paint_mode      ||
      blend_space     != filter->blend_space     ||
      composite_space != filter->composite_space ||
      composite_mode  != filter->composite_mode)
    {
      filter->paint_mode      = paint_mode;
      filter->blend_space     = blend_space;
      filter->composite_space = composite_space;
      filter->composite_mode  = composite_mode;

      ligma_drawable_filter_sync_mode (filter);

      if (ligma_drawable_filter_is_active (filter))
        ligma_drawable_filter_update_drawable (filter, NULL);
    }
}

void
ligma_drawable_filter_set_add_alpha (LigmaDrawableFilter *filter,
                                    gboolean            add_alpha)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE_FILTER (filter));

  if (add_alpha != filter->add_alpha)
    {
      filter->add_alpha = add_alpha;

      ligma_drawable_filter_sync_format (filter);

      if (ligma_drawable_filter_is_active (filter))
        ligma_drawable_filter_update_drawable (filter, NULL);
    }
}

void
ligma_drawable_filter_set_gamma_hack (LigmaDrawableFilter *filter,
                                     gboolean            gamma_hack)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE_FILTER (filter));

  if (gamma_hack != filter->gamma_hack)
    {
      filter->gamma_hack = gamma_hack;

      ligma_drawable_filter_sync_gamma_hack (filter);

      if (ligma_drawable_filter_is_active (filter))
        ligma_drawable_filter_update_drawable (filter, NULL);
    }
}

void
ligma_drawable_filter_set_override_constraints (LigmaDrawableFilter *filter,
                                               gboolean            override_constraints)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE_FILTER (filter));

  if (override_constraints != filter->override_constraints)
    {
      filter->override_constraints = override_constraints;

      ligma_drawable_filter_sync_affect (filter);
      ligma_drawable_filter_sync_format (filter);
      ligma_drawable_filter_sync_clip   (filter, TRUE);

      if (ligma_drawable_filter_is_active (filter))
        ligma_drawable_filter_update_drawable (filter, NULL);
    }
}

const Babl *
ligma_drawable_filter_get_format (LigmaDrawableFilter *filter)
{
  const Babl *format;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE_FILTER (filter), NULL);

  format = ligma_applicator_get_output_format (filter->applicator);

  if (! format)
    format = ligma_drawable_get_format (filter->drawable);

  return format;
}

void
ligma_drawable_filter_apply (LigmaDrawableFilter  *filter,
                            const GeglRectangle *area)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE_FILTER (filter));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (filter->drawable)));

  ligma_drawable_filter_add_filter (filter);

  ligma_drawable_filter_sync_clip (filter, TRUE);

  if (ligma_drawable_filter_is_active (filter))
    {
      ligma_drawable_update_bounding_box (filter->drawable);

      ligma_drawable_filter_update_drawable (filter, area);
    }
}

gboolean
ligma_drawable_filter_commit (LigmaDrawableFilter *filter,
                             LigmaProgress       *progress,
                             gboolean            cancellable)
{
  gboolean success = TRUE;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE_FILTER (filter), FALSE);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (filter->drawable)),
                        FALSE);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), FALSE);

  if (ligma_drawable_filter_is_added (filter))
    {
      const Babl *format;

      format = ligma_drawable_filter_get_format (filter);

      ligma_drawable_filter_set_preview_split (filter, FALSE,
                                              filter->preview_split_alignment,
                                              filter->preview_split_position);
      ligma_drawable_filter_set_preview (filter, TRUE);

      success = ligma_drawable_merge_filter (filter->drawable,
                                            LIGMA_FILTER (filter),
                                            progress,
                                            ligma_object_get_name (filter),
                                            format,
                                            filter->filter_clip,
                                            cancellable,
                                            FALSE);

      ligma_drawable_filter_remove_filter (filter);

      if (! success)
        ligma_drawable_filter_update_drawable (filter, NULL);

      g_signal_emit (filter, drawable_filter_signals[FLUSH], 0);
    }

  return success;
}

void
ligma_drawable_filter_abort (LigmaDrawableFilter *filter)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE_FILTER (filter));

  if (ligma_drawable_filter_remove_filter (filter))
    {
      ligma_drawable_filter_update_drawable (filter, NULL);
    }
}


/*  private functions  */

static void
ligma_drawable_filter_sync_active (LigmaDrawableFilter *filter)
{
  ligma_applicator_set_active (filter->applicator, filter->preview_enabled);
}

static void
ligma_drawable_filter_sync_clip (LigmaDrawableFilter *filter,
                                gboolean            sync_region)
{
  gboolean clip;

  if (filter->override_constraints)
    clip = filter->clip;
  else
    clip = ligma_item_get_clip (LIGMA_ITEM (filter->drawable), filter->clip);

  if (! clip)
    {
      LigmaImage   *image = ligma_item_get_image (LIGMA_ITEM (filter->drawable));
      LigmaChannel *mask  = ligma_image_get_mask (image);

      if (! ligma_channel_is_empty (mask))
        clip = TRUE;
    }

  if (! clip)
    {
      GeglRectangle bounding_box;

      bounding_box = gegl_node_get_bounding_box (filter->operation);

      if (gegl_rectangle_is_infinite_plane (&bounding_box))
        clip = TRUE;
    }

  if (clip != filter->filter_clip)
    {
      filter->filter_clip = clip;

      if (sync_region)
        ligma_drawable_filter_sync_region (filter);
    }
}

static void
ligma_drawable_filter_sync_region (LigmaDrawableFilter *filter)
{
  if (filter->region == LIGMA_FILTER_REGION_SELECTION)
    {
      if (filter->has_input)
        {
          gegl_node_set (filter->translate,
                         "x", (gdouble) -filter->filter_area.x,
                         "y", (gdouble) -filter->filter_area.y,
                         NULL);

          gegl_node_set (filter->crop_before,
                         "width",  (gdouble) filter->filter_area.width,
                         "height", (gdouble) filter->filter_area.height,
                         NULL);
        }

      if (filter->filter_clip)
        {
          gegl_node_set (filter->crop_after,
                         "operation", "gegl:crop",
                         "x",         0.0,
                         "y",         0.0,
                         "width",     (gdouble) filter->filter_area.width,
                         "height",    (gdouble) filter->filter_area.height,
                         NULL);
        }
      else
        {
          gegl_node_set (filter->crop_after,
                         "operation", "gegl:nop",
                         NULL);
        }

      ligma_applicator_set_apply_offset (filter->applicator,
                                        filter->filter_area.x,
                                        filter->filter_area.y);
    }
  else
    {
      LigmaItem *item   = LIGMA_ITEM (filter->drawable);
      gdouble   width  = ligma_item_get_width (item);
      gdouble   height = ligma_item_get_height (item);

      if (filter->has_input)
        {
          gegl_node_set (filter->translate,
                         "x", (gdouble) 0.0,
                         "y", (gdouble) 0.0,
                         NULL);

          gegl_node_set (filter->crop_before,
                         "width",  width,
                         "height", height,
                         NULL);
        }

      if (filter->filter_clip)
        {
          gegl_node_set (filter->crop_after,
                         "operation", "gegl:crop",
                         "x",         (gdouble) filter->filter_area.x,
                         "y",         (gdouble) filter->filter_area.y,
                         "width",     (gdouble) filter->filter_area.width,
                         "height",    (gdouble) filter->filter_area.height,
                         NULL);
        }
      else
        {
          gegl_node_set (filter->crop_after,
                         "operation", "gegl:nop",
                         NULL);
        }

      ligma_applicator_set_apply_offset (filter->applicator, 0, 0);
    }

  if (ligma_drawable_filter_is_active (filter))
    {
      if (ligma_drawable_update_bounding_box (filter->drawable))
        g_signal_emit (filter, drawable_filter_signals[FLUSH], 0);
    }
}

static gboolean
ligma_drawable_filter_get_crop_rect (LigmaDrawableFilter  *filter,
                                    gboolean             crop_enabled,
                                    const GeglRectangle *crop_rect,
                                    gboolean             preview_split_enabled,
                                    LigmaAlignmentType    preview_split_alignment,
                                    gint                 preview_split_position,
                                    GeglRectangle       *rect)
{
  GeglRectangle bounds;
  gint          x1, x2;
  gint          y1, y2;

  bounds = gegl_rectangle_infinite_plane ();

  x1 = bounds.x;
  x2 = bounds.x + bounds.width;

  y1 = bounds.y;
  y2 = bounds.y + bounds.height;

  if (preview_split_enabled)
    {
      switch (preview_split_alignment)
        {
        case LIGMA_ALIGN_LEFT:
          x2 = preview_split_position;
          break;

        case LIGMA_ALIGN_RIGHT:
          x1 = preview_split_position;
          break;

        case LIGMA_ALIGN_TOP:
          y2 = preview_split_position;
          break;

        case LIGMA_ALIGN_BOTTOM:
          y1 = preview_split_position;
          break;

        default:
          g_return_val_if_reached (FALSE);
        }
    }

  gegl_rectangle_set (rect, x1, y1, x2 - x1, y2 - y1);

  if (crop_enabled)
    gegl_rectangle_intersect (rect, rect, crop_rect);

  return ! gegl_rectangle_equal (rect, &bounds);
}

static void
ligma_drawable_filter_sync_crop (LigmaDrawableFilter  *filter,
                                gboolean             old_crop_enabled,
                                const GeglRectangle *old_crop_rect,
                                gboolean             old_preview_split_enabled,
                                LigmaAlignmentType    old_preview_split_alignment,
                                gint                 old_preview_split_position,
                                gboolean             update)
{
  GeglRectangle old_rect;
  GeglRectangle new_rect;
  gboolean      enabled;

  ligma_drawable_filter_get_crop_rect (filter,
                                      old_crop_enabled,
                                      old_crop_rect,
                                      old_preview_split_enabled,
                                      old_preview_split_alignment,
                                      old_preview_split_position,
                                      &old_rect);

  enabled = ligma_drawable_filter_get_crop_rect (filter,
                                                filter->crop_enabled,
                                                &filter->crop_rect,
                                                filter->preview_split_enabled,
                                                filter->preview_split_alignment,
                                                filter->preview_split_position,
                                                &new_rect);

  ligma_applicator_set_crop (filter->applicator, enabled ? &new_rect : NULL);

  if (update                                     &&
      ligma_drawable_filter_is_active (filter) &&
      ! gegl_rectangle_equal (&old_rect, &new_rect))
    {
      GeglRectangle diff_rects[4];
      gint          n_diff_rects;
      gint          i;

      ligma_drawable_update_bounding_box (filter->drawable);

      n_diff_rects = gegl_rectangle_xor (diff_rects, &old_rect, &new_rect);

      for (i = 0; i < n_diff_rects; i++)
        ligma_drawable_filter_update_drawable (filter, &diff_rects[i]);
    }
}

static void
ligma_drawable_filter_sync_opacity (LigmaDrawableFilter *filter)
{
  ligma_applicator_set_opacity (filter->applicator,
                               filter->opacity);
}

static void
ligma_drawable_filter_sync_mode (LigmaDrawableFilter *filter)
{
  LigmaLayerMode paint_mode = filter->paint_mode;

  if (! filter->has_input && paint_mode == LIGMA_LAYER_MODE_REPLACE)
    {
      /* if the filter's op has no input, use NORMAL instead of REPLACE, so
       * that we composite the op's output on top of the input, instead of
       * completely replacing it.
       */
      paint_mode = LIGMA_LAYER_MODE_NORMAL;
    }

  ligma_applicator_set_mode (filter->applicator,
                            paint_mode,
                            filter->blend_space,
                            filter->composite_space,
                            filter->composite_mode);
}

static void
ligma_drawable_filter_sync_affect (LigmaDrawableFilter *filter)
{
  ligma_applicator_set_affect (
    filter->applicator,
    filter->override_constraints ?

      LIGMA_COMPONENT_MASK_RED   |
      LIGMA_COMPONENT_MASK_GREEN |
      LIGMA_COMPONENT_MASK_BLUE  |
      LIGMA_COMPONENT_MASK_ALPHA :

      ligma_drawable_get_active_mask (filter->drawable));
}

static void
ligma_drawable_filter_sync_format (LigmaDrawableFilter *filter)
{
  const Babl *format;

  if (filter->add_alpha                                &&
      (ligma_drawable_supports_alpha (filter->drawable) ||
       filter->override_constraints))
    {
      format = ligma_drawable_get_format_with_alpha (filter->drawable);
    }
  else
    {
      format = ligma_drawable_get_format (filter->drawable);
    }

  ligma_applicator_set_output_format (filter->applicator, format);
}

static void
ligma_drawable_filter_sync_mask (LigmaDrawableFilter *filter)
{
  LigmaImage   *image = ligma_item_get_image (LIGMA_ITEM (filter->drawable));
  LigmaChannel *mask  = ligma_image_get_mask (image);

  if (ligma_channel_is_empty (mask))
    {
      ligma_applicator_set_mask_buffer (filter->applicator, NULL);
    }
  else
    {
      GeglBuffer *mask_buffer;
      gint        offset_x, offset_y;

      mask_buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (mask));
      ligma_item_get_offset (LIGMA_ITEM (filter->drawable),
                            &offset_x, &offset_y);

      ligma_applicator_set_mask_buffer (filter->applicator, mask_buffer);
      ligma_applicator_set_mask_offset (filter->applicator,
                                       -offset_x, -offset_y);
    }

  ligma_item_mask_intersect (LIGMA_ITEM (filter->drawable),
                            &filter->filter_area.x,
                            &filter->filter_area.y,
                            &filter->filter_area.width,
                            &filter->filter_area.height);
}

static void
ligma_drawable_filter_sync_gamma_hack (LigmaDrawableFilter *filter)
{
  if (filter->gamma_hack)
    {
      const Babl  *drawable_format;
      const Babl  *cast_format;
      LigmaTRCType  trc = LIGMA_TRC_LINEAR;

      switch (ligma_drawable_get_trc (filter->drawable))
        {
        case LIGMA_TRC_LINEAR:     trc = LIGMA_TRC_NON_LINEAR; break;
        case LIGMA_TRC_NON_LINEAR: trc = LIGMA_TRC_LINEAR;     break;
        case LIGMA_TRC_PERCEPTUAL: trc = LIGMA_TRC_LINEAR;     break;
        }

      drawable_format =
        ligma_drawable_get_format_with_alpha (filter->drawable);

      cast_format =
        ligma_babl_format (ligma_babl_format_get_base_type (drawable_format),
                          ligma_babl_precision (ligma_babl_format_get_component_type (drawable_format),
                                               trc),
                          TRUE,
                          babl_format_get_space (drawable_format));

      if (filter->has_input)
        {
          gegl_node_set (filter->cast_before,
                         "operation",     "gegl:cast-format",
                         "input-format",  drawable_format,
                         "output-format", cast_format,
                         NULL);
        }

      gegl_node_set (filter->cast_after,
                     "operation",     "gegl:cast-format",
                     "input-format",  cast_format,
                     "output-format", drawable_format,
                     NULL);
    }
  else
    {
      if (filter->has_input)
        {
          gegl_node_set (filter->cast_before,
                         "operation", "gegl:nop",
                         NULL);
        }

      gegl_node_set (filter->cast_after,
                     "operation", "gegl:nop",
                     NULL);
    }
}

static gboolean
ligma_drawable_filter_is_added (LigmaDrawableFilter *filter)
{
  return ligma_drawable_has_filter (filter->drawable,
                                   LIGMA_FILTER (filter));
}

static gboolean
ligma_drawable_filter_is_active (LigmaDrawableFilter *filter)
{
  return ligma_drawable_filter_is_added (filter) &&
         filter->preview_enabled;
}

static gboolean
ligma_drawable_filter_add_filter (LigmaDrawableFilter *filter)
{
  if (! ligma_drawable_filter_is_added (filter))
    {
      LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (filter->drawable));

      ligma_viewable_preview_freeze (LIGMA_VIEWABLE (filter->drawable));

      ligma_drawable_filter_sync_active (filter);
      ligma_drawable_filter_sync_mask (filter);
      ligma_drawable_filter_sync_clip (filter, FALSE);
      ligma_drawable_filter_sync_region (filter);
      ligma_drawable_filter_sync_crop (filter,
                                      filter->crop_enabled,
                                      &filter->crop_rect,
                                      filter->preview_split_enabled,
                                      filter->preview_split_alignment,
                                      filter->preview_split_position,
                                      TRUE);
      ligma_drawable_filter_sync_opacity (filter);
      ligma_drawable_filter_sync_mode (filter);
      ligma_drawable_filter_sync_affect (filter);
      ligma_drawable_filter_sync_format (filter);
      ligma_drawable_filter_sync_gamma_hack (filter);

      ligma_drawable_add_filter (filter->drawable,
                                LIGMA_FILTER (filter));

      ligma_drawable_update_bounding_box (filter->drawable);

      g_signal_connect (image, "component-active-changed",
                        G_CALLBACK (ligma_drawable_filter_affect_changed),
                        filter);
      g_signal_connect (image, "mask-changed",
                        G_CALLBACK (ligma_drawable_filter_mask_changed),
                        filter);
      g_signal_connect (filter->drawable, "lock-position-changed",
                        G_CALLBACK (ligma_drawable_filter_lock_position_changed),
                        filter);
      g_signal_connect (filter->drawable, "format-changed",
                        G_CALLBACK (ligma_drawable_filter_format_changed),
                        filter);
      g_signal_connect (filter->drawable, "removed",
                        G_CALLBACK (ligma_drawable_filter_drawable_removed),
                        filter);

      if (LIGMA_IS_LAYER (filter->drawable))
        {
          g_signal_connect (filter->drawable, "lock-alpha-changed",
                            G_CALLBACK (ligma_drawable_filter_lock_alpha_changed),
                            filter);
        }

      return TRUE;
    }

  return FALSE;
}

static gboolean
ligma_drawable_filter_remove_filter (LigmaDrawableFilter *filter)
{
  if (ligma_drawable_filter_is_added (filter))
    {
      LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (filter->drawable));

      if (LIGMA_IS_LAYER (filter->drawable))
        {
          g_signal_handlers_disconnect_by_func (filter->drawable,
                                                ligma_drawable_filter_lock_alpha_changed,
                                                filter);
        }

      g_signal_handlers_disconnect_by_func (filter->drawable,
                                            ligma_drawable_filter_drawable_removed,
                                            filter);
      g_signal_handlers_disconnect_by_func (filter->drawable,
                                            ligma_drawable_filter_format_changed,
                                            filter);
      g_signal_handlers_disconnect_by_func (filter->drawable,
                                            ligma_drawable_filter_lock_position_changed,
                                            filter);
      g_signal_handlers_disconnect_by_func (image,
                                            ligma_drawable_filter_mask_changed,
                                            filter);
      g_signal_handlers_disconnect_by_func (image,
                                            ligma_drawable_filter_affect_changed,
                                            filter);

      ligma_drawable_remove_filter (filter->drawable,
                                   LIGMA_FILTER (filter));

      ligma_drawable_update_bounding_box (filter->drawable);

      ligma_viewable_preview_thaw (LIGMA_VIEWABLE (filter->drawable));

      return TRUE;
    }

  return FALSE;
}

static void
ligma_drawable_filter_update_drawable (LigmaDrawableFilter  *filter,
                                      const GeglRectangle *area)
{
  GeglRectangle bounding_box;
  GeglRectangle update_area;

  bounding_box = ligma_drawable_get_bounding_box (filter->drawable);

  if (area)
    {
      if (! gegl_rectangle_intersect (&update_area,
                                      area, &bounding_box))
        {
          return;
        }
    }
  else
    {
      ligma_drawable_filter_get_crop_rect (filter,
                                          filter->crop_enabled,
                                          &filter->crop_rect,
                                          filter->preview_split_enabled,
                                          filter->preview_split_alignment,
                                          filter->preview_split_position,
                                          &update_area);

      if (! gegl_rectangle_intersect (&update_area,
                                      &update_area, &bounding_box))
        {
          return;
        }
    }

  if (update_area.width  > 0 &&
      update_area.height > 0)
    {
      ligma_drawable_update (filter->drawable,
                            update_area.x,
                            update_area.y,
                            update_area.width,
                            update_area.height);

      g_signal_emit (filter, drawable_filter_signals[FLUSH], 0);
    }
}

static void
ligma_drawable_filter_affect_changed (LigmaImage          *image,
                                     LigmaChannelType     channel,
                                     LigmaDrawableFilter *filter)
{
  ligma_drawable_filter_sync_affect (filter);
  ligma_drawable_filter_update_drawable (filter, NULL);
}

static void
ligma_drawable_filter_mask_changed (LigmaImage          *image,
                                   LigmaDrawableFilter *filter)
{
  ligma_drawable_filter_update_drawable (filter, NULL);

  ligma_drawable_filter_sync_mask (filter);
  ligma_drawable_filter_sync_clip (filter, FALSE);
  ligma_drawable_filter_sync_region (filter);

  ligma_drawable_filter_update_drawable (filter, NULL);
}

static void
ligma_drawable_filter_lock_position_changed (LigmaDrawable       *drawable,
                                            LigmaDrawableFilter *filter)
{
  ligma_drawable_filter_sync_clip (filter, TRUE);
  ligma_drawable_filter_update_drawable (filter, NULL);
}

static void
ligma_drawable_filter_format_changed (LigmaDrawable       *drawable,
                                     LigmaDrawableFilter *filter)
{
  ligma_drawable_filter_sync_format (filter);
  ligma_drawable_filter_update_drawable (filter, NULL);
}

static void
ligma_drawable_filter_drawable_removed (LigmaDrawable       *drawable,
                                       LigmaDrawableFilter *filter)
{
  ligma_drawable_filter_remove_filter (filter);
}

static void
ligma_drawable_filter_lock_alpha_changed (LigmaLayer          *layer,
                                         LigmaDrawableFilter *filter)
{
  ligma_drawable_filter_sync_affect (filter);
  ligma_drawable_filter_update_drawable (filter, NULL);
}
