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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"
#include "libgimp/stdplugins-intl.h"

#include "gdyntext.h"
#include "message_window.h"
#include "charmap_window.h"
#include "font_selection.h"

#include "graphics/antialias.xpm"
#include "graphics/align_left.xpm"
#include "graphics/align_center.xpm"
#include "graphics/align_right.xpm"
#include "graphics/charmap.xpm"
#include "graphics/font_preview.xpm"
#include "graphics/font_preview_clear.xpm"
#include "graphics/font_preview_default.xpm"
#include "graphics/gdyntext_logo.xpm"
#include "graphics/new_layer.xpm"
#include "graphics/text_load.xpm"
#include "graphics/layer_align_0.xpm"
#include "graphics/layer_align_1.xpm"
#include "graphics/layer_align_2.xpm"
#include "graphics/layer_align_3.xpm"
#include "graphics/layer_align_4.xpm"
#include "graphics/layer_align_5.xpm"
#include "graphics/layer_align_6.xpm"
#include "graphics/layer_align_7.xpm"
#include "graphics/layer_align_8.xpm"
#include "graphics/layer_align_9.xpm"


typedef struct {
	GtkWidget *window;
	GtkWidget *font_selection;
	GtkWidget *font_rotation;
	GtkWidget *line_spacing;
	GtkWidget *font_color;
	GtkWidget *textarea;
	GtkWidget *font_preview;
	GtkWidget *hbox_fp;
	GtkWidget *charmap_window_toggle;
	GtkWidget *new_layer_toggle;
	GtkWidget *statusbar;
	gint layer_alignment;
	gboolean font_preview_enabled;
	gboolean ok_pressed;
} GdtMainWindow;


GtkWidget *create_about_dialog(void);
GtkWidget *create_color_selection_dialog(void);
GdtMainWindow *create_main_window(GdtMainWindow **main_window, GdtVals *data);
GtkWidget *create_message_window(GtkWidget **mw);
void set_gdt_vals(GdtVals *data);
void gtk_text_set_font(GtkText *text, GdkFont *font);
void load_text(GtkWidget *widget, gpointer data);
void on_about_dialog_close(GtkWidget *widget, gpointer data);
void on_about_dialog_destroy(GtkWidget *widget, gpointer data);
void on_button_toggled(GtkWidget *widget, gpointer data);
void on_charmap_window_insert(GtkWidget *widget, gpointer data);
void on_charmap_button_toggled(GtkWidget *widget, gpointer data);
void on_color_selection_dialog_cancel_clicked(GtkWidget *widget, gpointer data);
void on_color_selection_dialog_ok_clicked(GtkWidget *widget, gpointer data);
void on_font_preview_button_clicked(GtkWidget *widget, gpointer data);
void on_font_preview_toggled(GtkWidget *widget, gpointer data);
void on_font_selection_changed(GtkWidget *widget, gpointer data);
void on_load_text_clicked(GtkWidget *widget, gpointer data);
void on_layer_align_change(GtkWidget *widget, gpointer data);
void on_main_window_about_clicked(GtkWidget *widget, gpointer data);
void on_main_window_align_c_clicked(GtkWidget *widget, gpointer data);
void on_main_window_align_l_clicked(GtkWidget *widget, gpointer data);
void on_main_window_align_r_clicked(GtkWidget *widget, gpointer data);
void on_main_window_apply_clicked(GtkWidget *widget, gpointer data);
void on_main_window_cancel_clicked(GtkWidget *widget, gpointer data);
void on_main_window_font_color_clicked(GtkWidget *widget, gpointer data);
void on_main_window_ok_press_event(GtkWidget *widget, GdkEvent *event, gpointer data);
void on_main_window_ok_clicked(GtkWidget *widget, gpointer data);
void on_window_close(GtkWidget *widget, gpointer data);
void on_window_destroy(GtkWidget *widget, gpointer data);
void toggle_button_update(GtkWidget *widget, GtkWidget *window);
void update_font_color_preview(void);


GdtMainWindow	*main_window = NULL;
GtkWidget			*message_window = NULL;
GtkWidget			*charmap_window = NULL;
GtkWidget			*color_selection_dialog = NULL;
GtkWidget			*about_dialog = NULL;
GtkWidget			*load_file_selection = NULL;
gdouble				col[3];


#define COLOR_PREVIEW_WIDTH		20
#define COLOR_PREVIEW_HEIGHT		20
#define DEFAULT_FONT_PREVIEW_TEXT	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"\
  					"abcdefghijklmnopqrstuvwxyz"\
					"0123456789"

#define TO_RGB(val) ( \
	((gint32)(val[0] * 255.0) << 16) + \
	((gint32)(val[1] * 255.0) << 8) + \
	(gint32)(val[2] * 255.0) \
)


GtkWidget *create_message_window(GtkWidget **mw)
{
	*mw = message_window_new(_("GDynText: Messages Window"));
	gtk_widget_set_usize(*mw, 430, 170);
	gtk_window_position(GTK_WINDOW(*mw), GTK_WIN_POS_CENTER);
	gtk_signal_connect(GTK_OBJECT(*mw), "destroy", GTK_SIGNAL_FUNC(on_window_destroy), mw);
	gtk_signal_connect(GTK_OBJECT(MESSAGE_WINDOW(*mw)->dismiss_button), "clicked", GTK_SIGNAL_FUNC(on_window_close), *mw);

	return *mw;
}


