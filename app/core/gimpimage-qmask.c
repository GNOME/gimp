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

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"

#include "widgets/gimpcolorpanel.h"

#include "gimpdisplay.h"
#include "gimpdisplay-foreach.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-qmask.h"

#include "floating_sel.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


typedef struct _EditQmaskOptions EditQmaskOptions;

struct _EditQmaskOptions
{
  GtkWidget *query_box;
  GtkWidget *name_entry;
  GtkWidget *color_panel;

  GimpImage *gimage;
};


/*  local function prototypes  */

static void edit_qmask_channel_query         (GimpDisplay     *gdisp);
static void edit_qmask_query_ok_callback     (GtkWidget       *widget, 
                                              gpointer         client_data);
static void edit_qmask_query_cancel_callback (GtkWidget       *widget, 
                                              gpointer         client_data);
static void qmask_query_scale_update         (GtkAdjustment   *adjustment,
                                              gpointer         data);
static void qmask_color_changed              (GimpColorButton *button,
					      gpointer         data);
static void qmask_removed_callback           (GtkObject       *qmask, 
					      gpointer         data);

/*  public functions  */

gboolean
gimp_display_shell_qmask_button_press (GtkWidget        *widget,
                                       GdkEventButton   *event,
                                       GimpDisplayShell *shell)
{
  GimpDisplay *gdisp;

  gdisp = shell->gdisp;

  if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1))
    {
      edit_qmask_channel_query (gdisp); 

      return TRUE;
    }

  return FALSE;
}

void
gimp_display_shell_qmask_off_toggled (GtkWidget        *widget,
                                      GimpDisplayShell *shell)
{
  GimpDisplay *gdisp;
  GimpImage   *gimage;
  GimpChannel *mask;

  gdisp = shell->gdisp;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      gimage = gdisp->gimage;

      if (! gimp_image_get_qmask_state (gimage))
        return; /* if already set do nothing */

      mask = gimp_image_get_channel_by_name (gimage, "Qmask");

      if (mask)
  	{ 
	  undo_push_group_start (gimage, QMASK_UNDO);
	  /*  push the undo here since removing the mask will
	   *  call the qmask_removed_callback() which will set
	   *  the qmask_state to FALSE
           */
	  undo_push_qmask (gimage);
	  gimp_image_mask_load (gimage, mask);
	  gimp_image_remove_channel (gimage, mask);
	  undo_push_group_end (gimage);
	}

      gimp_image_set_qmask_state (gdisp->gimage, FALSE);

      if (mask)
	gdisplays_flush ();
    }
}

void
gimp_display_shell_qmask_on_toggled (GtkWidget        *widget,
                                     GimpDisplayShell *shell)
{
  GimpDisplay *gdisp;
  GimpImage   *gimage;
  GimpChannel *mask;
  GimpLayer   *layer;
  GimpRGB      color;

  gdisp = shell->gdisp;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      gimage = gdisp->gimage;

      if (gimp_image_get_qmask_state (gimage))
	return; /* if already set, do nothing */
  
      /* Set the defaults */
      color = gimage->qmask_color;

      mask = gimp_image_get_channel_by_name (gimage, "Qmask");

      if (mask)
	{
	  /* if the user was clever and created his own */

	  gimp_image_set_qmask_state (gimage, TRUE);

	  return; 
	}

      undo_push_group_start (gimage, QMASK_UNDO);

      if (gimp_image_mask_is_empty (gimage))
	{ 
	  /* if no selection */

	  if ((layer = gimp_image_floating_sel (gimage)))
	    {
	      floating_sel_to_layer (layer);
	    }

	  mask = gimp_channel_new (gimage, 
                                   gimage->width, 
                                   gimage->height,
                                   "Qmask",
                                   &color);
	  gimp_image_add_channel (gimage, mask, 0);

	  gimp_drawable_fill_by_type (GIMP_DRAWABLE (mask),
				      gimp_get_user_context (gimage->gimp),
				      TRANSPARENT_FILL);
	}
      else /* if selection */
	{
	  mask = gimp_channel_copy (gimp_image_get_mask (gimage), TRUE);
	  gimp_image_add_channel (gimage, mask, 0);
	  gimp_channel_set_color (mask, &color);
	  gimp_object_set_name (GIMP_OBJECT (mask), "Qmask");
	  gimp_image_mask_none (gimage);           /* Clear the selection */
	}

      undo_push_qmask (gimage);
      undo_push_group_end (gimage);

      gimp_image_set_qmask_state (gimage, TRUE);

      gdisplays_flush ();

      /* connect to the removed signal, so the buttons get updated */
      g_signal_connect (G_OBJECT (mask), "removed", 
			G_CALLBACK (qmask_removed_callback),
			gdisp);
    }
}


