/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
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

#include <stdarg.h>
#include <stdio.h>

#include "coord.xpm"
#include "dimension.xpm"

#include "imap_statusbar.h"

StatusBar_t*
make_statusbar(GtkWidget *main_vbox, GtkWidget *window)
{
   StatusBar_t 	*statusbar = g_new(StatusBar_t, 1);
   GtkWidget 	*hbox, *iconw;
   GdkPixmap 	*icon;
   GdkBitmap 	*mask;
   GtkStyle  	*style;

   hbox = gtk_hbox_new(FALSE, 1);
   gtk_box_pack_start(GTK_BOX(main_vbox), hbox, FALSE, FALSE, 0);

   /* Status info */
   statusbar->status = gtk_statusbar_new();
   statusbar->status_id = gtk_statusbar_get_context_id(
      GTK_STATUSBAR(statusbar->status), "general_status");
   gtk_box_pack_start(GTK_BOX(hbox), statusbar->status, TRUE, TRUE, 0);
   gtk_widget_show(statusbar->status);

   /* (x, y) coordinate */
   style = gtk_widget_get_style(window);
   icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
				       &style->bg[GTK_STATE_NORMAL], 
				       coord_xpm);
   iconw = gtk_pixmap_new(icon, mask);
   gtk_box_pack_start(GTK_BOX(hbox), iconw, FALSE, FALSE, 10);
   gtk_widget_show(iconw);

   statusbar->xy = gtk_entry_new();
   gtk_widget_set_usize(statusbar->xy, 64, -1);
   gtk_entry_set_editable(GTK_ENTRY(statusbar->xy), FALSE);
   GTK_WIDGET_UNSET_FLAGS(statusbar->xy, GTK_CAN_FOCUS);
   gtk_box_pack_start(GTK_BOX(hbox), statusbar->xy, FALSE, FALSE, 0);
   gtk_widget_show(statusbar->xy);

   /* Dimension info */
   icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
				       &style->bg[GTK_STATE_NORMAL], 
				       dimension_xpm);
   iconw = gtk_pixmap_new(icon, mask); 
   gtk_box_pack_start(GTK_BOX(hbox), iconw, FALSE, FALSE, 10);
   gtk_widget_show(iconw);

   statusbar->dimension = gtk_entry_new();
   gtk_widget_set_usize(statusbar->dimension, 64, -1);
   gtk_entry_set_editable(GTK_ENTRY(statusbar->dimension), FALSE);
   GTK_WIDGET_UNSET_FLAGS(statusbar->dimension, GTK_CAN_FOCUS);
   gtk_box_pack_start(GTK_BOX(hbox), statusbar->dimension, FALSE, FALSE, 0);
   gtk_widget_show(statusbar->dimension);

   /* Zoom info */
   statusbar->zoom = gtk_statusbar_new();
   statusbar->zoom_id = gtk_statusbar_get_context_id(
      GTK_STATUSBAR(statusbar->zoom), "zoom_status");
   gtk_box_pack_start(GTK_BOX(hbox), statusbar->zoom, FALSE, FALSE, 5);
   gtk_widget_show(statusbar->zoom);

   gtk_widget_show(hbox);

   return statusbar;
}

void 
statusbar_set_status(StatusBar_t *statusbar, const gchar *format, ...)
{
   va_list ap;
   char scratch[256];

   va_start(ap, format);
   vsprintf(scratch, format, ap);
   va_end(ap);

   statusbar_clear_status(statusbar);
   statusbar->message_id = 
			gtk_statusbar_push(GTK_STATUSBAR(statusbar->status), 
					   statusbar->status_id, scratch);
}

void 
statusbar_clear_status(StatusBar_t *statusbar)
{
   if (statusbar->message_id)
      gtk_statusbar_remove(GTK_STATUSBAR(statusbar->status), 
			   statusbar->status_id,
			   statusbar->message_id);
}

void 
statusbar_set_xy(StatusBar_t *statusbar, gint x, gint y)
{
   char scratch[16];
 
   sprintf(scratch, "%d, %d", (int) x, (int) y);
   gtk_entry_set_text(GTK_ENTRY(statusbar->xy), scratch);
}

void statusbar_clear_xy(StatusBar_t *statusbar)
{
   gtk_entry_set_text(GTK_ENTRY(statusbar->xy), "");
}

void 
statusbar_set_dimension(StatusBar_t *statusbar, gint w, gint h)
{
   char scratch[16];
 
   sprintf(scratch, "%d x %d", (int) w, (int) h);
   gtk_entry_set_text(GTK_ENTRY(statusbar->dimension), scratch);
}

void 
statusbar_clear_dimension(StatusBar_t *statusbar)
{
   gtk_entry_set_text(GTK_ENTRY(statusbar->dimension), "");
}

void 
statusbar_set_zoom(StatusBar_t *statusbar, gint factor)
{
   char scratch[16];
 
   sprintf(scratch, "1:%d", factor);
   gtk_statusbar_push(GTK_STATUSBAR(statusbar->zoom), statusbar->zoom_id, 
		      scratch);
}