GtkWidget* create_about_dialog(void)
{
  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *label;
  GdkPixmap *glade_pixmap;
  GdkBitmap *glade_mask;
  GtkWidget *pixmap;
	GtkWidget *hbbox1;
	GtkWidget *hbox1;
	GtkWidget *vbox1;

  window = gtk_window_new(GTK_WINDOW_DIALOG);
  gtk_container_border_width(GTK_CONTAINER(window), 0);
  gtk_window_set_title(GTK_WINDOW(window), _("GDynText: About ..."));
  gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, FALSE);
  gtk_window_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_signal_connect(GTK_OBJECT(window), "destroy",
		GTK_SIGNAL_FUNC(on_about_dialog_destroy), NULL);
	/*
	gtk_widget_realize(window);
	gdk_window_set_decorations(window->window, !GDK_DECOR_ALL);
	*/

  hbox1 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox1);
  gtk_container_add(GTK_CONTAINER(window), hbox1);

  gtk_widget_realize(window);
  glade_pixmap = gdk_pixmap_create_from_xpm_d(window->window, &glade_mask,
		&window->style->bg[GTK_STATE_NORMAL], gdyntext_logo_xpm);
  pixmap = gtk_pixmap_new(glade_pixmap, glade_mask);
  gdk_pixmap_unref(glade_pixmap);
  gdk_bitmap_unref(glade_mask);
  gtk_widget_show(pixmap);
	gtk_box_pack_start(GTK_BOX(hbox1), pixmap, FALSE, TRUE, 0);

	vbox1 = gtk_vbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(hbox1), vbox1, TRUE, TRUE, 4);
	gtk_widget_show(vbox1);

	frame = gtk_frame_new(NULL);
  gtk_box_pack_start(GTK_BOX(vbox1), frame, TRUE, TRUE, 4);
	gtk_widget_show(frame);
	
  label = gtk_label_new ("GIMP Dynamic Text "GDYNTEXT_VERSION"\n"
			 "Copyright (C) 1998,1999,2000 Marco Lamberto\n"
			 "E-mail: lm@geocities.com\n"
			 "Web page: "GDYNTEXT_WEB_PAGE);
  gtk_widget_show(label);
  gtk_container_add(GTK_CONTAINER(frame), label);
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_padding(GTK_MISC(label), 5, 5);

  hbbox1 = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(vbox1), hbbox1, FALSE, FALSE, 4);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbbox1), GTK_BUTTONBOX_END);
  gtk_widget_show(hbbox1);

  button = gtk_button_new_with_label(_("OK"));
  gtk_widget_show(button);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_box_pack_end(GTK_BOX(hbbox1), button, FALSE, TRUE, 4);
  gtk_widget_grab_default(button);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
		GTK_SIGNAL_FUNC(on_about_dialog_close), NULL);

  return window;
}


GtkWidget* create_color_selection_dialog(void)
{
  GtkWidget *colseldlg;
  GtkWidget *ok_button1;
  GtkWidget *cancel_button1;

  colseldlg = gtk_color_selection_dialog_new(_("GDynText: Select Color"));
  gtk_container_border_width(GTK_CONTAINER(colseldlg), 4);
	gtk_signal_connect(GTK_OBJECT(&(GTK_COLOR_SELECTION_DIALOG(colseldlg)->window)), "destroy",
		GTK_SIGNAL_FUNC(on_color_selection_dialog_cancel_clicked), &color_selection_dialog);

  ok_button1 = GTK_COLOR_SELECTION_DIALOG(colseldlg)->ok_button;
  gtk_widget_show(ok_button1);
  GTK_WIDGET_SET_FLAGS(ok_button1, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(ok_button1), "clicked",
		GTK_SIGNAL_FUNC(on_color_selection_dialog_ok_clicked), NULL);

  cancel_button1 = GTK_COLOR_SELECTION_DIALOG(colseldlg)->cancel_button;
  gtk_widget_show(cancel_button1);
  GTK_WIDGET_SET_FLAGS(cancel_button1, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(cancel_button1), "clicked",
		GTK_SIGNAL_FUNC(on_color_selection_dialog_cancel_clicked), NULL);

  gtk_widget_hide(GTK_COLOR_SELECTION_DIALOG(colseldlg)->help_button);

  return colseldlg;
}