/*  private functions  */

static void
edit_qmask_channel_query (GimpDisplay *gdisp)
{
  EditQmaskOptions *options;
  GtkWidget        *hbox;
  GtkWidget        *vbox;
  GtkWidget        *table;
  GtkWidget        *opacity_scale;
  GtkObject        *opacity_scale_data;

  /*  the new options structure  */
  options = g_new0 (EditQmaskOptions, 1);

  options->gimage      = gdisp->gimage;
  options->color_panel = gimp_color_panel_new (_("Edit Qmask Color"),
					       &options->gimage->qmask_color,
					       GIMP_COLOR_AREA_LARGE_CHECKS, 
					       48, 64);

  /*  The dialog  */
  options->query_box =
    gimp_dialog_new (_("Edit Qmask Attributes"), "edit_qmask_attributes",
		     gimp_standard_help_func,
		     "dialogs/edit_qmask_attributes.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     GTK_STOCK_CANCEL, edit_qmask_query_cancel_callback,
		     options, NULL, NULL, FALSE, TRUE,

		     GTK_STOCK_OK, edit_qmask_query_ok_callback,
		     options, NULL, NULL, TRUE, FALSE,

		     NULL);

  /*  The main hbox  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
                     hbox);
  gtk_widget_show (hbox);

  /*  The vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /*  The table  */
  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  The opacity scale  */
  opacity_scale_data =
    gtk_adjustment_new (options->gimage->qmask_color.a * 100.0, 
			0.0, 100.0, 1.0, 1.0, 0.0);
  opacity_scale = gtk_hscale_new (GTK_ADJUSTMENT (opacity_scale_data));
  gtk_widget_set_usize (opacity_scale, 100, -1);
  gtk_scale_set_value_pos (GTK_SCALE (opacity_scale), GTK_POS_TOP);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Mask Opacity:"), 1.0, 1.0,
			     opacity_scale, 1, FALSE);

  g_signal_connect (G_OBJECT (opacity_scale_data), "value_changed",
		    G_CALLBACK (qmask_query_scale_update),
		    options->color_panel);

  /*  The color panel  */
  gtk_box_pack_start (GTK_BOX (hbox), options->color_panel,
                      TRUE, TRUE, 0);
  gtk_widget_show (options->color_panel);

  g_signal_connect (G_OBJECT (options->color_panel), "color_changed",
		    G_CALLBACK (qmask_color_changed),
		    opacity_scale_data);		      

  gtk_widget_show (options->query_box);
}

static void 
edit_qmask_query_ok_callback (GtkWidget *widget, 
			      gpointer   data) 
{
  EditQmaskOptions *options;
  GimpChannel      *channel;
  GimpRGB           color;

  options = (EditQmaskOptions *) data;

  channel = gimp_image_get_channel_by_name (options->gimage, "Qmask");

  if (options->gimage && channel)
    {
      gimp_color_button_get_color (GIMP_COLOR_BUTTON (options->color_panel),
				   &color);

      if (gimp_rgba_distance (&color, &channel->color) > 0.0001)
	{
	  gimp_channel_set_color (channel, &color);

	  gdisplays_flush ();
	}
    }

  /* update the qmask color no matter what */
  options->gimage->qmask_color = color;

  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void
edit_qmask_query_cancel_callback (GtkWidget *widget,
				  gpointer   data)
{
  EditQmaskOptions *options;

  options = (EditQmaskOptions *) data;

  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void 
qmask_query_scale_update (GtkAdjustment *adjustment, 
			  gpointer       data)
{
  GimpRGB  color;

  gimp_color_button_get_color (GIMP_COLOR_BUTTON (data), &color);
  gimp_rgb_set_alpha (&color, adjustment->value / 100.0);
  gimp_color_button_set_color (GIMP_COLOR_BUTTON (data), &color);
}

static void
qmask_color_changed (GimpColorButton *button,
		     gpointer         data)
{
  GtkAdjustment *adj = GTK_ADJUSTMENT (data);
  GimpRGB        color;

  gimp_color_button_get_color (button, &color);
  gtk_adjustment_set_value (adj, color.a * 100.0);
}

static void
qmask_removed_callback (GtkObject *qmask,
			gpointer   data)
{
  GimpDisplay *gdisp = (GimpDisplay *) data;
  
  if (!gdisp->gimage)
    return;

  gimp_image_set_qmask_state (gdisp->gimage, FALSE);
}
