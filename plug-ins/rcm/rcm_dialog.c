/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Colormap-Rotation plug-in. Exchanges two color ranges.
 *
 * Copyright (C) 1999 Sven Anders (anderss@fmi.uni-passau.de)
 *                    Based on code from Pavel Grinfeld (pavel@ml.com)
 *
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

/*---------------------------------------------------------------------------
 * Change log:
 * 
 * Version 2.0, 04 April 1999.
 *  Nearly complete rewrite, made plug-in stable.
 *  (Works with GIMP 1.1 and GTK+ 1.2)
 *
 * Version 1.0, 27 March 1997.
 *  Initial (unstable) release by Pavel Grinfeld
 *
 *---------------------------------------------------------------------------*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "rcm.h"
#include "rcm_misc.h"
#include "rcm_pixmaps.h"
#include "rcm_gdk.h"
#include "rcm_callback.h"
#include "rcm_dialog.h"

/*---------------------------------------------------------------------------*/
/* Defines */
/*---------------------------------------------------------------------------*/

#define INITIAL_ALPHA      0
#define INITIAL_BETA       (PI/2.0)
#define INITIAL_GRAY_SAT   0.0
#define INITIAL_GRAY_RSAT  0.0
#define INITIAL_GRAY_HUE   0.0

#define RANGE_ADJUST_MASK GDK_EXPOSURE_MASK | \
                          GDK_ENTER_NOTIFY_MASK | \
                          GDK_BUTTON_PRESS_MASK | \
                          GDK_BUTTON_RELEASE_MASK | \
                          GDK_BUTTON1_MOTION_MASK | \
                          GDK_POINTER_MOTION_HINT_MASK

/*---------------------------------------------------------------------------*/
/* Previews: create one preview */
/*---------------------------------------------------------------------------*/

void 
rcm_create_one_preview (GtkWidget **preview, 
			GtkWidget **frame,
			int         previewWidth, 
			int         previewHeight)
{
  *frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(*frame), GTK_SHADOW_IN);
  gtk_container_border_width(GTK_CONTAINER(*frame), 0);
  gtk_widget_show(*frame);
  
  *preview = gtk_preview_new(GTK_PREVIEW_COLOR);

  gtk_preview_size(GTK_PREVIEW(*preview), previewWidth, previewHeight);
  gtk_widget_show(*preview);
  gtk_container_add(GTK_CONTAINER(*frame), *preview);
}

/*---------------------------------------------------------------------------*/
/* Previews */
/*---------------------------------------------------------------------------*/

GtkWidget* 
rcm_create_previews (void)
{
  GtkWidget *frame, *blabel, *alabel, *bframe, *aframe, *table;

  /* Previews: create the previews */
  
  rcm_create_one_preview (&Current.Bna->before, &bframe, 
			  Current.reduced->width, Current.reduced->height);

  rcm_create_one_preview (&Current.Bna->after, &aframe, 
			  Current.reduced->width, Current.reduced->height);

  /* Previews: frame */
  frame = gtk_frame_new (_("Preview"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);

  alabel = gtk_label_new (_("Rotated"));
  gtk_widget_show (alabel);
  
  blabel = gtk_label_new (_("Original"));
  gtk_widget_show (blabel);

  table = gtk_table_new (4, 1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
 
  gtk_container_add (GTK_CONTAINER (frame), table);
  
  gtk_table_attach (GTK_TABLE (table), blabel, 0, 1, 0, 1, 
		    GTK_EXPAND, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), bframe, 0, 1, 1, 2,
		    GTK_EXPAND, GTK_EXPAND, 0, 0);

  gtk_table_attach (GTK_TABLE (table), alabel, 0, 1, 2, 3,
		    GTK_EXPAND, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), aframe, 0, 1, 3, 4,
		    GTK_EXPAND, GTK_EXPAND, 0, 0);

  gtk_widget_show (table);
  gtk_widget_show (frame);

  return frame;
}

/*---------------------------------------------------------------------------*/
/* Main: Create one pixmap button */
/*---------------------------------------------------------------------------*/