GdtMainWindow *create_main_window(GdtMainWindow **main_window, GdtVals *data)
{
  GdtMainWindow *mw;
  GtkObject *font_size_adj;
  GtkObject *line_spacing_adj;
  GtkTooltips *tooltips;
  GtkWidget *handlebox;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *hbox1;
  GtkWidget *hbox2;
  GtkWidget *hbox3;
  GtkWidget *hbox4;
  GtkWidget *hbbox1;
  GtkWidget *hbbox2;
  GtkWidget *label;
  GtkWidget *toolbar;
  GtkWidget *button_about;
  GtkWidget *button_ok;
  GtkWidget *button_cancel;
  GtkWidget *button_apply;
  GtkWidget *font_preview_toggle;
  GtkWidget *vscrollbar;
  GtkWidget *rbutt;
  GtkWidget *telem;
  GtkWidget *gtk_icon;
  GtkWidget *optmenu;
  GtkWidget *menu;
  GtkWidget *item;
  GdkPixmap *icon;
  GdkBitmap *mask;
  GdkColor *transparent = NULL;
  GSList *group;
  gint i;
  gchar *title;
  gchar *lalign_menu[] = 
  {
    (gchar *)layer_align_0_xpm,	N_("none"),
    (gchar *)layer_align_1_xpm,	N_("bottom-left"),
    (gchar *)layer_align_2_xpm,	N_("bottom-center"),
    (gchar *)layer_align_3_xpm,	N_("bottom-right"),
    (gchar *)layer_align_4_xpm,	N_("middle-left"),
    (gchar *)layer_align_5_xpm,	N_("center"),
    (gchar *)layer_align_6_xpm,	N_("middle-right"),
    (gchar *)layer_align_7_xpm,	N_("top-left"),
    (gchar *)layer_align_8_xpm,	N_("top-center"),
    (gchar *)layer_align_9_xpm,	N_("top-right"),
    NULL,	NULL,
  };

  *main_window = mw = g_new0(GdtMainWindow, 1);
  mw->font_preview_enabled = FALSE;
  mw->ok_pressed = FALSE;
  
  tooltips = gtk_tooltips_new();

  mw->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  title = g_strconcat (_("GDynText"), " ", GDYNTEXT_VERSION, NULL);
  gtk_window_set_title(GTK_WINDOW(mw->window), title);
  g_free (title);
  gtk_window_set_policy(GTK_WINDOW(mw->window), TRUE, TRUE, FALSE);
	gtk_widget_set_usize(mw->window, 550, 400);
  gtk_container_border_width(GTK_CONTAINER(mw->window), 0);
	gtk_signal_connect(GTK_OBJECT(mw->window), "destroy",
		GTK_SIGNAL_FUNC(on_main_window_cancel_clicked), &mw->ok_pressed);
  gtk_widget_realize(mw->window);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(mw->window), vbox);
  gtk_widget_show(vbox);

	handlebox = gtk_handle_box_new();
  gtk_box_pack_start(GTK_BOX(vbox), handlebox, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(handlebox), 4);
	gtk_widget_show(handlebox);
		
  hbox1 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox1);
	gtk_container_add(GTK_CONTAINER(handlebox), hbox1);

  toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_space_size(GTK_TOOLBAR(toolbar), 8);
	gtk_toolbar_set_button_relief(GTK_TOOLBAR(toolbar), GTK_RELIEF_NONE);
  gtk_box_pack_start(GTK_BOX(hbox1), toolbar, FALSE, FALSE, 2);
  gtk_widget_show(toolbar);
	
	/* NEW LAYER Toggle */
	icon = gdk_pixmap_create_from_xpm_d(mw->window->window, &mask, transparent, new_layer_xpm);
	gtk_icon = gtk_pixmap_new(icon, mask);
	mw->new_layer_toggle = telem = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL,
		NULL, _("Toggle creation of a new layer"), NULL, gtk_icon, NULL, NULL);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(telem), data->new_layer);
	gtk_signal_connect(GTK_OBJECT(telem), "toggled", GTK_SIGNAL_FUNC(on_button_toggled), &data->new_layer);
	gtk_widget_set_sensitive(telem, !data->new_layer);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	/* TEXT LOAD */
	icon = gdk_pixmap_create_from_xpm_d(mw->window->window, &mask, transparent, text_load_xpm);
	gtk_icon = gtk_pixmap_new(icon, mask);
	telem = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON, NULL,
		NULL, _("Load text from file"), NULL, gtk_icon, NULL, NULL);
	gtk_signal_connect(GTK_OBJECT(telem), "clicked", GTK_SIGNAL_FUNC(on_load_text_clicked), NULL);
	
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	
	/* FONT COLOR */
	mw->font_color = gtk_preview_new(GTK_PREVIEW_COLOR);
	gtk_preview_size(GTK_PREVIEW(mw->font_color), COLOR_PREVIEW_WIDTH, COLOR_PREVIEW_HEIGHT);
	col[0] = (gdouble)((data->color & 0xff0000) >> 16) / 255.0;
	col[1] = (gdouble)((data->color & 0xff00) >> 8) / 255.0;
	col[2] = (gdouble)(data->color & 0xff) / 255.0;
	update_font_color_preview();
	gtk_widget_show(mw->font_color);
	gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON, NULL,
		NULL, _("Text color"), NULL, mw->font_color,
		GTK_SIGNAL_FUNC(on_main_window_font_color_clicked), NULL);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	
	/* ANTIALIASING Toggle */
	icon = gdk_pixmap_create_from_xpm_d(mw->window->window, &mask, transparent, antialias_xpm);
	gtk_icon = gtk_pixmap_new(icon, mask);
	telem = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL,
		NULL, _("Toggle anti-aliased text"), NULL, gtk_icon, NULL, NULL);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(telem), data->antialias);
	gtk_signal_connect(GTK_OBJECT(telem), "clicked", GTK_SIGNAL_FUNC(on_button_toggled), &data->antialias);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	
	/* LEFT Align */
	icon = gdk_pixmap_create_from_xpm_d(mw->window->window, &mask, &toolbar->style->bg[GTK_STATE_NORMAL], align_left_xpm);
	gtk_icon = gtk_pixmap_new(icon, mask);
	rbutt = gtk_radio_button_new(NULL);
	telem = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_RADIOBUTTON, rbutt,
		NULL, _("Left aligned text"), NULL, gtk_icon, GTK_SIGNAL_FUNC(on_main_window_align_l_clicked), &data->alignment);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(telem), data->alignment == LEFT);
	
	/* CENTER Align */
	icon = gdk_pixmap_create_from_xpm_d(mw->window->window, &mask, &toolbar->style->bg[GTK_STATE_NORMAL], align_center_xpm);
	gtk_icon = gtk_pixmap_new(icon, mask);
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(rbutt));
	rbutt = gtk_radio_button_new(group);
	telem = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_RADIOBUTTON, rbutt,
		NULL, _("Centered text"), NULL, gtk_icon, GTK_SIGNAL_FUNC(on_main_window_align_c_clicked), &data->alignment);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(telem), data->alignment == CENTER);
	
	/* RIGHT Align */
	icon = gdk_pixmap_create_from_xpm_d(mw->window->window, &mask, &toolbar->style->bg[GTK_STATE_NORMAL], align_right_xpm);
	gtk_icon = gtk_pixmap_new(icon, mask);
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(rbutt));
	rbutt = gtk_radio_button_new(group);
	telem = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_RADIOBUTTON, rbutt,
		NULL, _("Right aligned text"), NULL, gtk_icon, GTK_SIGNAL_FUNC(on_main_window_align_r_clicked), &data->alignment);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(telem), data->alignment == RIGHT);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	
	/* FONT Preview Toggle */
	icon = gdk_pixmap_create_from_xpm_d(mw->window->window, &mask, transparent, font_preview_xpm);
	gtk_icon = gtk_pixmap_new(icon, mask);
	font_preview_toggle = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL,
		NULL, _("Toggle text font preview"), NULL, gtk_icon, NULL, NULL);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(font_preview_toggle), FALSE);
	gtk_signal_connect(GTK_OBJECT(font_preview_toggle), "toggled", GTK_SIGNAL_FUNC(on_font_preview_toggled), NULL);

	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	
	/* CHARMAP Window Toggle */
	icon = gdk_pixmap_create_from_xpm_d(mw->window->window, &mask, transparent, charmap_xpm);
	gtk_icon = gtk_pixmap_new(icon, mask);
	mw->charmap_window_toggle = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL,
		NULL, _("Toggle CharMap window"), NULL, gtk_icon,
		GTK_SIGNAL_FUNC(on_charmap_button_toggled), &mw->textarea);

	handlebox = gtk_handle_box_new();
  gtk_box_pack_start(GTK_BOX(vbox), handlebox, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(handlebox), 4);
	gtk_widget_show(handlebox);

  hbox1 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox1);
	gtk_container_add(GTK_CONTAINER(handlebox), hbox1);

	/* Layer Alignment */
	label = gtk_label_new(_("Layer\nAlignment"));
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start(GTK_BOX(hbox1), label, FALSE, TRUE, 3);
	gtk_widget_show(label);

	optmenu = gtk_option_menu_new();
  gtk_box_pack_start(GTK_BOX(hbox1), optmenu, FALSE, TRUE, 3);
	gtk_widget_show(optmenu);
	gtk_tooltips_set_tip(tooltips, optmenu, _("Set layer alignment"), "");

	menu = gtk_menu_new();
	for (i = 0; lalign_menu[i] != NULL; i += 2) {
		item = gtk_menu_item_new();
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(menu), item);
		gtk_signal_connect(GTK_OBJECT(item), "activate",
			GTK_SIGNAL_FUNC(on_layer_align_change), (void *)(i >> 1));
		if ((i >> 1) == data->layer_alignment)
			mw->layer_alignment = data->layer_alignment;

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_widget_show(hbox);
		gtk_container_add(GTK_CONTAINER(item), hbox);

		icon = gdk_pixmap_create_from_xpm_d(mw->window->window, &mask,
			transparent, (gchar **)lalign_menu[i]);
		gtk_icon = gtk_pixmap_new(icon, mask);
		gtk_widget_show(gtk_icon);
		gtk_box_pack_start(GTK_BOX(hbox), gtk_icon, FALSE, FALSE, 2);

		label = gtk_label_new (gettext (lalign_menu[i + 1]));
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	}
	gtk_option_menu_set_menu(GTK_OPTION_MENU(optmenu), menu);
	gtk_option_menu_set_history(GTK_OPTION_MENU(optmenu), mw->layer_alignment);

	label = gtk_label_new(_("Line\nSpacing"));
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start(GTK_BOX(hbox1), label, FALSE, TRUE, 3);
  gtk_widget_show(label);

  line_spacing_adj = gtk_adjustment_new(0, -1000, 1000, 1, 15, 15);
  mw->line_spacing = gtk_spin_button_new(GTK_ADJUSTMENT(line_spacing_adj), 1, 0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(mw->line_spacing), TRUE);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(mw->line_spacing), GTK_UPDATE_ALWAYS);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(mw->line_spacing), data->line_spacing);
	gtk_tooltips_set_tip(tooltips, mw->line_spacing, _("Set line spacing"), "");
  gtk_box_pack_start(GTK_BOX(hbox1), mw->line_spacing, FALSE, TRUE, 2);
  gtk_widget_show(mw->line_spacing);

	label = gtk_label_new(_("Rotation"));
  gtk_box_pack_start(GTK_BOX(hbox1), label, FALSE, TRUE, 3);
  gtk_widget_show(label);

  font_size_adj = gtk_adjustment_new(0, -360, 360, 1, 15, 15);
  mw->font_rotation = gtk_spin_button_new(GTK_ADJUSTMENT(font_size_adj), 1, 0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(mw->font_rotation), TRUE);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(mw->font_rotation), GTK_UPDATE_ALWAYS);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(mw->font_rotation), data->rotation);
	gtk_tooltips_set_tip(tooltips, mw->font_rotation, _("Set text rotation (degrees)"), "");
  gtk_box_pack_start(GTK_BOX(hbox1), mw->font_rotation, FALSE, TRUE, 2);
  gtk_widget_show(mw->font_rotation);
	
	/* Font Selection */
	handlebox = gtk_handle_box_new();
  gtk_box_pack_start(GTK_BOX(vbox), handlebox, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(handlebox), 4);
	gtk_widget_show(handlebox);

  hbox2 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox2);
	gtk_container_add(GTK_CONTAINER(handlebox), hbox2);

	mw->font_selection = font_selection_new();
	gtk_signal_connect(GTK_OBJECT(mw->font_selection), "font_changed",
		GTK_SIGNAL_FUNC(on_font_selection_changed), NULL);
  gtk_box_pack_start(GTK_BOX(hbox2), mw->font_selection, TRUE, TRUE, 2);
	gtk_widget_show(mw->font_selection);

	handlebox = gtk_handle_box_new();
  gtk_box_pack_start(GTK_BOX(vbox), handlebox, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(handlebox), 4);
	gtk_widget_show(handlebox);

	mw->hbox_fp = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(mw->hbox_fp);
	gtk_container_add(GTK_CONTAINER(handlebox), mw->hbox_fp);
	gtk_widget_set_sensitive(mw->hbox_fp, FALSE);

	mw->font_preview = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(mw->font_preview), DEFAULT_FONT_PREVIEW_TEXT);
	gtk_tooltips_set_tip(tooltips, mw->font_preview, _("Editable text sample"), NULL);
	gtk_box_pack_start(GTK_BOX(mw->hbox_fp), mw->font_preview, TRUE, TRUE, 5);
	gtk_widget_show(mw->font_preview);

	toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_space_size(GTK_TOOLBAR(toolbar), 0);
	gtk_toolbar_set_button_relief(GTK_TOOLBAR(toolbar), GTK_RELIEF_NONE);
	gtk_widget_show(toolbar);
	gtk_box_pack_start(GTK_BOX(mw->hbox_fp), toolbar, FALSE, TRUE, 0);

	icon = gdk_pixmap_create_from_xpm_d(mw->window->window, &mask, transparent, font_preview_clear_xpm);
	gtk_icon = gtk_pixmap_new(icon, mask);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL,
		_("Clear preview"), NULL, gtk_icon,
		GTK_SIGNAL_FUNC(on_font_preview_button_clicked), "");

	icon = gdk_pixmap_create_from_xpm_d(mw->window->window, &mask, transparent, font_preview_default_xpm);
	gtk_icon = gtk_pixmap_new(icon, mask);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL,
		_("Preview default text sample"), NULL, gtk_icon,
		GTK_SIGNAL_FUNC(on_font_preview_button_clicked), DEFAULT_FONT_PREVIEW_TEXT);

	hbox4 = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(vbox), hbox4, TRUE, TRUE, 2);
  gtk_widget_show(hbox4);

  mw->textarea = gtk_text_new(NULL, NULL);
  gtk_text_set_editable(GTK_TEXT(mw->textarea), TRUE);
  gtk_box_pack_start(GTK_BOX(hbox4), mw->textarea, TRUE, TRUE, 0);
  gtk_widget_show(mw->textarea);
	gtk_widget_realize(mw->textarea);
	gtk_text_insert(GTK_TEXT(mw->textarea), NULL, NULL, NULL, data->text, -1);
	gtk_text_set_point(GTK_TEXT(mw->textarea), 0);

	vscrollbar = gtk_vscrollbar_new(GTK_TEXT(mw->textarea)->vadj);
  gtk_box_pack_start(GTK_BOX(hbox4), vscrollbar, FALSE, TRUE, 0);
	gtk_widget_show(vscrollbar);

  hbox3 = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox3, FALSE, TRUE, 2);
  gtk_widget_show(hbox3);

  hbbox1 = gtk_hbutton_box_new();
  gtk_box_pack_start(GTK_BOX(hbox3), hbbox1, FALSE, FALSE, 2);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbbox1), GTK_BUTTONBOX_START);
  gtk_widget_show(hbbox1);

  button_about = gtk_button_new_with_label(_("About"));
  GTK_WIDGET_SET_FLAGS(button_about, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(button_about), "clicked",
		GTK_SIGNAL_FUNC(on_main_window_about_clicked), NULL);
  gtk_box_pack_start(GTK_BOX(hbbox1), button_about, FALSE, FALSE, 0);
  gtk_widget_show(button_about);

  hbbox2 = gtk_hbutton_box_new();
  gtk_box_pack_end(GTK_BOX(hbox3), hbbox2, FALSE, FALSE, 2);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbbox2), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbbox2), 4);
  gtk_widget_show(hbbox2);

  button_ok = gtk_button_new_with_label(_("OK"));
  GTK_WIDGET_SET_FLAGS(button_ok, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(button_ok), "button_press_event",
		GTK_SIGNAL_FUNC(on_main_window_ok_press_event), data);
	gtk_signal_connect(GTK_OBJECT(button_ok), "clicked",
		GTK_SIGNAL_FUNC(on_main_window_ok_clicked), &mw->ok_pressed);
  gtk_box_pack_start(GTK_BOX(hbbox2), button_ok, FALSE, FALSE, 0);
  gtk_widget_show(button_ok);

	gtk_tooltips_set_tip(tooltips, button_ok,
		_("Holding the Shift key while pressing this button will force GDynText "
		  "in changing the layer name as done in GIMP 1.0."), NULL);

  button_apply = gtk_button_new_with_label(_("Apply"));
  GTK_WIDGET_SET_FLAGS(button_apply, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(button_apply), "clicked",
		GTK_SIGNAL_FUNC(on_main_window_apply_clicked), data);
  gtk_box_pack_start(GTK_BOX(hbbox2), button_apply, FALSE, FALSE, 0);
  gtk_widget_show(button_apply);

  button_cancel = gtk_button_new_with_label(_("Close"));
  GTK_WIDGET_SET_FLAGS(button_cancel, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(button_cancel), "clicked",
		GTK_SIGNAL_FUNC(on_main_window_cancel_clicked), &mw->ok_pressed);
  gtk_box_pack_start(GTK_BOX(hbbox2), button_cancel, FALSE, FALSE, 0);
  gtk_widget_show(button_cancel);

	mw->statusbar = gtk_statusbar_new();
  gtk_box_pack_start(GTK_BOX(vbox), mw->statusbar, FALSE, TRUE, 0);
	gtk_widget_show(mw->statusbar);

  gtk_widget_grab_default(button_ok);

	/* setup font preview */
	if (data->preview) {
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(font_preview_toggle), TRUE);
		gtk_toggle_button_toggled(GTK_TOGGLE_BUTTON(font_preview_toggle));
	}

	font_selection_set_font_name(FONT_SELECTION(mw->font_selection), data->xlfd);

  return mw;
}


