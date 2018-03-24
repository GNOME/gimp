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
 * manipulation of drawable data, with live preview on screen.
 *
 * To create a tool that uses this, see app/tools/gimpfiltertool.c for
 * the interface and e.g. app/tools/gimpcolorbalancetool.c for an
 * example of using that interface.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimpapplicator.h"
#include "gegl/gimp-gegl-utils.h"

#include "gimpchannel.h"
#include "gimpdrawable-filters.h"
#include "gimpdrawablefilter.h"
#include "gimpimage.h"
#include "gimpmarshal.h"
#include "gimpprogress.h"


enum
{
  FLUSH,
  LAST_SIGNAL
};


struct _GimpDrawableFilter
{
  GimpFilter              parent_instance;

  GimpDrawable           *drawable;
  GeglNode               *operation;

  GimpFilterRegion        region;
  gboolean                preview_enabled;
  GimpAlignmentType       preview_alignment;
  gdouble                 preview_position;
  gdouble                 opacity;
  GimpLayerMode           paint_mode;
  GimpLayerColorSpace     blend_space;
  GimpLayerColorSpace     composite_space;
  GimpLayerCompositeMode  composite_mode;
  gboolean                color_managed;
  gboolean                gamma_hack;

  GeglRectangle           filter_area;

  GeglNode               *translate;
  GeglNode               *crop;
  GeglNode               *cast_before;
  GeglNode               *transform_before;
  GeglNode               *transform_after;
  GeglNode               *cast_after;
  GimpApplicator         *applicator;
};


static void       gimp_drawable_filter_dispose          (GObject             *object);
static void       gimp_drawable_filter_finalize         (GObject             *object);

