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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmamath/ligmamath.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "display-types.h"

#include "config/ligmadisplayconfig.h"

#include "core/ligmaimage.h"
#include "core/ligmapickable.h"
#include "core/ligmaprojectable.h"

#include "ligmadisplay.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-transform.h"
#include "ligmadisplayshell-filter.h"
#include "ligmadisplayshell-profile.h"
#include "ligmadisplayshell-render.h"


#define LIGMA_DISPLAY_RENDER_ENABLE_SCALING 1
#define LIGMA_DISPLAY_RENDER_MAX_SCALE      4


void
ligma_display_shell_render_set_scale (LigmaDisplayShell *shell,
                                     gint              scale)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

#if LIGMA_DISPLAY_RENDER_ENABLE_SCALING
  scale = CLAMP (scale, 1, LIGMA_DISPLAY_RENDER_MAX_SCALE);

  if (scale != shell->render_scale)
    {
      shell->render_scale = scale;

      ligma_display_shell_render_invalidate_full (shell);
    }
#endif
}

void
ligma_display_shell_render_invalidate_full (LigmaDisplayShell *shell)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  g_clear_pointer (&shell->render_cache_valid, cairo_region_destroy);
}

void
ligma_display_shell_render_invalidate_area (LigmaDisplayShell *shell,
                                           gint              x,
                                           gint              y,
                                           gint              width,
                                           gint              height)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (shell->render_cache_valid)
    {
      cairo_rectangle_int_t rect;

      rect.x      = x;
      rect.y      = y;
      rect.width  = width;
      rect.height = height;

      cairo_region_subtract_rectangle (shell->render_cache_valid, &rect);
    }
}

void
ligma_display_shell_render_validate_area (LigmaDisplayShell *shell,
                                         gint              x,
                                         gint              y,
                                         gint              width,
                                         gint              height)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (shell->render_cache_valid)
    {
      cairo_rectangle_int_t rect;

      rect.x      = x;
      rect.y      = y;
      rect.width  = width;
      rect.height = height;

      cairo_region_union_rectangle (shell->render_cache_valid, &rect);
    }
}

gboolean
ligma_display_shell_render_is_valid (LigmaDisplayShell *shell,
                                    gint              x,
                                    gint              y,
                                    gint              width,
                                    gint              height)
{
  if (shell->render_cache_valid)
    {
      cairo_rectangle_int_t  rect;
      cairo_region_overlap_t overlap;

      rect.x      = x;
      rect.y      = y;
      rect.width  = width;
      rect.height = height;

      overlap = cairo_region_contains_rectangle (shell->render_cache_valid,
                                                 &rect);

      return (overlap == CAIRO_REGION_OVERLAP_IN);
    }

  return FALSE;
}