gboolean gdt_create_ui(GdtVals *data)
{
	gimp_ui_init ("gdyntext", TRUE);

	create_message_window(&message_window);

	/*
	if (data->messages) {
		GList *l;

		for (l = g_list_first(data->messages); l; l = l->next)
			message_window_append(MESSAGE_WINDOW(message_window), (char *)l->data);
	}
	*/
  main_window = create_main_window(&main_window, data);
	if (data->messages) {
		GList *l;

		for (l = g_list_first(data->messages); l; l = l->next)
			gtk_statusbar_push(GTK_STATUSBAR(main_window->statusbar), 1,
				(char *)l->data);
	}

  gtk_widget_show(main_window->window);
	if (MESSAGE_WINDOW(message_window)->contains_messages)
		gtk_widget_show(message_window);

  gtk_main();

	if (main_window->ok_pressed)
		set_gdt_vals(data);
	return main_window->ok_pressed;
}


void set_gdt_vals(GdtVals *data) {
	data->preview = main_window->font_preview_enabled;
	strncpy(data->xlfd,
		font_selection_get_font_name(FONT_SELECTION(main_window->font_selection)),
		sizeof(data->xlfd));
	data->rotation = gtk_spin_button_get_value_as_int(
		GTK_SPIN_BUTTON(main_window->font_rotation));
	data->line_spacing = gtk_spin_button_get_value_as_int(
		GTK_SPIN_BUTTON(main_window->line_spacing));
	data->layer_alignment = main_window->layer_alignment;
	strncpy(data->text, gtk_editable_get_chars(
		GTK_EDITABLE(main_window->textarea), 0, -1), sizeof(data->text));
	data->color = TO_RGB(col);
}


