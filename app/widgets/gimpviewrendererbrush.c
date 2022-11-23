/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaviewrendererbrush.c
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

#include "widgets-types.h"

#include "core/ligmabrushpipe.h"
#include "core/ligmabrushgenerated.h"
#include "core/ligmatempbuf.h"

#include "ligmaviewrendererbrush.h"


static void   ligma_view_renderer_brush_finalize (GObject          *object);
static void   ligma_view_renderer_brush_render   (LigmaViewRenderer *renderer,
                                                 GtkWidget        *widget);
static void   ligma_view_renderer_brush_draw     (LigmaViewRenderer *renderer,
                                                 GtkWidget        *widget,
                                                 cairo_t          *cr,
                                                 gint              available_width,
                                                 gint              available_height);

static gboolean ligma_view_renderer_brush_render_timeout (gpointer    data);


G_DEFINE_TYPE (LigmaViewRendererBrush, ligma_view_renderer_brush,
               LIGMA_TYPE_VIEW_RENDERER)

#define parent_class ligma_view_renderer_brush_parent_class


static void
ligma_view_renderer_brush_class_init (LigmaViewRendererBrushClass *klass)
{
  GObjectClass          *object_class   = G_OBJECT_CLASS (klass);
  LigmaViewRendererClass *renderer_class = LIGMA_VIEW_RENDERER_CLASS (klass);

  object_class->finalize = ligma_view_renderer_brush_finalize;

  renderer_class->render = ligma_view_renderer_brush_render;
  renderer_class->draw   = ligma_view_renderer_brush_draw;
}

static void
ligma_view_renderer_brush_init (LigmaViewRendererBrush *renderer)
{
  renderer->pipe_timeout_id      = 0;
  renderer->pipe_animation_index = 0;
}

