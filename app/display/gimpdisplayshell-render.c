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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "config/gimpdisplayconfig.h"

#include "gegl/gimp-gegl-utils.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"
#include "core/gimpprojectable.h"
#include "core/gimpprojection.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-transform.h"
#include "gimpdisplayshell-filter.h"
#include "gimpdisplayshell-render.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayxfer.h"


void
gimp_display_shell_render (GimpDisplayShell *shell,
                           cairo_t          *cr,
                           gint              x,
                           gint              y,
                           gint              w,
                           gint              h)
{
  GimpImage       *image;
  GimpProjection  *projection;
  GeglBuffer      *buffer;
  gdouble          window_scale = 1.0;
  gint             viewport_offset_x;
  gint             viewport_offset_y;
  gint             viewport_width;
  gint             viewport_height;
  cairo_surface_t *xfer;
  gint             src_x;
  gint             src_y;
  gint             mask_src_x = 0;
  gint             mask_src_y = 0;
  gint             stride;
  guchar          *data;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (w > 0 && h > 0);

  image      = gimp_display_get_image (shell->display);
  projection = gimp_image_get_projection (image);
  buffer     = gimp_pickable_get_buffer (GIMP_PICKABLE (projection));

#ifdef GIMP_DISPLAY_RENDER_ENABLE_SCALING
  /* if we had this future API, things would look pretty on hires (retina) */
  window_scale = gdk_window_get_scale_factor (gtk_widget_get_window (gtk_widget_get_toplevel (GTK_WIDGET (shell))));
#endif

  window_scale = MIN (window_scale, GIMP_DISPLAY_RENDER_MAX_SCALE);

  gimp_display_shell_scroll_get_scaled_viewport (shell,
                                                 &viewport_offset_x,
                                                 &viewport_offset_y,
                                                 &viewport_width,
                                                 &viewport_height);
  if (shell->rotate_transform)
    {
      xfer = cairo_surface_create_similar_image (cairo_get_target (cr),
                                                 CAIRO_FORMAT_ARGB32,
                                                 w * window_scale,
                                                 h * window_scale);
      cairo_surface_mark_dirty (xfer);
      src_x = 0;
      src_y = 0;
    }
  else
    {
      xfer = gimp_display_xfer_get_surface (shell->xfer,
                                            w * window_scale,
                                            h * window_scale,
                                            &src_x, &src_y);
    }

  stride = cairo_image_surface_get_stride (xfer);
  data = cairo_image_surface_get_data (xfer);
  data += src_y * stride + src_x * 4;

  gegl_buffer_get (buffer,
                   GEGL_RECTANGLE ((x + viewport_offset_x) * window_scale,
                                   (y + viewport_offset_y) * window_scale,
                                   w * window_scale,
                                   h * window_scale),
                   shell->scale_x * window_scale,
                   babl_format ("cairo-ARGB32"),
		   data, stride,
                   GEGL_ABYSS_NONE);

  /*  apply filters to the rendered projection  */
  if (shell->filter_stack)
    {
      cairo_surface_t *image =
	cairo_image_surface_create_for_data (data, CAIRO_FORMAT_ARGB32,
					     w * window_scale,
					     h * window_scale,
					     stride);
      gimp_color_display_stack_convert_surface (shell->filter_stack, image);
      cairo_surface_destroy (image);
    }

  if (shell->mask)
    {
      gint mask_height;

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

      stride = cairo_image_surface_get_stride (shell->mask_surface);
      data = cairo_image_surface_get_data (shell->mask_surface);
      data += mask_src_y * stride + mask_src_x * 4;

      gegl_buffer_get (shell->mask,
                       GEGL_RECTANGLE ((x + viewport_offset_x) * window_scale,
                                       (y + viewport_offset_y) * window_scale,
                                       w * window_scale,
                                       h * window_scale),
                       shell->scale_x * window_scale,
                       babl_format ("Y u8"),
                       data, stride,
                       GEGL_ABYSS_NONE);

      /* invert the mask so what is *not* the foreground object is masked */
      mask_height = h * window_scale;
      while (mask_height--)
        {
          gint    mask_width = w * window_scale;
          guchar *d          = data;

          while (mask_width--)
            {
              guchar inv = 255 - *d;

              *d++ = inv;
            }

          data += stride;
        }
    }

  /*  put it to the screen  */
  cairo_save (cr);

  cairo_rectangle (cr, x, y, w, h);

  cairo_scale (cr, 1.0 / window_scale, 1.0 / window_scale);

  cairo_set_source_surface (cr, xfer,
                            (x - src_x) * window_scale,
                            (y - src_y) * window_scale);

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
                          (x - mask_src_x) * window_scale,
                          (y - mask_src_y) * window_scale);
    }

  cairo_restore (cr);
}