void update_font_color_preview(void)
{
	guchar row[COLOR_PREVIEW_WIDTH * 3];
	guchar *p;
	int i;

	memset(row, 0, sizeof(row));
	gtk_preview_draw_row(GTK_PREVIEW(main_window->font_color), row, 0, 0, COLOR_PREVIEW_WIDTH);
	gtk_preview_draw_row(GTK_PREVIEW(main_window->font_color), row, 0, COLOR_PREVIEW_HEIGHT, COLOR_PREVIEW_WIDTH);
	p = row;
	p[0] = p[1] = p[2] = 0;
	p += 3;
	for (i = 1; i < COLOR_PREVIEW_WIDTH - 1; i++) {
		p[0] = col[0] * 255;
		p[1] = col[1] * 255;
		p[2] = col[2] * 255;
		p += 3;
	}
	p[0] = p[1] = p[2] = 0;
	for (i = 1; i < COLOR_PREVIEW_HEIGHT - 1; i++)
		gtk_preview_draw_row(GTK_PREVIEW(main_window->font_color), row, 0, i, COLOR_PREVIEW_WIDTH);
	gtk_widget_draw(main_window->font_color, NULL);
	gdk_flush();
}


void on_main_window_apply_clicked(GtkWidget *widget, gpointer data0)
{
	GdtVals *data = (GdtVals *)data0;

	set_gdt_vals(data);
	gdt_render_text_p(data, FALSE);
	gimp_set_data("plug_in_gdyntext", data, sizeof(GdtVals));
	if (data->new_layer) {
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(main_window->new_layer_toggle), FALSE); 
		if (!GTK_WIDGET_SENSITIVE(main_window->new_layer_toggle))
			gtk_widget_set_sensitive(main_window->new_layer_toggle, TRUE);
	}
}