void 
rcm_create_pixmap_button (GtkWidget     **label, 
			  GtkWidget     **xpm_button,
			  GtkWidget     **label_box, 
			  GtkSignalFunc   callback,
			  gpointer        data, 
			  gchar          *text, 
			  GtkWidget      *parent, 
			  gint            pos)
{
  /* create button */
  *xpm_button = gtk_button_new();
  gtk_signal_connect(GTK_OBJECT(*xpm_button), "clicked", callback, data);
  gtk_widget_show(*xpm_button);

  gtk_table_attach(GTK_TABLE(parent), *xpm_button,
		   0, 1, pos, pos+1, GTK_EXPAND|GTK_FILL, GTK_FILL, 4, 2);

  /* create hbox */
  *label_box = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(*label_box);

  /* create label */
  *label = gtk_label_new(text);
  gtk_widget_show(*label);

  /* put label and pixmap in hbox */
  gtk_box_pack_end(GTK_BOX(*label_box), *label, TRUE, FALSE, 0);

  /* create hbox in button */
  gtk_container_add(GTK_CONTAINER(*xpm_button), *label_box);
}

/*---------------------------------------------------------------------------*/
/* Set buttons pixmaps */
/*---------------------------------------------------------------------------*/

void 
rcm_set_pixmaps (RcmCircle *circle)
{
  rcm_set_pixmap(&circle->cw_ccw_pixmap, circle->cw_ccw_button->parent, circle->cw_ccw_box, rcm_cw);
  rcm_set_pixmap(&circle->a_b_pixmap, circle->a_b_button->parent, circle->a_b_box, rcm_a_b);
  rcm_set_pixmap(&circle->f360_pixmap, circle->f360_button->parent, circle->f360_box, rcm_360);
}

/*---------------------------------------------------------------------------*/
/* Main: One circles with values and buttons */
/*---------------------------------------------------------------------------*/

