/*
 * GIMP Dynamic Text -- This is a plug-in for The GIMP 1.0
 * Copyright (C) 1998,1999,2000 Marco Lamberto <lm@geocities.com>
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

#include "config.h"

#include <stdio.h>
#include <gtk/gtk.h>

#include "libgimp/stdplugins-intl.h"

#include "charmap.h"


static void charmap_class_init(CharMapClass *class);
static void charmap_init(CharMap *cm);
void on_charmap_char_toggled(GtkWidget *widget, gpointer data);


enum {
	CHAR_SELECTED,
	LAST_SIGNAL
};


static gint charmap_signals[LAST_SIGNAL] = { 0 };


guint charmap_get_type(void)
{
	static guint cm_type = 0;

	if (!cm_type) {
		GtkTypeInfo cm_info = {
			"CharMap",
			sizeof(CharMap),
			sizeof(CharMapClass),
			(GtkClassInitFunc)charmap_class_init,
			(GtkObjectInitFunc)charmap_init,
			(GtkArgSetFunc)NULL,
			(GtkArgGetFunc)NULL,
#ifdef GTK_HAVE_FEATURES_1_1_12
			(GtkClassInitFunc)NULL,
#endif
		};
		cm_type = gtk_type_unique(gtk_vbox_get_type(), &cm_info);
	}
	return cm_type;
}


static void charmap_class_init(CharMapClass *klass)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *)klass;

	charmap_signals[CHAR_SELECTED] = gtk_signal_new("char_selected",
		GTK_RUN_FIRST, object_class->type,
		GTK_SIGNAL_OFFSET(CharMapClass, char_selected),
		gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

	gtk_object_class_add_signals(object_class, charmap_signals, LAST_SIGNAL);

	klass->char_selected = NULL;
}


static void charmap_init(CharMap *cm)
{
	GtkWidget *table;
	GtkWidget *button;
	guint x, y, i;
	gchar clabel[2];
	gchar *tip;
	GtkTooltips *tooltips;

	cm->width = 8;
	cm->height = 32;
	cm->current_char = -1;
	
  table = gtk_table_new(32, 8, TRUE);
  gtk_container_add(GTK_CONTAINER(cm), table);
  gtk_table_set_row_spacings(GTK_TABLE(table), 0);
  gtk_table_set_col_spacings(GTK_TABLE(table), 0);
  gtk_widget_show(table);

	tooltips = gtk_tooltips_new();
	clabel[1] = 0;
	for (y = 0, i = 0; y < cm->height; y++)
		for (x = 0; x < cm->width; x++, i++) {
			clabel[0] = i;
		  button = cm->buttons[x + y * cm->width] = gtk_toggle_button_new_with_label(clabel);
			gtk_button_set_relief(&GTK_TOGGLE_BUTTON(button)->button, GTK_RELIEF_HALF);
			if (i == 32)
				gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), TRUE);
		  gtk_table_attach(GTK_TABLE(table), button, x, x + 1, y, y + 1,
				(GtkAttachOptions)GTK_EXPAND | GTK_SHRINK | GTK_FILL,
				(GtkAttachOptions)GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
			gtk_signal_connect_after(GTK_OBJECT(button), "toggled",
				GTK_SIGNAL_FUNC(on_charmap_char_toggled), cm);
		  gtk_widget_show(button);
			tip = g_strdup_printf(_("Char: %c, %d, 0x%02x"), i < 32 ? ' ' : i, i, i);
			gtk_tooltips_set_tip(tooltips, button, tip, NULL);
			g_free(tip);
		}
}


GtkWidget* charmap_new(void)
{
	return GTK_WIDGET(gtk_type_new(charmap_get_type()));
}


void on_charmap_char_toggled(GtkWidget *widget, gpointer data)
{
	static gboolean in = FALSE;	/* flag for avoiding recursive calls in signal */

	if (!in) {
		CharMap *cm;
		gint i;

		in = TRUE;
		cm = (CharMap *)data;
		for (i = 0; i < 256; i++)
			if (cm->buttons[i] != widget && GTK_TOGGLE_BUTTON(cm->buttons[i])->active)
				gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(cm->buttons[i]), FALSE);
		if (!GTK_TOGGLE_BUTTON(widget)->active)
			gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(widget), TRUE);
		cm->current_char = GTK_LABEL(GTK_BUTTON(widget)->child)->label[0];
		gtk_signal_emit(GTK_OBJECT(data), charmap_signals[CHAR_SELECTED]);
		in = FALSE;
	}
}


void charmap_set_font(CharMap *cm, GdkFont *font)
{
	GtkStyle *style;
	gint i;
	
	if (font == NULL)
		return;

	style = gtk_style_new();
	gdk_font_unref(style->font);
	style->font = font;
	gdk_font_ref(style->font);
	
	for (i = 0; i < 256; i++)
		gtk_widget_set_style(GTK_BUTTON(cm->buttons[i])->child, style);
	gtk_widget_queue_resize(GTK_WIDGET(cm)->parent);
}

/* vim: set ts=2 sw=2 tw=79 ai nowrap: */
