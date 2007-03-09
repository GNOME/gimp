/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "rcm_stock.h"

#include "images/rcm-stock-pixbufs.h"

#include "libgimp/stdplugins-intl.h"


static GtkIconFactory *rcm_icon_factory = NULL;

static GtkStockItem rcm_stock_items[] =
{
  { STOCK_COLORMAP_SWITCH_CLOCKWISE,
    N_("Switch to Clockwise"),    0, 0, NULL },
  { STOCK_COLORMAP_SWITCH_COUNTERCLOCKWISE,
    N_("Switch to C/Clockwise"),  0, 0, NULL },
  { STOCK_COLORMAP_CHANGE_ORDER,
    N_("Change Order of Arrows"), 0, 0, NULL },
  { STOCK_COLORMAP_SELECT_ALL,
    N_("Select All"),             0, 0, NULL }
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

  gtk_icon_factory_add (rcm_icon_factory, stock_id, set);

  gtk_icon_set_unref (set);
}

void
rcm_stock_init (void)
{
  static gboolean initialized = FALSE;

  if (initialized)
    return;

  rcm_icon_factory = gtk_icon_factory_new ();

  add_stock_icon (STOCK_COLORMAP_SWITCH_CLOCKWISE,
                  GTK_ICON_SIZE_BUTTON, rcm_cw);
  add_stock_icon (STOCK_COLORMAP_SWITCH_COUNTERCLOCKWISE,
                  GTK_ICON_SIZE_BUTTON, rcm_ccw);
  add_stock_icon (STOCK_COLORMAP_CHANGE_ORDER,
                  GTK_ICON_SIZE_BUTTON, rcm_a_b);
  add_stock_icon (STOCK_COLORMAP_SELECT_ALL,
                  GTK_ICON_SIZE_BUTTON, rcm_360);

  gtk_icon_factory_add_default (rcm_icon_factory);

  gtk_stock_add_static (rcm_stock_items, G_N_ELEMENTS (rcm_stock_items));

  initialized = TRUE;
}
