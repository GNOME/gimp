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
#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libgimp/gimpintl.h"
#include "actionarea.h"
#include "appenv.h" 
#include "channel.h"
#include "color_panel.h"
#include "floating_sel.h"
#include "gdisplayF.h"
#include "gimpcontext.h"
#include "gimage_mask.h"
#include "gimpimage.h"
#include "global_edit.h"
#include "qmask.h"
#include "undo.h"


struct _EditQmaskOptions
{
  GtkWidget *query_box;
  GtkWidget *name_entry;

  GimpImage     *gimage;
  ColorPanel    *color_panel;
  double         opacity;
};

typedef struct _EditQmaskOptions EditQmaskOptions;

/*  Global variables  */
/*  Static variables  */
/*  Prototypes */
static void edit_qmask_channel_query         (GDisplay   *gdisp);
static void edit_qmask_query_ok_callback     (GtkWidget     *widget, 
                                              gpointer       client_data);
static void edit_qmask_query_cancel_callback (GtkWidget     *widget, 
                                              gpointer       client_data);
static void qmask_query_scale_update         (GtkAdjustment *adjustment,
                                              gdouble       *scale_val);
static gint qmask_query_delete_callback      (GtkWidget   *widget,
                                              GdkEvent      *event,
                                              gpointer       client_data);


/* Actual code */
static gint 
qmask_query_delete_callback (GtkWidget   *widget,
                             GdkEvent    *event,
                             gpointer     client_data)
{
  edit_qmask_query_cancel_callback (widget, client_data);

  return TRUE;
}


static void 
qmask_query_scale_update (GtkAdjustment *adjustment, gdouble *scale_val)
{
  *scale_val = adjustment->value;
}

void
qmask_buttons_update (GDisplay *gdisp)
{
g_assert(gdisp);

g_assert(gdisp->gimage);

if (gdisp->gimage->qmask_state != GTK_TOGGLE_BUTTON(gdisp->qmaskon)->active)
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
/* Flush event queue */
/*  while (g_main_iteration(FALSE)); */
  }
}


void 
qmask_click_handler (GtkWidget       *widget,
		     GdkEventButton  *event,
                     gpointer         data)
{
  GDisplay *gdisp;
  gdisp = (GDisplay *)data;

  if ((event->type == GDK_2BUTTON_PRESS) &&
      (event->button == 1))
  {
   edit_qmask_channel_query(gdisp); 
  }
}


void
qmask_deactivate(GtkWidget *w,
	    GDisplay *gdisp)
{
GimpImage *gimg;
GimpChannel *gmask;

if (gdisp)
  {
  gimg = gdisp->gimage;
  if (!gimg) return;

  if (!gdisp->gimage->qmask_state) {
	return; /* if already set do nothing */
	}

  undo_push_group_start (gimg, QMASK_UNDO);
  
  if ( (gmask = gimp_image_get_channel_by_name (gimg, "Qmask")) )
  	{ 
	gimage_mask_load (gimg, gmask);
	gimage_remove_channel(gimg, gmask);
	undo_push_qmask(gimg,1);
  	gdisp->gimage->qmask_state = 0;
        gdisplays_flush (); 
	}
  else
        gdisp->gimage->qmask_state = 0;
 
  undo_push_group_end (gimg);
  }
}

void
qmask_activate(GtkWidget *w,
           GDisplay *gdisp)
{
GimpImage *gimg;
GimpChannel *gmask;
GimpLayer *layer;

double opacity;
unsigned char *color;

if (gdisp)
  {
  gimg = gdisp->gimage;
  if (!gimg) return;

  if (gdisp->gimage->qmask_state) {
	return; /* If already set, do nothing */
	}
  
  /* Set the defaults */
  opacity = (double) gimg->qmask_opacity;
  color = gimg->qmask_color;
 
  if ( (gmask = gimp_image_get_channel_by_name (gimg, "Qmask")) ) {
    gimg->qmask_state = 1; /* if the user was clever and created his own */
    return; 
    }

  undo_push_group_start (gimg, QMASK_UNDO);
  if (gimage_mask_is_empty(gimg))
    { 
    if ((layer = gimage_floating_sel (gimg)))
      {
      floating_sel_to_layer (layer);
      }
   /* if no selection */
    gmask = channel_new(gimg, 
                        gimg->width, 
                        gimg->height,
                        "Qmask", 
    	                (int)(255*opacity)/100,
                        color);
    gimp_image_add_channel (gimg, gmask, 0);
    gimp_drawable_fill (GIMP_DRAWABLE(gmask), 0, 0, 0, 0);
    /* edit_clear(gimg,GIMP_DRAWABLE(gmask));  */
    undo_push_qmask(gimg,0);
    gdisp->gimage->qmask_state = 1;
    gdisplays_flush();  
    }
  else 
    { /* if selection */
    gmask = channel_copy (gimage_get_mask (gimg));
    gimp_image_add_channel (gimg, gmask, 0);
    channel_set_color(gmask, color);
    channel_set_name(gmask, "Qmask");
    channel_set_opacity(gmask, opacity);
    gimage_mask_none (gimg);           /* Clear the selection */
    undo_push_qmask(gimg,0);
    gdisp->gimage->qmask_state = 1;
    gdisplays_flush();
    }
  undo_push_group_end(gimg);
  }
}

