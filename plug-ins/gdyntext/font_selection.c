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
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "libgimp/stdplugins-intl.h"
#include "font_selection.h"


static void font_selection_class_init(FontSelectionClass *class);
static void font_selection_init(FontSelection *fs);
void on_font_selection_family_changed(GtkWidget *widget, gpointer data);
void on_font_selection_value_changed(GtkWidget *widget, gpointer data);
gboolean hash_entry_free(gpointer key, gpointer val, gpointer user_data);
void on_fs_family_changed(GtkWidget *widget, gint row, gint column,
	GdkEvent *event, gpointer data);
void on_fs_style_changed(GtkWidget *widget, gint row, gint column,
	GdkEvent *event, gpointer data);
void on_fs_size_changed(GtkWidget *widget, gpointer data);
void build_font_style_list(FontSelection *fs);


enum {
	FONT_CHANGED,
	LAST_SIGNAL
};


static gint font_selection_signals[LAST_SIGNAL] = { 0 };


guint font_selection_get_type(void)
{
	static guint fs_type = 0;

	if (!fs_type) {
		GtkTypeInfo fs_info = {
			"FontSelection",
			sizeof(FontSelection),
			sizeof(FontSelectionClass),
			(GtkClassInitFunc)font_selection_class_init,
			(GtkObjectInitFunc)font_selection_init,
			(GtkArgSetFunc)NULL,
			(GtkArgGetFunc)NULL,
#ifdef GTK_HAVE_FEATURES_1_1_12
			(GtkClassInitFunc)NULL,
#endif
		};
		fs_type = gtk_type_unique(gtk_hbox_get_type(), &fs_info);
	}
	return fs_type;
}


static void font_selection_class_init(FontSelectionClass *klass)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *)klass;

	font_selection_signals[FONT_CHANGED] = gtk_signal_new("font_changed",
		GTK_RUN_FIRST, object_class->type,
		GTK_SIGNAL_OFFSET(FontSelectionClass, font_changed),
		gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

	gtk_object_class_add_signals(object_class, font_selection_signals, LAST_SIGNAL);

	klass->font_changed = NULL;
}


static void font_selection_init(FontSelection *fs)
{
	GtkObject *font_size_adj;
	GList *list = NULL;
	gint pos;
	gchar *txt;

	fs->font_selection = gtk_font_selection_new();
#ifdef DEBUG_FONT_SELECTION
	/* FIXME: allow using the gtk_font_slector through a popup system! */
	gtk_box_pack_start(GTK_BOX(fs), fs->font_selection, TRUE, TRUE, 2);
  gtk_widget_show(fs->font_selection);
#endif

	fs->skip_fs_events = TRUE;
	gtk_signal_connect_after(
		GTK_OBJECT(GTK_FONT_SELECTION(fs->font_selection)->font_clist),
		"select_row", GTK_SIGNAL_FUNC(on_fs_family_changed), fs);
	gtk_signal_connect_after(
		GTK_OBJECT(GTK_FONT_SELECTION(fs->font_selection)->font_style_clist),
		"select_row", GTK_SIGNAL_FUNC(on_fs_style_changed), fs);
	gtk_signal_connect_after(
		GTK_OBJECT(GTK_FONT_SELECTION(fs->font_selection)->size_entry),
		"changed", GTK_SIGNAL_FUNC(on_fs_size_changed), fs);

	list = NULL;
	pos = GTK_CLIST(GTK_FONT_SELECTION(fs->font_selection)->font_clist)->rows;
	fs->font_names_hash = g_hash_table_new(g_str_hash, g_str_equal);
	while (pos-- > 0) {
		gtk_clist_get_text(GTK_CLIST(GTK_FONT_SELECTION(fs->font_selection)->font_clist), pos, 0, &txt);
		list = g_list_prepend(list, g_strdup(txt));
		g_hash_table_insert(fs->font_names_hash, g_strdup(txt), g_memdup(&pos, sizeof(gint)));
#ifdef DEBUG_FONT_SELECTION
		/*
		printf("FONT[%4d]: '%s'\n", pos, txt);
		*/
#endif
	}

  fs->font_family = gtk_combo_new();
	gtk_combo_set_popdown_strings(GTK_COMBO(fs->font_family), g_list_first(list));
	g_list_free(list);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(fs->font_family)->entry), FALSE);
	gtk_combo_set_value_in_list(GTK_COMBO(fs->font_family), 0, FALSE);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(fs->font_family)->entry),
		"changed", GTK_SIGNAL_FUNC(on_font_selection_family_changed), fs);
  gtk_box_pack_start(GTK_BOX(fs), fs->font_family, TRUE, TRUE, 2);
  gtk_widget_show(fs->font_family);

  fs->font_style = gtk_combo_new();
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(fs->font_style)->entry), FALSE);
	gtk_widget_set_usize(GTK_COMBO(fs->font_style)->entry, 120, -1);
	gtk_combo_set_value_in_list(GTK_COMBO(fs->font_family), 0, FALSE);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(fs->font_style)->entry), "changed",
		GTK_SIGNAL_FUNC(on_font_selection_value_changed), fs);
  gtk_box_pack_start(GTK_BOX(fs), fs->font_style, TRUE, TRUE, 2);
  gtk_widget_show(fs->font_style);

  font_size_adj = gtk_adjustment_new(14, 1, 1000, 1, 10, 10);
  fs->font_size = gtk_spin_button_new(GTK_ADJUSTMENT(font_size_adj), 1, 0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(fs->font_size), TRUE);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(fs->font_size), GTK_UPDATE_ALWAYS);
	gtk_signal_connect(GTK_OBJECT(fs->font_size), "changed",
		GTK_SIGNAL_FUNC(on_font_selection_value_changed), fs);
  gtk_box_pack_start(GTK_BOX(fs), fs->font_size, TRUE, TRUE, 2);
  gtk_widget_show(fs->font_size);

  fs->font_metric = gtk_combo_new();
	list = NULL;
  list = g_list_append(list, _("pixels"));
  list = g_list_append(list, _("points"));
  gtk_combo_set_popdown_strings(GTK_COMBO(fs->font_metric), list);
  g_list_free(list);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(fs->font_metric)->entry), FALSE);
	gtk_widget_set_usize(GTK_COMBO(fs->font_metric)->entry, 40, -1);
	gtk_combo_set_value_in_list(GTK_COMBO(fs->font_metric), 0, FALSE);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(fs->font_metric)->entry), "changed",
		GTK_SIGNAL_FUNC(on_font_selection_value_changed), fs);
  gtk_box_pack_start(GTK_BOX(fs), fs->font_metric, TRUE, TRUE, 2);
  gtk_widget_show(fs->font_metric);

	on_font_selection_family_changed(NULL, fs);
	on_font_selection_value_changed(NULL, fs);
}