RcmCircle*
rcm_create_one_circle (gint   height, 
		       gchar *label_content)
{
  GtkWidget *frame, *button_table, *legend_table;
  GtkWidget *label, *label_box, *xpm_button, *entry;
  GtkAdjustment *adj;
  RcmCircle *st;

  st = g_new(RcmCircle, 1);

  st->action_flag = VIRGIN;
  st->angle = g_new(RcmAngle, 1);
  st->angle->alpha = INITIAL_ALPHA;
  st->angle->beta = INITIAL_BETA;
  st->angle->cw_ccw = 1;

  /** Main: Circle: create (main) frame **/
  st->frame = gtk_frame_new(label_content);
  gtk_container_border_width(GTK_CONTAINER(st->frame), 0);
  gtk_widget_show(st->frame);

  /** Main: Circle: create frame & preview **/

  /* create frame */
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 0);
  gtk_widget_show(frame);
 
  /* create preview */
  st->preview = gtk_preview_new(GTK_PREVIEW_COLOR);
  gtk_preview_size(GTK_PREVIEW(st->preview), height, height);
  gtk_widget_show(st->preview);
  gtk_container_add(GTK_CONTAINER(frame), st->preview);

  /* set signals */
  gtk_widget_set_events(st->preview, RANGE_ADJUST_MASK);

  gtk_signal_connect_after(GTK_OBJECT(st->preview), "expose_event",
			   (GtkSignalFunc) rcm_expose_event, st);
  
  gtk_signal_connect(GTK_OBJECT(st->preview), "button_press_event",
		     (GtkSignalFunc) rcm_button_press_event, st);

  gtk_signal_connect(GTK_OBJECT(st->preview), "button_release_event",
		     (GtkSignalFunc) rcm_release_event, st);
  
  gtk_signal_connect(GTK_OBJECT(st->preview), "motion_notify_event",
		     (GtkSignalFunc) rcm_motion_notify_event, st);
  
  rcm_render_circle(st->preview, SUM, MARGIN);

  /** Main: Circle: create table for buttons **/ 
  button_table = gtk_table_new(3, 1, FALSE);
  gtk_widget_show(button_table);

  /** Main: Circle: Buttons **/
  rcm_create_pixmap_button(&label, &xpm_button, &label_box, rcm_cw_ccw, st,
			   (st->angle->cw_ccw>0) ?
			   _("Switch to clockwise") : _("Switch to c/clockwise"),
			   button_table, 0);
  st->cw_ccw_pixmap = NULL;
  st->cw_ccw_button = xpm_button;
  st->cw_ccw_box = label_box;
  st->cw_ccw_label = label;

  rcm_create_pixmap_button(&label, &xpm_button, &label_box, rcm_a_to_b, st,
			   _("Change order of arrows"), button_table, 1);
  st->a_b_pixmap = NULL;
  st->a_b_box = label_box;
  st->a_b_button = xpm_button;

  rcm_create_pixmap_button(&label, &xpm_button, &label_box, rcm_360_degrees, st,
			   _("Select all"), button_table, 2);
  st->f360_pixmap = NULL;
  st->f360_box = label_box;
  st->f360_button = xpm_button;

  /** Main: Circle: Legend **/
  legend_table = gtk_table_new(1, 6, FALSE);
  gtk_widget_show(legend_table);

  /* spinbutton 1 */
  label = gtk_label_new(_("From"));
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(legend_table), label, 0,1, 0,1,
		   0, GTK_EXPAND, 5, 5);

  st->angle->alpha = INITIAL_ALPHA;
  adj = (GtkAdjustment *) gtk_adjustment_new(st->angle->alpha, 0.0, 2.0, 0.0001, 0.001, 0.0);
  st->alpha_entry = entry = gtk_spin_button_new(adj, 0.01, 4);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(entry), TRUE);
  gtk_widget_show(entry);
  gtk_signal_connect(GTK_OBJECT(entry), "changed", (GtkSignalFunc)rcm_set_alpha, st);
  gtk_table_attach(GTK_TABLE(legend_table), entry, 1,2, 0,1,
		   GTK_EXPAND|GTK_FILL, GTK_EXPAND, 2, 4);

  /* label */
  st->alpha_units_label = gtk_label_new(rcm_units_string(Current.Units));
  gtk_widget_show(st->alpha_units_label);

  gtk_table_attach(GTK_TABLE(legend_table), st->alpha_units_label, 2, 3, 0, 1,
		   0, GTK_EXPAND, 4, 4);

  /* spinbutton 2 */
  label = gtk_label_new(_("to"));
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(legend_table), label, 3,4, 0,1,
		   0, GTK_EXPAND, 4, 4);

  st->angle->beta = INITIAL_BETA;
  adj = (GtkAdjustment *) gtk_adjustment_new(st->angle->beta, 0.0, 2.0, 0.0001, 0.001, 0.0);
  st->beta_entry = entry = gtk_spin_button_new(adj, 0.01, 4);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(entry), TRUE);
  gtk_widget_show(entry);
  gtk_signal_connect(GTK_OBJECT(entry), "changed", (GtkSignalFunc)rcm_set_beta, st);
  gtk_table_attach(GTK_TABLE(legend_table), entry, 4,5, 0,1,
		   GTK_EXPAND|GTK_FILL, GTK_EXPAND, 2, 4);

  /* label */
  st->beta_units_label = gtk_label_new(rcm_units_string(Current.Units));
  gtk_widget_show(st->beta_units_label);

  gtk_table_attach(GTK_TABLE(legend_table), st->beta_units_label, 5,6, 0,1,
		   0, GTK_EXPAND, 4, 4);

  /* Main: Circle: create table for Preview / Buttons / Legend */
  st->table= gtk_table_new(2, 2, FALSE);
  gtk_widget_show(st->table);
  
  gtk_table_attach(GTK_TABLE(st->table), frame, 0, 1, 0, 1,
  		   0, GTK_EXPAND, 4, 0);
  
  gtk_table_attach(GTK_TABLE(st->table), button_table, 1, 2, 0, 1,
		   0, GTK_EXPAND, 2, 0);
  
  gtk_table_attach(GTK_TABLE(st->table), legend_table, 0, 2, 1, 2,
		   GTK_EXPAND|GTK_FILL, GTK_EXPAND, 0, 2);

  /* add table to (main) frame */
  gtk_container_add(GTK_CONTAINER(st->frame), st->table);
 
  return st;
}

/*---------------------------------------------------------------------------*/
/* Main */
/*---------------------------------------------------------------------------*/

GtkWidget*
rcm_create_main (void)
{
  GtkWidget *vbox;

  Current.From = rcm_create_one_circle (SUM, _("From"));
  Current.To   = rcm_create_one_circle (SUM, _("To"));

  vbox = gtk_vbox_new (FALSE, 4);

  gtk_box_pack_start (GTK_BOX (vbox), Current.From->frame,
		      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), Current.To->frame,
		      FALSE, FALSE, 0);

  gtk_widget_show (vbox);

  return vbox;
}

