/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrendererimage.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimpimage.h"

#include "gimpviewrendererimage.h"


static void   gimp_view_renderer_image_render (GimpViewRenderer *renderer,
                                               GtkWidget        *widget);


G_DEFINE_TYPE (GimpViewRendererImage, gimp_view_renderer_image,
               GIMP_TYPE_VIEW_RENDERER)

#define parent_class gimp_view_renderer_image_parent_class


static void
gimp_view_renderer_image_class_init (GimpViewRendererImageClass *klass)
{
  GimpViewRendererClass *renderer_class = GIMP_VIEW_RENDERER_CLASS (klass);

  renderer_class->render = gimp_view_renderer_image_render;
}

static void
gimp_view_renderer_image_init (GimpViewRendererImage *renderer)
{
  renderer->channel = -1;
}

static void
gimp_view_renderer_image_render (GimpViewRenderer *renderer,
                                 GtkWidget        *widget)
{
  GimpViewRendererImage *rendererimage = GIMP_VIEW_RENDERER_IMAGE (renderer);
  GimpImage             *image         = GIMP_IMAGE (renderer->viewable);
  const gchar           *stock_id;

  /* The conditions checked here are mostly a hack to hide the fact that
   * we are creating the channel preview from the image preview and turning
   * off visibility of a channel has the side-effect of painting the channel
   * preview all black. See bug #459518 for details.
   */
  if (rendererimage->channel == -1 ||
      (gimp_image_get_component_visible (image, rendererimage->channel) &&
       gimp_image_get_component_visible (image, GIMP_ALPHA_CHANNEL)))
    {
      gint      view_width;
      gint      view_height;
      gboolean  scaling_up;
      TempBuf  *render_buf = NULL;

      gimp_viewable_calc_preview_size (image->width,
                                       image->height,
                                       renderer->width,
                                       renderer->height,
                                       renderer->dot_for_dot,
                                       image->xresolution,
                                       image->yresolution,
                                       &view_width,
                                       &view_height,
                                       &scaling_up);

      if (scaling_up)
        {
          TempBuf *temp_buf;

          temp_buf = gimp_viewable_get_new_preview (renderer->viewable,
                                                    renderer->context,
                                                    image->width,
                                                    image->height);

          if (temp_buf)
            {
              render_buf = temp_buf_scale (temp_buf, view_width, view_height);

              temp_buf_free (temp_buf);
            }
        }
      else
        {
          render_buf = gimp_viewable_get_new_preview (renderer->viewable,
                                                      renderer->context,
                                                      view_width,
                                                      view_height);
        }

      if (render_buf)
        {
          gint component_index = -1;

          /*  xresolution != yresolution */
          if (view_width > renderer->width || view_height > renderer->height)
            {
              TempBuf *temp_buf;

              temp_buf = temp_buf_scale (render_buf,
                                         renderer->width, renderer->height);

              temp_buf_free (render_buf);
              render_buf = temp_buf;
            }

          if (view_width  < renderer->width)
            render_buf->x = (renderer->width  - view_width)  / 2;

          if (view_height < renderer->height)
            render_buf->y = (renderer->height - view_height) / 2;

          if (rendererimage->channel != -1)
            component_index =
              gimp_image_get_component_index (image, rendererimage->channel);

          gimp_view_renderer_render_buffer (renderer, render_buf,
                                            component_index,
                                            GIMP_VIEW_BG_CHECKS,
                                            GIMP_VIEW_BG_WHITE);

          temp_buf_free (render_buf);

          return;
        }
    }

  switch (rendererimage->channel)
    {
    case GIMP_RED_CHANNEL:     stock_id = GIMP_STOCK_CHANNEL_RED;     break;
    case GIMP_GREEN_CHANNEL:   stock_id = GIMP_STOCK_CHANNEL_GREEN;   break;
    case GIMP_BLUE_CHANNEL:    stock_id = GIMP_STOCK_CHANNEL_BLUE;    break;
    case GIMP_GRAY_CHANNEL:    stock_id = GIMP_STOCK_CHANNEL_GRAY;    break;
    case GIMP_INDEXED_CHANNEL: stock_id = GIMP_STOCK_CHANNEL_INDEXED; break;
    case GIMP_ALPHA_CHANNEL:   stock_id = GIMP_STOCK_CHANNEL_ALPHA;   break;

    default:
      stock_id = gimp_viewable_get_stock_id (renderer->viewable);
      break;
    }

  gimp_view_renderer_default_render_stock (renderer, widget, stock_id);
}