void on_main_window_cancel_clicked(GtkWidget *widget, gpointer data)
{
	*(gboolean *)data = FALSE;
	gtk_main_quit();
}


void on_main_window_ok_press_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	/* holding the SHIFT while clicking on OK will force layer name change */
	((GdtVals *)data)->change_layer_name = (event->button.state & GDK_SHIFT_MASK);
}


void on_main_window_ok_clicked(GtkWidget *widget, gpointer data)
{
	*(gboolean *)data = TRUE;
	gtk_widget_hide(main_window->window);
	if (charmap_window)
		gtk_widget_hide(charmap_window);
	if (message_window)
		gtk_widget_hide(message_window);
	if (color_selection_dialog)
		gtk_widget_hide(color_selection_dialog);
	if (about_dialog)
		gtk_widget_hide(about_dialog);
	gtk_main_quit();
}


void on_main_window_about_clicked(GtkWidget *widget, gpointer data)
{
	if (about_dialog == NULL)
		about_dialog = create_about_dialog();
	gtk_widget_show(about_dialog);
}


void on_about_dialog_close(GtkWidget *widget, gpointer data)
{
	gtk_widget_hide(about_dialog);
}


void on_about_dialog_destroy(GtkWidget *widget, gpointer data)
{
	about_dialog = NULL;
}


