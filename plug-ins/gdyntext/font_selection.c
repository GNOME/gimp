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
#include <string.h>
#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include "libgimp/gimpintl.h"
#include "font_selection.h"


static void font_selection_class_init(FontSelectionClass *class);
static void font_selection_init(FontSelection *fs);
void on_font_selection_family_changed(GtkWidget *widget, gpointer data);
void on_font_selection_value_changed(GtkWidget *widget, gpointer data);


#define MAX_FONTS		32767


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
  GList *font_metric_items = NULL;
	GList *l = NULL;
	GList *plist = NULL;
	gchar **xfontnames, **font_info, font_style[1024];
	gint num_fonts, i;

	fs->font = GTK_WIDGET(fs)->style->font;

	/* get X font list and setup font names and styles */
#ifndef DEBUG_SPAM
/*
	// FIXME: non '-*' fonts, disabled because GIMP can't use them!!!
	xfontnames = XListFonts(GDK_DISPLAY(), "*", MAX_FONTS, &num_fonts);
*/
	xfontnames = XListFonts(GDK_DISPLAY(), "-*", MAX_FONTS, &num_fonts);
#else
	{
		FILE *f;
		xfontnames = g_new(char *, MAX_FONTS);
		f = fopen("spam_font.list", "r");
		if (f == NULL) {
			puts("Error opening font list");
			exit(1);
		}
		for (num_fonts = 0; !feof(f); num_fonts++) {
			xfontnames[num_fonts] = g_new0(char, 400);
			fscanf(f, "%[^\n]\n", xfontnames[num_fonts]);
			/*sprintf(xfontnames[num_fonts], "-*-%05d-*-*-*-*-*-*-*-*-*-*-*-*", num_fonts);*/
		}
		fclose(f);
		printf("NF: %d\n", num_fonts);
	}
#endif
	
#ifdef DEBUG_SPAM
	puts("FontSelection: START");
#endif
	if (num_fonts == MAX_FONTS)
		g_warning(_("MAX_FONTS exceeded. Some fonts may be missing."));
	fs->font_properties = g_hash_table_new(g_str_hash, g_str_equal);
	for (i = 0; i < num_fonts; i++) {
/*
		// FIXME: non '-*' fonts, disabled because GIMP can't use them!!!
		if (xfontnames[i][0] == '-') {
*/
			font_info = g_strsplit(xfontnames[i], "-", 20);
			if (g_hash_table_lookup(fs->font_properties, font_info[2]) == NULL)
				fs->font_names = g_list_insert_sorted(g_list_first(fs->font_names), g_strdup(font_info[2]), (GCompareFunc)strcmp);
			g_snprintf(font_style, sizeof(font_style), "%s-%s-%s", font_info[3], font_info[4], font_info[5]);
			l = NULL;
			if ((plist = (GList *)g_hash_table_lookup(fs->font_properties, font_info[2]))) {
				for (l = g_list_first(plist); l; l = g_list_next(l))
					if (g_str_equal((char *)l->data, font_style))
						break;
			}
			if (!l) {
				plist = g_list_insert_sorted(plist, g_strdup(font_style), (GCompareFunc)strcmp);
				g_hash_table_insert(fs->font_properties, g_strdup(font_info[2]), g_list_first(plist));
			}
			g_strfreev(font_info);
/*
		// FIXME: non '-*' fonts, disabled because GIMP can't use them!!!
		} else if (g_hash_table_lookup(fs->font_properties, xfontnames[i]) == NULL) {
			fs->font_names = g_list_insert_sorted(g_list_first(fs->font_names), g_strdup(xfontnames[i]), (GCompareFunc)strcmp);
			plist = g_list_insert_sorted(NULL, "", (GCompareFunc)strcmp);
			g_hash_table_insert(fs->font_properties, g_strdup(xfontnames[i]), g_list_first(plist));
		}
*/
	}
	XFreeFontNames(xfontnames);
#ifdef DEBUG_SPAM
	puts("FontSelection: DONE");
