/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-apply-operation.c
 * Copyright (C) 2012 Øyvind Kolås <pippin@gimp.org>
 *                    Sven Neumann <sven@gimp.org>
 *                    Michael Natterer <mitch@gimp.org>
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
#include <gio/gio.h>
#include <gegl.h>

#include "gimp-gegl-types.h"

#include "core/gimp-transform-utils.h"
#include "core/gimp-utils.h"
#include "core/gimpchunkiterator.h"
#include "core/gimpprogress.h"

#include "gimp-gegl-apply-operation.h"
#include "gimp-gegl-loops.h"
#include "gimp-gegl-nodes.h"
#include "gimp-gegl-utils.h"


/* iteration interval when applying an operation interactively
 * (with progress indication)
 */
#define APPLY_OPERATION_INTERACTIVE_INTERVAL    (1.0 / 8.0) /* seconds */

/* iteration interval when applying an operation non-interactively
 * (without progress indication)
 */
#define APPLY_OPERATION_NON_INTERACTIVE_INTERVAL 1.0 /* seconds */


void
gimp_gegl_apply_operation (GeglBuffer          *src_buffer,
                           GimpProgress        *progress,
                           const gchar         *undo_desc,
                           GeglNode            *operation,
                           GeglBuffer          *dest_buffer,
                           const GeglRectangle *dest_rect,
                           gboolean             crop_input)
{
  gimp_gegl_apply_cached_operation (src_buffer,
                                    progress, undo_desc,
                                    operation,
                                    src_buffer != NULL,
                                    dest_buffer,
                                    dest_rect,
                                    crop_input,
                                    NULL, NULL, 0,
                                    FALSE);
}

static void
gimp_gegl_apply_operation_cancel (GimpProgress *progress,
                                  gboolean     *cancel)
{
  *cancel = TRUE;
}

