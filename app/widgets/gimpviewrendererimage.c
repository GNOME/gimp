/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpImagePreview Widget
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

#include "apptypes.h"

#include "gimpimage.h"
#include "gimpimagepreview.h"
#include "gimpviewable.h"
#include "temp_buf.h"


static void   gimp_image_preview_class_init (GimpImagePreviewClass *klass);
static void   gimp_image_preview_init       (GimpImagePreview      *preview);

static TempBuf   * gimp_image_preview_create_preview (GimpPreview *preview);
static GtkWidget * gimp_image_preview_create_popup   (GimpPreview *preview);
static void        gimp_image_preview_calc_size      (GimpImage   *gimage,
						      gint         width,
						      gint         height,
						      gint        *return_width,
						      gint        *return_height,
						      gboolean    *scaling_up);


static GimpPreviewClass *parent_class = NULL;


GtkType
gimp_image_preview_get_type (void)
{
  static GtkType preview_type = 0;

  if (! preview_type)
    {
      GtkTypeInfo preview_info =
      {
	"GimpImagePreview",
	sizeof (GimpImagePreview),
	sizeof (GimpImagePreviewClass),
	(GtkClassInitFunc) gimp_image_preview_class_init,
	(GtkObjectInitFunc) gimp_image_preview_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      preview_type = gtk_type_unique (GIMP_TYPE_PREVIEW, &preview_info);
    }
  
  return preview_type;
}

static void
gimp_image_preview_class_init (GimpImagePreviewClass *klass)
{
  GtkObjectClass   *object_class;
  GimpPreviewClass *preview_class;

  object_class  = (GtkObjectClass *) klass;
  preview_class = (GimpPreviewClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_PREVIEW);

  preview_class->create_preview = gimp_image_preview_create_preview;
  preview_class->create_popup   = gimp_image_preview_create_popup;
}

static void
gimp_image_preview_init (GimpImagePreview *preview)
{
}

static TempBuf *
gimp_image_preview_create_preview (GimpPreview *preview)
{
  GimpImage *gimage;
  gint       width;
  gint       height;
  gint       preview_width;
  gint       preview_height;
  gboolean   scaling_up;
  TempBuf   *return_buf;

  gimage = GIMP_IMAGE (preview->viewable);

  width  = GTK_WIDGET (preview)->allocation.width;
  height = GTK_WIDGET (preview)->allocation.height;

  gimp_image_preview_calc_size (gimage,
				width,
				height,
				&preview_width,
				&preview_height,
				&scaling_up);

  if (scaling_up)
    {
      TempBuf *temp_buf;

      temp_buf = gimp_viewable_get_new_preview (preview->viewable,
						gimage->width, 
						gimage->height);
      return_buf = temp_buf_scale (temp_buf, preview_width, preview_height);

      temp_buf_free (temp_buf);
    }
  else
    {
      return_buf = gimp_viewable_get_new_preview (preview->viewable,
						  preview_width,
						  preview_height);
    }

  if (preview_width  < width)  return_buf->x = (width  - preview_width)  / 2;
  if (preview_height < height) return_buf->y = (height - preview_height) / 2;

  if (return_buf->x || return_buf->y)
    {
      TempBuf *temp_buf;
      guchar   white[4] = { 255, 255, 255, 255 };

      temp_buf = temp_buf_new (width, height,
			       return_buf->bytes,
			       0, 0,
			       white);

      temp_buf_copy_area (return_buf, temp_buf,
			  0, 0,
			  return_buf->width,
			  return_buf->height,
			  return_buf->x,
			  return_buf->y);

      temp_buf_free (return_buf);

      return temp_buf;
    }

  return return_buf;
}

static GtkWidget *
gimp_image_preview_create_popup (GimpPreview *preview)
{
  GimpImage *gimage;
  gint       popup_width;
  gint       popup_height;
  gboolean   scaling_up;

  gimage = GIMP_IMAGE (preview->viewable);

  gimp_image_preview_calc_size
    (gimage,
     MIN (GTK_WIDGET (preview)->allocation.width  * 2, 256),
     MIN (GTK_WIDGET (preview)->allocation.height * 2, 256),
     &popup_width,
     &popup_height,
     &scaling_up);

  if (scaling_up)
    {
      return gimp_preview_new (preview->viewable,
			       TRUE,
			       gimage->width,
			       gimage->height,
			       FALSE,
			       FALSE);
    }
  else
    {
      return gimp_preview_new (preview->viewable,
			       TRUE,
			       popup_width,
			       popup_height,
			       FALSE,
			       FALSE);
    }
}

static void
gimp_image_preview_calc_size (GimpImage *gimage,
			      gint       width,
			      gint       height,
			      gint      *return_width,
			      gint      *return_height,
			      gboolean  *scaling_up)
{
  gdouble ratio;

  if (gimage->width > gimage->height)
    {
      ratio = (gdouble) width / (gdouble) gimage->width;
    }
  else
    {
      ratio = (gdouble) height / (gdouble) gimage->height;
    }

  width  = RINT (ratio * (gdouble) gimage->width);
  height = RINT (ratio * (gdouble) gimage->height);

  if (width  < 1) width  = 1;
  if (height < 1) height = 1;

  *return_width  = width;
  *return_height = height;
  *scaling_up    = (ratio > 1.0);
}
