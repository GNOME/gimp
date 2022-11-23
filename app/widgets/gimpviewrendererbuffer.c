/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaviewrendererbuffer.c
 * Copyright (C) 2004-2006 Michael Natterer <mitch@ligma.org>
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

#include "core/ligmatempbuf.h"
#include "core/ligmaviewable.h"

#include "ligmaviewrendererbuffer.h"


static void   ligma_view_renderer_buffer_render (LigmaViewRenderer *renderer,
                                                GtkWidget        *widget);


G_DEFINE_TYPE (LigmaViewRendererBuffer, ligma_view_renderer_buffer,
               LIGMA_TYPE_VIEW_RENDERER)

#define parent_class ligma_view_renderer_buffer_class_init


static void
ligma_view_renderer_buffer_class_init (LigmaViewRendererBufferClass *klass)
{
  LigmaViewRendererClass *renderer_class = LIGMA_VIEW_RENDERER_CLASS (klass);

  renderer_class->render = ligma_view_renderer_buffer_render;
}

static void
ligma_view_renderer_buffer_init (LigmaViewRendererBuffer *renderer)
{
}

static void
ligma_view_renderer_buffer_render (LigmaViewRenderer *renderer,
                                  GtkWidget        *widget)
{
  gint         buffer_width;
  gint         buffer_height;
  gint         view_width;
  gint         view_height;
  gboolean     scaling_up;
  LigmaTempBuf *render_buf = NULL;

  ligma_viewable_get_size (renderer->viewable, &buffer_width, &buffer_height);

  ligma_viewable_calc_preview_size (buffer_width,
                                   buffer_height,
                                   renderer->width,
                                   renderer->height,
                                   TRUE, 1.0, 1.0,
                                   &view_width,
                                   &view_height,
                                   &scaling_up);

  if (scaling_up)
    {
      LigmaTempBuf *temp_buf;

      temp_buf = ligma_viewable_get_new_preview (renderer->viewable,
                                                renderer->context,
                                                buffer_width, buffer_height);

      if (temp_buf)
        {
          render_buf = ligma_temp_buf_scale (temp_buf, view_width, view_height);

          ligma_temp_buf_unref (temp_buf);
        }
    }
  else
    {
      render_buf = ligma_viewable_get_new_preview (renderer->viewable,
                                                  renderer->context,
                                                  view_width, view_height);
    }

  if (render_buf)
    {
      ligma_view_renderer_render_temp_buf_simple (renderer, widget, render_buf);

      ligma_temp_buf_unref (render_buf);
    }
  else /* no preview available */
    {
      const gchar *icon_name;

      icon_name = ligma_viewable_get_icon_name (renderer->viewable);

      ligma_view_renderer_render_icon (renderer, widget, icon_name);
    }
}