#endif

  fs->font_family = gtk_combo_new();
	gtk_combo_set_popdown_strings(GTK_COMBO(fs->font_family), g_list_first(fs->font_names));
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(fs->font_family)->entry), FALSE);
	gtk_combo_set_value_in_list(GTK_COMBO(fs->font_family), 0, FALSE);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(fs->font_family)->entry),
		"changed", GTK_SIGNAL_FUNC(on_font_selection_family_changed), fs);
  gtk_box_pack_start(GTK_BOX(fs), fs->font_family, TRUE, TRUE, 2);
  gtk_widget_show(fs->font_family);

  fs->font_style = gtk_combo_new();
	gtk_combo_set_popdown_strings(GTK_COMBO(fs->font_style),
		(GList *)g_hash_table_lookup(fs->font_properties, 
		gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(fs->font_family)->entry))));
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(fs->font_style)->entry), FALSE);
	gtk_widget_set_usize(GTK_COMBO(fs->font_style)->entry, 120, -1);
	gtk_combo_set_value_in_list(GTK_COMBO(fs->font_family), 0, FALSE);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(fs->font_style)->entry), "changed",
		GTK_SIGNAL_FUNC(on_font_selection_value_changed), fs);
  gtk_box_pack_start(GTK_BOX(fs), fs->font_style, TRUE, TRUE, 2);
  gtk_widget_show(fs->font_style);

  font_size_adj = gtk_adjustment_new(50, 1, 1000, 1, 10, 10);
  fs->font_size = gtk_spin_button_new(GTK_ADJUSTMENT(font_size_adj), 1, 0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(fs->font_size), TRUE);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(fs->font_size), GTK_UPDATE_ALWAYS);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(fs->font_size), 50);
	gtk_signal_connect(GTK_OBJECT(fs->font_size), "changed",
		GTK_SIGNAL_FUNC(on_font_selection_value_changed), fs);
  gtk_box_pack_start(GTK_BOX(fs), fs->font_size, TRUE, TRUE, 2);
  gtk_widget_show(fs->font_size);

  fs->font_metric = gtk_combo_new();
  font_metric_items = g_list_append(font_metric_items, _("pixels"));
  font_metric_items = g_list_append(font_metric_items, _("points"));
  gtk_combo_set_popdown_strings(GTK_COMBO(fs->font_metric), font_metric_items);
  g_list_free(font_metric_items);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(fs->font_metric)->entry), FALSE);
	gtk_widget_set_usize(GTK_COMBO(fs->font_metric)->entry, 40, -1);
	gtk_combo_set_value_in_list(GTK_COMBO(fs->font_metric), 0, FALSE);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(fs->font_metric)->entry), "changed",
		GTK_SIGNAL_FUNC(on_font_selection_value_changed), fs);
  gtk_box_pack_start(GTK_BOX(fs), fs->font_metric, TRUE, TRUE, 2);
  gtk_widget_show(fs->font_metric);

	on_font_selection_family_changed(NULL, fs);
}


GtkWidget* font_selection_new(void)
{
	return GTK_WIDGET(gtk_type_new(font_selection_get_type()));
}


void on_font_selection_family_changed(GtkWidget *widget, gpointer data)
{
	FontSelection *fs;

	fs = (FontSelection *)data;
	gtk_combo_set_popdown_strings(GTK_COMBO(fs->font_style),
		(GList *)g_hash_table_lookup(fs->font_properties, 
		gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(fs->font_family)->entry))));
}