void on_main_window_font_color_clicked(GtkWidget *widget, gpointer data)
{
	if (color_selection_dialog == NULL)
		color_selection_dialog = create_color_selection_dialog();
	if (GTK_WIDGET_VISIBLE(color_selection_dialog))
		return;
	/* set color twice for current and old */
	gtk_color_selection_set_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(color_selection_dialog)->colorsel), col);
	gtk_color_selection_set_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(color_selection_dialog)->colorsel), col);
	gtk_widget_show(color_selection_dialog);
}


void on_color_selection_dialog_ok_clicked(GtkWidget *widget, gpointer data)
{
	gtk_widget_hide(color_selection_dialog);
	gtk_color_selection_get_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(color_selection_dialog)->colorsel), col);
	update_font_color_preview();
}


void on_color_selection_dialog_cancel_clicked(GtkWidget *widget, gpointer data)
{
	gtk_widget_hide(color_selection_dialog);
	if ((GtkWidget *)data != NULL)	/* used for catching the destroy signal */
		color_selection_dialog = NULL;
}


void on_main_window_align_l_clicked(GtkWidget *widget, gpointer data)
{
	*(GdtAlign *)data = LEFT;
}


void on_main_window_align_c_clicked(GtkWidget *widget, gpointer data)
{
	*(GdtAlign *)data = CENTER;
}


void on_main_window_align_r_clicked(GtkWidget *widget, gpointer data)
{
	*(GdtAlign *)data = RIGHT;
}


void on_font_preview_toggled(GtkWidget *widget, gpointer data)
{
	main_window->font_preview_enabled = GTK_TOGGLE_BUTTON(widget)->active;
	on_font_selection_changed(widget, data);
}


void gtk_text_set_font(GtkText *text, GdkFont *font)
{
	GtkStyle *style;
	char *chars;
	int pos;
	
	gtk_text_freeze(text);
	pos = GTK_EDITABLE(main_window->textarea)->current_pos;
	chars = gtk_editable_get_chars(GTK_EDITABLE(text), 0, -1);
	gtk_editable_delete_text(GTK_EDITABLE(text), 0, -1);
	style = gtk_style_new();
	if (font) {
		gdk_font_unref(style->font);
		style->font = font;
		gdk_font_ref(style->font);
	}
	if (strlen(chars) > 0)
		gtk_text_insert(text, style->font, NULL, NULL, chars, -1);
	else {
		gtk_text_insert(text, style->font, NULL, NULL, " ", -1);
		gtk_editable_delete_text(GTK_EDITABLE(text), 0, -1);
	}
	gtk_widget_set_style(GTK_WIDGET(text), style);
	gtk_text_set_point(GTK_TEXT(main_window->textarea), pos);
	gtk_text_thaw(text);
}


void on_font_selection_changed(GtkWidget *widget, gpointer data)
{
	GdkFont *font;
	GtkStyle *style;


	font = font_selection_get_font(FONT_SELECTION(main_window->font_selection));

	/* update the font preview */
	gtk_widget_set_sensitive(main_window->hbox_fp,
		main_window->font_preview_enabled);
	
	style = gtk_style_new();
	if (main_window->font_preview_enabled && font != NULL) {
		gdk_font_unref(style->font);
		style->font = font;
		gdk_font_ref(style->font);
	} else {
		font = style->font;
	}
	gtk_widget_set_style(main_window->font_preview, style);
	gtk_entry_set_position(GTK_ENTRY(main_window->font_preview), 0);

	/* update also the text area font */
	gtk_text_set_font(GTK_TEXT(main_window->textarea), font);

	if (charmap_window != NULL && GTK_WIDGET_VISIBLE(charmap_window))
		charmap_set_font(CHARMAP(CHARMAP_WINDOW(charmap_window)->charmap), font);
}


void on_font_preview_button_clicked(GtkWidget *widget, gpointer data)
{
	gtk_entry_set_text(GTK_ENTRY(main_window->font_preview), (char *)data);
}


void on_button_toggled(GtkWidget *widget, gpointer data)
{
	*(gboolean *)data = !*(gboolean *)data;
}


void on_charmap_button_toggled(GtkWidget *widget, gpointer data)
{
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		if (charmap_window == NULL) {
			charmap_window = charmap_window_new(_("GDynText: CharMap"));
			gtk_widget_set_usize(charmap_window, 430, 270);
			gtk_window_position(GTK_WINDOW(charmap_window), GTK_WIN_POS_CENTER);
			gtk_signal_connect(GTK_OBJECT(charmap_window), "destroy", GTK_SIGNAL_FUNC(on_window_destroy), &charmap_window);
			gtk_signal_connect(GTK_OBJECT(CHARMAP_WINDOW(charmap_window)->close_button), "clicked", GTK_SIGNAL_FUNC(on_window_close), charmap_window);
			gtk_signal_connect(GTK_OBJECT(CHARMAP_WINDOW(charmap_window)->insert_button), "clicked", GTK_SIGNAL_FUNC(on_charmap_window_insert), charmap_window);
		} else if (GTK_WIDGET_VISIBLE(charmap_window))
			return;
		charmap_set_font(CHARMAP(CHARMAP_WINDOW(charmap_window)->charmap),
			font_selection_get_font(FONT_SELECTION(main_window->font_selection)));
		gtk_widget_show(charmap_window);
	} else if (charmap_window && GTK_WIDGET_VISIBLE(charmap_window))
		gtk_widget_hide(charmap_window);
}


