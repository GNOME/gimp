/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpBrushPreview Widget
 * Copyright (C) 2001 Michael Natterer
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

#include "apptypes.h"

#include "gimpbrush.h"
#include "gimpbrushpipe.h"
#include "gimpbrushpreview.h"
#include "temp_buf.h"


static void   gimp_brush_preview_class_init (GimpBrushPreviewClass *klass);
static void   gimp_brush_preview_init       (GimpBrushPreview      *preview);

static TempBuf   * gimp_brush_preview_create_preview (GimpPreview *preview);
static GtkWidget * gimp_brush_preview_create_popup   (GimpPreview *preview);
static gboolean    gimp_brush_preview_needs_popup    (GimpPreview *preview);


static GimpPreviewClass *parent_class = NULL;


GtkType
gimp_brush_preview_get_type (void)
{
  static GtkType preview_type = 0;

  if (! preview_type)
    {
      GtkTypeInfo preview_info =
      {
	"GimpBrushPreview",
	sizeof (GimpBrushPreview),
	sizeof (GimpBrushPreviewClass),
	(GtkClassInitFunc) gimp_brush_preview_class_init,
	(GtkObjectInitFunc) gimp_brush_preview_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      preview_type = gtk_type_unique (GIMP_TYPE_PREVIEW, &preview_info);
    }
  
  return preview_type;
}

static void
gimp_brush_preview_class_init (GimpBrushPreviewClass *klass)
{
  GtkObjectClass   *object_class;
  GimpPreviewClass *preview_class;

  object_class  = (GtkObjectClass *) klass;
  preview_class = (GimpPreviewClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_PREVIEW);

  preview_class->create_preview = gimp_brush_preview_create_preview;
  preview_class->create_popup   = gimp_brush_preview_create_popup;
  preview_class->needs_popup    = gimp_brush_preview_needs_popup;
}

static void
gimp_brush_preview_init (GimpBrushPreview *preview)
{
}

#define indicator_width  7
#define indicator_height 7

#define WHT { 255, 255, 255 }
#define BLK {   0,   0,   0 }
#define RED { 255, 127, 127 }

static TempBuf *
gimp_brush_preview_create_preview (GimpPreview *preview)
{
  GimpBrush *brush;
  TempBuf   *temp_buf;
  gint       width;
  gint       height;
  gint       brush_width;
  gint       brush_height;
  guchar    *buf;
  guchar    *b;
  gint       x, y;
  gint       offset_x;
  gint       offset_y;

  brush        = GIMP_BRUSH (preview->viewable);
  brush_width  = brush->mask->width;
  brush_height = brush->mask->height;

  width  = GTK_WIDGET (preview)->requisition.width;
  height = GTK_WIDGET (preview)->requisition.height;

  temp_buf = gimp_viewable_get_new_preview (preview->viewable,
					    width, height);

  if (preview->is_popup)
    return temp_buf;

  buf = temp_buf_data (temp_buf);

  if (width < brush_width || height < brush_height)
    {
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

      offset_x = width  - indicator_width;
      offset_y = height - indicator_height;

      b = buf + (offset_y * width + offset_x) * temp_buf->bytes;

      for (y = 0; y < indicator_height; y++)
        {
          for (x = 0; x < indicator_height; x++)
            {
              if (GIMP_IS_BRUSH_PIPE (brush))
                {
                  *b++ = scale_pipe_indicator_bits[y][x][0];
                  *b++ = scale_pipe_indicator_bits[y][x][1];
                  *b++ = scale_pipe_indicator_bits[y][x][2];
                }
              else
                {
                  *b++ = scale_indicator_bits[y][x][0];
                  *b++ = scale_indicator_bits[y][x][1];
                  *b++ = scale_indicator_bits[y][x][2];
                }
            }

          b += (width - indicator_width) * temp_buf->bytes;
        }
    }
  else if (GIMP_IS_BRUSH_PIPE (brush))
    {
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

      offset_x = width  - indicator_width;
      offset_y = height - indicator_height;

      b = buf + (offset_y * width + offset_x) * temp_buf->bytes;

      for (y = 0; y < indicator_height; y++)
        {
          for (x = 0; x < indicator_height; x++)
            {
              *b++ = pipe_indicator_bits[y][x][0];
              *b++ = pipe_indicator_bits[y][x][1];
              *b++ = pipe_indicator_bits[y][x][2];
            }

          b += (width - indicator_width) * temp_buf->bytes;
        }
    }

#undef indicator_width
#undef indicator_height

#undef WHT
#undef BLK
#undef RED

  return temp_buf;
}

static GtkWidget *
gimp_brush_preview_create_popup (GimpPreview *preview)
{
  gint popup_width;
  gint popup_height;

  popup_width  = GIMP_BRUSH (preview->viewable)->mask->width;
  popup_height = GIMP_BRUSH (preview->viewable)->mask->height;

  return gimp_preview_new (preview->viewable,
			   TRUE,
			   popup_width,
			   popup_height,
			   FALSE, FALSE);
}

static gboolean
gimp_brush_preview_needs_popup (GimpPreview *preview)
{
  GimpBrush *brush;
  gint       brush_width;
  gint       brush_height;
  gint       width;
  gint       height;

  brush        = GIMP_BRUSH (preview->viewable);
  brush_width  = brush->mask->width;
  brush_height = brush->mask->height;

  width  = GTK_WIDGET (preview)->requisition.width;
  height = GTK_WIDGET (preview)->requisition.height;

  if (GIMP_IS_BRUSH_PIPE (brush) ||
      brush_width  > width       ||
      brush_height > height)
    return TRUE;

  return FALSE;
}