static void       gimp_drawable_filter_sync_region      (GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_sync_preview     (GimpDrawableFilter  *filter,
                                                         gboolean             old_enabled,
                                                         GimpAlignmentType    old_alignment,
                                                         gdouble              old_position);
static void       gimp_drawable_filter_sync_opacity     (GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_sync_mode        (GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_sync_affect      (GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_sync_mask        (GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_sync_transform   (GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_sync_gamma_hack  (GimpDrawableFilter  *filter);

static gboolean   gimp_drawable_filter_is_filtering     (GimpDrawableFilter  *filter);
static gboolean   gimp_drawable_filter_add_filter       (GimpDrawableFilter  *filter);
static gboolean   gimp_drawable_filter_remove_filter    (GimpDrawableFilter  *filter);

static void       gimp_drawable_filter_update_drawable  (GimpDrawableFilter  *filter,
                                                         const GeglRectangle *area);

static void       gimp_drawable_filter_affect_changed   (GimpImage           *image,
                                                         GimpChannelType      channel,
                                                         GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_mask_changed     (GimpImage           *image,
                                                         GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_profile_changed  (GimpColorManaged    *managed,
                                                         GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_drawable_removed (GimpDrawable        *drawable,
                                                         GimpDrawableFilter  *filter);


G_DEFINE_TYPE (GimpDrawableFilter, gimp_drawable_filter, GIMP_TYPE_FILTER)

#define parent_class gimp_drawable_filter_parent_class

static guint drawable_filter_signals[LAST_SIGNAL] = { 0, };


static void
gimp_drawable_filter_class_init (GimpDrawableFilterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  drawable_filter_signals[FLUSH] =
    g_signal_new ("flush",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDrawableFilterClass, flush),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->dispose  = gimp_drawable_filter_dispose;
  object_class->finalize = gimp_drawable_filter_finalize;
}

static void
gimp_drawable_filter_init (GimpDrawableFilter *drawable_filter)
{
  drawable_filter->region            = GIMP_FILTER_REGION_SELECTION;
  drawable_filter->preview_alignment = GIMP_ALIGN_LEFT;
  drawable_filter->preview_position  = 1.0;
  drawable_filter->opacity           = GIMP_OPACITY_OPAQUE;
  drawable_filter->paint_mode        = GIMP_LAYER_MODE_REPLACE;
  drawable_filter->blend_space       = GIMP_LAYER_COLOR_SPACE_AUTO;
  drawable_filter->composite_space   = GIMP_LAYER_COLOR_SPACE_AUTO;
  drawable_filter->composite_mode    = GIMP_LAYER_COMPOSITE_AUTO;
}

static void
gimp_drawable_filter_dispose (GObject *object)
{
  GimpDrawableFilter *drawable_filter = GIMP_DRAWABLE_FILTER (object);

  if (drawable_filter->drawable)
    gimp_drawable_filter_remove_filter (drawable_filter);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_drawable_filter_finalize (GObject *object)
{
  GimpDrawableFilter *drawable_filter = GIMP_DRAWABLE_FILTER (object);

  g_clear_object (&drawable_filter->operation);
  g_clear_object (&drawable_filter->applicator);
  g_clear_object (&drawable_filter->drawable);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GimpDrawableFilter *
gimp_drawable_filter_new (GimpDrawable *drawable,
                          const gchar  *undo_desc,
                          GeglNode     *operation,
                          const gchar  *icon_name)
{
  GimpDrawableFilter *filter;
  GeglNode           *node;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GEGL_IS_NODE (operation), NULL);
  g_return_val_if_fail (gegl_node_has_pad (operation, "output"), NULL);

  filter = g_object_new (GIMP_TYPE_DRAWABLE_FILTER,
                         "name",      undo_desc,
                         "icon-name", icon_name,
                         NULL);

  filter->drawable  = g_object_ref (drawable);
  filter->operation = g_object_ref (operation);

  node = gimp_filter_get_node (GIMP_FILTER (filter));

  gegl_node_add_child (node, operation);

  filter->applicator = gimp_applicator_new (node, TRUE, TRUE);

  gimp_filter_set_applicator (GIMP_FILTER (filter), filter->applicator);

  filter->translate = gegl_node_new_child (node,
                                           "operation", "gegl:translate",
                                           NULL);
  filter->crop = gegl_node_new_child (node,
                                      "operation", "gegl:crop",
                                      NULL);

  filter->cast_before = gegl_node_new_child (node,
                                             "operation", "gegl:nop",
                                             NULL);
  filter->transform_before = gegl_node_new_child (node,
                                                  "operation", "gegl:nop",
                                                  NULL);
  filter->transform_after = gegl_node_new_child (node,
                                                 "operation", "gegl:nop",
                                                 NULL);
  filter->cast_after = gegl_node_new_child (node,
                                            "operation", "gegl:nop",
                                            NULL);

  if (gegl_node_has_pad (filter->operation, "input"))
    {
      GeglNode *input = gegl_node_get_input_proxy (node, "input");

      gegl_node_link_many (input,
                           filter->translate,
                           filter->crop,
                           filter->cast_before,
                           filter->transform_before,
                           filter->operation,
                           NULL);
    }

  gegl_node_link_many (filter->operation,
                       filter->transform_after,
                       filter->cast_after,
                       NULL);

  gegl_node_connect_to (filter->cast_after, "output",
                        node,               "aux");

  return filter;
}

void
gimp_drawable_filter_set_region (GimpDrawableFilter *filter,
                                 GimpFilterRegion    region)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  if (region != filter->region)
    {
      filter->region = region;

      gimp_drawable_filter_sync_region (filter);

      if (gimp_drawable_filter_is_filtering (filter))
        gimp_drawable_filter_update_drawable (filter, NULL);
    }
}

void
gimp_drawable_filter_set_preview (GimpDrawableFilter  *filter,
                                  gboolean             enabled,
                                  GimpAlignmentType    alignment,
                                  gdouble              position)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));
  g_return_if_fail (alignment == GIMP_ALIGN_LEFT  ||
                    alignment == GIMP_ALIGN_RIGHT ||
                    alignment == GIMP_ALIGN_TOP   ||
                    alignment == GIMP_ALIGN_BOTTOM);

  position = CLAMP (position, 0.0, 1.0);

  if (enabled   != filter->preview_enabled   ||
      alignment != filter->preview_alignment ||
      position  != filter->preview_position)
    {
      gboolean          old_enabled   = filter->preview_enabled;
      GimpAlignmentType old_alignment = filter->preview_alignment;
      gdouble           old_position  = filter->preview_position;

      filter->preview_enabled   = enabled;
      filter->preview_alignment = alignment;
      filter->preview_position  = position;

      gimp_drawable_filter_sync_preview (filter,
                                         old_enabled,
                                         old_alignment, old_position);
    }
}

void
gimp_drawable_filter_set_opacity (GimpDrawableFilter *filter,
                                  gdouble             opacity)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  if (opacity != filter->opacity)
    {
      filter->opacity = opacity;

      gimp_drawable_filter_sync_opacity (filter);

      if (gimp_drawable_filter_is_filtering (filter))
        gimp_drawable_filter_update_drawable (filter, NULL);
    }
}

void
gimp_drawable_filter_set_mode (GimpDrawableFilter     *filter,
                               GimpLayerMode           paint_mode,
                               GimpLayerColorSpace     blend_space,
                               GimpLayerColorSpace     composite_space,
                               GimpLayerCompositeMode  composite_mode)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  if (paint_mode      != filter->paint_mode      ||
      blend_space     != filter->blend_space     ||
      composite_space != filter->composite_space ||
      composite_mode  != filter->composite_mode)
    {
      filter->paint_mode      = paint_mode;
      filter->blend_space     = blend_space;
      filter->composite_space = composite_space;
      filter->composite_mode  = composite_mode;

      gimp_drawable_filter_sync_mode (filter);

      if (gimp_drawable_filter_is_filtering (filter))
        gimp_drawable_filter_update_drawable (filter, NULL);
    }
}

void
gimp_drawable_filter_set_color_managed (GimpDrawableFilter *filter,
                                        gboolean            color_managed)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  if (color_managed != filter->color_managed)
    {
      filter->color_managed = color_managed;

      gimp_drawable_filter_sync_transform (filter);

      if (gimp_drawable_filter_is_filtering (filter))
        gimp_drawable_filter_update_drawable (filter, NULL);
    }
}

void
gimp_drawable_filter_set_gamma_hack (GimpDrawableFilter *filter,
                                     gboolean            gamma_hack)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  if (gamma_hack != filter->gamma_hack)
    {
      filter->gamma_hack = gamma_hack;

      gimp_drawable_filter_sync_gamma_hack (filter);
      gimp_drawable_filter_sync_transform (filter);

      if (gimp_drawable_filter_is_filtering (filter))
        gimp_drawable_filter_update_drawable (filter, NULL);
    }
}

void
gimp_drawable_filter_apply (GimpDrawableFilter  *filter,
                            const GeglRectangle *area)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (filter->drawable)));

  gimp_drawable_filter_add_filter (filter);
  gimp_drawable_filter_update_drawable (filter, area);
}