void on_font_selection_value_changed(GtkWidget *widget, gpointer data)
{
	GdkFont *font;
	FontSelection *fs;
	gchar fontname[8192];

	fs = (FontSelection *)data;

	if (strcmp(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(fs->font_style)->entry)), "") == 0) {
		/* isn't a '-*' font */
		gtk_widget_set_sensitive(fs->font_style, FALSE);
		gtk_widget_set_sensitive(fs->font_size, FALSE);
		gtk_widget_set_sensitive(fs->font_metric, FALSE);
		g_snprintf(fontname, sizeof(fontname), "%s",
			gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(fs->font_family)->entry)));
	} else {
		/* is a '-*' font */
		gtk_widget_set_sensitive(fs->font_style, TRUE);
		gtk_widget_set_sensitive(fs->font_size, TRUE);
		gtk_widget_set_sensitive(fs->font_metric, TRUE);
		/* "-*-(fn)-(wg)-(sl)-(sp)-*-(px)-(po * 10)-*-*-*-*-*-*" */
		if (strcmp(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(fs->font_metric)->entry)), _("pixels")) == 0) {
			/* pixel size */
			g_snprintf(fontname, sizeof(fontname), "-*-%s-%s-*-%d-*-*-*-*-*-*-*",
				gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(fs->font_family)->entry)),
				gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(fs->font_style)->entry)),
				gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(fs->font_size)));
		} else {
			/* point size */
			g_snprintf(fontname, sizeof(fontname), "-*-%s-%s-*-*-%d-*-*-*-*-*-*",
				gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(fs->font_family)->entry)),
				gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(fs->font_style)->entry)),
				gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(fs->font_size)) * 10);
		}
	}
	gtk_entry_set_position(GTK_ENTRY(GTK_COMBO(fs->font_family)->entry), 0);
	gtk_entry_set_position(GTK_ENTRY(GTK_COMBO(fs->font_style)->entry), 0);
	gtk_entry_set_position(GTK_ENTRY(&GTK_SPIN_BUTTON(fs->font_size)->entry), 0);
	gtk_entry_set_position(GTK_ENTRY(GTK_COMBO(fs->font_metric)->entry), 0);

	gdk_error_warnings = 0;
	gdk_error_code = 0;
	font = gdk_font_load(fontname);
	if (!font || (gdk_error_code == -1)) {
#ifdef DEBUG
		printf("ERROR Loading font: %s\n", fontname);
#endif
		return;
	}
	fs->font = font;
#ifdef DEBUG
	printf("Loading font: %s\n", fontname);
#endif

	/* signal emit "font_changed" */
	gtk_signal_emit(GTK_OBJECT(data), font_selection_signals[FONT_CHANGED]);
}


void font_selection_set_font_family(FontSelection *fs, gchar *family)
{
	if (family == NULL || strlen(family) == 0)
		return;

	if (g_hash_table_lookup(fs->font_properties, family) == NULL) {
		printf(_("Unknown font family \"%s\".\n"), family);
		return;
	}
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(fs->font_family)->entry), family);
}


void font_selection_set_font_style(FontSelection *fs, const gchar *style)
{
	GList *l;
	gboolean exists = FALSE;
	gchar *family;

	if (style == NULL || strlen(style) == 0)
		return;

	family = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(fs->font_family)->entry));
	l = g_hash_table_lookup(fs->font_properties, family);
	while (l && !exists) {
		if (strcmp((char *)l->data, style) == 0)
			exists = TRUE;
		l = l->next;
	}
	if (!exists) {
		printf(_("Unknown font style \"%s\" for family \"%s\".\n"), style, family);
		return;
	}
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(fs->font_style)->entry), style);
}


void font_selection_set_font_size(FontSelection *fs, const gint size)
{
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(fs->font_size), size);
}


void font_selection_set_font_metric(FontSelection *fs, FontMetricType fm)
{
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(fs->font_metric)->entry),
		fm == FONT_METRIC_PIXELS ? _("pixels") : _("points"));
}


gchar *font_selection_get_font_family(FontSelection *fs)
{
	return gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(fs->font_family)->entry));
}


gchar * font_selection_get_font_style(FontSelection *fs)
{
	return gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(fs->font_style)->entry));
}


gint font_selection_get_font_size(FontSelection *fs)
{
	return gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(fs->font_size));
}


FontMetricType font_selection_get_font_metric(FontSelection *fs)
{
	return strcmp(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(fs->font_metric)->entry)), _("pixels")) == 0 ?
		FONT_METRIC_PIXELS : FONT_METRIC_POINTS;
}

/* vim: set ts=2 sw=2 tw=79 ai nowrap: */
