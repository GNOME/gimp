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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gio/gio.h>
#include <gegl.h>

#include "gimp-gegl-types.h"

#include "core/gimp-utils.h"
#include "core/gimpprogress.h"

#include "gimp-gegl-apply-operation.h"
#include "gimp-gegl-nodes.h"
#include "gegl/gimp-gegl-utils.h"


void
gimp_gegl_apply_operation (GeglBuffer          *src_buffer,
                           GimpProgress        *progress,
                           const gchar         *undo_desc,
                           GeglNode            *operation,
                           GeglBuffer          *dest_buffer,
                           const GeglRectangle *dest_rect)
{
  gimp_gegl_apply_cached_operation (src_buffer,
                                    progress, undo_desc,
                                    operation,
                                    dest_buffer,
                                    dest_rect,
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
                                  GeglBuffer          *dest_buffer,
                                  const GeglRectangle *dest_rect,
                                  GeglBuffer          *cache,
                                  const GeglRectangle *valid_rects,
                                  gint                 n_valid_rects,
                                  gboolean             cancellable)
{
  GeglNode      *gegl;
  GeglNode      *dest_node;
  GeglRectangle  rect = { 0, };
  GeglProcessor *processor        = NULL;
  gboolean       progress_started = FALSE;
  gdouble        value;
  gboolean       cancel           = FALSE;

  g_return_val_if_fail (src_buffer == NULL || GEGL_IS_BUFFER (src_buffer), FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (GEGL_IS_NODE (operation), FALSE);
  g_return_val_if_fail (GEGL_IS_BUFFER (dest_buffer), FALSE);
  g_return_val_if_fail (cache == NULL || GEGL_IS_BUFFER (cache), FALSE);
  g_return_val_if_fail (valid_rects == NULL || cache != NULL, FALSE);
  g_return_val_if_fail (valid_rects == NULL || n_valid_rects != 0, FALSE);

  if (dest_rect)
    {
      rect = *dest_rect;
    }
  else
    {
      rect = *GEGL_RECTANGLE (0, 0, gegl_buffer_get_width  (dest_buffer),
                                    gegl_buffer_get_height (dest_buffer));
    }

  gegl = gegl_node_new ();

  if (! gegl_node_get_parent (operation))
    gegl_node_add_child (gegl, operation);

  if (src_buffer && gegl_node_has_pad (operation, "input"))
    {
      GeglNode *src_node;

      /* dup() because reading and writing the same buffer doesn't
       * work with area ops when using a processor. See bug #701875.
       */
      if (progress && (src_buffer == dest_buffer))
        src_buffer = gegl_buffer_dup (src_buffer);
      else
        g_object_ref (src_buffer);

      src_node = gegl_node_new_child (gegl,
                                      "operation", "gegl:buffer-source",
                                      "buffer",    src_buffer,
                                      NULL);

      g_object_unref (src_buffer);

      gegl_node_connect_to (src_node,  "output",
                            operation, "input");
    }

  dest_node = gegl_node_new_child (gegl,
                                   "operation", "gegl:write-buffer",
                                   "buffer",    dest_buffer,
                                   NULL);

  gegl_node_connect_to (operation, "output",
                        dest_node, "input");

  if (progress)
    {
      processor = gegl_node_new_processor (dest_node, &rect);

      if (gimp_progress_is_active (progress))
        {
          if (undo_desc)
            gimp_progress_set_text_literal (progress, undo_desc);

          progress_started = FALSE;
          cancellable      = FALSE;
        }
      else
        {
          gimp_progress_start (progress, cancellable, "%s", undo_desc);

          if (cancellable)
            g_signal_connect (progress, "cancel",
                              G_CALLBACK (gimp_gegl_apply_operation_cancel),
                              &cancel);

          progress_started = TRUE;
        }
    }

  if (cache)
    {
      cairo_region_t *region;
      gint            all_pixels;
      gint            done_pixels = 0;
      gint            n_rects;
      gint            i;

      region = cairo_region_create_rectangle ((cairo_rectangle_int_t *) &rect);

      all_pixels = rect.width * rect.height;

      for (i = 0; i < n_valid_rects; i++)
        {
          gegl_buffer_copy (cache,       valid_rects + i, GEGL_ABYSS_NONE,
                            dest_buffer, valid_rects + i);

          cairo_region_subtract_rectangle (region,
                                           (cairo_rectangle_int_t *)
                                           valid_rects + i);

          done_pixels += valid_rects[i].width * valid_rects[i].height;

          if (progress)
            gimp_progress_set_value (progress,
                                     (gdouble) done_pixels /
                                     (gdouble) all_pixels);
        }

      n_rects = cairo_region_num_rectangles (region);

      for (i = 0; ! cancel && (i < n_rects); i++)
        {
          cairo_rectangle_int_t render_rect;

          cairo_region_get_rectangle (region, i, &render_rect);

          if (progress)
            {
              gint rect_pixels = render_rect.width * render_rect.height;

#ifdef REUSE_PROCESSOR
              gegl_processor_set_rectangle (processor,
                                            (GeglRectangle *) &render_rect);
#else
              g_object_unref (processor);
              processor = gegl_node_new_processor (dest_node,
                                                   (GeglRectangle *) &render_rect);
#endif

              while (! cancel && gegl_processor_work (processor, &value))
                {
                  gimp_progress_set_value (progress,
                                           ((gdouble) done_pixels +
                                            value * rect_pixels) /
                                           (gdouble) all_pixels);

                  if (cancellable)
                    while (! cancel && g_main_context_pending (NULL))
                      g_main_context_iteration (NULL, FALSE);
                }

              done_pixels += rect_pixels;
            }
          else
            {
              gegl_node_blit (dest_node, 1.0, (GeglRectangle *) &render_rect,
                              NULL, NULL, 0, GEGL_BLIT_DEFAULT);
            }
        }

      cairo_region_destroy (region);
    }
  else
    {
      if (progress)
        {
          while (! cancel && gegl_processor_work (processor, &value))
            {
              gimp_progress_set_value (progress, value);

              if (cancellable)
                while (! cancel && g_main_context_pending (NULL))
                  g_main_context_iteration (NULL, FALSE);
            }
        }
      else
        {
          gegl_node_blit (dest_node, 1.0, &rect,
                          NULL, NULL, 0, GEGL_BLIT_DEFAULT);
        }
    }

  if (processor)
    g_object_unref (processor);

  g_object_unref (gegl);

  if (progress_started)
    {
      gimp_progress_end (progress);

      if (cancellable)
        g_signal_handlers_disconnect_by_func (progress,
                                              gimp_gegl_apply_operation_cancel,
                                              &cancel);
    }

  return ! cancel;
}

void
gimp_gegl_apply_color_reduction (GeglBuffer   *src_buffer,
                                 GimpProgress *progress,
                                 const gchar  *undo_desc,
                                 GeglBuffer   *dest_buffer,
                                 gint          bits,
                                 gint          dither_type)
{
  GeglNode *node;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  node = gegl_node_new_child (NULL,
                              "operation",     "gegl:color-reduction",
                              "red-bits",      bits,
                              "green-bits",    bits,
                              "blue-bits",     bits,
                              "alpha-bits",    bits,
                              "dither-method", dither_type,
                              NULL);

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, NULL);
  g_object_unref (node);
}

void
gimp_gegl_apply_flatten (GeglBuffer    *src_buffer,
                         GimpProgress  *progress,
                         const gchar   *undo_desc,
                         GeglBuffer    *dest_buffer,
                         const GimpRGB *background)
{
  GeglNode *node;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));
  g_return_if_fail (background != NULL);

  node = gimp_gegl_create_flatten_node (background);

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, NULL);
  g_object_unref (node);
}

