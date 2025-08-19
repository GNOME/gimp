/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrendererimage.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpimage.h"
#include "core/gimpimageproxy.h"
#include "core/gimptempbuf.h"

#include "gimpviewrendererimage.h"


static void   gimp_view_renderer_image_render (GimpViewRenderer *renderer,
                                               GtkWidget        *widget);


G_DEFINE_TYPE (GimpViewRendererImage,
               gimp_view_renderer_image,
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
  GimpImage             *image;
  const gchar           *icon_name;
  gint                   width;
  gint                   height;

  if (GIMP_IS_IMAGE (renderer->viewable))
    {
      image = GIMP_IMAGE (renderer->viewable);
    }
  else if (GIMP_IS_IMAGE_PROXY (renderer->viewable))
    {
      image = gimp_image_proxy_get_image (
        GIMP_IMAGE_PROXY (renderer->viewable));
    }
  else
    {
      g_return_if_reached ();
    }

  gimp_viewable_get_size (renderer->viewable, &width, &height);

  /* The conditions checked here are mostly a hack to hide the fact that
   * we are creating the channel preview from the image preview and turning
   * off visibility of a channel has the side-effect of painting the channel
   * preview all black. See bug #459518 for details.
   */
  if (rendererimage->channel == -1 ||
      (gimp_image_get_component_visible (image, rendererimage->channel)))
    {
      gint         view_width;
      gint         view_height;
      gdouble      xres;
      gdouble      yres;
      gboolean     scaling_up;
      GimpTempBuf *render_buf = NULL;

      gimp_image_get_resolution (image, &xres, &yres);

      gimp_viewable_calc_preview_size (width,
                                       height,
                                       renderer->width,
                                       renderer->height,
                                       renderer->dot_for_dot,
                                       xres,
                                       yres,
                                       &view_width,
                                       &view_height,
                                       &scaling_up);

      if (scaling_up)
        {
          GimpTempBuf *temp_buf;

          temp_buf = gimp_viewable_get_new_preview (renderer->viewable,
                                                    renderer->context,
                                                    width, height, NULL);

          if (temp_buf)
            {
              render_buf = gimp_temp_buf_scale (temp_buf,
                                                view_width, view_height);
              gimp_temp_buf_unref (temp_buf);
            }
        }
      else
        {
          render_buf = gimp_viewable_get_new_preview (renderer->viewable,
                                                      renderer->context,
                                                      view_width,
                                                      view_height, NULL);
        }

      if (render_buf)
        {
          gint render_buf_x    = 0;
          gint render_buf_y    = 0;
          gint component_index = -1;

          /*  xresolution != yresolution */
          if (view_width > renderer->width || view_height > renderer->height)
            {
              GimpTempBuf *temp_buf;

              temp_buf = gimp_temp_buf_scale (render_buf,
                                              renderer->width, renderer->height);
              gimp_temp_buf_unref (render_buf);
              render_buf = temp_buf;
            }

          if (view_width  < renderer->width)
            render_buf_x = (renderer->width  - view_width)  / 2;

          if (view_height < renderer->height)
            render_buf_y = (renderer->height - view_height) / 2;

          if (rendererimage->channel != -1)
            component_index =
              gimp_image_get_component_index (image, rendererimage->channel);

          gimp_view_renderer_render_temp_buf (renderer, widget, render_buf,
                                              render_buf_x, render_buf_y,
                                              component_index,
                                              GIMP_VIEW_BG_CHECKS,
                                              GIMP_VIEW_BG_WHITE);
          gimp_temp_buf_unref (render_buf);

          return;
        }
    }

  switch (rendererimage->channel)
    {
    case GIMP_CHANNEL_RED:     icon_name = GIMP_ICON_CHANNEL_RED;     break;
    case GIMP_CHANNEL_GREEN:   icon_name = GIMP_ICON_CHANNEL_GREEN;   break;
    case GIMP_CHANNEL_BLUE:    icon_name = GIMP_ICON_CHANNEL_BLUE;    break;
    case GIMP_CHANNEL_GRAY:    icon_name = GIMP_ICON_CHANNEL_GRAY;    break;
    case GIMP_CHANNEL_INDEXED: icon_name = GIMP_ICON_CHANNEL_INDEXED; break;
    case GIMP_CHANNEL_ALPHA:   icon_name = GIMP_ICON_CHANNEL_ALPHA;   break;

    default:
      icon_name = gimp_viewable_get_icon_name (renderer->viewable);
      break;
    }

  gimp_view_renderer_render_icon (renderer, widget,
                                  icon_name,
                                  gtk_widget_get_scale_factor (widget));
}
