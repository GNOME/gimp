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
#include <stdlib.h>
#include <string.h>
#include "appenv.h"
#include "colormaps.h"
#include "cursorutil.h"
#include "fileops.h"
#include "gdisplay_ops.h"
#include "gimage.h"
#include "gimpui.h"
#include "gximage.h"
#include "menus.h"
#include "scale.h"
#include "gimprc.h"

#include "config.h"
#include "libgimp/gimpintl.h"

static void gdisplay_close_warning_callback  (GtkWidget *, gpointer);
static void gdisplay_cancel_warning_callback (GtkWidget *, gpointer);
static void gdisplay_close_warning_dialog    (char *, GDisplay *);

static GtkWidget *warning_dialog = NULL;

/*
 *  This file is for operations on the gdisplay object
 */

gulong
gdisplay_white_pixel (GDisplay *gdisp)
{
  return g_white_pixel;
}

gulong
gdisplay_gray_pixel (GDisplay *gdisp)
{
  return g_gray_pixel;
}

gulong
gdisplay_black_pixel (GDisplay *gdisp)
{
  return g_black_pixel;
}

gulong
gdisplay_color_pixel (GDisplay *gdisp)
{
  return g_color_pixel;
}


void
gdisplay_xserver_resolution (gdouble *xres,
			     gdouble *yres)
{
  gint width, height;
  gint widthMM, heightMM;

  width  = gdk_screen_width ();
  height = gdk_screen_height ();

  widthMM  = gdk_screen_width_mm ();
  heightMM = gdk_screen_height_mm ();

  /*
   * From xdpyinfo.c:
   *
   * there are 2.54 centimeters to an inch; so there are 25.4 millimeters.
   *
   *     dpi = N pixels / (M millimeters / (25.4 millimeters / 1 inch))
   *         = N pixels / (M inch / 25.4)
   *         = N * 25.4 pixels / M inch
   */

  *xres = (width  * 25.4) / ((gdouble) widthMM);
  *yres = (height * 25.4) / ((gdouble) heightMM);
}


void
gdisplay_new_view (GDisplay *gdisp)
{
  GDisplay *new_gdisp;

  /* make sure the image has been fully loaded... */
  if (gdisp->gimage)
    {
      new_gdisp = gdisplay_new (gdisp->gimage, gdisp->scale);
    }
}


void
gdisplay_close_window (GDisplay *gdisp,
		       int       kill_it)
{
  /*  If the image has been modified, give the user a chance to save
   *  it before nuking it--this only applies if its the last view
   *  to an image canvas.  (a gimage with disp_count = 1)
   */
  if (!kill_it && (gdisp->gimage->disp_count == 1) &&
      gdisp->gimage->dirty && confirm_on_close)
    {
      gdisplay_close_warning_dialog
	(g_basename (gimage_filename (gdisp->gimage)), gdisp);
    }
  else
    {
      gtk_widget_destroy (gdisp->shell);
    }
}


