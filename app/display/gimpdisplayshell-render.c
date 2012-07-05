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


void
gimp_display_shell_render (GimpDisplayShell *shell,
                           cairo_t          *cr,
                           gint              x,
                           gint              y,
                           gint              w,
                           gint              h)
{
  GimpImage      *image;
  GimpProjection *projection;
  GeglBuffer     *buffer;
  gint            viewport_offset_x;
  gint            viewport_offset_y;
  gint            viewport_width;
  gint            viewport_height;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (w > 0 && h > 0);

  image      = gimp_display_get_image (shell->display);
  projection = gimp_image_get_projection (image);
  buffer     = gimp_pickable_get_buffer (GIMP_PICKABLE (projection));

  gimp_display_shell_scroll_get_scaled_viewport (shell,
                                                 &viewport_offset_x,
                                                 &viewport_offset_y,
                                                 &viewport_width,
                                                 &viewport_height);

  gegl_buffer_get (buffer,
                   GEGL_RECTANGLE (x + viewport_offset_x,
                                   y + viewport_offset_y,
                                   w, h),
                   shell->scale_x,
                   babl_format ("cairo-ARGB32"),
                   cairo_image_surface_get_data (shell->render_surface),
                   cairo_image_surface_get_stride (shell->render_surface),
                   GEGL_ABYSS_NONE);

  /*  apply filters to the rendered projection  */
  if (shell->filter_stack)
    {
      cairo_surface_t *sub = shell->render_surface;

      if (w != GIMP_DISPLAY_RENDER_BUF_WIDTH ||
          h != GIMP_DISPLAY_RENDER_BUF_HEIGHT)
        sub = cairo_image_surface_create_for_data (cairo_image_surface_get_data (sub),
                                                   CAIRO_FORMAT_ARGB32, w, h,
                                                   GIMP_DISPLAY_RENDER_BUF_WIDTH * 4);

      gimp_color_display_stack_convert_surface (shell->filter_stack, sub);

      if (sub != shell->render_surface)
        cairo_surface_destroy (sub);
    }

  cairo_surface_mark_dirty_rectangle (shell->render_surface, 0, 0, w, h);

#if 0
  if (shell->mask)
    {
      GeglBuffer *buffer;

      if (! shell->mask_surface)
        {
          shell->mask_surface =
            cairo_image_surface_create (CAIRO_FORMAT_A8,
                                        GIMP_DISPLAY_RENDER_BUF_WIDTH,
                                        GIMP_DISPLAY_RENDER_BUF_HEIGHT);
        }

      buffer = gimp_drawable_get_buffer (shell->mask);
      tiles = gimp_gegl_buffer_get_tiles (buffer);

      /* The mask does not (yet) have an image pyramid, use 0 as level, */
      gimp_display_shell_render_info_init (&info,
                                           shell, x, y, w, h,
                                           shell->mask_surface,
                                           tiles, 0, FALSE);

      render_image_alpha (&info);

      cairo_surface_mark_dirty (shell->mask_surface);
    }
#endif

  /*  put it to the screen  */
  cairo_save (cr);

  cairo_rectangle (cr, x, y, w, h);
  cairo_clip (cr);

  cairo_set_source_surface (cr, shell->render_surface, x, y);
  cairo_paint (cr);

#if 0
  if (shell->mask)
    {
      gimp_cairo_set_source_rgba (cr, &shell->mask_color);
      cairo_mask_surface (cr, shell->mask_surface,
                          x + disp_xoffset, y + disp_yoffset);
    }
#endif

  cairo_restore (cr);
}
