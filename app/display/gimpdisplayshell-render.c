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
#include "core/gimpprojectable.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-transform.h"
#include "gimpdisplayshell-filter.h"
#include "gimpdisplayshell-profile.h"
#include "gimpdisplayshell-render.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayxfer.h"


void
gimp_display_shell_render (GimpDisplayShell *shell,
                           cairo_t          *cr,
                           gint              x,
                           gint              y,
                           gint              w,
                           gint              h,
                           gdouble           scale)
{
  GimpImage       *image;
  GeglBuffer      *buffer;
#ifdef USE_NODE_BLIT
  GeglNode        *node;
#endif
  GeglAbyssPolicy  abyss_policy;
  cairo_surface_t *xfer;
  gint             xfer_src_x;
  gint             xfer_src_y;
  gint             mask_src_x = 0;
  gint             mask_src_y = 0;
  gint             cairo_stride;
  guchar          *cairo_data;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (w > 0 && w <= GIMP_DISPLAY_RENDER_BUF_WIDTH);
  g_return_if_fail (h > 0 && h <= GIMP_DISPLAY_RENDER_BUF_HEIGHT);
  g_return_if_fail (scale > 0.0);

  image  = gimp_display_get_image (shell->display);
  buffer = gimp_pickable_get_buffer (
    gimp_display_shell_get_pickable (shell));
#ifdef USE_NODE_BLIT
  node   = gimp_projectable_get_graph (GIMP_PROJECTABLE (image));

  gimp_projectable_begin_render (GIMP_PROJECTABLE (image));
#endif

  if (shell->show_all)
    abyss_policy = GEGL_ABYSS_NONE;
  else
    abyss_policy = GEGL_ABYSS_CLAMP;

  xfer = gimp_display_xfer_get_surface (shell->xfer, w, h,
                                        &xfer_src_x, &xfer_src_y);

  cairo_stride = cairo_image_surface_get_stride (xfer);
  cairo_data   = cairo_image_surface_get_data (xfer) +
                 xfer_src_y * cairo_stride + xfer_src_x * 4;

  if (shell->profile_transform ||
      gimp_display_shell_has_filter (shell))
    {
      gboolean can_convert_to_u8;

      /*  if there is a profile transform or a display filter, we need
       *  to use temp buffers
       */

      can_convert_to_u8 = gimp_display_shell_profile_can_convert_to_u8 (shell);

      /*  create the filter buffer if we have filters, or can't convert
       *  to u8 directly
       */
      if ((gimp_display_shell_has_filter (shell) || ! can_convert_to_u8) &&
          ! shell->filter_buffer)
        {
          gint fw = GIMP_DISPLAY_RENDER_BUF_WIDTH;
          gint fh = GIMP_DISPLAY_RENDER_BUF_HEIGHT;

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

      if (! gimp_display_shell_has_filter (shell) || shell->filter_transform)
        {
          /*  if there are no filters, or there is a filter transform,
           *  load the projection pixels into the profile_buffer
           */
#ifndef USE_NODE_BLIT
          gegl_buffer_get (buffer,
                           GEGL_RECTANGLE (x, y, w, h), scale,
                           gimp_projectable_get_format (GIMP_PROJECTABLE (image)),
                           shell->profile_data, shell->profile_stride,
                           abyss_policy);
#else
          gegl_node_blit (node,
                          scale, GEGL_RECTANGLE (x, y, w, h),
                          gimp_projectable_get_format (GIMP_PROJECTABLE (image)),
                          shell->profile_data, shell->profile_stride,
                          GEGL_BLIT_CACHE);
#endif
        }
      else
        {
          /*  otherwise, load the pixels directly into the filter_buffer
           */
#ifndef USE_NODE_BLIT
          gegl_buffer_get (buffer,
                           GEGL_RECTANGLE (x, y, w, h), scale,
                           shell->filter_format,
                           shell->filter_data, shell->filter_stride,
                           abyss_policy);
#else
          gegl_node_blit (node,
                          scale, GEGL_RECTANGLE (x, y, w, h),
                          shell->filter_format,
                          shell->filter_data, shell->filter_stride,
                          GEGL_BLIT_CACHE);
#endif
        }

      /*  if there is a filter transform, convert the pixels from
       *  the profile_buffer to the filter_buffer
       */
      if (shell->filter_transform)
        {
          gimp_color_transform_process_buffer (shell->filter_transform,
                                               shell->profile_buffer,
                                               GEGL_RECTANGLE (0, 0, w, h),
                                               shell->filter_buffer,
                                               GEGL_RECTANGLE (0, 0, w, h));
        }

      /*  if there are filters, apply them
       */
      if (gimp_display_shell_has_filter (shell))
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
          gimp_color_display_stack_convert_buffer (shell->filter_stack,
                                                   filter_buffer,
                                                   GEGL_RECTANGLE (x, y, w, h));

          g_object_unref (filter_buffer);
        }

      /*  if there is a profile transform...
       */
      if (shell->profile_transform)
        {
          if (gimp_display_shell_has_filter (shell))
            {
              /*  if we have filters, convert the pixels in the filter_buffer
               *  in-place
               */
              gimp_color_transform_process_buffer (shell->profile_transform,
                                                   shell->filter_buffer,
                                                   GEGL_RECTANGLE (0, 0, w, h),
                                                   shell->filter_buffer,
                                                   GEGL_RECTANGLE (0, 0, w, h));
            }
          else if (! can_convert_to_u8)
            {
              /*  otherwise, if we can't convert to u8 directly, convert
               *  the pixels from the profile_buffer to the filter_buffer
               */
              gimp_color_transform_process_buffer (shell->profile_transform,
                                                   shell->profile_buffer,
                                                   GEGL_RECTANGLE (0, 0, w, h),
                                                   shell->filter_buffer,
                                                   GEGL_RECTANGLE (0, 0, w, h));
            }
          else
            {
              GeglBuffer *buffer =
                gegl_buffer_linear_new_from_data (cairo_data,
                                                  babl_format ("cairo-ARGB32"),
                                                  GEGL_RECTANGLE (0, 0, w, h),
                                                  cairo_stride,
                                                  NULL, NULL);

              /*  otherwise, convert the profile_buffer directly into
               *  the cairo_buffer
               */
              gimp_color_transform_process_buffer (shell->profile_transform,
                                                   shell->profile_buffer,
                                                   GEGL_RECTANGLE (0, 0, w, h),
                                                   buffer,
                                                   GEGL_RECTANGLE (0, 0, w, h));
              g_object_unref (buffer);
            }
        }

      /*  finally, copy the filter buffer to the cairo-ARGB32 buffer,
       *  if necessary
       */
      if (gimp_display_shell_has_filter (shell) || ! can_convert_to_u8)
        {
          gegl_buffer_get (shell->filter_buffer,
                           GEGL_RECTANGLE (0, 0, w, h), 1.0,
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
                       GEGL_RECTANGLE (x, y, w, h), scale,
                       babl_format ("cairo-ARGB32"),
                       cairo_data, cairo_stride,
                       abyss_policy);
#else
      gegl_node_blit (node,
                      scale, GEGL_RECTANGLE (x, y, w, h),
                      babl_format ("cairo-ARGB32"),
                      cairo_data, cairo_stride,
                      GEGL_BLIT_CACHE);
#endif
    }

#ifdef USE_NODE_BLIT
  gimp_projectable_end_render (GIMP_PROJECTABLE (image));
#endif

  if (shell->mask)
    {
      if (! shell->mask_surface)
        {
          shell->mask_surface =
            cairo_image_surface_create (CAIRO_FORMAT_A8,
                                        GIMP_DISPLAY_RENDER_BUF_WIDTH,
                                        GIMP_DISPLAY_RENDER_BUF_HEIGHT);
        }

      cairo_surface_mark_dirty (shell->mask_surface);

      cairo_stride = cairo_image_surface_get_stride (shell->mask_surface);
      cairo_data   = cairo_image_surface_get_data (shell->mask_surface) +
                     mask_src_y * cairo_stride + mask_src_x;

      gegl_buffer_get (shell->mask,
                       GEGL_RECTANGLE (x - floor (shell->mask_offset_x * scale),
                                       y - floor (shell->mask_offset_y * scale),
                                       w, h),
                       scale,
                       babl_format ("Y u8"),
                       cairo_data, cairo_stride,
                       GEGL_ABYSS_NONE);

      if (shell->mask_inverted)
        {
          gint mask_height = h;

          while (mask_height--)
            {
              gint    mask_width = w;
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
  cairo_set_source_surface (cr, xfer,
                            x - xfer_src_x,
                            y - xfer_src_y);
  cairo_paint (cr);

  if (shell->mask)
    {
      gimp_cairo_set_source_rgba (cr, &shell->mask_color);
      cairo_mask_surface (cr, shell->mask_surface,
                          x - mask_src_x,
                          y - mask_src_y);
    }
}
