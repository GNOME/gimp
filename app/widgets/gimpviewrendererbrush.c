/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrushpreview.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "core/gimpbrush.h"
#include "core/gimpbrushpipe.h"

#include "gimpbrushpreview.h"
#include "gimpdnd.h"


static void   gimp_brush_preview_class_init (GimpBrushPreviewClass *klass);
static void   gimp_brush_preview_init       (GimpBrushPreview      *preview);

static void        gimp_brush_preview_destroy        (GtkObject   *object);
static void        gimp_brush_preview_render         (GimpPreview *preview);

static gboolean    gimp_brush_preview_render_timeout (gpointer     data);


static GimpPreviewClass *parent_class = NULL;


GType
gimp_brush_preview_get_type (void)
{
  static GType preview_type = 0;

  if (! preview_type)
    {
      static const GTypeInfo preview_info =
      {
        sizeof (GimpBrushPreviewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_brush_preview_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpBrushPreview),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_brush_preview_init,
      };

      preview_type = g_type_register_static (GIMP_TYPE_PREVIEW,
                                             "GimpBrushPreview",
                                             &preview_info, 0);
    }
  
  return preview_type;
}

static void
gimp_brush_preview_class_init (GimpBrushPreviewClass *klass)
{
  GtkObjectClass   *object_class;
  GimpPreviewClass *preview_class;

  object_class  = GTK_OBJECT_CLASS (klass);
  preview_class = GIMP_PREVIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->destroy = gimp_brush_preview_destroy;

  preview_class->render = gimp_brush_preview_render;
}

static void
gimp_brush_preview_init (GimpBrushPreview *brush_preview)
{
  brush_preview->pipe_timeout_id      = 0;
  brush_preview->pipe_animation_index = 0;
}

