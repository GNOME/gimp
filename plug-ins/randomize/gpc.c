/****************************************************************************
 * This is a convenience library for plugins for the GIMP v 0.99.8 or later.
 * Documentation is available at http://www.rru.com/~meo/gimp/ .
 *
 * Copyright (C) 1997 Miles O'Neal  <meo@rru.com>  http://www.rru.com/~meo/
 * Blur code Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * GUI based on GTK code from:
 *    alienmap (Copyright (C) 1996, 1997 Daniel Cotting)
 *    plasma   (Copyright (C) 1996 Stephen Norris),
 *    oilify   (Copyright (C) 1996 Torsten Martinsen),
 *    ripple   (Copyright (C) 1997 Brian Degenhardt) and
 *    whirl    (Copyright (C) 1997 Federico Mena Quintero).
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
 *
 ****************************************************************************/

/****************************************************************************
 * gpc:
 *
 * gpc version 1.1 (3 Feb 1998, MEO)
 * history
 *     1.1 -  3 Feb 1998 MEO
 *         removed tooltips from action buttons
 *     1.0 -  2 Feb 1998 MEO
 *         FCS
 *
 * Please send any patches or suggestions to the author: meo@rru.com .
 * 
 ****************************************************************************/

#include <stdlib.h>
#include <time.h>
#include "libgimp/gimp.h"
#include "gtk/gtk.h"


/*
 *  TOGGLE UPDATE callback
 */
static void
gpc_toggle_update(GtkWidget *widget, gpointer data) {
    int *toggle_val;

    toggle_val = (int *) data;

    if (GTK_TOGGLE_BUTTON(widget)->active)
      *toggle_val = TRUE;
    else
      *toggle_val = FALSE;
}
/*
 *  DESTROY callback
 */
void
gpc_close_callback(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

/*
 *  CANCEL BUTTON callback
 */
void
gpc_cancel_callback(GtkWidget *widget, gpointer data) {
    gtk_widget_destroy(GTK_WIDGET(data));
}

/*
 *  SCALE UPDATE callback
 */
void
gpc_scale_update(GtkAdjustment *adjustment, double *scale_val) {
    *scale_val = adjustment->value;
}


/*
 *  TEXT UPDATE callback
 */
void
gpc_text_update(GtkWidget *widget, gpointer data) {
  gint *text_val;

  text_val = (gint *) data;

  *text_val = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
}



/*
 *  TOOLTIPS ROUTINES
 */
static GtkTooltips *tips;


void
gpc_setup_tooltips(GtkWidget *parent)
{
    static GdkColor tips_fg, tips_bg;

    tips = gtk_tooltips_new();

    /* black as foreground: */

    tips_fg.red   = 0;
    tips_fg.green = 0;
    tips_fg.blue  = 0;
    gdk_color_alloc(gtk_widget_get_colormap(parent), &tips_fg);

    /* postit yellow (khaki) as background: */

    tips_bg.red   = 61669;
    tips_bg.green = 59113;
    tips_bg.blue  = 35979;
    gdk_color_alloc(gtk_widget_get_colormap(parent), &tips_bg);

    gtk_tooltips_set_colors(tips, &tips_bg, &tips_fg);
}


void
gpc_set_tooltip(GtkWidget *widget, const char *tip)
{
    if (tip && tip[0])
        gtk_tooltips_set_tip (tips, widget, (char *) tip,NULL);
}

void
gpc_add_action_button(char *label, GtkSignalFunc callback, GtkWidget *dialog
    /* , char *tip */ )
{
    GtkWidget *button;

    button = gtk_button_new_with_label(label);
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_signal_connect(GTK_OBJECT(button), "clicked", callback, dialog);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
	button, TRUE, TRUE, 0);
    gtk_widget_grab_default(button);
    gtk_widget_show(button);
    /* gpc_set_tooltip(button, tip); */
}

void
gpc_add_radio_button(GSList **group, char *label, GtkWidget *box,
    gint *value, char *tip)
{ 
    GtkWidget *toggle;

    toggle = gtk_radio_button_new_with_label(*group, label);
    *group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(box), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
        (GtkSignalFunc) gpc_toggle_update, value);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), *value);
    gtk_widget_show(toggle);
    gtk_widget_show(box);
    gpc_set_tooltip(toggle, tip);
}

void
gpc_add_label(char *value, GtkWidget *table, int left, int right,
    int top, int bottom)
{
    GtkWidget *label;

    label = gtk_label_new(value);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, left, right, top, bottom,
        GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 0);
    gtk_widget_show(label);
}

void
gpc_add_hscale(GtkWidget *table, int width, float low, float high,
    gdouble *val, int left, int right, int top, int bottom, char *tip)
{
    GtkWidget *scale;
    GtkObject *scale_data;

    scale_data = gtk_adjustment_new(*val, low, high, 1.0, 1.0, 0.0);
    scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
    gtk_widget_set_usize(scale, width, 0);
    gtk_table_attach(GTK_TABLE(table), scale, left, right, top, bottom,
        GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_TOP);
    gtk_scale_set_digits(GTK_SCALE(scale), 0);
    gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_DELAYED);
    gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
        (GtkSignalFunc) gpc_scale_update, val);
    gtk_widget_show(scale);
    gpc_set_tooltip(scale, tip);
}
