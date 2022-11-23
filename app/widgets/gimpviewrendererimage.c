/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaviewrendererimage.c
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
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

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmaimage.h"
#include "core/ligmaimageproxy.h"
#include "core/ligmatempbuf.h"

#include "ligmaviewrendererimage.h"


static void   ligma_view_renderer_image_render (LigmaViewRenderer *renderer,
                                               GtkWidget        *widget);


G_DEFINE_TYPE (LigmaViewRendererImage, ligma_view_renderer_image,
               LIGMA_TYPE_VIEW_RENDERER)

#define parent_class ligma_view_renderer_image_parent_class


static void
ligma_view_renderer_image_class_init (LigmaViewRendererImageClass *klass)
{
  LigmaViewRendererClass *renderer_class = LIGMA_VIEW_RENDERER_CLASS (klass);

  renderer_class->render = ligma_view_renderer_image_render;
}

static void
ligma_view_renderer_image_init (LigmaViewRendererImage *renderer)
{
  renderer->channel = -1;
}

static void
ligma_view_renderer_image_render (LigmaViewRenderer *renderer,
                                 GtkWidget        *widget)
{
  LigmaViewRendererImage *rendererimage = LIGMA_VIEW_RENDERER_IMAGE (renderer);
  LigmaImage             *image;
  const gchar           *icon_name;
  gint                   width;
  gint                   height;

  if (LIGMA_IS_IMAGE (renderer->viewable))
    {
      image = LIGMA_IMAGE (renderer->viewable);
    }
  else if (LIGMA_IS_IMAGE_PROXY (renderer->viewable))
    {
      image = ligma_image_proxy_get_image (
        LIGMA_IMAGE_PROXY (renderer->viewable));
    }
  else
    {
      g_return_if_reached ();
    }

  ligma_viewable_get_size (renderer->viewable, &width, &height);

  /* The conditions checked here are mostly a hack to hide the fact that
   * we are creating the channel preview from the image preview and turning
   * off visibility of a channel has the side-effect of painting the channel
   * preview all black. See bug #459518 for details.
   */
  if (rendererimage->channel == -1 ||
      (ligma_image_get_component_visible (image, rendererimage->channel)))
    {
      gint         view_width;
      gint         view_height;
      gdouble      xres;
      gdouble      yres;
      gboolean     scaling_up;
      LigmaTempBuf *render_buf = NULL;

      ligma_image_get_resolution (image, &xres, &yres);

      ligma_viewable_calc_preview_size (width,
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
          LigmaTempBuf *temp_buf;

          temp_buf = ligma_viewable_get_new_preview (renderer->viewable,
                                                    renderer->context,
                                                    width, height);

          if (temp_buf)
            {
              render_buf = ligma_temp_buf_scale (temp_buf,
                                                view_width, view_height);
              ligma_temp_buf_unref (temp_buf);
            }
        }
      else
        {
          render_buf = ligma_viewable_get_new_preview (renderer->viewable,
                                                      renderer->context,
                                                      view_width,
                                                      view_height);
        }

      if (render_buf)
        {
          gint render_buf_x    = 0;
          gint render_buf_y    = 0;
          gint component_index = -1;

          /*  xresolution != yresolution */
          if (view_width > renderer->width || view_height > renderer->height)
            {
              LigmaTempBuf *temp_buf;

              temp_buf = ligma_temp_buf_scale (render_buf,
                                              renderer->width, renderer->height);
              ligma_temp_buf_unref (render_buf);
              render_buf = temp_buf;
            }

          if (view_width  < renderer->width)
            render_buf_x = (renderer->width  - view_width)  / 2;

          if (view_height < renderer->height)
            render_buf_y = (renderer->height - view_height) / 2;

          if (rendererimage->channel != -1)
            component_index =
              ligma_image_get_component_index (image, rendererimage->channel);

          ligma_view_renderer_render_temp_buf (renderer, widget, render_buf,
                                              render_buf_x, render_buf_y,
                                              component_index,
                                              LIGMA_VIEW_BG_CHECKS,
                                              LIGMA_VIEW_BG_WHITE);
          ligma_temp_buf_unref (render_buf);

          return;
        }
    }

  switch (rendererimage->channel)
    {
    case LIGMA_CHANNEL_RED:     icon_name = LIGMA_ICON_CHANNEL_RED;     break;
    case LIGMA_CHANNEL_GREEN:   icon_name = LIGMA_ICON_CHANNEL_GREEN;   break;
    case LIGMA_CHANNEL_BLUE:    icon_name = LIGMA_ICON_CHANNEL_BLUE;    break;
    case LIGMA_CHANNEL_GRAY:    icon_name = LIGMA_ICON_CHANNEL_GRAY;    break;
    case LIGMA_CHANNEL_INDEXED: icon_name = LIGMA_ICON_CHANNEL_INDEXED; break;
    case LIGMA_CHANNEL_ALPHA:   icon_name = LIGMA_ICON_CHANNEL_ALPHA;   break;

    default:
      icon_name = ligma_viewable_get_icon_name (renderer->viewable);
      break;
    }

  ligma_view_renderer_render_icon (renderer, widget, icon_name);
}
