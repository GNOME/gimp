/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-duplicate.h"
#include "core/gimpimage-resize.h"
#include "core/gimpimage-scale.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"

#include "convert-dialog.h"
#include "resize-dialog.h"

#include "gimpprogress.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


typedef struct
{
  Resize      *resize;
  GimpDisplay *gdisp;
  GimpImage   *gimage;
} ImageResize;


#define return_if_no_display(gdisp,data) \
  gdisp = gimp_context_get_display (gimp_get_user_context (GIMP (data))); \
  if (! gdisp) \
    return

#define return_if_no_image(gimage,data) \
  gimage = gimp_context_get_image (gimp_get_user_context (GIMP (data))); \
  if (! gimage) \
    return


/*  local functions  */

static void     image_resize_callback        (GtkWidget   *widget,
					      gpointer     data);
static void     image_scale_callback         (GtkWidget   *widget,
					      gpointer     data);
static void     image_scale_warn_callback    (GtkWidget   *widget,
					      gboolean     do_scale,
					      gpointer     data);
static void     image_scale_implement        (ImageResize *image_scale);


/*  public functions  */

void
image_convert_rgb_cmd_callback (GtkWidget *widget,
				gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  convert_to_rgb (gimage);
}

void
image_convert_grayscale_cmd_callback (GtkWidget *widget,
				      gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  convert_to_grayscale (gimage);
}

void
image_convert_indexed_cmd_callback (GtkWidget *widget,
				    gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  convert_to_indexed (gimage);
}

void
image_resize_cmd_callback (GtkWidget *widget,
			   gpointer   data)
{
  GimpDisplay *gdisp;
  GimpImage   *gimage;
  ImageResize *image_resize;
  return_if_no_display (gdisp, data);

  gimage = gdisp->gimage;

  image_resize = g_new0 (ImageResize, 1);

  image_resize->gdisp  = gdisp;
  image_resize->gimage = gimage;
  image_resize->resize = resize_widget_new (gimage,
                                            ResizeWidget,
					    ResizeImage,
					    G_OBJECT (gdisp),
					    "disconnect",
					    gimage->width,
					    gimage->height,
					    gimage->xresolution,
					    gimage->yresolution,
					    gimage->unit,
					    gdisp->dot_for_dot,
					    G_CALLBACK (image_resize_callback),
					    NULL,
					    image_resize);

  g_object_weak_ref (G_OBJECT (image_resize->resize->resize_shell),
		     (GWeakNotify) g_free,
		     image_resize);

  gtk_widget_show (image_resize->resize->resize_shell);
}

void
image_scale_cmd_callback (GtkWidget *widget,
			  gpointer   data)
{
  GimpDisplay *gdisp;
  GimpImage   *gimage;
  ImageResize *image_scale;
  return_if_no_display (gdisp, data);

  gimage = gdisp->gimage;

  image_scale = g_new0 (ImageResize, 1);

  image_scale->gdisp  = gdisp;
  image_scale->gimage = gimage;
  image_scale->resize = resize_widget_new (gimage,
                                           ScaleWidget,
					   ResizeImage,
					   G_OBJECT (gdisp),
					   "disconnect",
					   gimage->width,
					   gimage->height,
					   gimage->xresolution,
					   gimage->yresolution,
					   gimage->unit,
					   gdisp->dot_for_dot,
					   G_CALLBACK (image_scale_callback),
					   NULL,
					   image_scale);

  g_object_weak_ref (G_OBJECT (image_scale->resize->resize_shell),
		     (GWeakNotify) g_free,
		     image_scale);

  gtk_widget_show (image_scale->resize->resize_shell);
}

void
image_duplicate_cmd_callback (GtkWidget *widget,
			      gpointer   data)
{
  GimpImage *gimage;
  GimpImage *new_gimage;
  return_if_no_image (gimage, data);

  new_gimage = gimp_image_duplicate (gimage);

  gimp_create_display (new_gimage->gimp, new_gimage, 0x0101);

  g_object_unref (G_OBJECT (new_gimage));
}


/*  private functions  */

