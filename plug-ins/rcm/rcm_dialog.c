/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "rcm.h"
#include "rcm_misc.h"
#include "rcm_gdk.h"
#include "rcm_callback.h"
#include "rcm_dialog.h"
#include "rcm_stock.h"

#include "images/rcm-stock-pixbufs.h"

#include "libgimp/stdplugins-intl.h"


/* Defines */

#define INITIAL_ALPHA      0
#define INITIAL_BETA       G_PI_2
#define INITIAL_GRAY_SAT   0.0
#define INITIAL_GRAY_RSAT  0.0
#define INITIAL_GRAY_HUE   0.0

#define RANGE_ADJUST_MASK GDK_EXPOSURE_MASK | \
                          GDK_ENTER_NOTIFY_MASK | \
                          GDK_BUTTON_PRESS_MASK | \
                          GDK_BUTTON_RELEASE_MASK | \
                          GDK_BUTTON1_MOTION_MASK | \
                          GDK_POINTER_MOTION_HINT_MASK


/* Previews: create one preview */

static GtkWidget *
rcm_create_one_preview (GtkWidget **preview,
                        gint        mode,
			gint        width,
			gint        height)
{
  GtkWidget *align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  GtkWidget *frame;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (align), frame);
  gtk_widget_show (frame);

  *preview = gimp_preview_area_new ();

  gtk_widget_set_size_request (*preview, width, height);
  gtk_container_add (GTK_CONTAINER (frame), *preview);
  gtk_widget_show (*preview);

  g_object_set_data (G_OBJECT (*preview), "mode", GINT_TO_POINTER (mode));

  return align;
}


/* Previews */

static GtkWidget *
rcm_create_previews (void)
{
  GtkWidget *top_vbox;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *combo;

  top_vbox = gtk_vbox_new (FALSE, 12);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (top_vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Original"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  frame = rcm_create_one_preview (&Current.Bna->before, ORIGINAL,
                                  Current.reduced->width,
                                  Current.reduced->height);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_signal_connect_after (Current.Bna->before, "size-allocate",
                          G_CALLBACK (rcm_render_preview),
                          NULL);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (top_vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Rotated"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  frame = rcm_create_one_preview (&Current.Bna->after, CURRENT,
                                  Current.reduced->width,
                                  Current.reduced->height);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_signal_connect_after (Current.Bna->after, "size-allocate",
                          G_CALLBACK (rcm_render_preview),
                          NULL);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (top_vbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  button = gtk_check_button_new_with_label (_("Continuous update"));
  gtk_box_pack_start(GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), Current.RealTime);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (rcm_preview_as_you_drag),
                    &(Current.RealTime));

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Area:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = gimp_int_combo_box_new (_("Entire Layer"), ENTIRE_IMAGE,
                                  _("Selection"),    SELECTION,
                                  _("Context"),      SELECTION_IN_CONTEXT,
                                  NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), Current.Slctn);

  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (rcm_combo_callback),
                    NULL);

  gtk_widget_show (top_vbox);

  return top_vbox;
}


/* Main: One circles with values and buttons */

