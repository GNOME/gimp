/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  <alt@picnic.demon.co.uk>
 *               2003 Sven Neumann  <sven@gimp.org>
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
 */

#include "config.h"

#include <gtk/gtk.h>

#include "gfig-stock.h"

#include "images/gfig-stock-pixbufs.h"


static GtkIconFactory *gfig_icon_factory = NULL;

static GtkStockItem gfig_stock_items[] =
{
  { GFIG_STOCK_BEZIER,        NULL, 0, 0, NULL },
  { GFIG_STOCK_CIRCLE,        NULL, 0, 0, NULL },
  { GFIG_STOCK_COPY_OBJECT,   NULL, 0, 0, NULL },
  { GFIG_STOCK_CURVE,         NULL, 0, 0, NULL },
  { GFIG_STOCK_DELETE_OBJECT, NULL, 0, 0, NULL },
  { GFIG_STOCK_ELLIPSE,       NULL, 0, 0, NULL },
  { GFIG_STOCK_LINE,          NULL, 0, 0, NULL },
  { GFIG_STOCK_MOVE_OBJECT,   NULL, 0, 0, NULL },
  { GFIG_STOCK_MOVE_POINT,    NULL, 0, 0, NULL },
  { GFIG_STOCK_POLYGON,       NULL, 0, 0, NULL },
  { GFIG_STOCK_SPIRAL,        NULL, 0, 0, NULL },
  { GFIG_STOCK_STAR,          NULL, 0, 0, NULL }
};


static void
add_stock_icon (const gchar  *stock_id,
                GtkIconSize   size,
                const guint8 *inline_data)
{
  GtkIconSource *source;
  GtkIconSet    *set;
  GdkPixbuf     *pixbuf;

  source = gtk_icon_source_new ();

  gtk_icon_source_set_size (source, size);
  gtk_icon_source_set_size_wildcarded (source, FALSE);

  pixbuf = gdk_pixbuf_new_from_inline (-1, inline_data, FALSE, NULL);

  gtk_icon_source_set_pixbuf (source, pixbuf);
  g_object_unref (pixbuf);

  set = gtk_icon_set_new ();

  gtk_icon_set_add_source (set, source);
  gtk_icon_source_free (source);

  gtk_icon_factory_add (gfig_icon_factory, stock_id, set);

  gtk_icon_set_unref (set);
}

void
gfig_stock_init (void)
{
  static gboolean initialized = FALSE;

  if (initialized)
    return;

  gfig_icon_factory = gtk_icon_factory_new ();

  add_stock_icon (GFIG_STOCK_BEZIER,        GTK_ICON_SIZE_BUTTON, stock_bezier);
  add_stock_icon (GFIG_STOCK_CIRCLE,        GTK_ICON_SIZE_BUTTON, stock_circle);
  add_stock_icon (GFIG_STOCK_COPY_OBJECT,   GTK_ICON_SIZE_BUTTON, stock_copy_object);
  add_stock_icon (GFIG_STOCK_CURVE,         GTK_ICON_SIZE_BUTTON, stock_curve);
  add_stock_icon (GFIG_STOCK_DELETE_OBJECT, GTK_ICON_SIZE_BUTTON, stock_delete_object);
  add_stock_icon (GFIG_STOCK_ELLIPSE,       GTK_ICON_SIZE_BUTTON, stock_ellipse);
  add_stock_icon (GFIG_STOCK_LINE,          GTK_ICON_SIZE_BUTTON, stock_line);
  add_stock_icon (GFIG_STOCK_MOVE_OBJECT,   GTK_ICON_SIZE_BUTTON, stock_move_object);
  add_stock_icon (GFIG_STOCK_MOVE_POINT,    GTK_ICON_SIZE_BUTTON, stock_move_point);
  add_stock_icon (GFIG_STOCK_POLYGON,       GTK_ICON_SIZE_BUTTON, stock_polygon);
  add_stock_icon (GFIG_STOCK_SPIRAL,        GTK_ICON_SIZE_BUTTON, stock_spiral);
  add_stock_icon (GFIG_STOCK_STAR,          GTK_ICON_SIZE_BUTTON, stock_star);

  add_stock_icon (GFIG_STOCK_LOGO,          GTK_ICON_SIZE_DIALOG, stock_logo);

  gtk_icon_factory_add_default (gfig_icon_factory);

  gtk_stock_add_static (gfig_stock_items, G_N_ELEMENTS (gfig_stock_items));
}