static void
ligma_view_renderer_brush_finalize (GObject *object)
{
  LigmaViewRendererBrush *renderer = LIGMA_VIEW_RENDERER_BRUSH (object);

  if (renderer->pipe_timeout_id)
    {
      g_source_remove (renderer->pipe_timeout_id);
      renderer->pipe_timeout_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_view_renderer_brush_render (LigmaViewRenderer *renderer,
                                 GtkWidget        *widget)
{
  LigmaViewRendererBrush *renderbrush = LIGMA_VIEW_RENDERER_BRUSH (renderer);
  LigmaTempBuf           *temp_buf;
  gint                   temp_buf_x = 0;
  gint                   temp_buf_y = 0;
  gint                   temp_buf_width;
  gint                   temp_buf_height;

  if (renderbrush->pipe_timeout_id)
    {
      g_source_remove (renderbrush->pipe_timeout_id);
      renderbrush->pipe_timeout_id = 0;
    }

  temp_buf = ligma_viewable_get_new_preview (renderer->viewable,
                                            renderer->context,
                                            renderer->width,
                                            renderer->height);

  temp_buf_width  = ligma_temp_buf_get_width  (temp_buf);
  temp_buf_height = ligma_temp_buf_get_height (temp_buf);

  if (temp_buf_width < renderer->width)
    temp_buf_x = (renderer->width - temp_buf_width) / 2;

  if (temp_buf_height < renderer->height)
    temp_buf_y = (renderer->height - temp_buf_height) / 2;

  if (renderer->is_popup)
    {
      ligma_view_renderer_render_temp_buf (renderer, widget, temp_buf,
                                          temp_buf_x, temp_buf_y,
                                          -1,
                                          LIGMA_VIEW_BG_WHITE,
                                          LIGMA_VIEW_BG_WHITE);

      ligma_temp_buf_unref (temp_buf);

      if (LIGMA_IS_BRUSH_PIPE (renderer->viewable))
        {
          renderbrush->widget = widget;
          renderbrush->pipe_animation_index = 0;
          renderbrush->pipe_timeout_id =
            g_timeout_add (300, ligma_view_renderer_brush_render_timeout,
                           renderbrush);
        }

      return;
    }

  ligma_view_renderer_render_temp_buf (renderer, widget, temp_buf,
                                      temp_buf_x, temp_buf_y,
                                      -1,
                                      LIGMA_VIEW_BG_WHITE,
                                      LIGMA_VIEW_BG_WHITE);

  ligma_temp_buf_unref (temp_buf);
}

static gboolean
ligma_view_renderer_brush_render_timeout (gpointer data)
{
  LigmaViewRendererBrush *renderbrush = LIGMA_VIEW_RENDERER_BRUSH (data);
  LigmaViewRenderer      *renderer    = LIGMA_VIEW_RENDERER (data);
  LigmaBrushPipe         *brush_pipe;
  LigmaBrush             *brush;
  LigmaTempBuf           *temp_buf;
  gint                   temp_buf_x = 0;
  gint                   temp_buf_y = 0;
  gint                   temp_buf_width;
  gint                   temp_buf_height;

  if (! renderer->viewable)
    {
      renderbrush->pipe_timeout_id      = 0;
      renderbrush->pipe_animation_index = 0;

      return FALSE;
    }

  brush_pipe = LIGMA_BRUSH_PIPE (renderer->viewable);

  renderbrush->pipe_animation_index++;

  if (renderbrush->pipe_animation_index >= brush_pipe->n_brushes)
    renderbrush->pipe_animation_index = 0;

  brush =
    LIGMA_BRUSH (brush_pipe->brushes[renderbrush->pipe_animation_index]);

  temp_buf = ligma_viewable_get_new_preview (LIGMA_VIEWABLE (brush),
                                            renderer->context,
                                            renderer->width,
                                            renderer->height);

  temp_buf_width  = ligma_temp_buf_get_width  (temp_buf);
  temp_buf_height = ligma_temp_buf_get_height (temp_buf);

  if (temp_buf_width < renderer->width)
    temp_buf_x = (renderer->width - temp_buf_width) / 2;

  if (temp_buf_height < renderer->height)
    temp_buf_y = (renderer->height - temp_buf_height) / 2;

  ligma_view_renderer_render_temp_buf (renderer, renderbrush->widget, temp_buf,
                                      temp_buf_x, temp_buf_y,
                                      -1,
                                      LIGMA_VIEW_BG_WHITE,
                                      LIGMA_VIEW_BG_WHITE);

  ligma_temp_buf_unref (temp_buf);

  ligma_view_renderer_update (renderer);

  return TRUE;
}

static void
ligma_view_renderer_brush_draw (LigmaViewRenderer *renderer,
                               GtkWidget        *widget,
                               cairo_t          *cr,
                               gint              available_width,
                               gint              available_height)
{
  LIGMA_VIEW_RENDERER_CLASS (parent_class)->draw (renderer, widget, cr,
                                                 available_width,
                                                 available_height);

#define INDICATOR_WIDTH  7
#define INDICATOR_HEIGHT 7

  if (renderer->width  > 2 * INDICATOR_WIDTH &&
      renderer->height > 2 * INDICATOR_HEIGHT)
    {
      gboolean  pipe      = LIGMA_IS_BRUSH_PIPE (renderer->viewable);
      gboolean  generated = LIGMA_IS_BRUSH_GENERATED (renderer->viewable);
      gint      brush_width;
      gint      brush_height;

      if (generated || pipe)
        {
          cairo_move_to (cr, available_width, available_height);
          cairo_rel_line_to (cr, - INDICATOR_WIDTH, 0);
          cairo_rel_line_to (cr, INDICATOR_WIDTH, - INDICATOR_HEIGHT);
          cairo_rel_line_to (cr, 0, INDICATOR_HEIGHT);

          if (pipe)
            cairo_set_source_rgb (cr, 1.0, 0.5, 0.5);
          else
            cairo_set_source_rgb (cr, 0.5, 0.6, 1.0);

          cairo_fill (cr);
        }

      ligma_viewable_get_size (renderer->viewable, &brush_width, &brush_height);

      if (renderer->width < brush_width || renderer->height < brush_height)
        {
          cairo_move_to (cr,
                         available_width  - INDICATOR_WIDTH + 1,
                         available_height - INDICATOR_HEIGHT / 2.0);
          cairo_rel_line_to (cr, INDICATOR_WIDTH - 2, 0);

          cairo_move_to (cr,
                         available_width  - INDICATOR_WIDTH / 2.0,
                         available_height - INDICATOR_HEIGHT + 1);
          cairo_rel_line_to (cr, 0, INDICATOR_WIDTH - 2);

          cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
          cairo_set_line_width (cr, 1);
          cairo_stroke (cr);
        }
    }

#undef INDICATOR_WIDTH
#undef INDICATOR_HEIGHT
}
