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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "config/gimpdialogconfig.h"

#include "gegl/gimp-gegl-apply-operation.h"
#include "gegl/gimp-gegl-mask.h"
#include "gegl/gimp-gegl-mask-combine.h"
#include "gegl/gimp-gegl-utils.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpdrawable.h"
#include "gimpdrawable-bucket-fill.h"
#include "gimpfilloptions.h"
#include "gimpstrokeoptions.h"
#include "gimpimage.h"
#include "gimplineart.h"
#include "gimppickable.h"
#include "gimppickable-contiguous-region.h"

#include "gimp-intl.h"


/*  public functions  */

void
gimp_drawable_bucket_fill (GimpDrawable         *drawable,
                           GimpFillOptions      *options,
                           gboolean              fill_transparent,
                           GimpSelectCriterion   fill_criterion,
                           gdouble               threshold,
                           gboolean              sample_merged,
                           gboolean              diagonal_neighbors,
                           gdouble               seed_x,
                           gdouble               seed_y)
{
  GimpImage  *image;
  GeglBuffer *buffer;
  gdouble     mask_x;
  gdouble     mask_y;
  gint        width, height;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_FILL_OPTIONS (options));

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  gimp_set_busy (image->gimp);

  buffer = gimp_drawable_get_bucket_fill_buffer (drawable, options,
                                                 fill_transparent, fill_criterion,
                                                 threshold, FALSE, sample_merged,
                                                 diagonal_neighbors,
                                                 seed_x, seed_y, NULL,
                                                 &mask_x, &mask_y, &width, &height);

  if (buffer)
    {
      /*  Apply it to the image  */
      gimp_drawable_apply_buffer (drawable, buffer,
                                  GEGL_RECTANGLE (0, 0, width, height),
                                  TRUE, C_("undo-type", "Bucket Fill"),
                                  gimp_context_get_opacity (GIMP_CONTEXT (options)),
                                  gimp_context_get_paint_mode (GIMP_CONTEXT (options)),
                                  GIMP_LAYER_COLOR_SPACE_AUTO,
                                  GIMP_LAYER_COLOR_SPACE_AUTO,
                                  gimp_layer_mode_get_paint_composite_mode
                                  (gimp_context_get_paint_mode (GIMP_CONTEXT (options))),
                                  NULL, (gint) mask_x, mask_y);
      g_object_unref (buffer);

      gimp_drawable_update (drawable, mask_x, mask_y, width, height);
    }

  gimp_unset_busy (image->gimp);
}

/**
 * gimp_drawable_get_bucket_fill_buffer:
 * @drawable: the #GimpDrawable to edit.
 * @options:
 * @fill_transparent:
 * @fill_criterion:
 * @threshold:
 * @show_all:
 * @sample_merged:
 * @diagonal_neighbors:
 * @seed_x: X coordinate to start the fill.
 * @seed_y: Y coordinate to start the fill.
 * @mask_buffer: mask of the fill in-progress when in an interactive
 *               filling process. Set to NULL if you need a one-time
 *               fill.
 * @mask_x: returned x bound of @mask_buffer.
 * @mask_y: returned x bound of @mask_buffer.
 * @mask_width: returned width bound of @mask_buffer.
 * @mask_height: returned height bound of @mask_buffer.
 *
 * Creates the fill buffer for a bucket fill operation on @drawable,
 * without actually applying it (if you want to apply it directly as a
 * one-time operation, use gimp_drawable_bucket_fill() instead). If
 * @mask_buffer is not NULL, the intermediate fill mask will also be
 * returned. This fill mask can later be reused in successive calls to
 * gimp_drawable_get_bucket_fill_buffer() for interactive filling.
 *
 * Returns: a fill buffer which can be directly applied to @drawable, or
 *          used in a drawable filter as preview.
 */
