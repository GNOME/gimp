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

#include "config.h"

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "config/ligmadialogconfig.h"

#include "gegl/ligma-gegl-apply-operation.h"
#include "gegl/ligma-gegl-mask.h"
#include "gegl/ligma-gegl-mask-combine.h"
#include "gegl/ligma-gegl-utils.h"

#include "operations/layer-modes/ligma-layer-modes.h"

#include "ligma.h"
#include "ligmachannel.h"
#include "ligmadrawable.h"
#include "ligmadrawable-bucket-fill.h"
#include "ligmafilloptions.h"
#include "ligmastrokeoptions.h"
#include "ligmaimage.h"
#include "ligmalineart.h"
#include "ligmapickable.h"
#include "ligmapickable-contiguous-region.h"

#include "ligma-intl.h"


/*  public functions  */

void
ligma_drawable_bucket_fill (LigmaDrawable         *drawable,
                           LigmaFillOptions      *options,
                           gboolean              fill_transparent,
                           LigmaSelectCriterion   fill_criterion,
                           gdouble               threshold,
                           gboolean              sample_merged,
                           gboolean              diagonal_neighbors,
                           gdouble               seed_x,
                           gdouble               seed_y)
{
  LigmaImage  *image;
  GeglBuffer *buffer;
  gdouble     mask_x;
  gdouble     mask_y;
  gint        width, height;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)));
  g_return_if_fail (LIGMA_IS_FILL_OPTIONS (options));

  image = ligma_item_get_image (LIGMA_ITEM (drawable));

  ligma_set_busy (image->ligma);

  buffer = ligma_drawable_get_bucket_fill_buffer (drawable, options,
                                                 fill_transparent, fill_criterion,
                                                 threshold, FALSE, sample_merged,
                                                 diagonal_neighbors,
                                                 seed_x, seed_y, NULL,
                                                 &mask_x, &mask_y, &width, &height);

  if (buffer)
    {
      /*  Apply it to the image  */
      ligma_drawable_apply_buffer (drawable, buffer,
                                  GEGL_RECTANGLE (0, 0, width, height),
                                  TRUE, C_("undo-type", "Bucket Fill"),
                                  ligma_context_get_opacity (LIGMA_CONTEXT (options)),
                                  ligma_context_get_paint_mode (LIGMA_CONTEXT (options)),
                                  LIGMA_LAYER_COLOR_SPACE_AUTO,
                                  LIGMA_LAYER_COLOR_SPACE_AUTO,
                                  ligma_layer_mode_get_paint_composite_mode
                                  (ligma_context_get_paint_mode (LIGMA_CONTEXT (options))),
                                  NULL, (gint) mask_x, mask_y);
      g_object_unref (buffer);

      ligma_drawable_update (drawable, mask_x, mask_y, width, height);
    }

  ligma_unset_busy (image->ligma);
}

/**
 * ligma_drawable_get_bucket_fill_buffer:
 * @drawable: the #LigmaDrawable to edit.
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
 * one-time operation, use ligma_drawable_bucket_fill() instead). If
 * @mask_buffer is not NULL, the intermediate fill mask will also be
 * returned. This fill mask can later be reused in successive calls to
 * ligma_drawable_get_bucket_fill_buffer() for interactive filling.
 *
 * Returns: a fill buffer which can be directly applied to @drawable, or
 *          used in a drawable filter as preview.
 */