gboolean
gimp_gegl_apply_cached_operation (GeglBuffer          *src_buffer,
                                  GimpProgress        *progress,
                                  const gchar         *undo_desc,
                                  GeglNode            *operation,
                                  gboolean             connect_src_buffer,
                                  GeglBuffer          *dest_buffer,
                                  const GeglRectangle *dest_rect,
                                  gboolean             crop_input,
                                  GeglBuffer          *cache,
                                  const GeglRectangle *valid_rects,
                                  gint                 n_valid_rects,
                                  gboolean             cancelable)
{
  GeglNode          *gegl;
  GeglNode          *effect;
  GeglNode          *dest_node;
  GeglNode          *underlying_operation;
  GeglNode          *operation_src_node = NULL;
  GeglBuffer        *result_buffer;
  GimpChunkIterator *iter;
  cairo_region_t    *region;
  gboolean           progress_started   = FALSE;
  gboolean           cancel             = FALSE;
  gint64             all_pixels;
  gint64             done_pixels;

  g_return_val_if_fail (src_buffer == NULL || GEGL_IS_BUFFER (src_buffer), FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (GEGL_IS_NODE (operation), FALSE);
  g_return_val_if_fail (GEGL_IS_BUFFER (dest_buffer), FALSE);
  g_return_val_if_fail (cache == NULL || GEGL_IS_BUFFER (cache), FALSE);
  g_return_val_if_fail (valid_rects == NULL || cache != NULL, FALSE);
  g_return_val_if_fail (valid_rects == NULL || n_valid_rects != 0, FALSE);

  if (! dest_rect)
    dest_rect = gegl_buffer_get_extent (dest_buffer);

  if (progress)
    {
      if (gimp_progress_is_active (progress))
        {
          if (undo_desc)
            gimp_progress_set_text_literal (progress, undo_desc);

          progress_started = FALSE;
          cancelable       = FALSE;
        }
      else
        {
          gimp_progress_start (progress, cancelable, "%s", undo_desc);

          if (cancelable)
            g_signal_connect (progress, "cancel",
                              G_CALLBACK (gimp_gegl_apply_operation_cancel),
                              &cancel);

          progress_started = TRUE;
        }
    }
  else
    {
      cancelable = FALSE;
    }

  gegl_buffer_freeze_changed (dest_buffer);

  underlying_operation = gimp_gegl_node_get_underlying_operation (operation);

  result_buffer = dest_buffer;

  if (result_buffer == src_buffer &&
      ! (gimp_gegl_node_is_point_operation  (underlying_operation) ||
         gimp_gegl_node_is_source_operation (underlying_operation)))
    {
      /* Write the result to a temporary buffer, instead of directly to
       * dest_buffer, since reading and writing the same buffer doesn't
       * generally work with non-point ops when working in chunks.
       *
       * See bug #701875.
       */

      if (cache)
        {
          /* If we have a cache, use it directly as the temporary result
           * buffer, and skip copying the cached results to result_buffer
           * below.  Instead, the cached results are copied together with the
           * newly rendered results in a single step at the end of processing.
           */

          g_warn_if_fail (cache != dest_buffer);

          result_buffer = g_object_ref (cache);

          cache = NULL;
        }
      else
        {
          result_buffer = gegl_buffer_new (
            dest_rect, gegl_buffer_get_format (dest_buffer));
        }
    }

  all_pixels  = (gint64) dest_rect->width * (gint64) dest_rect->height;
  done_pixels = 0;

  region = cairo_region_create_rectangle ((cairo_rectangle_int_t *) dest_rect);

  if (n_valid_rects > 0)
    {
      gint i;

      for (i = 0; i < n_valid_rects; i++)
        {
          GeglRectangle valid_rect;

          if (! gegl_rectangle_intersect (&valid_rect,
                                          &valid_rects[i], dest_rect))
            {
              continue;
            }

          if (cache)
            {
              gimp_gegl_buffer_copy (
                cache,         &valid_rect, GEGL_ABYSS_NONE,
                result_buffer, &valid_rect);
            }

          cairo_region_subtract_rectangle (region,
                                           (cairo_rectangle_int_t *)
                                           &valid_rect);

          done_pixels += (gint64) valid_rect.width * (gint64) valid_rect.height;

          if (progress)
            {
              gimp_progress_set_value (progress,
                                       (gdouble) done_pixels /
                                       (gdouble) all_pixels);
            }
        }
    }

  gegl = gegl_node_new ();

  if (! gegl_node_get_parent (operation))
    gegl_node_add_child (gegl, operation);

  effect = operation;

  if (connect_src_buffer || crop_input)
    {
      GeglNode *src_node;

      operation_src_node = gegl_node_get_producer (operation, "input", NULL);

      src_node = operation_src_node;

      if (connect_src_buffer)
        {
          src_node = gegl_node_new_child (gegl,
                                          "operation", "gegl:buffer-source",
                                          "buffer",    src_buffer,
                                          NULL);
        }

      if (crop_input)
        {
          GeglNode *crop_node;

          crop_node = gegl_node_new_child (gegl,
                                           "operation", "gegl:crop",
                                           "x",         (gdouble) dest_rect->x,
                                           "y",         (gdouble) dest_rect->y,
                                           "width",     (gdouble) dest_rect->width,
                                           "height",    (gdouble) dest_rect->height,
                                           NULL);

          gegl_node_link (src_node, crop_node);

          src_node = crop_node;
        }

      if (! gegl_node_has_pad (operation, "input"))
        {
          effect = gegl_node_new_child (gegl,
                                        "operation", "gimp:normal",
                                        NULL);

          gegl_node_connect (operation, "output", effect,    "aux");
        }

      gegl_node_link (src_node, effect);
    }

  dest_node = gegl_node_new_child (gegl,
                                   "operation", "gegl:write-buffer",
                                   "buffer",    result_buffer,
                                   NULL);

  gegl_node_link (effect, dest_node);

  iter = gimp_chunk_iterator_new (region);

  if (progress &&
      /* avoid the interactive iteration interval for area filters (or meta ops
       * that potentially involve area filters), since their processing speed
       * tends to be sensitive to the chunk size.
       */
      ! gimp_gegl_node_is_area_filter_operation (underlying_operation))
    {
      /* we use a shorter iteration interval for interactive use (when there's
       * progress indication), to stay responsive.
       */
      gimp_chunk_iterator_set_interval (
        iter,
        APPLY_OPERATION_INTERACTIVE_INTERVAL);
    }
  else
    {
      /* we use a longer iteration interval for non-interactive use (when
       * there's no progress indication), or when applying an area filter (see
       * above), as this generally allows for faster processing.  we don't
       * avoid chunking altogether, since *some* chunking is still desirable to
       * reduce the space needed for intermediate results.
       */
      gimp_chunk_iterator_set_interval (
        iter,
        APPLY_OPERATION_NON_INTERACTIVE_INTERVAL);
    }

  while (gimp_chunk_iterator_next (iter))
    {
      GeglRectangle render_rect;

      if (progress)
        {
          while (! cancel && g_main_context_pending (NULL))
            g_main_context_iteration (NULL, FALSE);

          if (cancel)
            break;
        }

      while (gimp_chunk_iterator_get_rect (iter, &render_rect))
        {
          gegl_node_blit (dest_node, 1.0, &render_rect, NULL, NULL, 0,
                          GEGL_BLIT_DEFAULT);

          done_pixels += (gint64) render_rect.width *
                         (gint64) render_rect.height;
        }

      if (progress)
        {
          gimp_progress_set_value (progress,
                                   (gdouble) done_pixels /
                                   (gdouble) all_pixels);
        }
    }

  if (result_buffer != dest_buffer)
    {
      if (! cancel)
        gimp_gegl_buffer_copy (result_buffer, dest_rect, GEGL_ABYSS_NONE,
                               dest_buffer,   dest_rect);

      g_object_unref (result_buffer);
    }

  gegl_buffer_thaw_changed (dest_buffer);

  g_object_unref (gegl);

  if (operation_src_node)
    {
      gegl_node_link (operation_src_node, operation);
    }

  if (progress_started)
    {
      gimp_progress_end (progress);

      if (cancelable)
        g_signal_handlers_disconnect_by_func (progress,
                                              gimp_gegl_apply_operation_cancel,
                                              &cancel);
    }

  return ! cancel;
}

void
gimp_gegl_apply_dither (GeglBuffer   *src_buffer,
                        GimpProgress *progress,
                        const gchar  *undo_desc,
                        GeglBuffer   *dest_buffer,
                        gint          levels,
                        gint          dither_type)
{
  GeglNode *node;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  levels = CLAMP (levels, 2, 65536);

  node = gegl_node_new_child (NULL,
                              "operation",     "gegl:dither",
                              "red-levels",    levels,
                              "green-levels",  levels,
                              "blue-levels",   levels,
                              "alpha-bits",    levels,
                              "dither-method", dither_type,
                              NULL);

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, NULL, FALSE);
  g_object_unref (node);
}

