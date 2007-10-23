/*
 * This is a plug-in for GIMP.
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

#include "imap_stock.h"

#include "images/imap-stock-pixbufs.h"

static GtkIconFactory *imap_icon_factory = NULL;

static GtkStockItem imap_stock_items[] =
{
  { IMAP_STOCK_CIRCLE,		NULL, 0, 0, NULL },
  { IMAP_STOCK_COORD,  	 	NULL, 0, 0, NULL },
  { IMAP_STOCK_DIMENSION,       NULL, 0, 0, NULL },
  { IMAP_STOCK_JAVA, 		NULL, 0, 0, NULL },
  { IMAP_STOCK_POLYGON,       	NULL, 0, 0, NULL },
  { IMAP_STOCK_RECTANGLE,	NULL, 0, 0, NULL },
  { IMAP_STOCK_TO_BACK,    	NULL, 0, 0, NULL },
  { IMAP_STOCK_TO_FRONT,        NULL, 0, 0, NULL }
  };

static void
add_stock_icon (const gchar  *stock_id,
                const guint8 *inline_data)
{
  GtkIconSource *source;
  GtkIconSet    *set;
  GdkPixbuf     *pixbuf;

  source = gtk_icon_source_new ();

  gtk_icon_source_set_size (source, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_icon_source_set_size_wildcarded (source, TRUE);

  pixbuf = gdk_pixbuf_new_from_inline (-1, inline_data, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  g_object_unref (pixbuf);

  set = gtk_icon_set_new ();

  gtk_icon_set_add_source (set, source);
  gtk_icon_source_free (source);

  gtk_icon_factory_add (imap_icon_factory, stock_id, set);

  gtk_icon_set_unref (set);
}

void
init_stock_icons (void)
{
   imap_icon_factory = gtk_icon_factory_new ();

   add_stock_icon (IMAP_STOCK_CIRCLE,    stock_circle);
   add_stock_icon (IMAP_STOCK_COORD,     stock_coord);
   add_stock_icon (IMAP_STOCK_DIMENSION, stock_dimension);
   add_stock_icon (IMAP_STOCK_JAVA,      stock_java);
   add_stock_icon (IMAP_STOCK_POLYGON,   stock_polygon);
   add_stock_icon (IMAP_STOCK_RECTANGLE, stock_rectangle);
   add_stock_icon (IMAP_STOCK_TO_BACK,   stock_to_back);
   add_stock_icon (IMAP_STOCK_TO_FRONT,  stock_to_front);

   gtk_icon_factory_add_default (imap_icon_factory);

   gtk_stock_add_static (imap_stock_items, G_N_ELEMENTS (imap_stock_items));
}
