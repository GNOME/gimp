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
#include "config.h"

#include <stdio.h>

#include <gtk/gtk.h>

#include "message_window.h"

#include "libgimp/stdplugins-intl.h"

static void message_window_class_init(MessageWindowClass *class);
static void message_window_init(MessageWindow *mw);


static GtkWindowClass *message_window_parent_class = NULL;


guint message_window_get_type(void)
{
	static guint mw_type = 0;

	if (!mw_type) {
		GtkTypeInfo mw_info = {
			"MessageWindow",
			sizeof(MessageWindow),
			sizeof(MessageWindowClass),
			(GtkClassInitFunc)message_window_class_init,
			(GtkObjectInitFunc)message_window_init,
			(GtkArgSetFunc)NULL,
			(GtkArgGetFunc)NULL,
#ifdef GTK_HAVE_FEATURES_1_1_12
			(GtkClassInitFunc)NULL,
#endif
		};
		mw_type = gtk_type_unique(gtk_window_get_type(), &mw_info);
	}
	return mw_type;
}


static void message_window_class_init(MessageWindowClass *klass)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *)klass;
	message_window_parent_class = gtk_type_class(gtk_window_get_type());
}


static void message_window_init(MessageWindow *mw)
{
  GtkWidget *vbox;
  GtkWidget *hbox1;
  GtkWidget *hbbox1;
  GtkWidget *vscrollbar;
  GtkWidget *hseparator;
	GtkStyle *style;

	mw->contains_messages = FALSE;

  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(mw), vbox);
  gtk_widget_show(vbox);

  hbox1 = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox1, TRUE, TRUE, 0);
  gtk_widget_show(hbox1);

  mw->text = gtk_text_new(NULL, NULL);
  gtk_box_pack_start(GTK_BOX(hbox1), mw->text, TRUE, TRUE, 5);
  gtk_widget_show(mw->text);

	style = gtk_widget_get_style(mw->text);
	gtk_style_unref(style);
	style->base[GTK_STATE_NORMAL] = style->bg[GTK_STATE_NORMAL];
	gtk_style_ref(style);
	gtk_widget_set_style(mw->text, style);

  vscrollbar = gtk_vscrollbar_new(GTK_TEXT(mw->text)->vadj);
  gtk_box_pack_start(GTK_BOX(hbox1), vscrollbar, FALSE, TRUE, 0);
  gtk_widget_show(vscrollbar);
	
  hseparator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), hseparator, FALSE, TRUE, 0);
  gtk_widget_show(hseparator);

  hbbox1 = gtk_hbutton_box_new();
  gtk_box_pack_start(GTK_BOX(vbox), hbbox1, FALSE, FALSE, 0);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbbox1), GTK_BUTTONBOX_END);
  gtk_widget_show(hbbox1);

  mw->dismiss_button = gtk_button_new_with_label(_("Dismiss"));
  GTK_WIDGET_SET_FLAGS(mw->dismiss_button, GTK_CAN_DEFAULT);
  gtk_box_pack_end(GTK_BOX(hbbox1), mw->dismiss_button, FALSE, TRUE, 0);
  gtk_widget_grab_default(mw->dismiss_button);
  gtk_widget_show(mw->dismiss_button);
}


GtkWidget *message_window_new(const gchar *title)
{
	MessageWindow *mw;

	mw = gtk_type_new(message_window_get_type());
	gtk_window_set_title(GTK_WINDOW(mw), title);
	gtk_container_border_width(GTK_CONTAINER(mw), 4);
	gtk_window_set_policy(GTK_WINDOW(mw), TRUE, TRUE, FALSE);

	return GTK_WIDGET(mw);
}


void message_window_append(MessageWindow *mw, const gchar *msg)
{
	gtk_widget_realize(mw->text);
	gtk_text_freeze(GTK_TEXT(mw->text));
	gtk_text_insert(GTK_TEXT(mw->text), NULL, NULL, NULL, "* ", -1);
	gtk_text_insert(GTK_TEXT(mw->text), NULL, NULL, NULL, msg, -1);
	gtk_text_thaw(GTK_TEXT(mw->text));
	mw->contains_messages = TRUE;
	printf("* %s", msg);
}


void message_window_clear(MessageWindow *mw)
{
	gtk_widget_realize(mw->text);
	gtk_text_freeze(GTK_TEXT(mw->text));
	gtk_editable_delete_text(GTK_EDITABLE(mw->text), 0, -1);
	gtk_text_thaw(GTK_TEXT(mw->text));
	mw->contains_messages = FALSE;
}

/* vim: set ts=2 sw=2 tw=79 ai nowrap: */