GeglBuffer *
ligma_drawable_get_bucket_fill_buffer (LigmaDrawable         *drawable,
                                      LigmaFillOptions      *options,
                                      gboolean              fill_transparent,
                                      LigmaSelectCriterion   fill_criterion,
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
  LigmaImage    *image;
  LigmaPickable *pickable;
  GeglBuffer   *buffer;
  GeglBuffer   *new_mask;
  gboolean      antialias;
  gint          x, y, width, height;
  gint          mask_offset_x = 0;
  gint          mask_offset_y = 0;
  gint          sel_x, sel_y, sel_width, sel_height;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)), NULL);
  g_return_val_if_fail (LIGMA_IS_FILL_OPTIONS (options), NULL);

  image = ligma_item_get_image (LIGMA_ITEM (drawable));

  if (! ligma_item_mask_intersect (LIGMA_ITEM (drawable),
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

  ligma_set_busy (image->ligma);

  if (sample_merged)
    {
      if (! show_all)
        pickable = LIGMA_PICKABLE (image);
      else
        pickable = LIGMA_PICKABLE (ligma_image_get_projection (image));
    }
  else
    {
      pickable = LIGMA_PICKABLE (drawable);
    }

  antialias = ligma_fill_options_get_antialias (options);

  /*  Do a seed bucket fill...To do this, calculate a new
   *  contiguous region.
   */
  new_mask = ligma_pickable_contiguous_region_by_seed (pickable,
                                                      antialias,
                                                      threshold,
                                                      fill_transparent,
                                                      fill_criterion,
                                                      diagonal_neighbors,
                                                      (gint) seed_x,
                                                      (gint) seed_y);
  if (mask_buffer && *mask_buffer)
    {
      ligma_gegl_mask_combine_buffer (new_mask, *mask_buffer,
                                     LIGMA_CHANNEL_OP_ADD, 0, 0);
      g_object_unref (*mask_buffer);
    }

  if (mask_buffer)
    *mask_buffer = new_mask;

  ligma_gegl_mask_bounds (new_mask, &x, &y, &width, &height);
  width  -= x;
  height -= y;

  /*  If there is a selection, intersect the region bounds
   *  with the selection bounds, to avoid processing areas
   *  that are going to be masked out anyway.  The actual
   *  intersection of the fill region with the mask data
   *  happens when combining the fill buffer, in
   *  ligma_drawable_apply_buffer().
   */
  if (! ligma_channel_is_empty (ligma_image_get_mask (image)))
    {
      gint off_x = 0;
      gint off_y = 0;

      if (sample_merged)
        ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);

      if (! ligma_rectangle_intersect (x, y, width, height,

                                      sel_x + off_x, sel_y + off_y,
                                      sel_width,     sel_height,

                                      &x, &y, &width, &height))
        {
          /*  The fill region and the selection are disjoint; bail.  */

          if (! mask_buffer)
            g_object_unref (new_mask);

          ligma_unset_busy (image->ligma);

          return NULL;
        }
    }

  /*  make sure we handle the mask correctly if it was sample-merged  */
  if (sample_merged)
    {
      LigmaItem *item = LIGMA_ITEM (drawable);
      gint      off_x, off_y;

      /*  Limit the channel bounds to the drawable's extents  */
      ligma_item_get_offset (item, &off_x, &off_y);

      ligma_rectangle_intersect (x, y, width, height,

                                off_x, off_y,
                                ligma_item_get_width (item),
                                ligma_item_get_height (item),

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

  buffer = ligma_fill_options_create_buffer (options, drawable,
                                            GEGL_RECTANGLE (0, 0,
                                                            width, height),
                                            -x, -y);

  ligma_gegl_apply_opacity (buffer, NULL, NULL, buffer, new_mask,
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

  ligma_unset_busy (image->ligma);

  return buffer;
}

/**
 * ligma_drawable_get_line_art_fill_buffer:
 * @drawable: the #LigmaDrawable to edit.
 * @line_art: the #LigmaLineArt computed as fill source.
 * @options: the #LigmaFillOptions.
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
 * ligma_drawable_get_line_art_fill_buffer() for interactive filling.
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
ligma_drawable_get_line_art_fill_buffer (LigmaDrawable      *drawable,
                                        LigmaLineArt       *line_art,
                                        LigmaFillOptions   *options,
                                        gboolean           sample_merged,
                                        gboolean           fill_color_as_line_art,
                                        gdouble            fill_color_threshold,
                                        gboolean           line_art_stroke,
                                        LigmaStrokeOptions *stroke_options,
                                        gdouble            seed_x,
                                        gdouble            seed_y,
                                        GeglBuffer       **mask_buffer,
                                        gdouble           *mask_x,
                                        gdouble           *mask_y,
                                        gint              *mask_width,
                                        gint              *mask_height)
{
  LigmaImage  *image;
  GeglBuffer *buffer;
  GeglBuffer *new_mask;
  GeglBuffer *rendered_mask;
  GeglBuffer *fill_buffer   = NULL;
  LigmaRGB     fill_color;
  gint        fill_offset_x = 0;
  gint        fill_offset_y = 0;
  gint        x, y, width, height;
  gint        mask_offset_x = 0;
  gint        mask_offset_y = 0;
  gint        sel_x, sel_y, sel_width, sel_height;
  gdouble     feather_radius;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)), NULL);
  g_return_val_if_fail (LIGMA_IS_FILL_OPTIONS (options), NULL);

  image = ligma_item_get_image (LIGMA_ITEM (drawable));

  if (! ligma_item_mask_intersect (LIGMA_ITEM (drawable),
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

  ligma_set_busy (image->ligma);

  /*  Do a seed bucket fill...To do this, calculate a new
   *  contiguous region.
   */
  if (fill_color_as_line_art)
    {
      LigmaPickable *pickable = ligma_line_art_get_input (line_art);

      /* This cannot be a pattern fill. */
      g_return_val_if_fail (ligma_fill_options_get_style (options) == LIGMA_FILL_STYLE_SOLID,
                            NULL);
      /* Meaningful only in above/below layer cases. */
      g_return_val_if_fail (LIGMA_IS_DRAWABLE (pickable), NULL);

      /* Fill options foreground color is the expected color (can be
       * actual fg or bg in the user context).
       */
      ligma_context_get_foreground (LIGMA_CONTEXT (options), &fill_color);

      fill_buffer   = ligma_drawable_get_buffer (drawable);
      fill_offset_x = ligma_item_get_offset_x (LIGMA_ITEM (drawable)) -
                      ligma_item_get_offset_x (LIGMA_ITEM (pickable));
      fill_offset_y = ligma_item_get_offset_y (LIGMA_ITEM (drawable)) -
                      ligma_item_get_offset_y (LIGMA_ITEM (pickable));
    }
  new_mask = ligma_pickable_contiguous_region_by_line_art (NULL, line_art,
                                                          fill_buffer,
                                                          &fill_color,
                                                          fill_color_threshold,
                                                          fill_offset_x,
                                                          fill_offset_y,
                                                          (gint) seed_x,
                                                          (gint) seed_y);
  if (mask_buffer && *mask_buffer)
    {
      ligma_gegl_mask_combine_buffer (new_mask, *mask_buffer,
                                     LIGMA_CHANNEL_OP_ADD, 0, 0);
      g_object_unref (*mask_buffer);
    }
  if (mask_buffer)
    *mask_buffer = new_mask;

  rendered_mask = ligma_gegl_buffer_dup (new_mask);
  if (ligma_fill_options_get_feather (options, &feather_radius))
    {
      /* Feathering for the line art algorithm is not applied during
       * mask creation because we just want to apply it on the borders
       * of the mask at the end (since the mask can evolve, we don't
       * want to actually touch the mask, but only the intermediate
       * rendered results).
       */
      ligma_gegl_apply_feather (rendered_mask, NULL, NULL, rendered_mask, NULL,
                               feather_radius, feather_radius, TRUE);
    }

  if (line_art_stroke)
    {
      /* Similarly to feathering, stroke only happens on the rendered
       * result, not on the returned mask.
       */
      LigmaChannel       *channel;
      LigmaChannel       *stroked;
      GList             *drawables;
      LigmaContext       *context = ligma_get_user_context (image->ligma);
      GError            *error   = NULL;
      const LigmaRGB      white   = {1.0, 1.0, 1.0, 1.0};

      context = ligma_config_duplicate (LIGMA_CONFIG (context));
      /* As we are stroking a mask, we need to set color to white. */
      ligma_context_set_foreground (LIGMA_CONTEXT (context),
                                   &white);

      channel = ligma_channel_new_from_buffer (image, new_mask, NULL, NULL);
      stroked = ligma_channel_new_from_buffer (image, rendered_mask, NULL, NULL);
      ligma_image_add_hidden_item (image, LIGMA_ITEM (channel));
      ligma_image_add_hidden_item (image, LIGMA_ITEM (stroked));
      drawables = g_list_prepend (NULL, stroked);

      if (! ligma_item_stroke (LIGMA_ITEM (channel), drawables,
                              context,
                              stroke_options,
                              NULL, FALSE, NULL, &error))
        {
          g_warning ("%s: stroking failed with: %s\n",
                     G_STRFUNC, error ? error->message : "no error message");
          g_clear_error (&error);
        }

      g_list_free (drawables);

      ligma_pickable_flush (LIGMA_PICKABLE (stroked));
      g_object_unref (rendered_mask);
      rendered_mask = ligma_drawable_get_buffer (LIGMA_DRAWABLE (stroked));
      g_object_ref (rendered_mask);

      ligma_image_remove_hidden_item (image, LIGMA_ITEM (channel));
      g_object_unref (channel);
      ligma_image_remove_hidden_item (image, LIGMA_ITEM (stroked));
      g_object_unref (stroked);

      g_object_unref (context);
    }

  ligma_gegl_mask_bounds (rendered_mask, &x, &y, &width, &height);
  width  -= x;
  height -= y;

  /*  If there is a selection, intersect the region bounds
   *  with the selection bounds, to avoid processing areas
   *  that are going to be masked out anyway.  The actual
   *  intersection of the fill region with the mask data
   *  happens when combining the fill buffer, in
   *  ligma_drawable_apply_buffer().
   */
  if (! ligma_channel_is_empty (ligma_image_get_mask (image)))
    {
      gint off_x = 0;
      gint off_y = 0;

      if (sample_merged)
        ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);

      if (! ligma_rectangle_intersect (x, y, width, height,

                                      sel_x + off_x, sel_y + off_y,
                                      sel_width,     sel_height,

                                      &x, &y, &width, &height))
        {
          if (! mask_buffer)
            g_object_unref (new_mask);
          /*  The fill region and the selection are disjoint; bail.  */
          ligma_unset_busy (image->ligma);

          return NULL;
        }
    }

  /*  make sure we handle the mask correctly if it was sample-merged  */
  if (sample_merged)
    {
      LigmaItem *item = LIGMA_ITEM (drawable);
      gint      off_x, off_y;

      /*  Limit the channel bounds to the drawable's extents  */
      ligma_item_get_offset (item, &off_x, &off_y);

      ligma_rectangle_intersect (x, y, width, height,

                                off_x, off_y,
                                ligma_item_get_width (item),
                                ligma_item_get_height (item),

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

  buffer = ligma_fill_options_create_buffer (options, drawable,
                                            GEGL_RECTANGLE (0, 0,
                                                            width, height),
                                            -x, -y);

  ligma_gegl_apply_opacity (buffer, NULL, NULL, buffer, rendered_mask,
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

  ligma_unset_busy (image->ligma);

  return buffer;
}
