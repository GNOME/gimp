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

#include "imap_main.h"
#include "imap_misc.h"

GtkWidget*
make_toolbar_icon(GtkWidget *toolbar, GtkWidget *window, char **data, 
		  const char *identifier, const char *tooltip, 
		  void (*callback)(GtkWidget*, gpointer), gpointer udata)
{
   GtkWidget *iconw;
   GdkPixmap *icon;
   GdkBitmap *mask;
   GtkStyle  *style;

   style = gtk_widget_get_style(window);
   icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
				       &style->bg[GTK_STATE_NORMAL], 
				       data);
   iconw = gtk_pixmap_new(icon, mask);
   return gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
				  identifier, tooltip, "Private", iconw,
				  GTK_SIGNAL_FUNC(callback), udata);
}

GtkWidget*
make_toolbar_radio_icon(GtkWidget *toolbar, GtkWidget *window, 
			 GtkWidget *prev, char **data, 
			 const char *identifier, const char *tooltip, 
			 void (*callback)(GtkWidget*, gpointer),
			 gpointer udata)
{
   GtkWidget *iconw;
   GdkPixmap *icon;
   GdkBitmap *mask;
   GtkStyle  *style;

   style = gtk_widget_get_style(window);
   icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
				       &style->bg[GTK_STATE_NORMAL], 
				       data);
   iconw = gtk_pixmap_new(icon, mask);
   return gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
				     GTK_TOOLBAR_CHILD_RADIOBUTTON, prev,
				     identifier, tooltip, "Private", iconw,
				     GTK_SIGNAL_FUNC(callback), udata);
}

GtkWidget*
make_toolbar_toggle_icon(GtkWidget *toolbar, GtkWidget *window, 
			 char **data, const char *identifier, 
			 const char *tooltip, 
			 void (*callback)(GtkWidget*, gpointer),
			 gpointer udata)
{
   GtkWidget *iconw;
   GdkPixmap *icon;
   GdkBitmap *mask;
   GtkStyle  *style;

   style = gtk_widget_get_style(window);
   icon = gdk_pixmap_create_from_xpm_d(window->window, &mask,
				       &style->bg[GTK_STATE_NORMAL], 
				       data);
   iconw = gtk_pixmap_new(icon, mask);
   return gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
				     GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL,
				     identifier, tooltip, "Private", iconw,
				     GTK_SIGNAL_FUNC(callback), udata);
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
