/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2003 Maurits Rijk  lpeek.mrijk@consunet.nl
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

#include "arrow.xpm"
#include "circle.xpm"
#include "coord.xpm"
#include "dimension.xpm"
#include "java.xpm"
#include "link.xpm"
#include "map_info.xpm"
#include "polygon.xpm"
#include "rectangle.xpm"
#include "to_back.xpm"
#include "to_front.xpm"

#include "libgimp/stdplugins-intl.h"

static GtkStockItem imap_stock_items[] =
{
  { IMAP_STOCK_MAP_INFO,
    N_("Edit Map Info..."), 0, 0, GETTEXT_PACKAGE "-std-plug-ins" },
};

static void
add_stock_icon (GtkIconFactory  *factory,
                const gchar     *stock_id,
                const gchar    **xpm_data)
{
   GtkIconSet *icon_set;
   GdkPixbuf  *pixbuf;

   pixbuf = gdk_pixbuf_new_from_xpm_data (xpm_data);
   icon_set = gtk_icon_set_new_from_pixbuf (pixbuf);
   gtk_icon_factory_add (factory, stock_id, icon_set);
   g_object_unref (pixbuf);
   gtk_icon_set_unref (icon_set);
}

void
init_stock_icons (void)
{
   GtkIconFactory *factory = gtk_icon_factory_new ();

   add_stock_icon (factory,
                   IMAP_STOCK_ARROW,     (const gchar**) arrow_xpm);
   add_stock_icon (factory,
                   IMAP_STOCK_CIRCLE,    (const gchar**) circle_xpm);
   add_stock_icon (factory,
                   IMAP_STOCK_COORD,     (const gchar**) coord_xpm);
   add_stock_icon (factory,
                   IMAP_STOCK_DIMENSION, (const gchar**) dimension_xpm);
   add_stock_icon (factory,
                   IMAP_STOCK_JAVA,      (const gchar**) java_xpm);
   add_stock_icon (factory,
                   IMAP_STOCK_LINK,      (const gchar**) link_xpm);
   add_stock_icon (factory,
                   IMAP_STOCK_MAP_INFO,  (const gchar**) map_info_xpm);
   add_stock_icon (factory,
                   IMAP_STOCK_POLYGON,   (const gchar**) polygon_xpm);
   add_stock_icon (factory,
                   IMAP_STOCK_RECTANGLE, (const gchar**) rectangle_xpm);
   add_stock_icon (factory,
                   IMAP_STOCK_TO_BACK,   (const gchar**) to_back_xpm);
   add_stock_icon (factory,
                   IMAP_STOCK_TO_FRONT,  (const gchar**) to_front_xpm);

   gtk_icon_factory_add_default(factory);

   gtk_stock_add_static (imap_stock_items, G_N_ELEMENTS (imap_stock_items));
}
