/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
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

#include <gtk/gtk.h>

#include "imap_main.h"
#include "imap_misc.h"

static GtkWidget*
make_toolbar_icon (GtkWidget *toolbar, GtkToolItem *item,
		   const char *identifier, const char *tooltip,
		   void (*callback)(GtkWidget*, gpointer), gpointer udata)
{
  static GtkTooltips *tips;
  if (!tips)
    {
      tips = gtk_tooltips_new ();
    }
  gtk_tool_item_set_tooltip (item, tips, tooltip, identifier);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
  gtk_widget_show (GTK_WIDGET (item));
  return GTK_WIDGET (item);
}

void
toolbar_add_space (GtkWidget *toolbar)
{
  GtkToolItem *item = gtk_separator_tool_item_new ();
  gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (item), FALSE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
  gtk_widget_show (GTK_WIDGET (item));
}

GtkWidget*
make_toolbar_stock_icon(GtkWidget *toolbar, const gchar *stock_id,
			const char *identifier, const char *tooltip,
			void (*callback)(GtkWidget*, gpointer), gpointer udata)
{
   GtkToolItem *item = gtk_tool_button_new_from_stock (stock_id);
   g_signal_connect (item, "clicked", G_CALLBACK (callback), udata);
   return make_toolbar_icon (toolbar, item, identifier, tooltip,
			     callback, udata);
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