void
ligma_display_shell_render (LigmaDisplayShell *shell,
                           cairo_t          *cr,
                           gint              tx,
                           gint              ty,
                           gint              twidth,
                           gint              theight,
                           gdouble           scale)
{
  LigmaDisplayConfig *display_config;
  LigmaImage         *image;
  GeglBuffer        *buffer;
#ifdef USE_NODE_BLIT
  GeglNode          *node;
#endif

  cairo_t           *my_cr;
  gint               cairo_stride;
  guchar            *cairo_data;
  gdouble            x1, y1;
  gdouble            x2, y2;
  gint               x;
  gint               y;
  gint               width;
  gint               height;
  GeglAbyssPolicy    abyss_policy;
  gint               filter = GEGL_BUFFER_FILTER_AUTO;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (scale > 0.0);

  /* map chunk from screen space to scaled image space */
  ligma_display_shell_untransform_bounds_with_scale (shell, scale,
                                                    tx, ty,
                                                    tx + twidth, ty + theight,
                                                    &x1, &y1,
                                                    &x2, &y2);

  x      = floor (x1);
  y      = floor (y1);
  width  = ceil  (x2) - x;
  height = ceil  (y2) - y;

  g_return_if_fail (width  > 0 && width  <= shell->render_buf_width);
  g_return_if_fail (height > 0 && height <= shell->render_buf_height);

  tx      *= shell->render_scale;
  ty      *= shell->render_scale;
  twidth  *= shell->render_scale;
  theight *= shell->render_scale;

  display_config = shell->display->config;

  if (shell->show_all)
    abyss_policy = GEGL_ABYSS_NONE;
  else
    abyss_policy = GEGL_ABYSS_CLAMP;

  if (display_config->zoom_quality != LIGMA_ZOOM_QUALITY_HIGH)
    {
      filter = GEGL_BUFFER_FILTER_NEAREST;
    }

  image  = ligma_display_get_image (shell->display);
  buffer = ligma_pickable_get_buffer (
    ligma_display_shell_get_pickable (shell));
#ifdef USE_NODE_BLIT
  node   = ligma_projectable_get_graph (LIGMA_PROJECTABLE (image));

  ligma_projectable_begin_render (LIGMA_PROJECTABLE (image));
#endif

  if (! shell->render_surface)
    {
      shell->render_surface =
        cairo_surface_create_similar_image (cairo_get_target (cr),
                                            CAIRO_FORMAT_ARGB32,
                                            shell->render_buf_width,
                                            shell->render_buf_height);
    }

  cairo_surface_flush (shell->render_surface);

  if (! shell->render_cache)
    {
      shell->render_cache = cairo_surface_create_similar_image (
        cairo_get_target (cr),
        CAIRO_FORMAT_ARGB32,
        shell->disp_width  * shell->render_scale,
        shell->disp_height * shell->render_scale);
    }

  if (! shell->render_cache_valid)
    {
      shell->render_cache_valid = cairo_region_create ();
    }

  my_cr = cairo_create (shell->render_cache);

  /* clip to chunk bounds, in screen space */
  cairo_rectangle (my_cr, tx, ty, twidth, theight);
  cairo_clip (my_cr);

  /* transform to scaled image space, and apply uneven scaling */
  cairo_scale (my_cr, shell->render_scale, shell->render_scale);
  if (shell->rotate_transform)
    cairo_transform (my_cr, shell->rotate_transform);
  cairo_translate (my_cr, -shell->offset_x, -shell->offset_y);
  cairo_scale (my_cr, shell->scale_x / scale, shell->scale_y / scale);

  cairo_stride = cairo_image_surface_get_stride (shell->render_surface);
  cairo_data   = cairo_image_surface_get_data (shell->render_surface);

  if (shell->profile_transform ||
      ligma_display_shell_has_filter (shell))
    {
      gboolean can_convert_to_u8;

      /*  if there is a profile transform or a display filter, we need
       *  to use temp buffers
       */

      can_convert_to_u8 = ligma_display_shell_profile_can_convert_to_u8 (shell);

      /*  create the filter buffer if we have filters, or can't convert
       *  to u8 directly
       */
      if ((ligma_display_shell_has_filter (shell) || ! can_convert_to_u8) &&
          ! shell->filter_buffer)
        {
          gint fw = shell->render_buf_width;
          gint fh = shell->render_buf_height;

          shell->filter_data =
            gegl_malloc (fw * fh *
                         babl_format_get_bytes_per_pixel (shell->filter_format));

          shell->filter_stride =
            fw * babl_format_get_bytes_per_pixel (shell->filter_format);

          shell->filter_buffer =
            gegl_buffer_linear_new_from_data (shell->filter_data,
                                              shell->filter_format,
                                              GEGL_RECTANGLE (0, 0, fw, fh),
                                              GEGL_AUTO_ROWSTRIDE,
                                              (GDestroyNotify) gegl_free,
                                              shell->filter_data);
        }

      if (! ligma_display_shell_has_filter (shell) || shell->filter_transform)
        {
          /*  if there are no filters, or there is a filter transform,
           *  load the projection pixels into the profile_buffer
           */
#ifndef USE_NODE_BLIT
          gegl_buffer_get (buffer,
                           GEGL_RECTANGLE (x, y, width, height), scale,
                           ligma_projectable_get_format (LIGMA_PROJECTABLE (image)),
                           shell->profile_data, shell->profile_stride,
                           abyss_policy | filter);
#else
          gegl_node_blit (node,
                          scale, GEGL_RECTANGLE (x, y, width, height),
                          ligma_projectable_get_format (LIGMA_PROJECTABLE (image)),
                          shell->profile_data, shell->profile_stride,
                          GEGL_BLIT_CACHE | filter);
#endif
        }
      else
        {
          /*  otherwise, load the pixels directly into the filter_buffer
           */
#ifndef USE_NODE_BLIT
          gegl_buffer_get (buffer,
                           GEGL_RECTANGLE (x, y, width, height), scale,
                           shell->filter_format,
                           shell->filter_data, shell->filter_stride,
                           abyss_policy | filter);
#else
          gegl_node_blit (node,
                          scale, GEGL_RECTANGLE (x, y, width, height),
                          shell->filter_format,
                          shell->filter_data, shell->filter_stride,
                          GEGL_BLIT_CACHE | filter);
#endif
        }

      /*  if there is a filter transform, convert the pixels from
       *  the profile_buffer to the filter_buffer
       */
      if (shell->filter_transform)
        {
          ligma_color_transform_process_buffer (shell->filter_transform,
                                               shell->profile_buffer,
                                               GEGL_RECTANGLE (0, 0,
                                                               width, height),
                                               shell->filter_buffer,
                                               GEGL_RECTANGLE (0, 0,
                                                               width, height));
        }

      /*  if there are filters, apply them
       */
      if (ligma_display_shell_has_filter (shell))
        {
          GeglBuffer *filter_buffer;

          /*  shift the filter_buffer so that the area passed to
           *  the filters is the real render area, allowing for
           *  position-dependent filters
           */
          filter_buffer = g_object_new (GEGL_TYPE_BUFFER,
                                        "source", shell->filter_buffer,
                                        "shift-x", -x,
                                        "shift-y", -y,
                                        NULL);

          /*  convert the filter_buffer in place
           */
          ligma_color_display_stack_convert_buffer (shell->filter_stack,
                                                   filter_buffer,
                                                   GEGL_RECTANGLE (x, y,
                                                                   width, height));

          g_object_unref (filter_buffer);
        }

      /*  if there is a profile transform...
       */
      if (shell->profile_transform)
        {
          if (ligma_display_shell_has_filter (shell))
            {
              /*  if we have filters, convert the pixels in the filter_buffer
               *  in-place
               */
              ligma_color_transform_process_buffer (shell->profile_transform,
                                                   shell->filter_buffer,
                                                   GEGL_RECTANGLE (0, 0,
                                                                   width, height),
                                                   shell->filter_buffer,
                                                   GEGL_RECTANGLE (0, 0,
                                                                   width, height));
            }
          else if (! can_convert_to_u8)
            {
              /*  otherwise, if we can't convert to u8 directly, convert
               *  the pixels from the profile_buffer to the filter_buffer
               */
              ligma_color_transform_process_buffer (shell->profile_transform,
                                                   shell->profile_buffer,
                                                   GEGL_RECTANGLE (0, 0,
                                                                   width, height),
                                                   shell->filter_buffer,
                                                   GEGL_RECTANGLE (0, 0,
                                                                   width, height));
            }
          else
            {
              GeglBuffer *buffer =
                gegl_buffer_linear_new_from_data (cairo_data,
                                                  babl_format ("cairo-ARGB32"),
                                                  GEGL_RECTANGLE (0, 0,
                                                                  width, height),
                                                  cairo_stride,
                                                  NULL, NULL);

              /*  otherwise, convert the profile_buffer directly into
               *  the cairo_buffer
               */
              ligma_color_transform_process_buffer (shell->profile_transform,
                                                   shell->profile_buffer,
                                                   GEGL_RECTANGLE (0, 0,
                                                                   width, height),
                                                   buffer,
                                                   GEGL_RECTANGLE (0, 0,
                                                                   width, height));
              g_object_unref (buffer);
            }
        }

      /*  finally, copy the filter buffer to the cairo-ARGB32 buffer,
       *  if necessary
       */
      if (ligma_display_shell_has_filter (shell) || ! can_convert_to_u8)
        {
          gegl_buffer_get (shell->filter_buffer,
                           GEGL_RECTANGLE (0, 0, width, height), 1.0,
                           babl_format ("cairo-ARGB32"),
                           cairo_data, cairo_stride,
                           GEGL_ABYSS_NONE);
        }
    }
  else
    {
      /*  otherwise we can copy the projection pixels straight to the
       *  cairo-ARGB32 buffer
       */
#ifndef USE_NODE_BLIT
      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (x, y, width, height), scale,
                       babl_format ("cairo-ARGB32"),
                       cairo_data, cairo_stride,
                       abyss_policy | filter);
#else
      gegl_node_blit (node,
                      scale, GEGL_RECTANGLE (x, y, width, height),
                      babl_format ("cairo-ARGB32"),
                      cairo_data, cairo_stride,
                      GEGL_BLIT_CACHE | filter);
#endif
    }

