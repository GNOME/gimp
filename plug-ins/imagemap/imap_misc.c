/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2004 Maurits Rijk  m.rijk@chello.nl
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
 */

#include "config.h"

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include "imap_main.h"
#include "imap_misc.h"

GtkWidget*
make_toolbar_stock_icon(GtkWidget *toolbar, const gchar *stock_id,
			const char *identifier, const char *tooltip,
			void (*callback)(GtkWidget*, gpointer), gpointer udata)
{
   GtkWidget *iconw;

   iconw = gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_SMALL_TOOLBAR);
   return gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
				  identifier, tooltip, NULL, iconw,
				  GTK_SIGNAL_FUNC(callback), udata);
}

GtkWidget*
make_toolbar_radio_icon(GtkWidget *toolbar, const gchar *stock_id,
			 GtkWidget *prev, const char *identifier,
			const char *tooltip,
			 void (*callback)(GtkWidget*, gpointer),
			 gpointer udata)
{
   GtkWidget *iconw = gtk_image_new_from_stock(stock_id,
					       GTK_ICON_SIZE_SMALL_TOOLBAR);
   return gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
				     GTK_TOOLBAR_CHILD_RADIOBUTTON, prev,
				     identifier, tooltip, NULL, iconw,
				     GTK_SIGNAL_FUNC(callback), udata);
}

GtkWidget*
make_toolbar_toggle_icon(GtkWidget *toolbar, const gchar *stock_id,
			 const char *identifier, const char *tooltip,
			 void (*callback)(GtkWidget*, gpointer),
			 gpointer udata)
{
   GtkWidget *iconw = gtk_image_new_from_stock(stock_id,
					       GTK_ICON_SIZE_SMALL_TOOLBAR);
   return gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
				     GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL,
				     identifier, tooltip, NULL, iconw,
				     GTK_SIGNAL_FUNC(callback), udata);
}

static Alert_t*
create_base_alert(const gchar *stock_id)
{
   Alert_t *alert = g_new(Alert_t, 1);
   DefaultDialog_t *dialog;
   GtkWidget *hbox;
   GtkWidget *image;

   alert->dialog = dialog = make_default_dialog("");
   default_dialog_hide_apply_button(dialog);

   hbox = gtk_hbox_new (FALSE, 12);
   gtk_box_pack_start(GTK_BOX(dialog->vbox), hbox, FALSE, FALSE, 0);
   gtk_widget_show(hbox);

   image = gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_DIALOG);
   gtk_container_add(GTK_CONTAINER(hbox), image);
   gtk_widget_show(image);

   alert->label = gtk_label_new("");
   gtk_misc_set_alignment(GTK_MISC(alert->label), 0.0, 0.0);
   gtk_container_add(GTK_CONTAINER(hbox), alert->label);
   gtk_widget_show(alert->label);

   return alert;
}

Alert_t*
create_alert(const gchar *stock_id)
{
   Alert_t *alert = create_base_alert(stock_id);
   default_dialog_hide_cancel_button(alert->dialog);

   return alert;
}

Alert_t*
create_confirm_alert(const gchar *stock_id)
{
   return create_base_alert(stock_id);
}

void
alert_set_text(Alert_t *alert, const char *primary_text,
	       const char *secondary_text)
{
   gchar *text;

   text =
      g_strdup_printf("<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s",
		      primary_text, secondary_text);
   gtk_label_set_markup (GTK_LABEL(alert->label), text);

   g_free(text);
}

#define SASH_SIZE 8

static gint _sash_size = SASH_SIZE;

void
set_sash_size(gboolean double_size)
{
   _sash_size = (double_size) ? 2 * SASH_SIZE : SASH_SIZE;
}

void
draw_sash(GdkWindow *window, GdkGC *gc, gint x, gint y)
{
   draw_rectangle(window, gc, TRUE, x - _sash_size / 2, y - _sash_size / 2,
		  _sash_size, _sash_size);
}

gboolean
near_sash(gint sash_x, gint sash_y, gint x, gint y)
{
   return x >= sash_x - _sash_size / 2 && x <= sash_x + _sash_size / 2 &&
      y >= sash_y - _sash_size / 2 && y <= sash_y + _sash_size / 2;
}
