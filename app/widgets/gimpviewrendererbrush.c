/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrendererbrush.c
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

#include "widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimpbrushpipe.h"
#include "core/gimpbrushgenerated.h"

#include "gimpviewrendererbrush.h"


static void   gimp_view_renderer_brush_finalize   (GObject          *object);
static void   gimp_view_renderer_brush_render     (GimpViewRenderer *renderer,
                                                   GtkWidget        *widget);

static gboolean gimp_view_renderer_brush_render_timeout (gpointer  data);


G_DEFINE_TYPE (GimpViewRendererBrush, gimp_view_renderer_brush,
               GIMP_TYPE_VIEW_RENDERER)

#define parent_class gimp_view_renderer_brush_parent_class


static void
gimp_view_renderer_brush_class_init (GimpViewRendererBrushClass *klass)
{
  GObjectClass          *object_class   = G_OBJECT_CLASS (klass);
  GimpViewRendererClass *renderer_class = GIMP_VIEW_RENDERER_CLASS (klass);

  object_class->finalize = gimp_view_renderer_brush_finalize;

  renderer_class->render = gimp_view_renderer_brush_render;
}

static void
gimp_view_renderer_brush_init (GimpViewRendererBrush *renderer)
{
  renderer->pipe_timeout_id      = 0;
  renderer->pipe_animation_index = 0;
}

