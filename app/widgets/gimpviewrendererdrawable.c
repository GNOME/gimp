/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawablepreview.c
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

#include "libgimpmath/gimpmath.h"

#include "widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "gimpdrawablepreview.h"


static void   gimp_drawable_preview_class_init (GimpDrawablePreviewClass *klass);
static void   gimp_drawable_preview_init       (GimpDrawablePreview      *preview);

static void        gimp_drawable_preview_render       (GimpPreview *preview);
static void        gimp_drawable_preview_get_size     (GimpPreview *preview,
						       gint         size,
						       gint        *width,
						       gint        *height);
static GtkWidget * gimp_drawable_preview_create_popup (GimpPreview *preview);


static GimpPreviewClass *parent_class = NULL;


GType
gimp_drawable_preview_get_type (void)
{
  static GType preview_type = 0;

  if (! preview_type)
    {
      static const GTypeInfo preview_info =
      {
        sizeof (GimpDrawablePreviewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_drawable_preview_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpDrawablePreview),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_drawable_preview_init,
      };

      preview_type = g_type_register_static (GIMP_TYPE_PREVIEW,
                                             "GimpDrawablePreview",
                                             &preview_info, 0);
    }
  
  return preview_type;
}

static void
gimp_drawable_preview_class_init (GimpDrawablePreviewClass *klass)
{
  GimpPreviewClass *preview_class;

  preview_class = GIMP_PREVIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  preview_class->render       = gimp_drawable_preview_render;
  preview_class->get_size     = gimp_drawable_preview_get_size;
  preview_class->create_popup = gimp_drawable_preview_create_popup;
}

static void
gimp_drawable_preview_init (GimpDrawablePreview *preview)
{
}

static void
gimp_drawable_preview_get_size (GimpPreview *preview,
                                gint         size,
                                gint        *width,
                                gint        *height)
{
  GimpDrawable *drawable;
  GimpImage    *gimage;
  gboolean      scaling_up;

  drawable = GIMP_DRAWABLE (preview->viewable);
  gimage   = gimp_drawable_gimage (drawable);

  if (gimage && ! preview->is_popup)
    {
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
  else
    {
      gimp_preview_calc_size (preview,
			      drawable->width,
			      drawable->height,
			      size,
			      size,
			      1.0,
			      1.0,
			      width,
			      height,
			      &scaling_up);
    }
}

static void
gimp_drawable_preview_render (GimpPreview *preview)
{
  GimpDrawable *drawable;
  GimpImage    *gimage;
  gint          width;
  gint          height;
  gint          preview_width;
  gint          preview_height;
  gboolean      scaling_up;
  TempBuf      *render_buf;

  drawable = GIMP_DRAWABLE (preview->viewable);
  gimage   = gimp_drawable_gimage (drawable);

  width  = preview->width;
  height = preview->height;

  if (gimage && ! preview->is_popup)
    {
      width  = MAX (1, ROUND ((((gdouble) width / (gdouble) gimage->width) *
			       (gdouble) drawable->width)));
      height = MAX (1, ROUND ((((gdouble) height / (gdouble) gimage->height) *
			      (gdouble) drawable->height)));

      gimp_preview_calc_size (preview,
			      drawable->width,
			      drawable->height,
			      width,
			      height,
			      gimage->xresolution,
			      gimage->yresolution,
			      &preview_width,
			      &preview_height,
			      &scaling_up);
    }
  else
    {
      gimp_preview_calc_size (preview,
			      drawable->width,
			      drawable->height,
			      width,
			      height,
			      gimage ? gimage->xresolution : 1.0,
			      gimage ? gimage->yresolution : 1.0,
			      &preview_width,
			      &preview_height,
			      &scaling_up);
    }

  if (scaling_up)
    {
      TempBuf *temp_buf;

      temp_buf = gimp_viewable_get_new_preview (preview->viewable,
						drawable->width, 
						drawable->height);
      render_buf = temp_buf_scale (temp_buf, preview_width, preview_height);

      temp_buf_free (temp_buf);
    }
  else
    {
      render_buf = gimp_viewable_get_new_preview (preview->viewable,
						  preview_width,
						  preview_height);
    }

  if (gimage && ! preview->is_popup)
    {
      if (preview_width < preview->width)
	render_buf->x =
	  ROUND ((((gdouble) preview->width / (gdouble) gimage->width) *
		  (gdouble) drawable->offset_x));

      if (preview_height < preview->height)
	render_buf->y =
	  ROUND ((((gdouble) preview->height / (gdouble) gimage->height) *
		  (gdouble) drawable->offset_y));
    }
  else
    {
      if (preview_width < width)
	render_buf->x = (width - preview_width) / 2;

      if (preview_height < height)
	render_buf->y = (height - preview_height) / 2;
    }

  if (! gimage && (render_buf->x || render_buf->y))
    {
      TempBuf *temp_buf;
      guchar   white[4] = { 0, 0, 0, 0 };

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
				     -1);

      temp_buf_free (temp_buf);

      return;
    }

  gimp_preview_render_and_flush (preview,
				 render_buf,
				 -1);

  temp_buf_free (render_buf);
}

static GtkWidget *
gimp_drawable_preview_create_popup (GimpPreview *preview)
{
  GimpDrawable *drawable;
  GimpImage    *gimage;
  gint          popup_width;
  gint          popup_height;
  gboolean      scaling_up;

  drawable = GIMP_DRAWABLE (preview->viewable);
  gimage   = gimp_drawable_gimage (drawable);

  gimp_preview_calc_size (preview,
			  drawable->width,
			  drawable->height,
			  MIN (preview->width  * 2, 256),
			  MIN (preview->height * 2, 256),
			  gimage ? gimage->xresolution : 1.0,
			  gimage ? gimage->yresolution : 1.0,
			  &popup_width,
			  &popup_height,
			  &scaling_up);

  if (scaling_up)
    {
      return gimp_preview_new_full (preview->viewable,
				    drawable->width,
				    drawable->height,
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
