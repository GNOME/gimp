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

#include "core/core-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-desaturate.h"
#include "core/gimpdrawable-equalize.h"
#include "core/gimpimage.h"
#include "core/gimpimage-duplicate.h"

#include "pdb/procedural_db.h"

#include "convert-dialog.h"
#include "offset-dialog.h"
#include "resize-dialog.h"

#include "gdisplay.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


#define return_if_no_display(gdisp) \
        gdisp = gdisplay_active (); \
        if (!gdisp) return


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
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  convert_to_rgb (gdisp->gimage);
}

void
image_convert_grayscale_cmd_callback (GtkWidget *widget,
				      gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  convert_to_grayscale (gdisp->gimage);
}

void
image_convert_indexed_cmd_callback (GtkWidget *widget,
				    gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  convert_to_indexed (gdisp->gimage);
}

void
image_desaturate_cmd_callback (GtkWidget *widget,
			       gpointer   data)
{
  GDisplay     *gdisp;
  GimpDrawable *drawable;

  return_if_no_display (gdisp);

  drawable = gimp_image_active_drawable (gdisp->gimage);

  if (! gimp_drawable_is_rgb (drawable))
    {
      g_message (_("Desaturate operates only on RGB color drawables."));
      return;
    }

  gimp_drawable_desaturate (drawable);

  gdisplays_flush ();
}

void
image_invert_cmd_callback (GtkWidget *widget,
			   gpointer   data)
{
  GDisplay     *gdisp;
  GimpDrawable *drawable;
  Argument     *return_vals;
  gint          nreturn_vals;

  return_if_no_display (gdisp);

  drawable = gimp_image_active_drawable (gdisp->gimage);

  if (gimp_drawable_is_indexed (drawable))
    {
      g_message (_("Invert does not operate on indexed drawables."));
      return;
    }

  return_vals =
    procedural_db_run_proc (gdisp->gimage->gimp,
			    "gimp_invert",
			    &nreturn_vals,
			    GIMP_PDB_DRAWABLE, gimp_drawable_get_ID (drawable),
			    GIMP_PDB_END);

  if (!return_vals || return_vals[0].value.pdb_int != GIMP_PDB_SUCCESS)
    g_message (_("Invert operation failed."));

  procedural_db_destroy_args (return_vals, nreturn_vals);

  gdisplays_flush ();
}

void
image_equalize_cmd_callback (GtkWidget *widget,
			     gpointer   data)
{
  GDisplay     *gdisp;
  GimpDrawable *drawable;

  return_if_no_display (gdisp);

  drawable = gimp_image_active_drawable (gdisp->gimage);

  if (gimp_drawable_is_indexed (drawable))
    {
      g_message (_("Equalize does not operate on indexed drawables."));
      return;
    }

  gimp_drawable_equalize (drawable, TRUE);

  gdisplays_flush ();
}

void
image_offset_cmd_callback (GtkWidget *widget,
			   gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  offset_dialog_create (gdisp->gimage);
}

void
image_resize_cmd_callback (GtkWidget *widget,
			   gpointer   data)
{
  GDisplay    *gdisp;
  GimpImage   *gimage;
  ImageResize *image_resize;

  return_if_no_display (gdisp);

  gimage = gdisp->gimage;

  image_resize = g_new0 (ImageResize, 1);

  image_resize->gimage = gimage;
  image_resize->resize = resize_widget_new (ResizeWidget,
					    ResizeImage,
					    G_OBJECT (gimage),
					    "destroy",
					    gimage->width,
					    gimage->height,
					    gimage->xresolution,
					    gimage->yresolution,
					    gimage->unit,
					    gdisp->dot_for_dot,
					    G_CALLBACK (image_resize_callback),
					    NULL,
					    image_resize);

  gtk_signal_connect_object (GTK_OBJECT (image_resize->resize->resize_shell),
			     "destroy",
			     GTK_SIGNAL_FUNC (g_free),
			     (GtkObject *) image_resize);

  gtk_widget_show (image_resize->resize->resize_shell);
}

void
image_scale_cmd_callback (GtkWidget *widget,
			  gpointer   data)
{
  GDisplay    *gdisp;
  GimpImage   *gimage;
  ImageResize *image_scale;

  return_if_no_display (gdisp);

  gimage = gdisp->gimage;

  image_scale = g_new0 (ImageResize, 1);

  image_scale->gimage = gimage;
  image_scale->resize = resize_widget_new (ScaleWidget,
					   ResizeImage,
					   G_OBJECT (gimage),
					   "destroy",
					   gimage->width,
					   gimage->height,
					   gimage->xresolution,
					   gimage->yresolution,
					   gimage->unit,
					   gdisp->dot_for_dot,
					   G_CALLBACK (image_scale_callback),
					   NULL,
					   image_scale);

  gtk_signal_connect_object (GTK_OBJECT (image_scale->resize->resize_shell),
			     "destroy",
			     GTK_SIGNAL_FUNC (g_free),
			     (GtkObject *) image_scale);

  gtk_widget_show (image_scale->resize->resize_shell);
}

void
image_duplicate_cmd_callback (GtkWidget *widget,
			      gpointer   data)
{
  GDisplay  *gdisp;
  GimpImage *gimage;

  return_if_no_display (gdisp);

  gimage = gimp_image_duplicate (gdisp->gimage);

  gimp_create_display (gimage->gimp, gimage);
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
				FALSE,
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
  gboolean   rulers_flush  = FALSE;
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

      rulers_flush = TRUE;
      display_flush = TRUE;
    }

  if (image_scale->resize->unit != gimage->unit)
    {
      if (!display_flush)
	undo_push_group_start (gimage, IMAGE_SCALE_UNDO);

      gimp_image_set_unit (gimage, image_scale->resize->unit);
      gdisplays_setup_scale (gimage);
      gdisplays_resize_cursor_label (gimage);

      rulers_flush = TRUE;
      display_flush = TRUE;
    }

  if (image_scale->resize->width != gimage->width ||
      image_scale->resize->height != gimage->height)
    {
      if (image_scale->resize->width > 0 &&
	  image_scale->resize->height > 0) 
	{
	  if (!display_flush)
	    undo_push_group_start (gimage, IMAGE_SCALE_UNDO);

	  gimp_image_scale (gimage,
			    image_scale->resize->width,
			    image_scale->resize->height);

	  display_flush = TRUE;
	}
      else
	{
	  g_message (_("Scale Error: Both width and height must be "
		       "greater than zero."));
	  return;
	}
    }

  if (rulers_flush)
    {
      gdisplays_setup_scale (gimage);
      gdisplays_resize_cursor_label (gimage);
    }
      
  if (display_flush)
    {
      undo_push_group_end (gimage);
      gdisplays_flush ();
    }
}