GeglBuffer *
gimp_drawable_get_bucket_fill_buffer (GimpDrawable         *drawable,
                                      GimpFillOptions      *options,
                                      gboolean              fill_transparent,
                                      GimpSelectCriterion   fill_criterion,
                                      gdouble               threshold,
                                      gboolean              show_all,
                                      gboolean              sample_merged,
                                      gboolean              diagonal_neighbors,
                                      gdouble               seed_x,
                                      gdouble               seed_y,
                                      GeglBuffer          **mask_buffer,
                                      gdouble              *mask_x,
                                      gdouble              *mask_y,
                                      gint                 *mask_width,
                                      gint                 *mask_height)
{
  GimpImage    *image;
  GimpPickable *pickable;
  GeglBuffer   *buffer;
  GeglBuffer   *new_mask;
  gboolean      antialias;
  gint          x, y, width, height;
  gint          mask_offset_x = 0;
  gint          mask_offset_y = 0;
  gint          sel_x, sel_y, sel_width, sel_height;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_FILL_OPTIONS (options), NULL);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable),
                                  &sel_x, &sel_y, &sel_width, &sel_height))
    return NULL;

  if (mask_buffer && *mask_buffer && threshold == 0.0)
    {
      gfloat pixel;

      gegl_buffer_sample (*mask_buffer, seed_x, seed_y, NULL, &pixel,
                          babl_format ("Y float"),
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

      if (pixel != 0.0)
        /* Already selected. This seed won't change the selection. */
        return NULL;
    }

  gimp_set_busy (image->gimp);

  if (sample_merged)
    {
      if (! show_all)
        pickable = GIMP_PICKABLE (image);
      else
        pickable = GIMP_PICKABLE (gimp_image_get_projection (image));
    }
  else
    {
      pickable = GIMP_PICKABLE (drawable);
    }

  antialias = gimp_fill_options_get_antialias (options);

  /*  Do a seed bucket fill...To do this, calculate a new
   *  contiguous region.
   */
  new_mask = gimp_pickable_contiguous_region_by_seed (pickable,
                                                      antialias,
                                                      threshold,
                                                      fill_transparent,
                                                      fill_criterion,
                                                      diagonal_neighbors,
                                                      (gint) seed_x,
                                                      (gint) seed_y);
  if (mask_buffer && *mask_buffer)
    {
      gimp_gegl_mask_combine_buffer (new_mask, *mask_buffer,
                                     GIMP_CHANNEL_OP_ADD, 0, 0);
      g_object_unref (*mask_buffer);
    }

  if (mask_buffer)
    *mask_buffer = new_mask;

  gimp_gegl_mask_bounds (new_mask, &x, &y, &width, &height);
  width  -= x;
  height -= y;

  /*  If there is a selection, intersect the region bounds
   *  with the selection bounds, to avoid processing areas
   *  that are going to be masked out anyway.  The actual
   *  intersection of the fill region with the mask data
   *  happens when combining the fill buffer, in
   *  gimp_drawable_apply_buffer().
   */
  if (! gimp_channel_is_empty (gimp_image_get_mask (image)))
    {
      gint off_x = 0;
      gint off_y = 0;

      if (sample_merged)
        gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

      if (! gimp_rectangle_intersect (x, y, width, height,

                                      sel_x + off_x, sel_y + off_y,
                                      sel_width,     sel_height,

                                      &x, &y, &width, &height))
        {
          /*  The fill region and the selection are disjoint; bail.  */

          if (! mask_buffer)
            g_object_unref (new_mask);

          gimp_unset_busy (image->gimp);

          return NULL;
        }
    }

  /*  make sure we handle the mask correctly if it was sample-merged  */
  if (sample_merged)
    {
      GimpItem *item = GIMP_ITEM (drawable);
      gint      off_x, off_y;

      /*  Limit the channel bounds to the drawable's extents  */
      gimp_item_get_offset (item, &off_x, &off_y);

      gimp_rectangle_intersect (x, y, width, height,

                                off_x, off_y,
                                gimp_item_get_width (item),
                                gimp_item_get_height (item),

                                &x, &y, &width, &height);

      mask_offset_x = x;
      mask_offset_y = y;

      /*  translate mask bounds to drawable coords  */
      x -= off_x;
      y -= off_y;
    }
  else
    {
      mask_offset_x = x;
      mask_offset_y = y;
    }

  buffer = gimp_fill_options_create_buffer (options, drawable,
                                            GEGL_RECTANGLE (0, 0,
                                                            width, height),
                                            -x, -y);

  gimp_gegl_apply_opacity (buffer, NULL, NULL, buffer, new_mask,
                           -mask_offset_x, -mask_offset_y, 1.0);

  if (mask_x)
    *mask_x = x;
  if (mask_y)
    *mask_y = y;
  if (mask_width)
    *mask_width = width;
  if (mask_height)
    *mask_height = height;

  if (! mask_buffer)
    g_object_unref (new_mask);

  gimp_unset_busy (image->gimp);

  return buffer;
}