void
gdisplay_shrink_wrap (GDisplay *gdisp)
{
  /* FIXME: Still not perfect - have to hide and show the window to 
   * get it to work.  Adding a queue_resize to the shell doesn't 
   * seem to help. Anyone have any good ideas here? 
   */
  gint x, y;
  gint disp_width, disp_height;
  gint width, height;
  gint shell_x, shell_y;
  gint shell_width, shell_height;
  gint max_auto_width, max_auto_height;
  gint border_x, border_y;
  int s_width, s_height;

  s_width = gdk_screen_width ();
  s_height = gdk_screen_height ();

  width = SCALEX (gdisp, gdisp->gimage->width);
  height = SCALEY (gdisp, gdisp->gimage->height);

  disp_width = gdisp->disp_width;
  disp_height = gdisp->disp_height;

  shell_width = gdisp->shell->allocation.width;
  shell_height = gdisp->shell->allocation.height;

  border_x = shell_width - disp_width;
  border_y = shell_height - disp_height;

  max_auto_width = (s_width - border_x) * 0.75;
  max_auto_height = (s_height - border_y) * 0.75;

  /*  If 1) the projected width & height are smaller than screen size, &
   *     2) the current display size isn't already the desired size, expand
   */
  if (((width + border_x) < s_width || (height + border_y) < s_height) &&
      (width != disp_width || height != disp_height))
    {
      width = ((width + border_x) < s_width) ? width : max_auto_width;
      height = ((height + border_y) < s_height) ? height : max_auto_height;

      if (width < gdisp->statusarea->requisition.width) 
        { 
          width = gdisp->statusarea->requisition.width; 
        }
      /* I don't know why, but if I don't hide the window, it doesn't work */
      gtk_widget_hide(gdisp->shell); 
      gtk_drawing_area_size(GTK_DRAWING_AREA(gdisp->canvas),
	                    width, height);
      gtk_widget_show(gdisp->shell); 

      /*printf("1w:%d/%d d:%d/%d s:%d/%d b:%d/%d\n",
	     width, height,
	     disp_width, disp_height,
	     shell_width, shell_height,
	     border_x, border_y);fflush(stdout);*/

      gdk_window_get_origin (gdisp->shell->window, &shell_x, &shell_y);

      shell_width = width + border_x;
      shell_height = height + border_y;

      x = MIN (shell_x, CLAMP (s_width - shell_width, border_x, s_width));
      y = MIN (shell_y, CLAMP (s_height - shell_height, border_y, s_height));

      if (x != shell_x || y != shell_y)
	gdk_window_move (gdisp->shell->window, x, y);

      /*  Set the new disp_width and disp_height values  */
      gdisp->disp_width = width;
      gdisp->disp_height = height;
    }
  /*  If the projected width is greater than current, but less than
   *  3/4 of the screen size, expand automagically
   */
  else if ((width > disp_width || height > disp_height) &&
	   (disp_width < max_auto_width || disp_height < max_auto_height))
    {
      max_auto_width = MIN (max_auto_width, width);
      max_auto_height = MIN (max_auto_height, height);
      
      if (width < gdisp->statusarea->requisition.width) 
        { 
          width = gdisp->statusarea->requisition.width; 
        }

      /* I don't know why, but if I don't hide the window, it doesn't work */
      gtk_widget_hide(gdisp->shell); 
      gtk_drawing_area_size(GTK_DRAWING_AREA(gdisp->canvas),
	                    width, height);
      gtk_widget_show(gdisp->shell); 

      /*printf("2w:%d/%d d:%d/%d s:%d/%d b:%d/%d\n",
	     width, height,
	     disp_width, disp_height,
	     shell_width, shell_height,
	     border_x, border_y);fflush(stdout);*/

      gdk_window_get_origin (gdisp->shell->window, &shell_x, &shell_y);

      shell_width = width + border_x;
      shell_height = height + border_y;

      x = MIN (shell_x, CLAMP (s_width - shell_width, border_x, s_width));
      y = MIN (shell_y, CLAMP (s_height - shell_height, border_y, s_height));

      if (x != shell_x || y != shell_y)
	gdk_window_move (gdisp->shell->window, x, y);

      /*  Set the new disp_width and disp_height values  */
      gdisp->disp_width = max_auto_width;
      gdisp->disp_height = max_auto_height;
    }
  /*  Otherwise, reexpose by hand to reflect changes  */
  else
    gdisplay_expose_full (gdisp);

  /*  If the width or height of the display has changed, recalculate
   *  the display offsets...
   */
  if (disp_width != gdisp->disp_width ||
      disp_height != gdisp->disp_height)
    {
      gdisp->offset_x += (disp_width - gdisp->disp_width) / 2;
      gdisp->offset_y += (disp_height - gdisp->disp_height) / 2;
      bounds_checking (gdisp);
    }
}