void toggle_button_update(GtkWidget *widget, GtkWidget *window)
{
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(widget), 
		window ? GTK_WIDGET_VISIBLE(window) : FALSE);
}


void on_window_close(GtkWidget *widget, gpointer data)
{
	gtk_widget_hide((GtkWidget *)data);
	toggle_button_update(main_window->charmap_window_toggle, charmap_window);
}


void on_window_destroy(GtkWidget *widget, gpointer data)
{
	gtk_widget_hide(*(GtkWidget **)data);
	*(GtkWidget **)data = NULL;
	toggle_button_update(main_window->charmap_window_toggle, charmap_window);
}


void on_charmap_window_insert(GtkWidget *widget, gpointer data)
{
	gchar *lab;

	gtk_label_get(GTK_LABEL(((CharMapWindow *)data)->label), &lab);
	lab[1] = 0;
	gtk_text_set_point(GTK_TEXT(main_window->textarea), GTK_EDITABLE(main_window->textarea)->current_pos);
	gtk_text_insert(GTK_TEXT(main_window->textarea), NULL, NULL, NULL, lab, -1);
}


void on_load_text_clicked(GtkWidget *widget, gpointer data)
{
	if (!load_file_selection) {
		load_file_selection = gtk_file_selection_new(_("GDynText: Load text"));
		gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(load_file_selection));
		gtk_signal_connect(GTK_OBJECT(load_file_selection), "destroy", GTK_SIGNAL_FUNC(on_window_destroy), &load_file_selection);
		gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(load_file_selection)->cancel_button), "clicked",
			GTK_SIGNAL_FUNC(on_window_close), load_file_selection);
		gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(load_file_selection)->ok_button), "clicked",
			GTK_SIGNAL_FUNC(load_text), load_file_selection);
	}
	if (GTK_WIDGET_VISIBLE(load_file_selection))
		return;
	gtk_widget_show(load_file_selection);
}

void on_layer_align_change(GtkWidget *widget, gpointer data)
{
	main_window->layer_alignment = (gint)data;
}

void load_text(GtkWidget *widget, gpointer data)
{
	FILE *is;
	gchar *file;
	gchar text[MAX_TEXT_SIZE];
	gchar msg[1024];
	struct stat sbuf;
	
	if (!message_window)
		create_message_window(&message_window);
	if (!GTK_WIDGET_VISIBLE(message_window))
		message_window_clear(MESSAGE_WINDOW(message_window));

	file = gtk_file_selection_get_filename(GTK_FILE_SELECTION(load_file_selection));
	if ((is = fopen(file, "rt"))) {
		gtk_widget_hide(load_file_selection);
		memset(text, 0, MAX_TEXT_SIZE);
		fread(text, MAX_TEXT_SIZE - 1, 1, is);
		fclose(is);
		if (!stat(file, &sbuf) && sbuf.st_size > MAX_TEXT_SIZE) {
			g_snprintf(msg, sizeof(msg), _("Warning file \"%s\" is larger than the maximum allowed text length (%d).\n"), file, MAX_TEXT_SIZE);
			message_window_append(MESSAGE_WINDOW(message_window), msg);
		}
		gtk_text_freeze(GTK_TEXT(main_window->textarea));
		gtk_editable_delete_text(GTK_EDITABLE(main_window->textarea), 0, -1);
		gtk_text_insert(GTK_TEXT(main_window->textarea), NULL, NULL, NULL, text, -1);
		gtk_text_thaw(GTK_TEXT(main_window->textarea));
	} else {
		g_snprintf(msg, sizeof(msg), _("Error opening \"%s\"!\n"), file);
		message_window_append(MESSAGE_WINDOW(message_window), msg);
	}
	if (MESSAGE_WINDOW(message_window)->contains_messages)
		gtk_widget_show(message_window);
}


#ifdef DEBUG_UI
int main(void)
{
	GdtVals *data;
	gboolean retval;
	
	data = g_new0(GdtVals, 1);
	strcpy(data->text, "Test\nLine 1\nLine 2");
	strcpy(data->xlfd,
		"-star division-helmet-medium-r-normal-*-*-200-*-*-p-*-adobe-fontspecific");
	data->color						= 0xffdead;
	data->antialias				= TRUE;
	data->alignment				= CENTER;
	data->rotation				= 45;
	data->layer_alignment	= 4;
	data->preview					= TRUE;
	data->messages				= g_list_append(data->messages, "Test message!!!!");
	printf("GDT UI Returns: %d\n", retval = gdt_create_ui(data));
	printf(
		"  XLFD:\n   '%s'\n"
		"  COLOR:           '0x%06X'\n"
		"  ANTIALIAS:       '%d'\n"
		"  ALIGNMENT:       '%d'\n"
		"  ROTATION:        '%d'\n"
		"  LINE_SPACING:    '%d'\n"
		"  LAYER_ALIGNMENT: '%d'\n"
		"  CHANGE_LNAME:    '%d'\n",
		data->xlfd, data->color, data->antialias, data->alignment,
		data->rotation, data->line_spacing, data->layer_alignment,
		data->change_layer_name);
	g_free(data);
	return retval;
}
#endif

/* vim: set ts=2 sw=2 ai tw=79 nowrap: */