/*---------------------------------------------------------------------------*/
/* Misc: Gray */
/*---------------------------------------------------------------------------*/

RcmGray*
rcm_create_gray (void)
{
  GtkWidget *frame, *preview, *as_or_to_frame;
  GtkWidget *table, *previewframe, *legend_table;
  GtkWidget *mini_table;
  GtkWidget *label, *entry;
  GtkWidget *gray_sat_frame;
  GtkWidget *radio_box, *button;
  GSList *group = NULL;
  RcmGray *st;
  GtkAdjustment *adj;

  st = g_new (RcmGray, 1);
  st->hue         = 0;
  st->satur       = 0;
  st->action_flag = VIRGIN;

  /** Gray **/
  st->frame = frame = gtk_frame_new (_("Gray"));
  gtk_widget_show (frame);

  /* Gray: Circle: Circle (Preview) */
  previewframe = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (previewframe), GTK_SHADOW_IN);
  gtk_widget_show (previewframe);
   
  st->preview = preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), GRAY_SUM, GRAY_SUM);
  gtk_widget_show (preview);
  gtk_container_add (GTK_CONTAINER (previewframe), preview);

  gtk_widget_set_events (preview, RANGE_ADJUST_MASK);

  gtk_signal_connect_after (GTK_OBJECT (preview), "expose_event",
			    (GtkSignalFunc) rcm_gray_expose_event, st);

  gtk_signal_connect (GTK_OBJECT (preview), "button_press_event",
		      (GtkSignalFunc) rcm_gray_button_press_event, st);

  gtk_signal_connect (GTK_OBJECT (preview), "button_release_event",
		      (GtkSignalFunc) rcm_gray_release_event, st);

  gtk_signal_connect (GTK_OBJECT (preview), "motion_notify_event",
		      (GtkSignalFunc) rcm_gray_motion_notify_event, st);

  /* Gray: Circle: Legend */  
  legend_table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (legend_table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (legend_table), 2);
  gtk_widget_show (legend_table);

  /* Gray: Circle: Spinbutton 1 */
  label = gtk_label_new(_("Hue:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (legend_table), label, 0, 1, 0, 1, 
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  st->hue = INITIAL_GRAY_HUE;
  adj = (GtkAdjustment *) gtk_adjustment_new (st->hue, 0.0, 2.0, 0.0001, 0.001, 0.0);
  st->hue_entry = entry = gtk_spin_button_new (adj, 0.01, 4);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON(entry), TRUE);
  gtk_widget_set_usize (entry, 75, 0);
  gtk_signal_connect (GTK_OBJECT(entry), "changed",
		      (GtkSignalFunc) rcm_set_hue, st);
  gtk_table_attach (GTK_TABLE (legend_table), entry, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  gtk_widget_show (entry);

  /* Gray: Circle: units label */
  st->hue_units_label = gtk_label_new (rcm_units_string (Current.Units));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (legend_table), st->hue_units_label, 2, 3, 0, 1,
		    GTK_EXPAND, GTK_EXPAND, 0, 0);
  gtk_widget_show (st->hue_units_label);

  /* Gray: Circle: Spinbutton 2 */
  label = gtk_label_new (_("Saturation:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (legend_table), label, 0, 1, 1, 2, 
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  st->satur = INITIAL_GRAY_SAT;
  adj = (GtkAdjustment *) gtk_adjustment_new (st->satur, 0.0, 1.0, 0.0001, 0.001, 0.0);
  st->satur_entry = entry = gtk_spin_button_new (adj, 0.01, 4);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (entry), TRUE);
  gtk_widget_set_usize (entry, 75, 0);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) rcm_set_satur, st);
  gtk_table_attach (GTK_TABLE (legend_table), entry, 1, 2, 1, 2, 
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 2);
  gtk_widget_show (entry);

  /** Gray: Operation-Mode **/
  as_or_to_frame = gtk_frame_new (_("Mode"));
  gtk_widget_show (as_or_to_frame);

  radio_box = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (as_or_to_frame), radio_box);
  gtk_container_set_border_width (GTK_CONTAINER (radio_box), 4);
  gtk_widget_show (radio_box);

  /* Gray: Operation-Mode: two radio buttons */
  button = gtk_radio_button_new_with_label(group, _("Treat as this"));
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (radio_box), button, FALSE, FALSE, 0);
  if (Current.Gray_to_from == GRAY_FROM)
    gtk_button_clicked (GTK_BUTTON (button));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) rcm_switch_to_gray_from,
		      &(Current.Gray_to_from));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

  button = gtk_radio_button_new_with_label (group, _("Change to this"));
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (radio_box), button, FALSE, FALSE, 0);
  if (Current.Gray_to_from == GRAY_TO)
    gtk_button_clicked (GTK_BUTTON (button));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) rcm_switch_to_gray_to,
		      &(Current.Gray_to_from));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

  /** Gray: What is gray? **/ 
  gray_sat_frame = gtk_frame_new(_("What is Gray?"));
  gtk_widget_show (gray_sat_frame);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_widget_show (table);

  label = gtk_label_new(_("Saturation"));
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);

  label = gtk_label_new ("<=");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);

  st->gray_sat = INITIAL_GRAY_RSAT;
  adj = (GtkAdjustment *) gtk_adjustment_new (st->gray_sat, 0.0, 1.0, 0.0001, 0.001, 0.0);

  st->gray_sat_entry = entry = gtk_spin_button_new (adj, 0.01, 4);
  gtk_widget_set_usize (entry, 75, 0);
  gtk_widget_show (entry);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) rcm_set_gray_sat, st);
  gtk_table_attach (GTK_TABLE (table), entry, 2, 3, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);

  gtk_container_add (GTK_CONTAINER (gray_sat_frame), table);

  /** add all frames to table **/

  /* add preview and legend to table */
  mini_table = gtk_table_new (2, 1, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (mini_table), 4);
  gtk_widget_show (mini_table);
    
  gtk_table_attach (GTK_TABLE (mini_table), previewframe, 0, 1, 0, 1,
		    GTK_SHRINK, GTK_SHRINK, 0, 0);
    
  gtk_table_attach (GTK_TABLE (mini_table), legend_table, 0, 1, 1, 2,
		    GTK_SHRINK, GTK_SHRINK, 0, 0);

  /* add mini_table & two frames to table */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_widget_show (table);

  gtk_table_attach (GTK_TABLE (table), mini_table, 0, 1, 0, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), as_or_to_frame, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), gray_sat_frame, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_container_add (GTK_CONTAINER (frame), table);

  /* update circle (preview) */
  rcm_render_circle (preview, GRAY_SUM, GRAY_MARGIN);

  return st;
}