static RcmCircle*
rcm_create_one_circle (gint   height,
		       gchar *label_content)
{
  GtkWidget     *frame, *button_table, *legend_table;
  GtkWidget     *label, *button, *entry;
  GtkAdjustment *adj;
  RcmCircle     *st;

  st = g_new (RcmCircle, 1);

  st->action_flag   = VIRGIN;
  st->angle         = g_new (RcmAngle, 1);
  st->angle->alpha  = INITIAL_ALPHA;
  st->angle->beta   = INITIAL_BETA;
  st->angle->cw_ccw = 1;

  /** Main: Circle: create (main) frame **/
  st->frame = gimp_frame_new (label_content);
  gtk_widget_show (st->frame);

  /** Main: Circle: create frame & preview **/

  /* create frame */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_widget_show (frame);

  /* create preview */
  st->preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (st->preview, height, height);
  gtk_container_add (GTK_CONTAINER (frame), st->preview);
  gtk_widget_show (st->preview);

  /* set signals */
  gtk_widget_set_events (st->preview, RANGE_ADJUST_MASK);

  g_signal_connect_after (st->preview, "expose-event",
                          G_CALLBACK (rcm_expose_event),
                          st);

  g_signal_connect (st->preview, "button-press-event",
                    G_CALLBACK (rcm_button_press_event),
                    st);

  g_signal_connect (st->preview, "button-release-event",
                    G_CALLBACK (rcm_release_event),
                    st);

  g_signal_connect (st->preview, "motion-notify-event",
                    G_CALLBACK (rcm_motion_notify_event),
                    st);

  /** Main: Circle: create table for buttons **/
  button_table = gtk_table_new (3, 1, FALSE);
  gtk_widget_show (button_table);

  /** Main: Circle: Buttons **/

  button = gtk_button_new_from_stock (st->angle->cw_ccw > 0 ?
                                      STOCK_COLORMAP_SWITCH_CLOCKWISE :
                                      STOCK_COLORMAP_SWITCH_COUNTERCLOCKWISE);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (rcm_cw_ccw),
                    st);
  gtk_widget_show (button);
  gtk_table_attach (GTK_TABLE (button_table), button,
                    0, 1, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 2);

  st->cw_ccw_pixmap = NULL;
  st->cw_ccw_button = button;
  st->cw_ccw_box    = NULL;
  st->cw_ccw_label  = NULL;

  button = gtk_button_new_from_stock (STOCK_COLORMAP_CHANGE_ORDER);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (rcm_a_to_b),
                    st);
  gtk_widget_show (button);
  gtk_table_attach (GTK_TABLE (button_table), button,
                    0, 1, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 2);

  st->a_b_pixmap = NULL;
  st->a_b_box    = NULL;
  st->a_b_button = button;

  button = gtk_button_new_from_stock (STOCK_COLORMAP_SELECT_ALL);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (rcm_360_degrees),
                    st);
  gtk_widget_show (button);
  gtk_table_attach (GTK_TABLE (button_table), button,
                    0, 1, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 4, 2);

  st->f360_pixmap = NULL;
  st->f360_box    = NULL;
  st->f360_button = button;

  /** Main: Circle: Legend **/
  legend_table = gtk_table_new (1, 6, FALSE);
  gtk_widget_show (legend_table);

  /* spinbutton 1 */
  label = gtk_label_new (_("From:"));
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (legend_table), label, 0, 1, 0, 1,
                    0, GTK_EXPAND, 5, 5);

  st->angle->alpha = INITIAL_ALPHA;
  adj = (GtkAdjustment *) gtk_adjustment_new(st->angle->alpha, 0.0, 2.0, 0.0001, 0.001, 0.0);
  st->alpha_entry = entry = gtk_spin_button_new(adj, 0.01, 4);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(entry), TRUE);
  gtk_table_attach(GTK_TABLE(legend_table), entry, 1,2, 0,1,
		   GTK_EXPAND|GTK_FILL, GTK_EXPAND, 2, 4);
  gtk_widget_show(entry);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (rcm_set_alpha),
                    st);

  /* label */
  st->alpha_units_label = gtk_label_new (rcm_units_string (Current.Units));
  gtk_widget_show (st->alpha_units_label);

  gtk_table_attach (GTK_TABLE (legend_table),
                    st->alpha_units_label, 2, 3, 0, 1,
                    0, GTK_EXPAND, 4, 4);

  /* spinbutton 2 */
  label = gtk_label_new (_("To:"));
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (legend_table), label, 3,4, 0,1,
                    0, GTK_EXPAND, 4, 4);

  st->angle->beta = INITIAL_BETA;
  adj = (GtkAdjustment *) gtk_adjustment_new (st->angle->beta,
                                              0.0, 2.0, 0.0001, 0.001, 0.0);
  st->beta_entry = entry = gtk_spin_button_new (adj, 0.01, 4);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (entry), TRUE);
  gtk_table_attach (GTK_TABLE (legend_table), entry, 4,5, 0,1,
                    GTK_EXPAND|GTK_FILL, GTK_EXPAND, 2, 4);
  gtk_widget_show (entry);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (rcm_set_beta),
                    st);

  /* label */
  st->beta_units_label = gtk_label_new (rcm_units_string (Current.Units));
  gtk_widget_show (st->beta_units_label);

  gtk_table_attach (GTK_TABLE (legend_table), st->beta_units_label, 5,6, 0,1,
                    0, GTK_EXPAND, 4, 4);

  /* Main: Circle: create table for Preview / Buttons / Legend */
  st->table= gtk_table_new (2, 2, FALSE);
  gtk_widget_show (st->table);

  gtk_table_attach (GTK_TABLE (st->table), frame, 0, 1, 0, 1,
                    0, GTK_EXPAND, 4, 0);

  gtk_table_attach (GTK_TABLE (st->table), button_table, 1, 2, 0, 1,
                    0, GTK_EXPAND, 2, 0);

  gtk_table_attach (GTK_TABLE (st->table), legend_table, 0, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 2);

  /* add table to (main) frame */
  gtk_container_add (GTK_CONTAINER (st->frame), st->table);

  return st;
}


