/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpImagePreview Widget
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

#include "core/gimpimage.h"

#include "gimpimagepreview.h"


static void   gimp_image_preview_class_init (GimpImagePreviewClass *klass);
static void   gimp_image_preview_init       (GimpImagePreview      *preview);

static void        gimp_image_preview_render       (GimpPreview *preview);
static void        gimp_image_preview_get_size     (GimpPreview *preview,
						    gint         size,
						    gint        *width,
						    gint        *height);
static GtkWidget * gimp_image_preview_create_popup (GimpPreview *preview);


static GimpPreviewClass *parent_class = NULL;


GType
gimp_image_preview_get_type (void)
{
  static GType preview_type = 0;

  if (! preview_type)
    {
      static const GTypeInfo preview_info =
      {
        sizeof (GimpImagePreviewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_image_preview_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpImagePreview),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_image_preview_init,
      };

      preview_type = g_type_register_static (GIMP_TYPE_PREVIEW,
                                             "GimpImagePreview",
                                             &preview_info, 0);
    }
  
  return preview_type;
}

static void
gimp_image_preview_class_init (GimpImagePreviewClass *klass)
{
  GimpPreviewClass *preview_class;

  preview_class = GIMP_PREVIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  preview_class->render       = gimp_image_preview_render;
  preview_class->get_size     = gimp_image_preview_get_size;
  preview_class->create_popup = gimp_image_preview_create_popup;
}

static void
gimp_image_preview_init (GimpImagePreview *preview)
{
  preview->channel = -1;
}

static void
gimp_image_preview_get_size (GimpPreview *preview,
			     gint         size,
			     gint        *width,
			     gint        *height)
{
  GimpImage *gimage;
  gboolean   scaling_up;

  gimage = GIMP_IMAGE (preview->viewable);

  gimp_preview_calc_size (preview,
			  gimage->width,
			  gimage->height,
			  size,
			  size,
			  gimage->xresolution,
			  gimage->yresolution,
			  width,
			  height,
			  &scaling_up);
}

static void
gimp_image_preview_render (GimpPreview *preview)
{
  GimpImage *gimage;
  gint       width;
  gint       height;
  gint       preview_width;
  gint       preview_height;
  gboolean   scaling_up;
  TempBuf   *render_buf;

  gimage = GIMP_IMAGE (preview->viewable);

  width  = preview->width;
  height = preview->height;

  gimp_preview_calc_size (preview,
			  gimage->width,
			  gimage->height,
			  width,
			  height,
			  gimage->xresolution,
			  gimage->yresolution,
			  &preview_width,
			  &preview_height,
			  &scaling_up);

  if (scaling_up)
    {
      TempBuf *temp_buf;

      temp_buf = gimp_viewable_get_new_preview (preview->viewable,
						gimage->width, 
						gimage->height);
      render_buf = temp_buf_scale (temp_buf, preview_width, preview_height);

      temp_buf_free (temp_buf);
    }
  else
    {
      render_buf = gimp_viewable_get_new_preview (preview->viewable,
						  preview_width,
						  preview_height);
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

      temp_buf_free (render_buf);

      gimp_preview_render_and_flush (preview,
				     temp_buf,
				     GIMP_IMAGE_PREVIEW (preview)->channel);

      temp_buf_free (temp_buf);

      return;
    }

  gimp_preview_render_and_flush (preview,
				 render_buf,
				 GIMP_IMAGE_PREVIEW (preview)->channel);

  temp_buf_free (render_buf);
}

static GtkWidget *
gimp_image_preview_create_popup (GimpPreview *preview)
{
  GimpImage *gimage;
  gint       popup_width;
  gint       popup_height;
  gboolean   scaling_up;

  gimage = GIMP_IMAGE (preview->viewable);

  gimp_preview_calc_size (preview,
			  gimage->width,
			  gimage->height,
			  MIN (preview->width  * 2, 256),
			  MIN (preview->height * 2, 256),
			  gimage->xresolution,
			  gimage->yresolution,
			  &popup_width,
			  &popup_height,
			  &scaling_up);

  if (scaling_up)
    {
      return gimp_preview_new_full (preview->viewable,
				    gimage->width,
				    gimage->height,
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