/*----------------------------------------------------------------------------*/
/* Misc */
/*----------------------------------------------------------------------------*/

GtkWidget *
rcm_create_misc (void)
{
  GtkWidget *label, *table;
  GtkWidget *units_frame, *units_vbox;
  GtkWidget *preview_frame, *preview_vbox;
  GtkWidget *item, *menu, *root, *hbox;
  GtkWidget *button;

  GSList *units_group = NULL;
  GSList *preview_group = NULL;
  
  /** Misc: Gray circle **/
  Current.Gray = rcm_create_gray ();

  /** Misc: Used unit selection **/
  units_frame = gtk_frame_new (_("Units"));
  gtk_widget_show (units_frame);

  units_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (units_frame), units_vbox);
  gtk_container_set_border_width (GTK_CONTAINER (units_vbox), 4);
  gtk_widget_show (units_vbox);

  /* Misc: Used unit selection: 3 radio buttons */
  button = gtk_radio_button_new_with_label (units_group, _("Radians"));
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (units_vbox), button, FALSE, FALSE, 0);
  if (Current.Units == RADIANS)
    gtk_button_clicked (GTK_BUTTON (button));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) rcm_switch_to_radians, NULL);
  units_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

  button = gtk_radio_button_new_with_label (units_group, _("Radians/Pi"));
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (units_vbox), button, FALSE, FALSE, 0);
  if (Current.Units == RADIANS_OVER_PI)
    gtk_button_clicked (GTK_BUTTON (button));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) rcm_switch_to_radians_over_PI, NULL);
  units_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

  button = gtk_radio_button_new_with_label (units_group, _("Degrees"));
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (units_vbox), button, FALSE, FALSE, 0);
  if (Current.Units == DEGREES)
    gtk_button_clicked (GTK_BUTTON (button));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) rcm_switch_to_degrees, NULL);

  /** Misc: Preview settings **/

  /* Misc: Preview settings: Continuous update ?! */
  preview_frame = gtk_frame_new (_("Preview"));
  preview_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (preview_vbox), 4);
  gtk_widget_show (preview_vbox);

  gtk_container_add (GTK_CONTAINER (preview_frame), preview_vbox);
  gtk_widget_show (preview_frame);
  
  button = gtk_check_button_new_with_label (_("Continuous update"));
  gtk_box_pack_start(GTK_BOX(preview_vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), Current.RealTime);
   
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) (GtkSignalFunc) rcm_preview_as_you_drag,
		      &(Current.RealTime));

  gtk_widget_show (button);

  /* Misc: Preview settings: Area */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (preview_vbox), hbox, FALSE, FALSE, 4);

  label = gtk_label_new (_("Area:"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  /* create menu entries */
  menu = gtk_menu_new ();

  item = gtk_radio_menu_item_new_with_label (preview_group, _("Entire Image"));
  preview_group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (item));
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);
  gtk_signal_connect (GTK_OBJECT (item), "activate",
		      (GtkSignalFunc) rcm_entire_image, NULL);
    
  item = gtk_radio_menu_item_new_with_label (preview_group, _("Selection"));
  preview_group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (item));
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);
  gtk_signal_connect (GTK_OBJECT (item), "activate",
		      (GtkSignalFunc) rcm_selection, NULL);

  item = gtk_radio_menu_item_new_with_label (preview_group, _("Context"));
  preview_group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (item));
  gtk_widget_show (item);
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_signal_connect (GTK_OBJECT (item), "activate",
		      (GtkSignalFunc) rcm_selection_in_context, NULL);

  /* create (options) menu */
  root =  gtk_option_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (root), menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (root), 4);
  gtk_widget_show (root);

  gtk_box_pack_start (GTK_BOX (hbox), root, FALSE, FALSE, 0);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);

  /** add frames (gray/preview/units) to table **/
  gtk_table_attach (GTK_TABLE (table), Current.Gray->frame,  0, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), preview_frame, 0, 1, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), units_frame, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_widget_show (table);

  return table;
}

