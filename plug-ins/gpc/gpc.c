/****************************************************************************
 * This is a convenience library for plugins for the GIMP v 0.99.8 or later.
 * Documentation is available at http://www.rru.com/~meo/gimp/ .
 *
 * Copyright (C) 1997, 1998 Miles O'Neal <meo@rru.com> http://www.rru.com/~meo/
 * GUI may include GTK code from:
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
 * gpc: GTK Plug-in Convenience library
 *
 * history
 *     1.4 - 30 Apr 1998 MEO
 *         added man page
 *     1.3 - 29 Apr 1998 MEO
 *         GTK 1.0 port (minor tooltips change)
 *         restored tooltips to action buttons
 *     1.2 - 11 Feb 1998 MEO
 *         added basic comments
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
 *  TOGGLE UPDATE callback - toggles the TOGGLE widget's data
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
 *  SCALE UPDATE callback - update the SCALE widget's data
 */
void
gpc_scale_update(GtkAdjustment *adjustment, double *scale_val) {
    *scale_val = adjustment->value;
}


/*
 *  TEXT UPDATE callback - update the TEXT widget's data
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


/*
 *  TOOLTIP INITIALIZATION
 */
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


/*
 *  SET TOOLTIP for a widget
 */
void
gpc_set_tooltip(GtkWidget *widget, const char *tip)
{
    if (tip && tip[0])
        gtk_tooltips_set_tip(tips, widget, (char *) tip, NULL);
}


/*
 *  ADD RADIO BUTTON to a dialog
 */
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
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), *value);
    gtk_widget_show(toggle);
    gtk_widget_show(box);
    gpc_set_tooltip(toggle, tip);
}


/*
 *  ADD HORIZONTAL SCALE widget to a dialog at given location
 */
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
