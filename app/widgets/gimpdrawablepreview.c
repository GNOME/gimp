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

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "gimpdrawablepreview.h"

#include "temp_buf.h"


static void   gimp_drawable_preview_class_init (GimpDrawablePreviewClass *klass);
static void   gimp_drawable_preview_init       (GimpDrawablePreview      *preview);

static void        gimp_drawable_preview_render       (GimpPreview *preview);
static void        gimp_drawable_preview_get_size     (GimpPreview *preview,
						       gint         size,
						       gint        *width,
						       gint        *height);
static GtkWidget * gimp_drawable_preview_create_popup (GimpPreview *preview);


static GimpPreviewClass *parent_class = NULL;


GtkType
gimp_drawable_preview_get_type (void)
{
  static GtkType preview_type = 0;

  if (! preview_type)
    {
      GtkTypeInfo preview_info =
      {
	"GimpDrawablePreview",
	sizeof (GimpDrawablePreview),
	sizeof (GimpDrawablePreviewClass),
	(GtkClassInitFunc) gimp_drawable_preview_class_init,
	(GtkObjectInitFunc) gimp_drawable_preview_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      preview_type = gtk_type_unique (GIMP_TYPE_PREVIEW, &preview_info);
    }
  
  return preview_type;
}

static void
gimp_drawable_preview_class_init (GimpDrawablePreviewClass *klass)
{
  GtkObjectClass   *object_class;
  GimpPreviewClass *preview_class;

  object_class  = (GtkObjectClass *) klass;
  preview_class = (GimpPreviewClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_PREVIEW);

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
  gimage   = drawable->gimage;

  if (gimage)
    {
      gimp_preview_calc_size (gimage->width,
			      gimage->height,
			      size,
			      size,
			      width,
			      height,
			      &scaling_up);
    }
  else
    {
      gimp_preview_calc_size (drawable->width,
			      drawable->height,
			      size,
			      size,
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
  gimage   = drawable->gimage;

  width  = preview->width;
  height = preview->height;

  if (gimage && ! preview->is_popup)
    {
      width  = MAX (1, ROUND ((((gdouble) width / (gdouble) gimage->width) *
			       (gdouble) drawable->width)));
      height = MAX (1, ROUND ((((gdouble) height / (gdouble) gimage->height) *
			      (gdouble) drawable->height)));

      gimp_preview_calc_size (drawable->width,
			      drawable->height,
			      width,
			      height,
			      &preview_width,
			      &preview_height,
			      &scaling_up);
    }
  else
    {
      gimp_preview_calc_size (drawable->width,
			      drawable->height,
			      width,
			      height,
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
  gint          popup_width;
  gint          popup_height;
  gboolean      scaling_up;

  drawable = GIMP_DRAWABLE (preview->viewable);

  gimp_preview_calc_size (drawable->width,
			  drawable->height,
			  MIN (preview->width  * 2, 256),
			  MIN (preview->height * 2, 256),
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