/**
 * gimp_drawable_get_line_art_fill_buffer:
 * @drawable: the #GimpDrawable to edit.
 * @line_art: the #GimpLineArt computed as fill source.
 * @options: the #GimpFillOptions.
 * @sample_merged:
 * @fill_color_as_line_art: do we add pixels in @drawable filled with
 *                          fill color to the line art?
 * @fill_color_threshold: threshold value to determine fill color.
 * @seed_x: X coordinate to start the fill.
 * @seed_y: Y coordinate to start the fill.
 * @mask_buffer: mask of the fill in-progress when in an interactive
 *               filling process. Set to NULL if you need a one-time
 *               fill.
 * @mask_x: returned x bound of @mask_buffer.
 * @mask_y: returned y bound of @mask_buffer.
 * @mask_width: returned width bound of @mask_buffer.
 * @mask_height: returned height bound of @mask_buffer.
 *
 * Creates the fill buffer for a bucket fill operation on @drawable
 * based on @line_art and @options, without actually applying it.
 * If @mask_buffer is not NULL, the intermediate fill mask will also be
 * returned. This fill mask can later be reused in successive calls to
 * gimp_drawable_get_line_art_fill_buffer() for interactive filling.
 *
 * The @fill_color_as_line_art option is a special feature where we
 * consider pixels in @drawable already in the fill color as part of the
 * line art. This is a post-process, i.e. that this is not taken into
 * account while @line_art is computed, making this a fast addition
 * processing allowing to close some area manually.
 *
 * Returns: a fill buffer which can be directly applied to @drawable, or
 *          used in a drawable filter as preview.
 */