static void
edit_qmask_channel_query (GDisplay * gdisp)
{
  EditQmaskOptions *options;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *opacity_scale;
  GtkObject *opacity_scale_data;
 
  gint i;
  guchar  channel_color[3] = { 0, 0, 0 };

  static ActionAreaItem action_items[] =
  {
    { N_("OK"), edit_qmask_query_ok_callback, NULL, NULL },
    { N_("Cancel"), edit_qmask_query_cancel_callback, NULL, NULL }
  };

  /* channel = gimp_image_get_channel_by_name (gdisp->gimage, "Qmask"); */
  /*  the new options structure  */
  options = g_new (EditQmaskOptions, 1);
  options->gimage  = gdisp->gimage;
  options->opacity = (gdouble) options->gimage->qmask_opacity;

  for (i = 0; i < 3; i++)
    channel_color[i] = options->gimage->qmask_color[i];

  options->color_panel = color_panel_new (channel_color, 48, 64);

  /*  The dialog  */
  options->query_box = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (options->query_box),
                          "edit_qmask_atributes", "Gimp");
  gtk_window_set_title (GTK_WINDOW (options->query_box),
                        _("Edit Qmask Attributes"));
  gtk_window_position (GTK_WINDOW (options->query_box), GTK_WIN_POS_MOUSE);

  /*  Handle the wm close signal  */
  gtk_signal_connect (GTK_OBJECT (options->query_box), "delete_event",
                      GTK_SIGNAL_FUNC (qmask_query_delete_callback),
                      options);

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
    gtk_adjustment_new (options->opacity, 0.0, 100.0, 1.0, 1.0, 0.0);
  opacity_scale = gtk_hscale_new (GTK_ADJUSTMENT (opacity_scale_data));
  gtk_table_attach_defaults (GTK_TABLE (table), opacity_scale, 1, 2, 1, 2);
  gtk_scale_set_value_pos (GTK_SCALE (opacity_scale), GTK_POS_TOP);
  gtk_signal_connect (GTK_OBJECT (opacity_scale_data), "value_changed",
                      (GtkSignalFunc) qmask_query_scale_update,
                      &options->opacity);
  gtk_widget_show (opacity_scale);

  /*  The color panel  */
  gtk_box_pack_start (GTK_BOX (hbox), options->color_panel->color_panel_widget,
                      TRUE, TRUE, 0);
  gtk_widget_show (options->color_panel->color_panel_widget);

  action_items[0].user_data = options;
  action_items[1].user_data = options;
  build_action_area (GTK_DIALOG (options->query_box), action_items, 2, 0);

  gtk_widget_show (table);
  gtk_widget_show (vbox);
  gtk_widget_show (hbox);
  gtk_widget_show (options->query_box);
}

static void edit_qmask_query_ok_callback (GtkWidget *widget, 
                                          gpointer client_data) 
{
  EditQmaskOptions *options;
  Channel *channel;
  gint opacity;
  gint update = FALSE;
  guchar *tmpcolp;
  guchar tmpcol[3];
  gint i;
  
  options = (EditQmaskOptions *) client_data;
  channel = gimp_image_get_channel_by_name(options->gimage, "Qmask");
  opacity = (int) (255* options->opacity/100);

  if (options->gimage && channel)
    {     /* don't update if opacity hasn't changed */
      if (channel_get_opacity(channel) != opacity)
        {
          update = TRUE;
        }
      tmpcolp = channel_get_color(channel);
      for (i = 0; i < 3; i++)
	{  /* don't update if color hasn't changed */
 	tmpcol[i] = tmpcolp[i]; /* initialize to same values */
        if (options->color_panel->color[i] != tmpcolp[i])
          {
            tmpcol[i] = options->color_panel->color[i];
            update = TRUE;
          }
        }

      if (update)
      { 
        channel_set_opacity(channel, 100*opacity/255);
        channel_set_color(channel,tmpcol);
	channel_update (channel);
      }
    }
  /* update the qmask color no matter what */
  for (i = 0; i < 3; i++)
  { /* TODO: should really have accessor functions for gimage private stuff */
    options->gimage->qmask_color[i] = options->color_panel->color[i];
    options->gimage->qmask_opacity = (gint) 100*opacity/255;
  }

  color_panel_free (options->color_panel);
  gtk_widget_destroy (options->query_box);
  g_free (options);
}

static void edit_qmask_query_cancel_callback (GtkWidget *widget, gpointer client_data)
{
  EditQmaskOptions *options;

  options = (EditQmaskOptions *) client_data;

  color_panel_free (options->color_panel);
  gtk_widget_destroy (options->query_box);
  g_free (options);
}