int
gdisplay_resize_image (GDisplay *gdisp)
{
  int sx, sy;
  int width, height;

  /*  Calculate the width and height of the new canvas  */
  sx = SCALEX (gdisp, gdisp->gimage->width);
  sy = SCALEY (gdisp, gdisp->gimage->height);
  width = MIN (sx, gdisp->disp_width);
  height = MIN (sy, gdisp->disp_height);

  /* if the new dimensions of the ximage are different than the old...resize */
  if (width != gdisp->disp_width || height != gdisp->disp_height)
    {
      /*  adjust the gdisplay offsets -- we need to set them so that the
       *  center of our viewport is at the center of the image.
       */
      gdisp->offset_x = (sx / 2) - (width / 2);
      gdisp->offset_y = (sy / 2) - (height / 2);

      gdisp->disp_width = width;
      gdisp->disp_height = height;

      if (GTK_WIDGET_VISIBLE (gdisp->canvas))
	gtk_widget_hide (gdisp->canvas);

      gtk_widget_set_usize (gdisp->canvas,
			    gdisp->disp_width,
			    gdisp->disp_height);

      gtk_widget_show (gdisp->canvas);
    }

  return 1;
}


/********************************************************
 *   Routines to query before closing a dirty image     *
 ********************************************************/

static void
gdisplay_close_warning_callback (GtkWidget *widget,
				 gpointer   client_data)
{
  GDisplay *gdisp;
  GtkWidget *mbox;

  menus_set_sensitive ("<Image>/File/Close", TRUE);
  mbox = (GtkWidget *) client_data;
  gdisp = (GDisplay *) gtk_object_get_user_data (GTK_OBJECT (mbox));

  gtk_widget_destroy (gdisp->shell);
  gtk_widget_destroy (mbox);
}


static void
gdisplay_cancel_warning_callback (GtkWidget *widget,
				  gpointer   client_data)
{
  GtkWidget *mbox;

  menus_set_sensitive ("<Image>/File/Close", TRUE);
  mbox = (GtkWidget *) client_data;
  gtk_widget_destroy (mbox);
}

static void
gdisplay_destroy_warning_callback (GtkWidget *widget,
				   gpointer   client_data)
{
  warning_dialog = NULL;
}

static void
gdisplay_close_warning_dialog (char     *image_name,
			       GDisplay *gdisp)
{
  GtkWidget *mbox;
  GtkWidget *vbox;
  GtkWidget *label;
  gchar *warning_buf;

  /* FIXUP this will raise any prexsisting close dialogs, which can be a
     a bit confusing if you tried to close a new window because you had
     forgotten the old dialog was still around */
  /* If a warning dialog already exists raise the window and get out */
  if (warning_dialog != NULL)
    {
      gdk_window_raise (warning_dialog->window);
      return;
    }

  menus_set_sensitive ("<Image>/File/Close", FALSE);

  warning_dialog = mbox =
    gimp_dialog_new (image_name, "really_close",
		     gimp_standard_help_func,
		     "dialogs/really_close.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, FALSE, FALSE,

		     _("Close"), gdisplay_close_warning_callback,
		     NULL, NULL, NULL, FALSE, FALSE,
		     _("Cancel"), gdisplay_cancel_warning_callback,
		     NULL, NULL, NULL, TRUE, TRUE,

		     NULL);

  gtk_object_set_user_data (GTK_OBJECT (mbox), gdisp);

  gtk_signal_connect (GTK_OBJECT (mbox), "destroy",
		      GTK_SIGNAL_FUNC (gdisplay_destroy_warning_callback),
		      mbox);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (mbox)->vbox), vbox);
  gtk_widget_show (vbox);

  warning_buf =
    g_strdup_printf (_("Changes were made to %s.\n"
		       "Close anyway?"), image_name);
  label = gtk_label_new (warning_buf);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, FALSE, 0);
  gtk_widget_show (label);
  g_free (warning_buf);

  gtk_widget_show (mbox);
}
