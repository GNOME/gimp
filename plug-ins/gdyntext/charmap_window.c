/*
 * GIMP Dynamic Text -- This is a plug-in for The GIMP 1.0
 * Copyright (C) 1998,1999 Marco Lamberto <lm@geocities.com>
 * Web page: http://www.geocities.com/Tokyo/1474/gimp/
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
 * $Id$
 */

#include <stdio.h>
#include <gtk/gtk.h>
#include "libgimp/stdplugins-intl.h"
#include "charmap.h"
#include "charmap_window.h"


static void charmap_window_class_init(CharMapWindowClass *class);
static void charmap_window_init(CharMapWindow *mw);
void on_charmap_char_selected(GtkWidget *widget, gpointer data);


static GtkWindowClass *charmap_window_parent_class = NULL;


guint charmap_window_get_type(void)
{
	static guint cmw_type = 0;

	if (!cmw_type) {
		GtkTypeInfo cmw_info = {
			"CharMapWindow",
			sizeof(CharMapWindow),
			sizeof(CharMapWindowClass),
			(GtkClassInitFunc)charmap_window_class_init,
			(GtkObjectInitFunc)charmap_window_init,
			(GtkArgSetFunc)NULL,
			(GtkArgGetFunc)NULL,
#ifdef GTK_HAVE_FEATURES_1_1_12
			(GtkClassInitFunc)NULL,
#endif
		};
		cmw_type = gtk_type_unique(gtk_window_get_type(), &cmw_info);
	}
	return cmw_type;
}


static void charmap_window_class_init(CharMapWindowClass *klass)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *)klass;
	charmap_window_parent_class = gtk_type_class(gtk_window_get_type());
}


static void charmap_window_init(CharMapWindow *cmw)
{
	GtkWidget *vbox;
	GtkWidget *hbox1;
	GtkWidget *hbox2;
	GtkWidget *hbbox1;
	GtkWidget *hseparator;
	GtkWidget *frame;
	GtkWidget *label;
	GtkTooltips *tooltips;
	
	tooltips = gtk_tooltips_new();

  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(cmw), vbox);
  gtk_widget_show(vbox);

  cmw->scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_box_pack_start(GTK_BOX(vbox), cmw->scrolledwindow, TRUE, TRUE, 0);
  gtk_container_border_width(GTK_CONTAINER(cmw->scrolledwindow), 2);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cmw->scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_show(cmw->scrolledwindow);

  cmw->charmap = charmap_new();
#ifdef GTK_HAVE_FEATURES_1_1_12
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(cmw->scrolledwindow), cmw->charmap);
#else
  gtk_container_add(GTK_CONTAINER(cmw->scrolledwindow), cmw->charmap);
#endif
	gtk_signal_connect(GTK_OBJECT(cmw->charmap), "char_selected",
		GTK_SIGNAL_FUNC(on_charmap_char_selected), cmw);
  gtk_widget_show(cmw->charmap);

  hseparator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), hseparator, FALSE, TRUE, 0);
  gtk_widget_show(hseparator);

  hbox1 = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, TRUE, 0);
  gtk_widget_show(hbox1);

	frame = gtk_frame_new(NULL);
  gtk_box_pack_start(GTK_BOX(hbox1), frame, FALSE, FALSE, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_widget_show(frame);

  hbox2 = gtk_hbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(frame), hbox2);
  gtk_widget_show(hbox2);

	label = gtk_label_new(_("Selected char:"));
  gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 4);
  gtk_widget_show(label);

	cmw->label = gtk_label_new("   ");
  gtk_box_pack_start(GTK_BOX(hbox2), cmw->label, FALSE, TRUE, 5);
  gtk_widget_show(cmw->label);

	hbbox1 = gtk_hbutton_box_new();
  gtk_box_pack_end(GTK_BOX(hbox1), hbbox1, FALSE, FALSE, 0);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbbox1), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbbox1), 4);
  gtk_widget_show(hbbox1);

  cmw->insert_button = gtk_button_new_with_label(_("Insert"));
  gtk_box_pack_start(GTK_BOX(hbbox1), cmw->insert_button, FALSE, FALSE, 0);
  GTK_WIDGET_SET_FLAGS(cmw->insert_button, GTK_CAN_DEFAULT);
	gtk_tooltips_set_tip(tooltips, cmw->insert_button, _("Insert the selected char at the cursor position"), NULL);
  gtk_widget_show(cmw->insert_button);

  cmw->close_button = gtk_button_new_with_label(_("Close"));
  gtk_box_pack_start(GTK_BOX(hbbox1), cmw->close_button, FALSE, FALSE, 0);
  GTK_WIDGET_SET_FLAGS(cmw->close_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(cmw->close_button);
  gtk_widget_show(cmw->close_button);
}


GtkWidget* charmap_window_new(const gchar *title)
{
	CharMapWindow *cmw;

	cmw = gtk_type_new(charmap_window_get_type());
	gtk_window_set_title(GTK_WINDOW(cmw), title);
	gtk_container_border_width(GTK_CONTAINER(cmw), 4);
	gtk_window_set_policy(GTK_WINDOW(cmw), TRUE, TRUE, FALSE);

	return GTK_WIDGET(cmw);
}


void on_charmap_char_selected(GtkWidget *widget, gpointer data)
{
	CharMapWindow *cmw;
	gchar lab[2];

	cmw = (CharMapWindow *)data;
	lab[0] = CHARMAP(cmw->charmap)->current_char;
	lab[1] = 0;
	gtk_label_set(GTK_LABEL(cmw->label), lab);
}

/* vim: set ts=2 sw=2 tw=79 ai nowrap: */