void
gimp_gegl_apply_flatten (GeglBuffer          *src_buffer,
                         GimpProgress        *progress,
                         const gchar         *undo_desc,
                         GeglBuffer          *dest_buffer,
                         GeglColor           *background,
                         GimpLayerColorSpace  composite_space)
{
  GeglNode *node;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));
  g_return_if_fail (GEGL_IS_COLOR (background));

  node = gimp_gegl_create_flatten_node (background, composite_space);

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, NULL, FALSE);
  g_object_unref (node);
}

void
gimp_gegl_apply_feather (GeglBuffer          *src_buffer,
                         GimpProgress        *progress,
                         const gchar         *undo_desc,
                         GeglBuffer          *dest_buffer,
                         const GeglRectangle *dest_rect,
                         gdouble              radius_x,
                         gdouble              radius_y,
                         gboolean             edge_lock)
{
  GaussianBlurAbyssPolicy abyss_policy;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  if (edge_lock)
    abyss_policy = GAUSSIAN_BLUR_ABYSS_CLAMP;
  else
    abyss_policy = GAUSSIAN_BLUR_ABYSS_NONE;

  /* 3.5 is completely magic and picked to visually match the old
   * gaussian_blur_region() on a crappy laptop display
   */
  gimp_gegl_apply_gaussian_blur (src_buffer,
                                 progress, undo_desc,
                                 dest_buffer, dest_rect,
                                 radius_x / 3.5,
                                 radius_y / 3.5,
                                 abyss_policy);
}