/* Main */

static GtkWidget *
rcm_create_main (void)
{
  GtkWidget *vbox;

  Current.From = rcm_create_one_circle (SUM, _("From:"));
  Current.To   = rcm_create_one_circle (SUM, _("To:"));

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_widget_show (vbox);

  gtk_box_pack_start (GTK_BOX (vbox), Current.From->frame,
		      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), Current.To->frame,
		      FALSE, FALSE, 0);

  return vbox;
}


/* Misc: Gray */

static GtkWidget *
rcm_create_gray (void)
{
  GtkWidget *top_vbox, *vbox;
  GtkWidget *frame, *preview;
  GtkWidget *table;
  GtkWidget *entry;
  GtkWidget *radio_box;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;
  GSList    *group = NULL;
  RcmGray   *st;
  GtkAdjustment *adj;

  Current.Gray = st = g_new (RcmGray, 1);
  st->hue         = 0;
  st->satur       = 0;
  st->action_flag = VIRGIN;

  top_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (top_vbox), 12);
  gtk_widget_show (top_vbox);

  frame = gimp_frame_new (_("Gray"));
  gtk_box_pack_start (GTK_BOX (top_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  st->preview = preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (preview, GRAY_SUM, GRAY_SUM);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);

  gtk_widget_add_events (preview, RANGE_ADJUST_MASK);

  g_signal_connect_after (preview, "expose-event",
                          G_CALLBACK (rcm_gray_expose_event),
                          st);

  g_signal_connect (preview, "button-press-event",
                    G_CALLBACK (rcm_gray_button_press_event),
                    st);

  g_signal_connect (preview, "button-release-event",
                    G_CALLBACK (rcm_gray_release_event),
                    st);

  g_signal_connect (preview, "motion-notify-event",
                    G_CALLBACK (rcm_gray_motion_notify_event),
                    st);

  /* Gray: Circle: Legend */
  table = gtk_table_new (2, 3, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_widget_show (table);

  /* Gray: Circle: Spinbutton 1 */
  label = gtk_label_new(_("Hue:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  st->hue = INITIAL_GRAY_HUE;
  adj = (GtkAdjustment *) gtk_adjustment_new (st->hue,
                                              0.0, 2.0, 0.0001, 0.001, 0.0);
  st->hue_entry = entry = gtk_spin_button_new (adj, 0.01, 4);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON(entry), TRUE);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (entry);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (rcm_set_hue),
                    st);

  /* Gray: Circle: units label */
  st->hue_units_label = gtk_label_new (rcm_units_string (Current.Units));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), st->hue_units_label, 2, 3, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (st->hue_units_label);

  /* Gray: Circle: Spinbutton 2 */
  label = gtk_label_new (_("Saturation:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  st->satur = INITIAL_GRAY_SAT;
  adj = (GtkAdjustment *) gtk_adjustment_new (st->satur,
                                              0.0, 1.0, 0.0001, 0.001, 0.0);
  st->satur_entry = entry = gtk_spin_button_new (adj, 0.01, 4);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (entry), TRUE);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 2);
  gtk_widget_show (entry);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (rcm_set_satur),
                    st);

  /** Gray: Operation-Mode **/
  frame = gimp_frame_new (_("Gray Mode"));
  gtk_box_pack_start (GTK_BOX (top_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  radio_box = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);
  gtk_widget_show (radio_box);

  /* Gray: Operation-Mode: two radio buttons */
  button = gtk_radio_button_new_with_label(group, _("Treat as this"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  if (Current.Gray_to_from == GRAY_FROM)
    gtk_button_clicked (GTK_BUTTON (button));

  g_signal_connect (button, "clicked",
                    G_CALLBACK (rcm_switch_to_gray_from),
                    &(Current.Gray_to_from));

  button = gtk_radio_button_new_with_label (group, _("Change to this"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  if (Current.Gray_to_from == GRAY_TO)
    gtk_button_clicked (GTK_BUTTON (button));

  g_signal_connect (button, "clicked",
                    G_CALLBACK (rcm_switch_to_gray_to),
                    &(Current.Gray_to_from));

  /** Gray: What is gray? **/
  frame = gimp_frame_new (_("Gray Threshold"));
  gtk_box_pack_start (GTK_BOX (top_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Saturation:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  st->gray_sat = INITIAL_GRAY_RSAT;
  adj = (GtkAdjustment *) gtk_adjustment_new (st->gray_sat,
                                              0.0, 1.0, 0.0001, 0.001, 0.0);

  entry = gtk_spin_button_new (adj, 0.01, 4);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (entry), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (rcm_set_gray_sat),
                    st);

  return top_vbox;
}


/* Misc */

static GtkWidget *
rcm_create_units (void)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *button;
  GSList    *group = NULL;

  /** Misc: Used unit selection **/
  frame = gimp_frame_new (_("Units"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 12);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /* Misc: Used unit selection: 3 radio buttons */
  button = gtk_radio_button_new_with_label (group, _("Radians"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  if (Current.Units == RADIANS)
    gtk_button_clicked (GTK_BUTTON (button));

  g_signal_connect (button, "clicked",
                    G_CALLBACK (rcm_switch_to_radians),
                    NULL);

  button = gtk_radio_button_new_with_label (group, _("Radians/Pi"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  if (Current.Units == RADIANS_OVER_PI)
    gtk_button_clicked (GTK_BUTTON (button));

  g_signal_connect (button, "clicked",
                    G_CALLBACK (rcm_switch_to_radians_over_PI),
                    NULL);

  button = gtk_radio_button_new_with_label (group, _("Degrees"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  if (Current.Units == DEGREES)
    gtk_button_clicked (GTK_BUTTON (button));

  g_signal_connect (button, "clicked",
                    G_CALLBACK (rcm_switch_to_degrees),
                    NULL);

  return frame;
}


/* create and call main dialog */

gboolean
rcm_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *notebook;
  GtkWidget *previews;
  gboolean   run;

  Current.Bna = g_new (RcmBna, 1);

  gimp_ui_init (PLUG_IN_BINARY, TRUE);
  rcm_stock_init ();

  dialog = gimp_dialog_new (_("Rotate Colors"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  Current.Bna->dlg = dialog;

  /* Create sub-dialogs */
  Current.reduced = rcm_reduce_image (Current.drawable, Current.mask,
                                      MAX_PREVIEW_SIZE, Current.Slctn);

  Current.Bna->bna_frame = previews = rcm_create_previews ();

  /* H-Box */
  hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  gtk_box_pack_start (GTK_BOX (hbox), previews, TRUE, TRUE, 0);

  /* Notebook */
  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (hbox), notebook, TRUE, TRUE, 0);
  gtk_widget_show (notebook);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), rcm_create_main (),
			    gtk_label_new (_("Main Options")));

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), rcm_create_gray (),
			    gtk_label_new (_("Gray Options")));

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), rcm_create_units (),
			    gtk_label_new (_("Units")));

  gtk_widget_show (dialog);

  rcm_render_circle (Current.From->preview, SUM, MARGIN);
  rcm_render_circle (Current.To->preview, SUM, MARGIN);
  rcm_render_circle (Current.Gray->preview, GRAY_SUM, GRAY_MARGIN);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