#ifdef USE_NODE_BLIT
  ligma_projectable_end_render (LIGMA_PROJECTABLE (image));
#endif

  cairo_surface_mark_dirty (shell->render_surface);

  /*  SOURCE so the destination's alpha is replaced  */
  cairo_set_operator (my_cr, CAIRO_OPERATOR_SOURCE);

  cairo_set_source_surface (my_cr, shell->render_surface, x, y);
  cairo_paint (my_cr);

  cairo_set_operator (my_cr, CAIRO_OPERATOR_OVER);

  if (shell->mask)
    {
      if (! shell->mask_surface)
        {
          shell->mask_surface =
            cairo_image_surface_create (CAIRO_FORMAT_A8,
                                        shell->render_buf_width,
                                        shell->render_buf_height);
        }

      cairo_surface_flush (shell->mask_surface);

      cairo_stride = cairo_image_surface_get_stride (shell->mask_surface);
      cairo_data   = cairo_image_surface_get_data (shell->mask_surface);

      gegl_buffer_get (shell->mask,
                       GEGL_RECTANGLE (x - floor (shell->mask_offset_x * scale),
                                       y - floor (shell->mask_offset_y * scale),
                                       width, height),
                       scale,
                       babl_format ("Y u8"),
                       cairo_data, cairo_stride,
                       GEGL_ABYSS_NONE | filter);

      if (shell->mask_inverted)
        {
          gint mask_height = height;

          while (mask_height--)
            {
              gint    mask_width = width;
              guchar *d          = cairo_data;

              while (mask_width--)
                {
                  guchar inv = 255 - *d;

                  *d++ = inv;
                }

              cairo_data += cairo_stride;
            }
        }

      cairo_surface_mark_dirty (shell->mask_surface);

      ligma_cairo_set_source_rgba (my_cr, &shell->mask_color);
      cairo_mask_surface (my_cr, shell->mask_surface, x, y);
    }

  cairo_destroy (my_cr);
}
