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

#ifndef _IMAP_MISC_H
#define _IMAP_MISC_H

void toolbar_add_space (GtkWidget *toolbar);
GtkWidget *make_toolbar_stock_icon(GtkWidget *toolbar, const gchar *stock_id, 
				   const char *identifier, 
				   const char *tooltip, 
				   void (*callback)(GtkWidget*, gpointer), 
				   gpointer udata);
GtkWidget *make_toolbar_radio_icon(GtkWidget *toolbar, const gchar *stock_id,
				   GtkWidget *prev, const char *identifier, 
				   const char *tooltip, 
				   void (*callback)(GtkWidget*, gpointer),
				   gpointer udata);
GtkWidget *make_toolbar_toggle_icon(GtkWidget *toolbar, const gchar *stock_id,
				    const char *identifier, 
				    const char *tooltip, 
				    void (*callback)(GtkWidget*, gpointer),
				    gpointer udata);

void set_sash_size(gboolean double_size);
void draw_sash(GdkWindow *window, GdkGC *gc, gint x, gint y);
gboolean near_sash(gint sash_x, gint sash_y, gint x, gint y);

typedef struct {
   DefaultDialog_t *dialog;
   GtkWidget *label;
} Alert_t;

Alert_t *create_alert(const gchar *stock_id);
Alert_t *create_confirm_alert(const gchar *stock_id);
void alert_set_text(Alert_t *alert, const char *primary_text, 
		    const char *secondary_text);

#endif /* _IMAP_MISC_H */

