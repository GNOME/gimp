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

void
gpc_close_callback(GtkWidget *widget, gpointer data);

void
gpc_cancel_callback(GtkWidget *widget, gpointer data);

void
gpc_scale_update(GtkAdjustment *adjustment, double *scale_val);

void
gpc_text_update(GtkWidget *widget, gpointer data);

void
gpc_setup_tooltips(GtkWidget *parent);

void
gpc_set_tooltip(GtkWidget *widget, const char *tip);

void
gpc_add_action_button(char *label, GtkSignalFunc callback, GtkWidget *dialog,
    char *tip);

void
gpc_add_radio_button(GSList **group, char *label, GtkWidget *box,
    gint *value, char *tip);

void
gpc_add_label(char *value, GtkWidget *parent, int left, int right,
    int top, int bottom);

void
gpc_add_hscale(GtkWidget *table, int width, float low, float high,
    gdouble *val, int left, int right, int top, int bottom, char *tip);
