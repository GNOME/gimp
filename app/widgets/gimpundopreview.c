/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpUndoPreview Widget
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

#include "core/gimpundo.h"

#include "gimpundopreview.h"


static void   gimp_undo_preview_class_init (GimpUndoPreviewClass *klass);
static void   gimp_undo_preview_init       (GimpUndoPreview      *preview);

static void   gimp_undo_preview_render     (GimpPreview          *preview);


static GimpPreviewClass *parent_class = NULL;


GType
gimp_undo_preview_get_type (void)
{
  static GType preview_type = 0;

  if (! preview_type)
    {
      static const GTypeInfo preview_info =
      {
        sizeof (GimpUndoPreviewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_undo_preview_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpUndoPreview),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_undo_preview_init,
      };

      preview_type = g_type_register_static (GIMP_TYPE_PREVIEW,
                                             "GimpUndoPreview",
                                             &preview_info, 0);
    }
  
  return preview_type;
}

static void
gimp_undo_preview_class_init (GimpUndoPreviewClass *klass)
{
  GimpPreviewClass *preview_class;

  preview_class = GIMP_PREVIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  preview_class->render = gimp_undo_preview_render;
}

static void
gimp_undo_preview_init (GimpUndoPreview *preview)
{
}

static void
gimp_undo_preview_render (GimpPreview *preview)
{
  GimpUndo *undo;
  gint      width;
  gint      height;
  gint      preview_width;
  gint      preview_height;
  gboolean  scaling_up;
  TempBuf  *render_buf;
  gboolean  free_buf = FALSE;

  undo = GIMP_UNDO (preview->viewable);

  width  = preview->width;
  height = preview->height;

  render_buf = gimp_viewable_get_preview (preview->viewable, width, height);

  if (! render_buf)
    return;

  gimp_viewable_calc_preview_size (preview->viewable,
                                   render_buf->width,
                                   render_buf->height,
                                   width,
                                   height,
                                   preview->dot_for_dot,
                                   1.0, 1.0,
                                   &preview_width,
                                   &preview_height,
                                   &scaling_up);

  if (preview_width  != render_buf->width ||
      preview_height != render_buf->height)
    {
      render_buf = temp_buf_scale (render_buf, preview_width, preview_height);

      free_buf = TRUE;
    }

  if (preview_width  < width)  render_buf->x = (width  - preview_width)  / 2;
  if (preview_height < height) render_buf->y = (height - preview_height) / 2;

  if (render_buf->x || render_buf->y)
    {
      TempBuf *temp_buf;
      guchar   white[4] = { 255, 255, 255, 255 };

      temp_buf = temp_buf_new (width, height,
			       render_buf->bytes,
			       0, 0,
			       white);

      temp_buf_copy_area (render_buf, temp_buf,
			  0, 0,
			  render_buf->width,
			  render_buf->height,
			  render_buf->x,
			  render_buf->y);

      if (free_buf)
        temp_buf_free (render_buf);

      render_buf = temp_buf;

      free_buf = TRUE;
    }

  gimp_preview_render_and_flush (preview, render_buf, -1);

  if (free_buf)
    temp_buf_free (render_buf);
}