GeglBuffer *
gimp_drawable_get_line_art_fill_buffer (GimpDrawable      *drawable,
                                        GimpLineArt       *line_art,
                                        GimpFillOptions   *options,
                                        gboolean           sample_merged,
                                        gboolean           fill_color_as_line_art,
                                        gdouble            fill_color_threshold,
                                        gboolean           line_art_stroke,
                                        GimpStrokeOptions *stroke_options,
                                        gdouble            seed_x,
                                        gdouble            seed_y,
                                        GeglBuffer       **mask_buffer,
                                        gdouble           *mask_x,
                                        gdouble           *mask_y,
                                        gint              *mask_width,
                                        gint              *mask_height)
{
  GimpImage  *image;
  GeglBuffer *buffer;
  GeglBuffer *new_mask;
  GeglBuffer *rendered_mask;
  GeglBuffer *fill_buffer   = NULL;
  GeglColor  *fill_color    = NULL;
  gint        fill_offset_x = 0;
  gint        fill_offset_y = 0;
  gint        x, y, width, height;
  gint        mask_offset_x = 0;
  gint        mask_offset_y = 0;
  gint        sel_x, sel_y, sel_width, sel_height;
  gdouble     feather_radius;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_FILL_OPTIONS (options), NULL);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable),
                                  &sel_x, &sel_y, &sel_width, &sel_height))
    return NULL;

  if (mask_buffer && *mask_buffer)
    {
      gfloat pixel;

      gegl_buffer_sample (*mask_buffer, seed_x, seed_y, NULL, &pixel,
                          babl_format ("Y float"),
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

      if (pixel != 0.0)
        /* Already selected. This seed won't change the selection. */
        return NULL;
    }

  gimp_set_busy (image->gimp);

  /*  Do a seed bucket fill...To do this, calculate a new
   *  contiguous region.
   */
  if (fill_color_as_line_art)
    {
      GimpPickable *pickable = gimp_line_art_get_input (line_art);

      /* This cannot be a pattern fill. */
      g_return_val_if_fail (gimp_fill_options_get_style (options) != GIMP_FILL_STYLE_PATTERN,
                            NULL);
      /* Meaningful only in above/below layer cases. */
      g_return_val_if_fail (GIMP_IS_DRAWABLE (pickable), NULL);

      if (gimp_fill_options_get_style (options) == GIMP_FILL_STYLE_FG_COLOR)
        fill_color = gimp_context_get_foreground (GIMP_CONTEXT (options));
      else if (gimp_fill_options_get_style (options) == GIMP_FILL_STYLE_BG_COLOR)
        fill_color = gimp_context_get_background (GIMP_CONTEXT (options));

      g_return_val_if_fail (fill_color != NULL, NULL);

      fill_buffer   = gimp_drawable_get_buffer (drawable);
      fill_offset_x = gimp_item_get_offset_x (GIMP_ITEM (drawable)) -
                      gimp_item_get_offset_x (GIMP_ITEM (pickable));
      fill_offset_y = gimp_item_get_offset_y (GIMP_ITEM (drawable)) -
                      gimp_item_get_offset_y (GIMP_ITEM (pickable));
    }
  new_mask = gimp_pickable_contiguous_region_by_line_art (NULL, line_art,
                                                          fill_buffer,
                                                          fill_color,
                                                          fill_color_threshold,
                                                          fill_offset_x,
                                                          fill_offset_y,
                                                          (gint) seed_x,
                                                          (gint) seed_y);
  if (mask_buffer && *mask_buffer)
    {
      gimp_gegl_mask_combine_buffer (new_mask, *mask_buffer,
                                     GIMP_CHANNEL_OP_ADD, 0, 0);
      g_object_unref (*mask_buffer);
    }
  if (mask_buffer)
    *mask_buffer = new_mask;

  rendered_mask = gimp_gegl_buffer_dup (new_mask);
  if (gimp_fill_options_get_feather (options, &feather_radius))
    {
      /* Feathering for the line art algorithm is not applied during
       * mask creation because we just want to apply it on the borders
       * of the mask at the end (since the mask can evolve, we don't
       * want to actually touch the mask, but only the intermediate
       * rendered results).
       */
      gimp_gegl_apply_feather (rendered_mask, NULL, NULL, rendered_mask, NULL,
                               feather_radius, feather_radius, TRUE);
    }

  if (line_art_stroke)
    {
      /* Similarly to feathering, stroke only happens on the rendered
       * result, not on the returned mask.
       */
      GimpChannel       *channel;
      GimpChannel       *stroked;
      GList             *drawables;
      GimpContext       *context = gimp_get_user_context (image->gimp);
      GError            *error   = NULL;
      GeglColor         *white   = gegl_color_new ("white");

      context = gimp_config_duplicate (GIMP_CONFIG (context));
      /* As we are stroking a mask, we need to set color to white. */
      gimp_context_set_foreground (GIMP_CONTEXT (context), white);
      g_object_unref (white);

      channel = gimp_channel_new_from_buffer (image, new_mask, NULL, NULL);
      stroked = gimp_channel_new_from_buffer (image, rendered_mask, NULL, NULL);
      gimp_image_add_hidden_item (image, GIMP_ITEM (channel));
      gimp_image_add_hidden_item (image, GIMP_ITEM (stroked));
      drawables = g_list_prepend (NULL, stroked);

      if (! gimp_item_stroke (GIMP_ITEM (channel), drawables,
                              context,
                              stroke_options,
                              NULL, FALSE, NULL, &error))
        {
          g_warning ("%s: stroking failed with: %s\n",
                     G_STRFUNC, error ? error->message : "no error message");
          g_clear_error (&error);
        }

      g_list_free (drawables);

      gimp_pickable_flush (GIMP_PICKABLE (stroked));
      g_object_unref (rendered_mask);
      rendered_mask = gimp_drawable_get_buffer (GIMP_DRAWABLE (stroked));
      g_object_ref (rendered_mask);

      gimp_image_remove_hidden_item (image, GIMP_ITEM (channel));
      g_object_unref (channel);
      gimp_image_remove_hidden_item (image, GIMP_ITEM (stroked));
      g_object_unref (stroked);

      g_object_unref (context);
    }

  gimp_gegl_mask_bounds (rendered_mask, &x, &y, &width, &height);
  width  -= x;
  height -= y;

  /*  If there is a selection, intersect the region bounds
   *  with the selection bounds, to avoid processing areas
   *  that are going to be masked out anyway.  The actual
   *  intersection of the fill region with the mask data
   *  happens when combining the fill buffer, in
   *  gimp_drawable_apply_buffer().
   */
  if (! gimp_channel_is_empty (gimp_image_get_mask (image)))
    {
      gint off_x = 0;
      gint off_y = 0;

      if (sample_merged)
        gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

      if (! gimp_rectangle_intersect (x, y, width, height,

                                      sel_x + off_x, sel_y + off_y,
                                      sel_width,     sel_height,

                                      &x, &y, &width, &height))
        {
          if (! mask_buffer)
            g_object_unref (new_mask);
          /*  The fill region and the selection are disjoint; bail.  */
          gimp_unset_busy (image->gimp);

          return NULL;
        }
    }

  /*  make sure we handle the mask correctly if it was sample-merged  */
  if (sample_merged)
    {
      GimpItem *item = GIMP_ITEM (drawable);
      gint      off_x, off_y;

      /*  Limit the channel bounds to the drawable's extents  */
      gimp_item_get_offset (item, &off_x, &off_y);

      gimp_rectangle_intersect (x, y, width, height,

                                off_x, off_y,
                                gimp_item_get_width (item),
                                gimp_item_get_height (item),

                                &x, &y, &width, &height);

      mask_offset_x = x;
      mask_offset_y = y;

      /*  translate mask bounds to drawable coords  */
      x -= off_x;
      y -= off_y;
    }
  else
    {
      mask_offset_x = x;
      mask_offset_y = y;
    }

  buffer = gimp_fill_options_create_buffer (options, drawable,
                                            GEGL_RECTANGLE (0, 0,
                                                            width, height),
                                            -x, -y);

  gimp_gegl_apply_opacity (buffer, NULL, NULL, buffer, rendered_mask,
                           -mask_offset_x, -mask_offset_y, 1.0);

  if (mask_x)
    *mask_x = x;
  if (mask_y)
    *mask_y = y;
  if (mask_width)
    *mask_width = width;
  if (mask_height)
    *mask_height = height;

  if (! mask_buffer)
    g_object_unref (new_mask);

  g_object_unref (rendered_mask);

  gimp_unset_busy (image->gimp);

  return buffer;
}
