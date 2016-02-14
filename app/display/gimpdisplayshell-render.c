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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "config/gimpdisplayconfig.h"

#include "gegl/gimp-gegl-utils.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"
#ifdef USE_NODE_BLIT
#include "core/gimpprojectable.h"
#endif

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-transform.h"
#include "gimpdisplayshell-filter.h"
#include "gimpdisplayshell-profile.h"
#include "gimpdisplayshell-render.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayxfer.h"


/* #define GIMP_DISPLAY_RENDER_ENABLE_SCALING 1 */


void
gimp_display_shell_render (GimpDisplayShell *shell,
                           cairo_t          *cr,
                           gint              x,
                           gint              y,
                           gint              w,
                           gint              h)
{
  GimpImage       *image;
  GeglBuffer      *buffer;
#ifdef USE_NODE_BLIT
  GeglNode        *node;
#endif
  gdouble          scale_x       = 1.0;
  gdouble          scale_y       = 1.0;
  gdouble          buffer_scale  = 1.0;
  gint             viewport_offset_x;
  gint             viewport_offset_y;
  gint             viewport_width;
  gint             viewport_height;
  gint             scaled_x;
  gint             scaled_y;
  gint             scaled_width;
  gint             scaled_height;
  cairo_surface_t *xfer;
  gint             xfer_src_x;
  gint             xfer_src_y;
  gint             mask_src_x = 0;
  gint             mask_src_y = 0;
  gint             cairo_stride;
  guchar          *cairo_data;
  GeglBuffer      *cairo_buffer;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (w > 0 && h > 0);

  image  = gimp_display_get_image (shell->display);
  buffer = gimp_pickable_get_buffer (GIMP_PICKABLE (image));
#ifdef USE_NODE_BLIT
  node   = gimp_projectable_get_graph (GIMP_PROJECTABLE (image));
#endif

#ifdef GIMP_DISPLAY_RENDER_ENABLE_SCALING
  /* if we had this future API, things would look pretty on hires (retina) */
  scale_x = gdk_window_get_scale_factor (gtk_widget_get_window (gtk_widget_get_toplevel (GTK_WIDGET (shell))));
#endif

  scale_x = MIN (scale_x, GIMP_DISPLAY_RENDER_MAX_SCALE);
  scale_y = scale_x;

  if (shell->scale_x > shell->scale_y)
    {
      scale_y *= (shell->scale_x / shell->scale_y);

      buffer_scale = shell->scale_y * scale_y;
    }
  else if (shell->scale_y > shell->scale_x)
    {
      scale_x *= (shell->scale_y / shell->scale_x);

      buffer_scale = shell->scale_x * scale_x;
    }
  else
    {
      buffer_scale = shell->scale_x * scale_x;
    }

  gimp_display_shell_scroll_get_scaled_viewport (shell,
                                                 &viewport_offset_x,
                                                 &viewport_offset_y,
                                                 &viewport_width,
                                                 &viewport_height);

  scaled_x      = floor ((x + viewport_offset_x) * scale_x);
  scaled_y      = floor ((y + viewport_offset_y) * scale_y);
  scaled_width  = ceil (w * scale_x);
  scaled_height = ceil (h * scale_y);

  if (shell->rotate_transform)
    {
      xfer = cairo_surface_create_similar_image (cairo_get_target (cr),
                                                 CAIRO_FORMAT_ARGB32,
                                                 scaled_width,
                                                 scaled_height);
      cairo_surface_mark_dirty (xfer);
      xfer_src_x = 0;
      xfer_src_y = 0;
    }
  else
    {
      xfer = gimp_display_xfer_get_surface (shell->xfer,
                                            scaled_width,
                                            scaled_height,
                                            &xfer_src_x,
                                            &xfer_src_y);
    }

  cairo_stride = cairo_image_surface_get_stride (xfer);
  cairo_data   = cairo_image_surface_get_data (xfer) +
                 xfer_src_y * cairo_stride + xfer_src_x * 4;

  cairo_buffer = gegl_buffer_linear_new_from_data (cairo_data,
                                                   babl_format ("cairo-ARGB32"),
                                                   GEGL_RECTANGLE (0, 0,
                                                                   scaled_width,
                                                                   scaled_height),
                                                   cairo_stride,
                                                   NULL, NULL);

  if (shell->profile_transform ||
      gimp_display_shell_has_filter (shell))
    {
      gboolean can_convert_to_u8;

      /*  if there is a profile transform or a display filter, we need
       *  to use temp buffers
       */

      can_convert_to_u8 = gimp_display_shell_profile_can_convert_to_u8 (shell);

      /*  create the filter buffer if we have filters
       */
      if ((gimp_display_shell_has_filter (shell) || ! can_convert_to_u8) &&
          ! shell->filter_buffer)
        {
          gint w = GIMP_DISPLAY_RENDER_BUF_WIDTH  * GIMP_DISPLAY_RENDER_MAX_SCALE;
          gint h = GIMP_DISPLAY_RENDER_BUF_HEIGHT * GIMP_DISPLAY_RENDER_MAX_SCALE;

          shell->filter_data =
            gegl_malloc (w * h * babl_format_get_bytes_per_pixel (shell->filter_format));

          shell->filter_stride =
            w * babl_format_get_bytes_per_pixel (shell->filter_format);

          shell->filter_buffer =
            gegl_buffer_linear_new_from_data (shell->filter_data,
                                              shell->filter_format,
                                              GEGL_RECTANGLE (0, 0, w, h),
                                              GEGL_AUTO_ROWSTRIDE,
                                              (GDestroyNotify) gegl_free,
                                              shell->filter_data);
        }

      if (shell->profile_transform)
        {
          /*  if there is a profile transform, load the projection
           *  pixels into the profile_buffer
           */
#ifndef USE_NODE_BLIT
          gegl_buffer_get (buffer,
                           GEGL_RECTANGLE (scaled_x, scaled_y,
                                           scaled_width, scaled_height),
                           buffer_scale,
                           shell->profile_src_format,
                           shell->profile_data, shell->profile_stride,
                           GEGL_ABYSS_CLAMP);
#else
          gegl_node_blit (node,
                          buffer_scale,
                          GEGL_RECTANGLE (scaled_x, scaled_y,
                                          scaled_width, scaled_height),
                          shell->profile_src_format,
                          shell->profile_data, shell->profile_stride,
                          GEGL_BLIT_CACHE);
#endif

          if (gimp_display_shell_has_filter (shell) || ! can_convert_to_u8)
            {
              /*  if there are filters, convert the pixels from the
               *  profile_buffer to the filter_buffer
               */
              gimp_display_shell_profile_convert_buffer (shell,
                                                         shell->profile_buffer,
                                                         GEGL_RECTANGLE (0, 0,
                                                                         scaled_width,
                                                                         scaled_height),
                                                         shell->filter_buffer,
                                                         GEGL_RECTANGLE (0, 0,
                                                                         scaled_width,
                                                                         scaled_height));
            }
          else
            {
              /*  otherwise, convert the profile_buffer directly into
               *  the cairo_buffer
               */
              gimp_display_shell_profile_convert_buffer (shell,
                                                         shell->profile_buffer,
                                                         GEGL_RECTANGLE (0, 0,
                                                                         scaled_width,
                                                                         scaled_height),
                                                         cairo_buffer,
                                                         GEGL_RECTANGLE (0, 0,
                                                                         scaled_width,
                                                                         scaled_height));
            }
        }
      else
        {
          /*  otherwise, load the projection pixels directly into the
           *  filter_buffer
           */
#ifndef USE_NODE_BLIT
          gegl_buffer_get (buffer,
                           GEGL_RECTANGLE (scaled_x, scaled_y,
                                           scaled_width, scaled_height),
                           buffer_scale,
                           shell->filter_format,
                           shell->filter_data, shell->filter_stride,
                           GEGL_ABYSS_CLAMP);
#else
          gegl_node_blit (node,
                          buffer_scale,
                          GEGL_RECTANGLE (scaled_x, scaled_y,
                                          scaled_width, scaled_height),
                          shell->filter_format,
                          shell->filter_data, shell->filter_stride,
                          GEGL_BLIT_CACHE);
#endif
        }

      if (gimp_display_shell_has_filter (shell))
        {
          /*  convert the filter_buffer in place
           */
          gimp_color_display_stack_convert_buffer (shell->filter_stack,
                                                   shell->filter_buffer,
                                                   GEGL_RECTANGLE (0, 0,
                                                                   scaled_width,
                                                                   scaled_height));
        }

      if (gimp_display_shell_has_filter (shell) || ! can_convert_to_u8)
        {
          /*  finally, copy the filter buffer to the cairo-ARGB32 buffer
           */
          gegl_buffer_get (shell->filter_buffer,
                           GEGL_RECTANGLE (0, 0,
                                           scaled_width,
                                           scaled_height),
                           1.0,
                           babl_format ("cairo-ARGB32"),
                           cairo_data, cairo_stride,
                           GEGL_ABYSS_CLAMP);
        }
    }
  else
    {
      /*  otherwise we can copy the projection pixels straight to the
       *  cairo-ARGB32 buffer
       */
#ifndef USE_NODE_BLIT
      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (scaled_x, scaled_y,
                                       scaled_width, scaled_height),
                       buffer_scale,
                       babl_format ("cairo-ARGB32"),
                       cairo_data, cairo_stride,
                       GEGL_ABYSS_CLAMP);
#else
      gegl_node_blit (node,
                      buffer_scale,
                      GEGL_RECTANGLE (scaled_x, scaled_y,
                                      scaled_width, scaled_height),
                      babl_format ("cairo-ARGB32"),
                      cairo_data, cairo_stride,
                      GEGL_BLIT_CACHE);
#endif
    }

  g_object_unref (cairo_buffer);

  if (shell->mask)
    {
      if (! shell->mask_surface)
        {
          shell->mask_surface =
            cairo_image_surface_create (CAIRO_FORMAT_A8,
                                        GIMP_DISPLAY_RENDER_BUF_WIDTH  *
                                        GIMP_DISPLAY_RENDER_MAX_SCALE,
                                        GIMP_DISPLAY_RENDER_BUF_HEIGHT *
                                        GIMP_DISPLAY_RENDER_MAX_SCALE);
        }

      cairo_surface_mark_dirty (shell->mask_surface);

      cairo_stride = cairo_image_surface_get_stride (shell->mask_surface);
      cairo_data   = cairo_image_surface_get_data (shell->mask_surface) +
                     mask_src_y * cairo_stride + mask_src_x * 4;

      gegl_buffer_get (shell->mask,
                       GEGL_RECTANGLE (scaled_x - shell->mask_offset_x,
                                       scaled_y - shell->mask_offset_y,
                                       scaled_width, scaled_height),
                       buffer_scale,
                       babl_format ("Y u8"),
                       cairo_data, cairo_stride,
                       GEGL_ABYSS_NONE);

      if (shell->mask_inverted)
        {
          gint mask_height = scaled_height;

          while (mask_height--)
            {
              gint    mask_width = scaled_width;
              guchar *d          = cairo_data;

              while (mask_width--)
                {
                  guchar inv = 255 - *d;

                  *d++ = inv;
                }

              cairo_data += cairo_stride;
            }
        }
    }

  /*  put it to the screen  */
  cairo_save (cr);

  cairo_rectangle (cr, x, y, w, h);

  cairo_scale (cr, 1.0 / scale_x, 1.0 / scale_y);

  cairo_set_source_surface (cr, xfer,
                            x * scale_x - xfer_src_x,
                            y * scale_y - xfer_src_y);

  if (shell->rotate_transform)
    {
      cairo_pattern_t *pattern;

      pattern = cairo_get_source (cr);
      cairo_pattern_set_extend (pattern, CAIRO_EXTEND_PAD);

      cairo_set_line_width (cr, 1.0);
      cairo_stroke_preserve (cr);

      cairo_surface_destroy (xfer);
    }

  cairo_clip (cr);
  cairo_paint (cr);

  if (shell->mask)
    {
      gimp_cairo_set_source_rgba (cr, &shell->mask_color);
      cairo_mask_surface (cr, shell->mask_surface,
                          (x - mask_src_x) * scale_x,
                          (y - mask_src_y) * scale_y);
    }

  cairo_restore (cr);
}