gboolean
gimp_drawable_filter_commit (GimpDrawableFilter *filter,
                             GimpProgress       *progress,
                             gboolean            cancellable)
{
  gboolean success = TRUE;

  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER (filter), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (filter->drawable)),
                        FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);

  if (gimp_drawable_filter_is_filtering (filter))
    {
      success = gimp_drawable_merge_filter (filter->drawable,
                                            GIMP_FILTER (filter),
                                            progress,
                                            gimp_object_get_name (filter),
                                            cancellable);

      gimp_drawable_filter_remove_filter (filter);

      g_signal_emit (filter, drawable_filter_signals[FLUSH], 0);
    }

  return success;
}

void
gimp_drawable_filter_abort (GimpDrawableFilter *filter)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  if (gimp_drawable_filter_remove_filter (filter))
    {
      gimp_drawable_filter_update_drawable (filter, NULL);
    }
}


/*  private functions  */

static void
gimp_drawable_filter_sync_region (GimpDrawableFilter *filter)
{
  if (filter->region == GIMP_FILTER_REGION_SELECTION)
    {
      gegl_node_set (filter->translate,
                     "x", (gdouble) -filter->filter_area.x,
                     "y", (gdouble) -filter->filter_area.y,
                     NULL);

      gegl_node_set (filter->crop,
                     "width",  (gdouble) filter->filter_area.width,
                     "height", (gdouble) filter->filter_area.height,
                     NULL);

      gimp_applicator_set_apply_offset (filter->applicator,
                                        filter->filter_area.x,
                                        filter->filter_area.y);
    }
  else
    {
      GimpItem *item   = GIMP_ITEM (filter->drawable);
      gdouble   width  = gimp_item_get_width (item);
      gdouble   height = gimp_item_get_height (item);

      gegl_node_set (filter->translate,
                     "x", (gdouble) 0.0,
                     "y", (gdouble) 0.0,
                     NULL);

      gegl_node_set (filter->crop,
                     "width",  width,
                     "height", height,
                     NULL);

      gimp_applicator_set_apply_offset (filter->applicator, 0, 0);
    }
}