static void
gimp_view_renderer_brush_finalize (GObject *object)
{
  GimpViewRendererBrush *renderer = GIMP_VIEW_RENDERER_BRUSH (object);

  if (renderer->pipe_timeout_id)
    {
      g_source_remove (renderer->pipe_timeout_id);
      renderer->pipe_timeout_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_view_renderer_brush_render (GimpViewRenderer *renderer,
                                 GtkWidget        *widget)
{
  GimpViewRendererBrush *renderbrush = GIMP_VIEW_RENDERER_BRUSH (renderer);
  TempBuf               *temp_buf;
  gint                   brush_width;
  gint                   brush_height;

  if (renderbrush->pipe_timeout_id)
    {
      g_source_remove (renderbrush->pipe_timeout_id);
      renderbrush->pipe_timeout_id = 0;
    }

  gimp_viewable_get_size (renderer->viewable, &brush_width, &brush_height);

  temp_buf = gimp_viewable_get_new_preview (renderer->viewable,
                                            renderer->context,
                                            renderer->width,
                                            renderer->height);

  if (temp_buf->width < renderer->width)
    temp_buf->x = (renderer->width - temp_buf->width) / 2;

  if (temp_buf->height < renderer->height)
    temp_buf->y = (renderer->height - temp_buf->height) / 2;

  if (renderer->is_popup)
    {
      gimp_view_renderer_render_buffer (renderer, temp_buf, -1,
                                        GIMP_VIEW_BG_WHITE,
                                        GIMP_VIEW_BG_WHITE);

      temp_buf_free (temp_buf);

      if (GIMP_IS_BRUSH_PIPE (renderer->viewable))
        {
          renderbrush->pipe_animation_index = 0;
          renderbrush->pipe_timeout_id =
            g_timeout_add (300, gimp_view_renderer_brush_render_timeout,
                           renderbrush);
        }

      return;
    }

  gimp_view_renderer_render_buffer (renderer, temp_buf, -1,
                                    GIMP_VIEW_BG_WHITE,
                                    GIMP_VIEW_BG_WHITE);

  temp_buf_free (temp_buf);

#define INDICATOR_WIDTH  7
#define INDICATOR_HEIGHT 7

  if (renderer->width  >= INDICATOR_WIDTH  * 2 &&
      renderer->height >= INDICATOR_HEIGHT * 2 &&
      (renderer->width  < brush_width          ||
       renderer->height < brush_height         ||
       GIMP_IS_BRUSH_PIPE (renderer->viewable) ||
       GIMP_IS_BRUSH_GENERATED (renderer->viewable)))
    {
#define WHT { 255, 255, 255 }
#define BLK {   0,   0,   0 }
#define RED { 255, 127, 127 }
#define BLU { 127, 150, 255 }

      static const guchar scale_indicator_bits[7][7][3] =
      {
        { WHT, WHT, WHT, WHT, WHT, WHT, WHT },
        { WHT, WHT, WHT, BLK, WHT, WHT, WHT },
        { WHT, WHT, WHT, BLK, WHT, WHT, WHT },
        { WHT, BLK, BLK, BLK, BLK, BLK, WHT },
        { WHT, WHT, WHT, BLK, WHT, WHT, WHT },
        { WHT, WHT, WHT, BLK, WHT, WHT, WHT },
        { WHT, WHT, WHT, WHT, WHT, WHT, WHT }
      };

      static const guchar scale_pipe_indicator_bits[7][7][3] =
      {
        { WHT, WHT, WHT, WHT, WHT, WHT, WHT },
        { WHT, WHT, WHT, BLK, WHT, WHT, RED },
        { WHT, WHT, WHT, BLK, WHT, RED, RED },
        { WHT, BLK, BLK, BLK, BLK, BLK, RED },
        { WHT, WHT, WHT, BLK, RED, RED, RED },
        { WHT, WHT, RED, BLK, RED, RED, RED },
        { WHT, RED, RED, RED, RED, RED, RED }
      };

      static const guchar pipe_indicator_bits[7][7][3] =
      {
        { WHT, WHT, WHT, WHT, WHT, WHT, WHT },
        { WHT, WHT, WHT, WHT, WHT, WHT, RED },
        { WHT, WHT, WHT, WHT, WHT, RED, RED },
        { WHT, WHT, WHT, WHT, RED, RED, RED },
        { WHT, WHT, WHT, RED, RED, RED, RED },
        { WHT, WHT, RED, RED, RED, RED, RED },
        { WHT, RED, RED, RED, RED, RED, RED }
      };

      static const guchar scale_genbrush_indicator_bits[7][7][3] =
      {
        { WHT, WHT, WHT, WHT, WHT, WHT, WHT },
        { WHT, WHT, WHT, BLK, WHT, WHT, BLU },
        { WHT, WHT, WHT, BLK, WHT, BLU, BLU },
        { WHT, BLK, BLK, BLK, BLK, BLK, BLU },
        { WHT, WHT, WHT, BLK, WHT, BLU, BLU },
        { WHT, WHT, BLU, BLK, BLU, BLU, BLU },
        { WHT, WHT, BLU, BLU, BLU, BLU, BLU }
      };

      static const guchar genbrush_indicator_bits[7][7][3] =
      {
        { WHT, WHT, WHT, WHT, WHT, WHT, WHT },
        { WHT, WHT, WHT, WHT, WHT, WHT, BLU },
        { WHT, WHT, WHT, WHT, WHT, BLU, BLU },
        { WHT, WHT, WHT, WHT, BLU, BLU, BLU },
        { WHT, WHT, WHT, BLU, BLU, BLU, BLU },
        { WHT, WHT, BLU, BLU, BLU, BLU, BLU },
        { WHT, BLU, BLU, BLU, BLU, BLU, BLU }
      };

#undef WHT
#undef BLK
#undef RED
#undef BLU

      guchar   *buf;
      guchar   *b;
      gint      x, y;
      gint      offset_x;
      gint      offset_y;
      gboolean  alpha;
      gboolean  pipe;
      gboolean  genbrush;
      gboolean  scale;

      offset_x = renderer->width  - INDICATOR_WIDTH;
      offset_y = renderer->height - INDICATOR_HEIGHT;

      buf = renderer->buffer + (offset_y * renderer->rowstride +
                                offset_x * renderer->bytes);

      pipe     = GIMP_IS_BRUSH_PIPE (renderer->viewable);
      genbrush = GIMP_IS_BRUSH_GENERATED (renderer->viewable);
      scale    = (renderer->width  < brush_width ||
                  renderer->height < brush_height);
      alpha    = (renderer->bytes == 4);

      for (y = 0; y < INDICATOR_HEIGHT; y++)
        {
          b = buf;

          for (x = 0; x < INDICATOR_WIDTH; x++)
            {
              if (scale)
                {
                  if (pipe)
                    {
                      *b++ = scale_pipe_indicator_bits[y][x][0];
                      *b++ = scale_pipe_indicator_bits[y][x][1];
                      *b++ = scale_pipe_indicator_bits[y][x][2];
                    }
                  else if (genbrush)
                    {
                      *b++ = scale_genbrush_indicator_bits[y][x][0];
                      *b++ = scale_genbrush_indicator_bits[y][x][1];
                      *b++ = scale_genbrush_indicator_bits[y][x][2];
                    }
                  else
                    {
                      *b++ = scale_indicator_bits[y][x][0];
                      *b++ = scale_indicator_bits[y][x][1];
                      *b++ = scale_indicator_bits[y][x][2];
                    }
                }
              else if (pipe)
                {
                  *b++ = pipe_indicator_bits[y][x][0];
                  *b++ = pipe_indicator_bits[y][x][1];
                  *b++ = pipe_indicator_bits[y][x][2];
                }
              else if (genbrush)
                {
                  *b++ = genbrush_indicator_bits[y][x][0];
                  *b++ = genbrush_indicator_bits[y][x][1];
                  *b++ = genbrush_indicator_bits[y][x][2];
                }

              if (alpha)
                *b++ = 255;
            }

          buf += renderer->rowstride;
        }
    }

#undef INDICATOR_WIDTH
#undef INDICATOR_HEIGHT
}

static gboolean
gimp_view_renderer_brush_render_timeout (gpointer data)
{
  GimpViewRendererBrush *renderbrush = GIMP_VIEW_RENDERER_BRUSH (data);
  GimpViewRenderer      *renderer    = GIMP_VIEW_RENDERER (data);
  GimpBrushPipe         *brush_pipe;
  GimpBrush             *brush;
  TempBuf               *temp_buf;

  if (! renderer->viewable)
    {
      renderbrush->pipe_timeout_id      = 0;
      renderbrush->pipe_animation_index = 0;

      return FALSE;
    }

  brush_pipe = GIMP_BRUSH_PIPE (renderer->viewable);

  renderbrush->pipe_animation_index++;

  if (renderbrush->pipe_animation_index >= brush_pipe->nbrushes)
    renderbrush->pipe_animation_index = 0;

  brush =
    GIMP_BRUSH (brush_pipe->brushes[renderbrush->pipe_animation_index]);

  temp_buf = gimp_viewable_get_new_preview (GIMP_VIEWABLE (brush),
                                            renderer->context,
                                            renderer->width,
                                            renderer->height);

  if (temp_buf->width < renderer->width)
    temp_buf->x = (renderer->width - temp_buf->width) / 2;

  if (temp_buf->height < renderer->height)
    temp_buf->y = (renderer->height - temp_buf->height) / 2;

  gimp_view_renderer_render_buffer (renderer, temp_buf, -1,
                                    GIMP_VIEW_BG_WHITE,
                                    GIMP_VIEW_BG_WHITE);

  temp_buf_free (temp_buf);

  gimp_view_renderer_update (renderer);

  return TRUE;
}