void on_fs_family_changed(GtkWidget *widget, gint row, gint column,
	GdkEvent *event, gpointer data)
{
	FontSelection *fs;

	fs = FONT_SELECTION(data);

	if (fs->skip_fs_events)
		return;
	
#ifdef DEBUG_FONT_SELECTION
	printf("SELECT_FAMILY: %d\n", row);
#endif
	gtk_list_select_item(GTK_LIST(GTK_COMBO(fs->font_family)->list), row);
}

void on_fs_style_changed(GtkWidget *widget, gint row, gint column,
	GdkEvent *event, gpointer data)
{
	FontSelection *fs;

	fs = FONT_SELECTION(data);

	if (fs->skip_fs_events)
		return;

#ifdef DEBUG_FONT_SELECTION
	printf("SELECT_STYLE: %d\n", row);
#endif
	build_font_style_list(fs);
	gtk_list_select_item(GTK_LIST(GTK_COMBO(fs->font_style)->list), row);
}

void on_fs_size_changed(GtkWidget *widget, gpointer data)
{
	FontSelection *fs;

	fs = FONT_SELECTION(data);

	if (fs->skip_fs_events)
		return;

#ifdef DEBUG_FONT_SELECTION
	printf("SELECT_SIZE: '%s'\n", gtk_entry_get_text(GTK_ENTRY(widget)));
#endif
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(fs->font_size),
		atof(gtk_entry_get_text(GTK_ENTRY(widget))));
}

GtkWidget* font_selection_new(void)
{
	return GTK_WIDGET(gtk_type_new(font_selection_get_type()));
}


void on_font_selection_family_changed(GtkWidget *widget, gpointer data)
{
	FontSelection *fs = NULL;
	gint *pos = NULL;

	fs = FONT_SELECTION(data);
	
	if (!fs->skip_fs_events)
		return;

	pos = g_hash_table_lookup(fs->font_names_hash,
		gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(fs->font_family)->entry)));
	if (pos == NULL) {
		printf("ERR_on_font_selection_family_changed '%s'\n",
			gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(fs->font_family)->entry)));
		return;
	}

	gtk_clist_select_row(
		GTK_CLIST(GTK_FONT_SELECTION(fs->font_selection)->font_clist), *pos, 0);
	build_font_style_list(fs);
}

void build_font_style_list(FontSelection *fs)
{
	gint row, i;
	gchar *txt = NULL;
	gchar *pfx = NULL;
	GList *list = NULL;

	if (fs->font_styles_hash != NULL) {
		g_hash_table_foreach_remove(fs->font_styles_hash, hash_entry_free, NULL);
		g_hash_table_destroy(fs->font_styles_hash);
	}
	fs->font_styles_hash = g_hash_table_new(g_str_hash, g_str_equal);

	row = GTK_CLIST(GTK_FONT_SELECTION(
		fs->font_selection)->font_style_clist)->rows;
	for (i = 0; i < row; i++) {
		gtk_clist_get_text(GTK_CLIST(GTK_FONT_SELECTION(fs->font_selection)->font_style_clist), i, 0, &txt);
		if (strchr(txt, '-') != NULL) {
			pfx = txt;
		} else {
			if (pfx != NULL)
				txt = g_strdup_printf("%s (%s)", txt, pfx);

			g_hash_table_insert(fs->font_styles_hash, txt,
				g_memdup(&i, sizeof(gint)));
			list = g_list_append(list, txt);
#ifdef DEBUG_FONT_SELECTION
			/*
			printf("\t'%s'\n", txt);
			*/
#endif
		}
	}

	gtk_combo_set_popdown_strings(GTK_COMBO(fs->font_style), list);
	g_list_free(list);
}


