/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbufferpreview.c
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

#include "core/gimpbuffer.h"

#include "gimpbufferpreview.h"


static void   gimp_buffer_preview_class_init (GimpBufferPreviewClass *klass);
static void   gimp_buffer_preview_init       (GimpBufferPreview      *preview);

static void        gimp_buffer_preview_render       (GimpPreview *preview);
static GtkWidget * gimp_buffer_preview_create_popup (GimpPreview *preview);
static gboolean    gimp_buffer_preview_needs_popup  (GimpPreview *preview);


static GimpPreviewClass *parent_class = NULL;


GType
gimp_buffer_preview_get_type (void)
{
  static GType preview_type = 0;

  if (! preview_type)
    {
      static const GTypeInfo preview_info =
      {
        sizeof (GimpBufferPreviewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_buffer_preview_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpBufferPreview),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_buffer_preview_init,
      };

      preview_type = g_type_register_static (GIMP_TYPE_PREVIEW,
                                             "GimpBufferPreview",
                                             &preview_info, 0);
    }
  
  return preview_type;
}

static void
gimp_buffer_preview_class_init (GimpBufferPreviewClass *klass)
{
  GimpPreviewClass *preview_class;

  preview_class = GIMP_PREVIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  preview_class->render       = gimp_buffer_preview_render;
  preview_class->create_popup = gimp_buffer_preview_create_popup;
  preview_class->needs_popup  = gimp_buffer_preview_needs_popup;
}

static void
gimp_buffer_preview_init (GimpBufferPreview *buffer_preview)
{
}

static void
gimp_buffer_preview_render (GimpPreview *preview)
{
  GimpBuffer *buffer;
  gint        buffer_width;
  gint        buffer_height;
  gint        width;
  gint        height;
  gint        preview_width;
  gint        preview_height;
  gboolean    scaling_up;
  TempBuf    *render_buf;

  buffer = GIMP_BUFFER (preview->viewable);

  buffer_width  = gimp_buffer_get_width (buffer);
  buffer_height = gimp_buffer_get_height (buffer);

  width  = preview->width;
  height = preview->height;

  gimp_viewable_calc_preview_size (preview->viewable,
                                   buffer_width,
                                   buffer_height,
                                   width,
                                   height,
                                   preview->dot_for_dot,
                                   1.0,
                                   1.0,
                                   &preview_width,
                                   &preview_height,
                                   &scaling_up);

  if (scaling_up)
    {
      TempBuf *temp_buf;

      temp_buf = gimp_viewable_get_new_preview (preview->viewable,
						buffer_width,
						buffer_height);
      render_buf = temp_buf_scale (temp_buf, preview_width, preview_height);

      temp_buf_free (temp_buf);
    }
  else
    {
      render_buf = gimp_viewable_get_new_preview (preview->viewable,
						  preview_width,
						  preview_height);
    }

  if (preview_width < width)
    render_buf->x = (width - preview_width) / 2;

  if (preview_height < height)
    render_buf->y = (height - preview_height) / 2;

  gimp_preview_render_preview (preview,
                               render_buf,
                               -1,
                               GIMP_PREVIEW_BG_WHITE,
                               GIMP_PREVIEW_BG_WHITE);

  temp_buf_free (render_buf);
}

static GtkWidget *
gimp_buffer_preview_create_popup (GimpPreview *preview)
{
  GimpBuffer *buffer;
  gint        buffer_width;
  gint        buffer_height;
  gint        popup_width;
  gint        popup_height;
  gboolean    scaling_up;

  buffer = GIMP_BUFFER (preview->viewable);

  buffer_width  = gimp_buffer_get_width (buffer);
  buffer_height = gimp_buffer_get_height (buffer);

  gimp_viewable_calc_preview_size (preview->viewable,
                                   buffer_width,
                                   buffer_height,
                                   MIN (preview->width  * 2,
                                        GIMP_PREVIEW_MAX_POPUP_SIZE),
                                   MIN (preview->height * 2,
                                        GIMP_PREVIEW_MAX_POPUP_SIZE),
                                   preview->dot_for_dot,
                                   1.0,
                                   1.0,
                                   &popup_width,
                                   &popup_height,
                                   &scaling_up);

  if (scaling_up)
    {
      return gimp_preview_new_full (preview->viewable,
				    buffer_width,
				    buffer_height,
				    0,
				    TRUE, FALSE, FALSE);
    }
  else
    {
      return gimp_preview_new_full (preview->viewable,
				    popup_width,
				    popup_height,
				    0,
				    TRUE, FALSE, FALSE);
    }
}

static gboolean
gimp_buffer_preview_needs_popup (GimpPreview *preview)
{
  GimpBuffer *buffer;
  gint        buffer_width;
  gint        buffer_height;

  buffer = GIMP_BUFFER (preview->viewable);

  buffer_width  = gimp_buffer_get_width (buffer);
  buffer_height = gimp_buffer_get_height (buffer);

  if (buffer_width > preview->width || buffer_height > preview->height)
    return TRUE;

  return FALSE;
}