/*---------------------------------------------------------------------------*/
/* create and call main dialog */
/*---------------------------------------------------------------------------*/

gint 
rcm_dialog (void)
{
  GtkWidget *table, *dlg, *hbox, *notebook;
  GtkWidget *previews, *mains, *miscs;
  
  guchar  *color_cube;
  gchar  **argv;
  gint     argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("rcm");

  Current.Bna = g_new (RcmBna, 1);

  /* init GTK and install colormap */
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());
  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
			      color_cube[2], color_cube[3]);

  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

  /* Create dialog */

  dlg = gimp_dialog_new (_("Colormap Rotation"), "rcm",
			 gimp_plugin_help_func, "filters/rcm.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), rcm_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  Current.Bna->dlg = dlg;

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /* Create sub-dialogs */
  Current.reduced = rcm_reduce_image (Current.drawable, Current.mask,
				      MAX_PREVIEW_SIZE, ENTIRE_IMAGE);
  previews = rcm_create_previews ();
  mains    = rcm_create_main ();
  miscs    = rcm_create_misc ();
  Current.Bna->bna_frame = previews;

  rcm_render_preview (Current.Bna->before, ORIGINAL);
  rcm_render_preview (Current.Bna->after, CURRENT);

  /* H-Box */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dlg)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  gtk_box_pack_start (GTK_BOX(hbox), previews, TRUE, TRUE, 0);

  /* Notebook */
  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK(notebook), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (hbox), notebook, TRUE, TRUE, 0);
  gtk_widget_show (notebook);

  table = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_widget_show (table);

  gtk_table_attach (GTK_TABLE (table), mains, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table,
			    gtk_label_new (_("Main")));

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), miscs,
			    gtk_label_new (_("Misc")));

  gtk_widget_show (dlg);

  rcm_set_pixmaps (Current.From);
  rcm_set_pixmaps (Current.To);

  gdk_flush (); 
  gtk_main ();

  return Current.Success;
}