void on_font_selection_value_changed(GtkWidget *widget, gpointer data)
{
	FontSelection *fs;

	fs = FONT_SELECTION(data);

	if (!fs->skip_fs_events)
		return;

	if (strcmp(gtk_entry_get_text(
		GTK_ENTRY(GTK_COMBO(fs->font_metric)->entry)), _("pixels")) == 0) {
		gtk_button_clicked(
			GTK_BUTTON(GTK_FONT_SELECTION(fs->font_selection)->pixels_button));
	} else {
		gtk_button_clicked(
			GTK_BUTTON(GTK_FONT_SELECTION(fs->font_selection)->points_button));
	}

	gtk_entry_set_text(
		GTK_ENTRY(GTK_FONT_SELECTION(fs->font_selection)->size_entry),
		gtk_entry_get_text(GTK_ENTRY(fs->font_size)));

	/* force update the gtk_font_selection font_size */
	{
		GdkEvent *event;

		event = g_new0(GdkEvent, 1);
		event->type = GDK_KEY_PRESS;
		event->key.keyval = GDK_Return;
		gtk_widget_event(GTK_FONT_SELECTION(fs->font_selection)->size_entry,
			event);
		g_free(event);
	}

	if (fs->font_styles_hash != NULL) {
		gint *pos;
	
		pos = g_hash_table_lookup(fs->font_styles_hash,
			gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(fs->font_style)->entry)));
		gtk_clist_select_row(
			GTK_CLIST(GTK_FONT_SELECTION(fs->font_selection)->font_style_clist),
			*pos, 0);
	}

#ifdef DEBUG_FONT_SELECTION
	printf("Loading font: %s\n",
		gtk_font_selection_get_font_name(GTK_FONT_SELECTION(fs->font_selection)));
#endif

	gtk_entry_set_position(GTK_ENTRY(GTK_COMBO(fs->font_family)->entry), 0);
	gtk_entry_set_position(GTK_ENTRY(GTK_COMBO(fs->font_style)->entry), 0);
	gtk_entry_set_position(GTK_ENTRY(&GTK_SPIN_BUTTON(fs->font_size)->entry), 0);
	gtk_entry_set_position(GTK_ENTRY(GTK_COMBO(fs->font_metric)->entry), 0);

	/* signal emit "font_changed" */
	gtk_signal_emit(GTK_OBJECT(data), font_selection_signals[FONT_CHANGED]);
}

GdkFont* font_selection_get_font(FontSelection *fs)
{
	return gtk_font_selection_get_font(GTK_FONT_SELECTION(fs->font_selection));
}

gchar* font_selection_get_font_name(FontSelection *fs)
{
	return gtk_font_selection_get_font_name(
		GTK_FONT_SELECTION(fs->font_selection));
}

gboolean font_selection_set_font_name(FontSelection *fs,
	const gchar *fontname)
{
	gboolean rval;

#ifdef DEBUG_FONT_SELECTION
	printf("Req. font: '%s'\n", fontname);
#endif
	fs->skip_fs_events = FALSE;
	rval = gtk_font_selection_set_font_name(
		GTK_FONT_SELECTION(fs->font_selection), fontname);
	fs->skip_fs_events = TRUE;

	/* signal emit "font_changed" */
	gtk_signal_emit(GTK_OBJECT(fs), font_selection_signals[FONT_CHANGED]);

	return rval;
}

gboolean hash_entry_free(gpointer key, gpointer val, gpointer user_data)
{
	g_free(val);
	return TRUE;
}

#ifdef DEBUG_FONT_SELECTION
int main(void)
{
	GtkWidget *window, *fs;

	gtk_init(NULL, NULL);
		
  window = gtk_window_new(GTK_WINDOW_DIALOG);
  gtk_container_border_width(GTK_CONTAINER(window), 4);
	gtk_signal_connect(GTK_OBJECT(window), "destroy",
		GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
	gtk_widget_show(window);

	fs = font_selection_new();
	gtk_widget_show(fs);
  gtk_container_add(GTK_CONTAINER(window), fs);

	font_selection_set_font_name(FONT_SELECTION(fs),
		"-unknown-helvetica-black-r-normal--24-*-75-75-p-74-adobe-fontspecific");

  gtk_main();

	return FALSE;
}
#endif

/* vim: set ts=2 sw=2 tw=79 ai nowrap: */