void
gimp_gegl_apply_border (GeglBuffer             *src_buffer,
                        GimpProgress           *progress,
                        const gchar            *undo_desc,
                        GeglBuffer             *dest_buffer,
                        const GeglRectangle    *dest_rect,
                        gint                    radius_x,
                        gint                    radius_y,
                        GimpChannelBorderStyle  style,
                        gboolean                edge_lock)
{
  GeglNode *node;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  switch (style)
    {
    case GIMP_CHANNEL_BORDER_STYLE_HARD:
    case GIMP_CHANNEL_BORDER_STYLE_FEATHERED:
      {
        gboolean feather = style == GIMP_CHANNEL_BORDER_STYLE_FEATHERED;

        node = gegl_node_new_child (NULL,
                                    "operation", "gimp:border",
                                    "radius-x",  radius_x,
                                    "radius-y",  radius_y,
                                    "feather",   feather,
                                    "edge-lock", edge_lock,
                                    NULL);
      }
      break;

    case GIMP_CHANNEL_BORDER_STYLE_SMOOTH:
      {
        GeglNode *input, *output;
        GeglNode *grow, *shrink, *subtract;

        node   = gegl_node_new ();

        input  = gegl_node_get_input_proxy (node, "input");
        output = gegl_node_get_output_proxy (node, "output");

        /* Duplicate special-case behavior of "gimp:border". */
        if (radius_x == 1 && radius_y == 1)
          {
            grow   = gegl_node_new_child (node,
                                          "operation", "gegl:nop",
                                          NULL);
            shrink = gegl_node_new_child (node,
                                          "operation", "gimp:shrink",
                                          "radius-x",  1,
                                          "radius-y",  1,
                                          "edge-lock", edge_lock,
                                          NULL);
          }
        else
          {
            grow   = gegl_node_new_child (node,
                                          "operation", "gimp:grow",
                                          "radius-x",  radius_x,
                                          "radius-y",  radius_y,
                                          NULL);
            shrink = gegl_node_new_child (node,
                                          "operation", "gimp:shrink",
                                          "radius-x",  radius_x + 1,
                                          "radius-y",  radius_y + 1,
                                          "edge-lock", edge_lock,
                                          NULL);
          }

        subtract = gegl_node_new_child (node,
                                        "operation", "gegl:subtract",
                                        NULL);

        gegl_node_link_many (input, grow, subtract, output, NULL);
        gegl_node_link (input, shrink);
        gegl_node_connect (shrink, "output", subtract, "aux");
      }
      break;

    default:
      gimp_assert_not_reached ();
    }

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, dest_rect, TRUE);
  g_object_unref (node);
}

void
gimp_gegl_apply_grow (GeglBuffer          *src_buffer,
                      GimpProgress        *progress,
                      const gchar         *undo_desc,
                      GeglBuffer          *dest_buffer,
                      const GeglRectangle *dest_rect,
                      gint                 radius_x,
                      gint                 radius_y)
{
  GeglNode *node;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  node = gegl_node_new_child (NULL,
                              "operation", "gimp:grow",
                              "radius-x",  radius_x,
                              "radius-y",  radius_y,
                              NULL);

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, dest_rect, TRUE);
  g_object_unref (node);
}

void
gimp_gegl_apply_shrink (GeglBuffer          *src_buffer,
                        GimpProgress        *progress,
                        const gchar         *undo_desc,
                        GeglBuffer          *dest_buffer,
                        const GeglRectangle *dest_rect,
                        gint                 radius_x,
                        gint                 radius_y,
                        gboolean             edge_lock)
{
  GeglNode *node;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  node = gegl_node_new_child (NULL,
                              "operation", "gimp:shrink",
                              "radius-x",  radius_x,
                              "radius-y",  radius_y,
                              "edge-lock", edge_lock,
                              NULL);

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, dest_rect, TRUE);
  g_object_unref (node);
}

void
gimp_gegl_apply_flood (GeglBuffer          *src_buffer,
                       GimpProgress        *progress,
                       const gchar         *undo_desc,
                       GeglBuffer          *dest_buffer,
                       const GeglRectangle *dest_rect)
{
  GeglNode *node;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  node = gegl_node_new_child (NULL,
                              "operation", "gimp:flood",
                              NULL);

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, dest_rect, TRUE);
  g_object_unref (node);
}