static void
gimp_drawable_filter_get_preview_rect (GimpDrawableFilter *filter,
                                       gboolean            enabled,
                                       GimpAlignmentType   alignment,
                                       gdouble             position,
                                       GeglRectangle      *rect)
{
  gint width;
  gint height;

  rect->x      = 0;
  rect->y      = 0;
  rect->width  = gimp_item_get_width  (GIMP_ITEM (filter->drawable));
  rect->height = gimp_item_get_height (GIMP_ITEM (filter->drawable));

  width  = rect->width;
  height = rect->height;

  if (enabled)
    {
      switch (alignment)
        {
        case GIMP_ALIGN_LEFT:
          rect->width *= position;
          break;

        case GIMP_ALIGN_RIGHT:
          rect->width *= (1.0 - position);
          rect->x = width - rect->width;
          break;

        case GIMP_ALIGN_TOP:
          rect->height *= position;
          break;

        case GIMP_ALIGN_BOTTOM:
          rect->height *= (1.0 - position);
          rect->y = height - rect->height;
          break;

        default:
          g_return_if_reached ();
        }
    }
}

static void
gimp_drawable_filter_sync_preview (GimpDrawableFilter *filter,
                                   gboolean            old_enabled,
                                   GimpAlignmentType   old_alignment,
                                   gdouble             old_position)
{
  GeglRectangle old_rect;
  GeglRectangle new_rect;

  gimp_drawable_filter_get_preview_rect (filter,
                                         old_enabled,
                                         old_alignment,
                                         old_position,
                                         &old_rect);

  gimp_drawable_filter_get_preview_rect (filter,
                                         filter->preview_enabled,
                                         filter->preview_alignment,
                                         filter->preview_position,
                                         &new_rect);

  gimp_applicator_set_preview (filter->applicator,
                               filter->preview_enabled,
                               &new_rect);

  if (old_rect.x      != new_rect.x     ||
      old_rect.y      != new_rect.y     ||
      old_rect.width  != new_rect.width ||
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

          gimp_drawable_filter_update_drawable (filter,
                                                (const GeglRectangle *) &rect);
        }

      cairo_region_destroy (region);
    }
}

static void
gimp_drawable_filter_sync_opacity (GimpDrawableFilter *filter)
{
  gimp_applicator_set_opacity (filter->applicator,
                               filter->opacity);
}

static void
gimp_drawable_filter_sync_mode (GimpDrawableFilter *filter)
{
  gimp_applicator_set_mode (filter->applicator,
                            filter->paint_mode,
                            filter->blend_space,
                            filter->composite_space,
                            filter->composite_mode);
}

static void
gimp_drawable_filter_sync_affect (GimpDrawableFilter *filter)
{
  GimpComponentMask active_mask;

  active_mask = gimp_drawable_get_active_mask (filter->drawable);

  /*  don't let the filter affect the drawable projection's alpha,
   *  because it can't affect the drawable buffer's alpha either when
   *  finally merged (see bug #699279)
   */
  if (! gimp_drawable_has_alpha (filter->drawable))
    active_mask &= ~GIMP_COMPONENT_MASK_ALPHA;

  gimp_applicator_set_affect (filter->applicator, active_mask);
}

static void
gimp_drawable_filter_sync_mask (GimpDrawableFilter *filter)
{
  GimpImage   *image = gimp_item_get_image (GIMP_ITEM (filter->drawable));
  GimpChannel *mask  = gimp_image_get_mask (image);

  if (gimp_channel_is_empty (mask))
    {
      gimp_applicator_set_mask_buffer (filter->applicator, NULL);
    }
  else
    {
      GeglBuffer *mask_buffer;
      gint        offset_x, offset_y;

      mask_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));
      gimp_item_get_offset (GIMP_ITEM (filter->drawable),
                            &offset_x, &offset_y);

      gimp_applicator_set_mask_buffer (filter->applicator, mask_buffer);
      gimp_applicator_set_mask_offset (filter->applicator,
                                       -offset_x, -offset_y);
    }

  gimp_item_mask_intersect (GIMP_ITEM (filter->drawable),
                            &filter->filter_area.x,
                            &filter->filter_area.y,
                            &filter->filter_area.width,
                            &filter->filter_area.height);
}