static void
image_resize_callback (GtkWidget *widget,
		       gpointer   data)
{
  ImageResize *image_resize;

  image_resize = (ImageResize *) data;

  g_assert (image_resize != NULL);
  g_assert (image_resize->gimage != NULL);

  gtk_widget_set_sensitive (image_resize->resize->resize_shell, FALSE);

  if (image_resize->resize->width > 0 &&
      image_resize->resize->height > 0) 
    {
      gimp_image_resize (image_resize->gimage,
			 image_resize->resize->width,
			 image_resize->resize->height,
			 image_resize->resize->offset_x,
			 image_resize->resize->offset_y);
      gdisplays_flush ();
    }
  else 
    {
      g_message (_("Resize Error: Both width and height must be "
		   "greater than zero."));
    }

  gtk_widget_destroy (image_resize->resize->resize_shell);
}

static void
image_scale_callback (GtkWidget *widget,
		      gpointer   data)
{
  ImageResize *image_scale;

  image_scale = (ImageResize *) data;

  g_assert (image_scale != NULL);
  g_assert (image_scale->gimage != NULL);

  gtk_widget_set_sensitive (image_scale->resize->resize_shell, FALSE);

  if (gimp_image_check_scaling (image_scale->gimage,
				image_scale->resize->width,
				image_scale->resize->height))
    {
      image_scale_implement (image_scale);

      gtk_widget_destroy (image_scale->resize->resize_shell);
    }
  else
    {
      GtkWidget *dialog;

      dialog =
	gimp_query_boolean_box (_("Layer Too Small"),
				gimp_standard_help_func,
				"dialogs/scale_layer_warn.html",
				GTK_STOCK_DIALOG_QUESTION,
				_("The chosen image size will shrink\n"
				  "some layers completely away.\n"
				  "Is this what you want?"),
				GTK_STOCK_OK, GTK_STOCK_CANCEL,
				G_OBJECT (image_scale->resize->resize_shell),
				"destroy",
				image_scale_warn_callback,
				image_scale);
      gtk_widget_show (dialog);
    }
}

static void
image_scale_warn_callback (GtkWidget *widget,
			   gboolean   do_scale,
			   gpointer   data)
{
  ImageResize *image_scale;
  GimpImage   *gimage;

  image_scale = (ImageResize *) data;
  gimage      = image_scale->gimage;

  if (do_scale) /* User doesn't mind losing layers... */
    {
      image_scale_implement (image_scale);

      gtk_widget_destroy (image_scale->resize->resize_shell);
    }
  else
    {
      gtk_widget_set_sensitive (image_scale->resize->resize_shell, TRUE);
    }
}

static void
image_scale_implement (ImageResize *image_scale)
{
  GimpImage *gimage        = NULL;
  gboolean   display_flush = FALSE;  /* this is a bit ugly: 
					we hijack the flush variable 
					to check if an undo_group was 
					already started */

  g_assert (image_scale != NULL);
  g_assert (image_scale->gimage != NULL);

  gimage = image_scale->gimage;

  if (image_scale->resize->resolution_x != gimage->xresolution ||
      image_scale->resize->resolution_y != gimage->yresolution)
    {
      undo_push_group_start (gimage, IMAGE_SCALE_UNDO);
	  
      gimp_image_set_resolution (gimage,
				 image_scale->resize->resolution_x,
				 image_scale->resize->resolution_y);

      display_flush = TRUE;
    }

  if (image_scale->resize->unit != gimage->unit)
    {
      if (! display_flush)
	undo_push_group_start (gimage, IMAGE_SCALE_UNDO);

      gimp_image_set_unit (gimage, image_scale->resize->unit);

      display_flush = TRUE;
    }

  if (image_scale->resize->width != gimage->width ||
      image_scale->resize->height != gimage->height)
    {
      if (image_scale->resize->width > 0 &&
	  image_scale->resize->height > 0) 
	{
          GimpProgress *progress;

	  if (! display_flush)
	    undo_push_group_start (gimage, IMAGE_SCALE_UNDO);

          progress = progress_start (image_scale->gdisp,
                                     _("Scaling..."),
                                     TRUE, NULL, NULL);

	  gimp_image_scale (gimage,
			    image_scale->resize->width,
			    image_scale->resize->height,
                            image_scale->resize->interpolation,
                            progress_update_and_flush, progress);

          progress_end (progress);

	  display_flush = TRUE;
	}
      else
	{
	  g_message (_("Scale Error: Both width and height must be "
		       "greater than zero."));
	  return;
	}
    }

  if (display_flush)
    {
      undo_push_group_end (gimage);
      gdisplays_flush ();
    }
}