void
gimp_gegl_apply_gaussian_blur (GeglBuffer              *src_buffer,
                               GimpProgress            *progress,
                               const gchar             *undo_desc,
                               GeglBuffer              *dest_buffer,
                               const GeglRectangle     *dest_rect,
                               gdouble                  std_dev_x,
                               gdouble                  std_dev_y,
                               GaussianBlurAbyssPolicy  abyss_policy)
{
  GeglNode *node;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  node = gegl_node_new_child (NULL,
                              "operation",    "gegl:gaussian-blur",
                              "std-dev-x",    std_dev_x,
                              "std-dev-y",    std_dev_y,
                              "abyss-policy", abyss_policy,
                              NULL);

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, dest_rect, FALSE);
  g_object_unref (node);
}

void
gimp_gegl_apply_invert_gamma (GeglBuffer    *src_buffer,
                              GimpProgress  *progress,
                              const gchar   *undo_desc,
                              GeglBuffer    *dest_buffer)
{
  GeglNode *node;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  node = gegl_node_new_child (NULL,
                              "operation", "gegl:invert-gamma",
                              NULL);

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, NULL, FALSE);
  g_object_unref (node);
}

void
gimp_gegl_apply_invert_linear (GeglBuffer    *src_buffer,
                               GimpProgress  *progress,
                               const gchar   *undo_desc,
                               GeglBuffer    *dest_buffer)
{
  GeglNode *node;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  node = gegl_node_new_child (NULL,
                              "operation", "gegl:invert-linear",
                              NULL);

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, NULL, FALSE);
  g_object_unref (node);
}

void
gimp_gegl_apply_opacity (GeglBuffer    *src_buffer,
                         GimpProgress  *progress,
                         const gchar   *undo_desc,
                         GeglBuffer    *dest_buffer,
                         GeglBuffer    *mask,
                         gint           mask_offset_x,
                         gint           mask_offset_y,
                         gdouble        opacity)
{
  GeglNode *node;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));
  g_return_if_fail (mask == NULL || GEGL_IS_BUFFER (mask));

  node = gimp_gegl_create_apply_opacity_node (mask,
                                              mask_offset_x,
                                              mask_offset_y,
                                              opacity);

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, NULL, FALSE);
  g_object_unref (node);
}

void
gimp_gegl_apply_scale (GeglBuffer            *src_buffer,
                       GimpProgress          *progress,
                       const gchar           *undo_desc,
                       GeglBuffer            *dest_buffer,
                       GimpInterpolationType  interpolation_type,
                       gdouble                x,
                       gdouble                y)
{
  GeglNode *node;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  node = gegl_node_new_child (NULL,
                              "operation",   "gegl:scale-ratio",
                              "origin-x",     0.0,
                              "origin-y",     0.0,
                              "sampler",      interpolation_type,
                              "abyss-policy", GEGL_ABYSS_CLAMP,
                              "x",            x,
                              "y",            y,
                              NULL);

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, NULL, FALSE);
  g_object_unref (node);
}

void
gimp_gegl_apply_set_alpha (GeglBuffer    *src_buffer,
                           GimpProgress  *progress,
                           const gchar   *undo_desc,
                           GeglBuffer    *dest_buffer,
                           gdouble        value)
{
  GeglNode *node;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  node = gegl_node_new_child (NULL,
                              "operation", "gimp:set-alpha",
                              "value",     value,
                              NULL);

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, NULL, FALSE);
  g_object_unref (node);
}

void
gimp_gegl_apply_threshold (GeglBuffer    *src_buffer,
                           GimpProgress  *progress,
                           const gchar   *undo_desc,
                           GeglBuffer    *dest_buffer,
                           gdouble        value)
{
  GeglNode *node;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  node = gegl_node_new_child (NULL,
                              "operation", "gegl:threshold",
                              "value",     value,
                              NULL);

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, NULL, FALSE);
  g_object_unref (node);
}

void
gimp_gegl_apply_transform (GeglBuffer            *src_buffer,
                           GimpProgress          *progress,
                           const gchar           *undo_desc,
                           GeglBuffer            *dest_buffer,
                           GimpInterpolationType  interpolation_type,
                           GimpMatrix3           *transform)
{
  GeglNode *node;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  node = gegl_node_new_child (NULL,
                              "operation", "gegl:transform",
                              "near-z",    GIMP_TRANSFORM_NEAR_Z,
                              "sampler",   interpolation_type,
                              NULL);

  gimp_gegl_node_set_matrix (node, transform);

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, NULL, FALSE);
  g_object_unref (node);
}