static void
gimp_drawable_filter_sync_transform (GimpDrawableFilter *filter)
{
  GimpColorManaged *managed = GIMP_COLOR_MANAGED (filter->drawable);

  if (filter->color_managed)
    {
      gboolean          has_input;
      const Babl       *drawable_format;
      const Babl       *input_format;
      const Babl       *output_format;
      GimpColorProfile *drawable_profile;
      GimpColorProfile *input_profile;
      GimpColorProfile *output_profile;
      guint32           dummy;

      has_input = gegl_node_has_pad (filter->operation, "input");

      drawable_format = gimp_drawable_get_format (filter->drawable);
      if (has_input)
        input_format  = gimp_gegl_node_get_format (filter->operation, "input");
      output_format   = gimp_gegl_node_get_format (filter->operation, "output");

      g_printerr ("drawable format:      %s\n", babl_get_name (drawable_format));
      if (has_input)
        g_printerr ("filter input format:  %s\n", babl_get_name (input_format));
      g_printerr ("filter output format: %s\n", babl_get_name (output_format));

      /*  convert the drawable format to float, so we get a precise
       *  color transform
       */
      drawable_format =
        gimp_babl_format (gimp_babl_format_get_base_type (drawable_format),
                          gimp_babl_precision (GIMP_COMPONENT_TYPE_FLOAT,
                                               gimp_babl_format_get_linear (drawable_format)),
                          babl_format_has_alpha (drawable_format));

      /*  convert the filter input/output formats to something we have
       *  built-in color profiles for (see the get_color_profile()
       *  calls below)
       */
      if (has_input)
        input_format = gimp_color_profile_get_lcms_format (input_format,  &dummy);
      output_format  = gimp_color_profile_get_lcms_format (output_format, &dummy);

      g_printerr ("profile transform drawable format: %s\n",
                  babl_get_name (drawable_format));
      if (has_input)
        g_printerr ("profile transform input format:    %s\n",
                    babl_get_name (input_format));
      g_printerr ("profile transform output format:   %s\n",
                  babl_get_name (output_format));

      drawable_profile = gimp_color_managed_get_color_profile (managed);
      if (has_input)
        input_profile  = gimp_babl_format_get_color_profile (input_format);
      output_profile   = gimp_babl_format_get_color_profile (output_format);

      if ((has_input && ! gimp_color_transform_can_gegl_copy (drawable_profile,
                                                              input_profile)) ||
          ! gimp_color_transform_can_gegl_copy (output_profile,
                                                drawable_profile))
        {
          g_printerr ("using gimp:profile-transform\n");

          if (has_input)
            {
              gegl_node_set (filter->transform_before,
                             "operation",    "gimp:profile-transform",
                             "src-profile",  drawable_profile,
                             "src-format",   drawable_format,
                             "dest-profile", input_profile,
                             "dest-format",  input_format,
                             NULL);
            }

          gegl_node_set (filter->transform_after,
                         "operation",    "gimp:profile-transform",
                         "src-profile",  output_profile,
                         "src-format",   output_format,
                         "dest-profile", drawable_profile,
                         "dest-format",  drawable_format,
                         NULL);

          return;
        }
    }

  g_printerr ("using gegl copy\n");

  gegl_node_set (filter->transform_before,
                 "operation", "gegl:nop",
                 NULL);

  gegl_node_set (filter->transform_after,
                 "operation", "gegl:nop",
                 NULL);
}

static void
gimp_drawable_filter_sync_gamma_hack (GimpDrawableFilter *filter)
{
  if (filter->gamma_hack)
    {
      const Babl *drawable_format;
      const Babl *cast_format;

      drawable_format =
        gimp_drawable_get_format_with_alpha (filter->drawable);

      cast_format =
        gimp_babl_format (gimp_babl_format_get_base_type (drawable_format),
                          gimp_babl_precision (gimp_babl_format_get_component_type (drawable_format),
                                               ! gimp_babl_format_get_linear (drawable_format)),
                          TRUE);

      gegl_node_set (filter->cast_before,
                     "operation",     "gegl:cast-format",
                     "input-format",  drawable_format,
                     "output-format", cast_format,
                     NULL);

      gegl_node_set (filter->cast_after,
                     "operation",     "gegl:cast-format",
                     "input-format",  cast_format,
                     "output-format", drawable_format,
                     NULL);
    }
  else
    {
      gegl_node_set (filter->cast_before,
                     "operation", "gegl:nop",
                     NULL);

      gegl_node_set (filter->cast_after,
                     "operation", "gegl:nop",
                     NULL);
    }
}

static gboolean
gimp_drawable_filter_is_filtering (GimpDrawableFilter *filter)
{
  return gimp_drawable_has_filter (filter->drawable,
                                   GIMP_FILTER (filter));
}

