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
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"

#include "widgets/gimpcolorpanel.h"

#include "drawable.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "global_edit.h"
#include "qmask.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


struct _EditQmaskOptions
{
  GtkWidget   *query_box;
  GtkWidget   *name_entry;
  GtkWidget   *color_panel;

  GimpImage   *gimage;
};

typedef struct _EditQmaskOptions EditQmaskOptions;

/*  Global variables  */
/*  Static variables  */
/*  Prototypes */
static void edit_qmask_channel_query         (GDisplay        *gdisp);
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

/* Actual code */

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
  GDisplay *gdisp = (GDisplay*) data;
  
  if (!gdisp->gimage)
    return;
  
  gdisp->gimage->qmask_state = FALSE;

  qmask_buttons_update (gdisp);
}


void
qmask_buttons_update (GDisplay *gdisp)
{
  g_assert (gdisp);
  g_assert (gdisp->gimage);

  if (gdisp->gimage->qmask_state != GTK_TOGGLE_BUTTON (gdisp->qmaskon)->active)
    {
      /* Disable toggle from doing anything */
      gtk_signal_handler_block_by_func(GTK_OBJECT(gdisp->qmaskoff), 
				       (GtkSignalFunc) qmask_deactivate,
				       gdisp);
      gtk_signal_handler_block_by_func(GTK_OBJECT(gdisp->qmaskon), 
				       (GtkSignalFunc) qmask_activate,
				       gdisp);
   
      /* Change the state of the buttons */
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gdisp->qmaskon), 
				   gdisp->gimage->qmask_state);

      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gdisp->qmaskoff),
				   !gdisp->gimage->qmask_state);
   
      /* Enable toggle again */
      gtk_signal_handler_unblock_by_func(GTK_OBJECT(gdisp->qmaskoff), 
					 (GtkSignalFunc) qmask_deactivate,
					 gdisp);
      gtk_signal_handler_unblock_by_func(GTK_OBJECT(gdisp->qmaskon), 
					 (GtkSignalFunc) qmask_activate,
					 gdisp);
    }
}

void 
qmask_click_handler (GtkWidget       *widget,
		     GdkEventButton  *event,
                     gpointer         data)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) data;

  if ((event->type == GDK_2BUTTON_PRESS) &&
      (event->button == 1))
    {
      edit_qmask_channel_query (gdisp); 
    }
}

void
qmask_deactivate (GtkWidget *widget,
		  GDisplay  *gdisp)
{
  GimpImage   *gimg;
  GimpChannel *gmask;

  if (gdisp)
    {
      gimg = gdisp->gimage;
      if (!gimg) 
	return;
      
      if (!gdisp->gimage->qmask_state)
	return; /* if already set do nothing */

      if ( (gmask = gimp_image_get_channel_by_name (gimg, "Qmask")) )
  	{ 
	  undo_push_group_start (gimg, QMASK_UNDO);
	  /*  push the undo here since removing the mask will
	      call the qmask_removed_callback() which will set
	      the qmask_state to FALSE  */
	  undo_push_qmask (gimg);
	  gimage_mask_load (gimg, gmask);
	  gimp_image_remove_channel (gimg, gmask);
	  undo_push_group_end (gimg);
	}

      gdisp->gimage->qmask_state = FALSE;

      if (gmask)
	gdisplays_flush ();
    }
}

void
qmask_activate (GtkWidget *widget,
		GDisplay  *gdisp)
{
  GimpImage   *gimg;
  GimpChannel *gmask;
  GimpLayer   *layer;
  GimpRGB      color;

  if (gdisp)
    {
      gimg = gdisp->gimage;
      if (!gimg) 
	return;

      if (gdisp->gimage->qmask_state)
	return; /* If already set, do nothing */
  
      /* Set the defaults */
      color = gimg->qmask_color;
 
      if ((gmask = gimp_image_get_channel_by_name (gimg, "Qmask"))) 
	{
	  gimg->qmask_state = TRUE; 
	  /* if the user was clever and created his own */
	  return; 
	}

      undo_push_group_start (gimg, QMASK_UNDO);

      if (gimage_mask_is_empty (gimg))
	{ 
	  /* if no selection */

	  if ((layer = gimp_image_floating_sel (gimg)))
	    {
	      floating_sel_to_layer (layer);
	    }

	  gmask = gimp_channel_new (gimg, 
				    gimg->width, 
				    gimg->height,
				    "Qmask",
				    &color);
	  gimp_image_add_channel (gimg, gmask, 0);
	  drawable_fill (GIMP_DRAWABLE (gmask), TRANSPARENT_FILL);
	}
      else
	{
	  /* if selection */

	  gmask = gimp_channel_copy (gimp_image_get_mask (gimg), TRUE);
	  gimp_image_add_channel (gimg, gmask, 0);
	  gimp_channel_set_color (gmask, &color);
	  gimp_object_set_name (GIMP_OBJECT (gmask), "Qmask");
	  gimage_mask_none (gimg);           /* Clear the selection */
	}

      undo_push_qmask (gimg);
      undo_push_group_end (gimg);
      gdisp->gimage->qmask_state = TRUE;
      gdisplays_flush ();

      /* connect to the removed signal, so the buttons get updated */
      gtk_signal_connect (GTK_OBJECT (gmask), "removed", 
			  GTK_SIGNAL_FUNC (qmask_removed_callback),
			  gdisp);
    }
}

static void
edit_qmask_channel_query (GDisplay * gdisp)
{
  EditQmaskOptions *options;
  GtkWidget        *hbox;
  GtkWidget        *vbox;
  GtkWidget        *table;
  GtkWidget        *label;
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

		     _("OK"), edit_qmask_query_ok_callback,
		     options, NULL, NULL, TRUE, FALSE,
		     _("Cancel"), edit_qmask_query_cancel_callback,
		     options, NULL, NULL, FALSE, TRUE,

		     NULL);

  /*  The main hbox  */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
                     hbox);
  /*  The vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

  /*  The table  */
  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  The opacity scale  */
  label = gtk_label_new (_("Mask Opacity:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  opacity_scale_data =
    gtk_adjustment_new (options->gimage->qmask_color.a * 100.0, 
			0.0, 100.0, 1.0, 1.0, 0.0);
  opacity_scale = gtk_hscale_new (GTK_ADJUSTMENT (opacity_scale_data));
  gtk_table_attach_defaults (GTK_TABLE (table), opacity_scale, 1, 2, 1, 2);
  gtk_scale_set_value_pos (GTK_SCALE (opacity_scale), GTK_POS_TOP);
  gtk_signal_connect (GTK_OBJECT (opacity_scale_data), "value_changed",
                      GTK_SIGNAL_FUNC (qmask_query_scale_update),
                      options->color_panel);
  gtk_widget_show (opacity_scale);

  /*  The color panel  */
  gtk_signal_connect (GTK_OBJECT (options->color_panel), "color_changed",
		      GTK_SIGNAL_FUNC (qmask_color_changed),
		      opacity_scale_data);		      
  gtk_box_pack_start (GTK_BOX (hbox), options->color_panel,
                      TRUE, TRUE, 0);
  gtk_widget_show (options->color_panel);

  gtk_widget_show (table);
  gtk_widget_show (vbox);
  gtk_widget_show (hbox);
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
	  channel->color = color;

	  drawable_update (GIMP_DRAWABLE (channel),
			   0, 0,
			   GIMP_DRAWABLE (channel)->width,
			   GIMP_DRAWABLE (channel)->height);

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