void
gimp_gegl_apply_feather (GeglBuffer          *src_buffer,
                         GimpProgress        *progress,
                         const gchar         *undo_desc,
                         GeglBuffer          *dest_buffer,
                         const GeglRectangle *dest_rect,
                         gdouble              radius_x,
                         gdouble              radius_y)
{
  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  /* 3.5 is completely magic and picked to visually match the old
   * gaussian_blur_region() on a crappy laptop display
   */
  gimp_gegl_apply_gaussian_blur (src_buffer,
                                 progress, undo_desc,
                                 dest_buffer, dest_rect,
                                 radius_x / 3.5,
                                 radius_y / 3.5);
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
        gegl_node_connect_to (shrink, "output", subtract, "aux");
      }
      break;

    default:
      g_assert_not_reached ();
    }

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, dest_rect);
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
                             node, dest_buffer, dest_rect);
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
                             node, dest_buffer, dest_rect);
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
                             node, dest_buffer, dest_rect);
  g_object_unref (node);
}

void
gimp_gegl_apply_gaussian_blur (GeglBuffer          *src_buffer,
                               GimpProgress        *progress,
                               const gchar         *undo_desc,
                               GeglBuffer          *dest_buffer,
                               const GeglRectangle *dest_rect,
                               gdouble              std_dev_x,
                               gdouble              std_dev_y)
{
  GeglNode *node;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  node = gegl_node_new_child (NULL,
                              "operation", "gegl:gaussian-blur",
                              "std-dev-x", std_dev_x,
                              "std-dev-y", std_dev_y,
                              NULL);

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, dest_rect);
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
                             node, dest_buffer, NULL);
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
                             node, dest_buffer, NULL);
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
                             node, dest_buffer, NULL);
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
                              "operation", "gegl:scale-ratio",
                              "origin-x",   0.0,
                              "origin-y",   0.0,
                              "sampler",    interpolation_type,
                              "x",          x,
                              "y",          y,
                              NULL);

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, NULL);
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
                             node, dest_buffer, NULL);
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
                             node, dest_buffer, NULL);
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
                              "sampler",   interpolation_type,
                              NULL);

  gimp_gegl_node_set_matrix (node, transform);

  gimp_gegl_apply_operation (src_buffer, progress, undo_desc,
                             node, dest_buffer, NULL);
  g_object_unref (node);
}