static gboolean
gimp_drawable_filter_add_filter (GimpDrawableFilter *filter)
{
  if (! gimp_drawable_filter_is_filtering (filter))
    {
      GimpImage *image = gimp_item_get_image (GIMP_ITEM (filter->drawable));

      gimp_viewable_preview_freeze (GIMP_VIEWABLE (filter->drawable));

      gimp_drawable_filter_sync_mask (filter);
      gimp_drawable_filter_sync_region (filter);
      gimp_drawable_filter_sync_preview (filter,
                                         filter->preview_enabled,
                                         filter->preview_alignment,
                                         filter->preview_position);
      gimp_drawable_filter_sync_opacity (filter);
      gimp_drawable_filter_sync_mode (filter);
      gimp_drawable_filter_sync_affect (filter);
      gimp_drawable_filter_sync_transform (filter);
      gimp_drawable_filter_sync_gamma_hack (filter);

      gimp_drawable_add_filter (filter->drawable,
                                GIMP_FILTER (filter));

      g_signal_connect (image, "component-active-changed",
                        G_CALLBACK (gimp_drawable_filter_affect_changed),
                        filter);
      g_signal_connect (image, "mask-changed",
                        G_CALLBACK (gimp_drawable_filter_mask_changed),
                        filter);
      g_signal_connect (image, "profile-changed",
                        G_CALLBACK (gimp_drawable_filter_profile_changed),
                        filter);
      g_signal_connect (filter->drawable, "removed",
                        G_CALLBACK (gimp_drawable_filter_drawable_removed),
                        filter);

      return TRUE;
    }

  return FALSE;
}

static gboolean
gimp_drawable_filter_remove_filter (GimpDrawableFilter *filter)
{
  if (gimp_drawable_filter_is_filtering (filter))
    {
      GimpImage *image = gimp_item_get_image (GIMP_ITEM (filter->drawable));

      g_signal_handlers_disconnect_by_func (filter->drawable,
                                            gimp_drawable_filter_drawable_removed,
                                            filter);
      g_signal_handlers_disconnect_by_func (image,
                                            gimp_drawable_filter_profile_changed,
                                            filter);
      g_signal_handlers_disconnect_by_func (image,
                                            gimp_drawable_filter_mask_changed,
                                            filter);
      g_signal_handlers_disconnect_by_func (image,
                                            gimp_drawable_filter_affect_changed,
                                            filter);

      gimp_drawable_remove_filter (filter->drawable,
                                   GIMP_FILTER (filter));

      gimp_viewable_preview_thaw (GIMP_VIEWABLE (filter->drawable));

      return TRUE;
    }

  return FALSE;
}

static void
gimp_drawable_filter_update_drawable (GimpDrawableFilter  *filter,
                                      const GeglRectangle *area)
{
  GeglRectangle update_area;

  if (area)
    {
      if (! gimp_rectangle_intersect (area->x,
                                      area->y,
                                      area->width,
                                      area->height,
                                      filter->filter_area.x,
                                      filter->filter_area.y,
                                      filter->filter_area.width,
                                      filter->filter_area.height,
                                      &update_area.x,
                                      &update_area.y,
                                      &update_area.width,
                                      &update_area.height))
        {
          return;
        }
    }
  else
    {
      update_area = filter->filter_area;
    }

  if (update_area.width  > 0 &&
      update_area.height > 0)
    {
      gimp_drawable_update (filter->drawable,
                            update_area.x,
                            update_area.y,
                            update_area.width,
                            update_area.height);

      g_signal_emit (filter, drawable_filter_signals[FLUSH], 0);
    }
}

static void
gimp_drawable_filter_affect_changed (GimpImage          *image,
                                     GimpChannelType     channel,
                                     GimpDrawableFilter *filter)
{
  gimp_drawable_filter_sync_affect (filter);
  gimp_drawable_filter_update_drawable (filter, NULL);
}

static void
gimp_drawable_filter_mask_changed (GimpImage          *image,
                                   GimpDrawableFilter *filter)
{
  gimp_drawable_filter_update_drawable (filter, NULL);

  gimp_drawable_filter_sync_mask (filter);
  gimp_drawable_filter_sync_region (filter);

  gimp_drawable_filter_update_drawable (filter, NULL);
}

static void
gimp_drawable_filter_profile_changed (GimpColorManaged   *managed,
                                      GimpDrawableFilter *filter)
{
  gimp_drawable_filter_sync_transform (filter);
  gimp_drawable_filter_update_drawable (filter, NULL);
}

static void
gimp_drawable_filter_drawable_removed (GimpDrawable       *drawable,
                                       GimpDrawableFilter *filter)
{
  gimp_drawable_filter_remove_filter (filter);
}