static void
gimp_brush_preview_destroy (GtkObject *object)
{
  GimpBrushPreview *brush_preview;

  brush_preview = GIMP_BRUSH_PREVIEW (object);

  if (brush_preview->pipe_timeout_id)
    {
      g_source_remove (brush_preview->pipe_timeout_id);

      brush_preview->pipe_timeout_id      = 0;
      brush_preview->pipe_animation_index = 0;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_brush_preview_render (GimpPreview *preview)
{
  GimpBrushPreview *brush_preview;
  GimpBrush        *brush;
  TempBuf          *temp_buf;
  gint              width;
  gint              height;
  gint              brush_width;
  gint              brush_height;

  brush_preview = GIMP_BRUSH_PREVIEW (preview);

  if (brush_preview->pipe_timeout_id)
    {
      g_source_remove (brush_preview->pipe_timeout_id);
      brush_preview->pipe_timeout_id = 0;
    }

  brush        = GIMP_BRUSH (preview->viewable);
  brush_width  = brush->mask->width;
  brush_height = brush->mask->height;

  width  = preview->width;
  height = preview->height;

  temp_buf = gimp_viewable_get_new_preview (preview->viewable,
                                            width, height);

  if (temp_buf->width < width)
    temp_buf->x = (width - temp_buf->width) / 2;

  if (temp_buf->height < height)
    temp_buf->y = (height - temp_buf->height) / 2;

  if (preview->is_popup)
    {
      gimp_preview_render_preview (preview, temp_buf, -1,
                                   GIMP_PREVIEW_BG_WHITE,
                                   GIMP_PREVIEW_BG_WHITE);

      temp_buf_free (temp_buf);

      if (GIMP_IS_BRUSH_PIPE (brush))
	{
	  if (width != brush_width || height != brush_height)
	    {
	      g_warning ("%s(): non-fullsize pipe popups are not supported yet.",
			 G_GNUC_FUNCTION);
	      return;
	    }

	  brush_preview->pipe_animation_index = 0;
	  brush_preview->pipe_timeout_id =
            g_timeout_add (300, gimp_brush_preview_render_timeout,
                           brush_preview);
	}

      return;
    }

#define INDICATOR_WIDTH  7
#define INDICATOR_HEIGHT 7

  if (temp_buf->width  >= INDICATOR_WIDTH  &&
      temp_buf->height >= INDICATOR_HEIGHT &&
      (width  < brush_width  ||
       height < brush_height ||
       GIMP_IS_BRUSH_PIPE (brush)))
    {
#define WHT { 255, 255, 255 }
#define BLK {   0,   0,   0 }
#define RED { 255, 127, 127 }

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

#undef WHT
#undef BLK
#undef RED

      guchar   *buf;
      gint      x, y;
      gint      offset_x;
      gint      offset_y;
      gboolean  alpha;
      gboolean  pipe;
      gboolean  scale;

      buf = temp_buf_data (temp_buf);

      offset_x = temp_buf->width  - INDICATOR_WIDTH;
      offset_y = temp_buf->height - INDICATOR_HEIGHT;

      buf += (offset_y * temp_buf->width + offset_x) * temp_buf->bytes;

      pipe  = GIMP_IS_BRUSH_PIPE (brush);
      scale = (width < brush_width || height < brush_height);
      alpha = (temp_buf->bytes == 4);

      for (y = 0; y < INDICATOR_HEIGHT; y++)
        {
          for (x = 0; x < INDICATOR_WIDTH; x++)
            {
              if (scale)
                {
                  if (pipe)
                    {
                      *buf++ = scale_pipe_indicator_bits[y][x][0];
                      *buf++ = scale_pipe_indicator_bits[y][x][1];
                      *buf++ = scale_pipe_indicator_bits[y][x][2];
                    }
                  else
                    {
                      *buf++ = scale_indicator_bits[y][x][0];
                      *buf++ = scale_indicator_bits[y][x][1];
                      *buf++ = scale_indicator_bits[y][x][2];
                    }
                }
              else if (pipe)
                {
                  *buf++ = pipe_indicator_bits[y][x][0];
                  *buf++ = pipe_indicator_bits[y][x][1];
                  *buf++ = pipe_indicator_bits[y][x][2];
                }

              if (alpha)
                *buf++ = 255;
            }

          buf += (temp_buf->width - INDICATOR_WIDTH) * temp_buf->bytes;
        }
    }

#undef INDICATOR_WIDTH
#undef INDICATOR_HEIGHT

  gimp_preview_render_preview (preview, temp_buf, -1,
                               GIMP_PREVIEW_BG_WHITE,
                               GIMP_PREVIEW_BG_WHITE);

  temp_buf_free (temp_buf);
}

static gboolean
gimp_brush_preview_render_timeout (gpointer data)
{
  GimpBrushPreview *brush_preview;
  GimpPreview      *preview;
  GimpBrushPipe    *brush_pipe;
  GimpBrush        *brush;
  TempBuf          *temp_buf;

  brush_preview = GIMP_BRUSH_PREVIEW (data);

  preview = GIMP_PREVIEW (brush_preview);

  if (! preview->viewable)
    {
      brush_preview->pipe_timeout_id      = 0;
      brush_preview->pipe_animation_index = 0;

      return FALSE;
    }

  brush_pipe = GIMP_BRUSH_PIPE (preview->viewable);

  brush_preview->pipe_animation_index++;

  if (brush_preview->pipe_animation_index >= brush_pipe->nbrushes)
    brush_preview->pipe_animation_index = 0;

  brush =
    GIMP_BRUSH (brush_pipe->brushes[brush_preview->pipe_animation_index]);

  temp_buf = gimp_viewable_get_new_preview (GIMP_VIEWABLE (brush),
					    preview->width,
					    preview->height);

  gimp_preview_render_preview (preview, temp_buf, -1,
                               GIMP_PREVIEW_BG_WHITE,
                               GIMP_PREVIEW_BG_WHITE);

  temp_buf_free (temp_buf);

  gtk_widget_queue_draw (GTK_WIDGET (preview));

  return TRUE;
}
