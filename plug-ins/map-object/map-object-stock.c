/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "map-object-stock.h"

#include "../lighting/images/stock-pixbufs.h"


static GtkIconFactory *mapobject_icon_factory = NULL;

static GtkStockItem mapobject_stock_items[] =
{
  { STOCK_INTENSITY_AMBIENT_LOW,       NULL, 0, 0, NULL },
  { STOCK_INTENSITY_AMBIENT_HIGH,      NULL, 0, 0, NULL },
  { STOCK_INTENSITY_DIFFUSE_LOW,       NULL, 0, 0, NULL },
  { STOCK_INTENSITY_DIFFUSE_HIGH,      NULL, 0, 0, NULL },
  { STOCK_REFLECTIVITY_DIFFUSE_LOW,    NULL, 0, 0, NULL },
  { STOCK_REFLECTIVITY_DIFFUSE_HIGH,   NULL, 0, 0, NULL },
  { STOCK_REFLECTIVITY_SPECULAR_LOW,   NULL, 0, 0, NULL },
  { STOCK_REFLECTIVITY_SPECULAR_HIGH,  NULL, 0, 0, NULL },
  { STOCK_REFLECTIVITY_HIGHLIGHT_LOW,  NULL, 0, 0, NULL },
  { STOCK_REFLECTIVITY_HIGHLIGHT_HIGH, NULL, 0, 0, NULL }
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

  gtk_icon_factory_add (mapobject_icon_factory, stock_id, set);

  gtk_icon_set_unref (set);
}

void
mapobject_stock_init (void)
{
  static gboolean initialized = FALSE;

  if (initialized)
    return;

  mapobject_icon_factory = gtk_icon_factory_new ();

  add_stock_icon (STOCK_INTENSITY_AMBIENT_LOW,       GTK_ICON_SIZE_BUTTON,
                  stock_intensity_ambient_low);
  add_stock_icon (STOCK_INTENSITY_AMBIENT_HIGH,      GTK_ICON_SIZE_BUTTON,
                  stock_intensity_ambient_high);
  add_stock_icon (STOCK_INTENSITY_DIFFUSE_LOW,       GTK_ICON_SIZE_BUTTON,
                  stock_intensity_diffuse_low);
  add_stock_icon (STOCK_INTENSITY_DIFFUSE_HIGH,      GTK_ICON_SIZE_BUTTON,
                  stock_intensity_diffuse_high);
  add_stock_icon (STOCK_REFLECTIVITY_DIFFUSE_LOW,    GTK_ICON_SIZE_BUTTON,
                  stock_reflectivity_diffuse_low);
  add_stock_icon (STOCK_REFLECTIVITY_DIFFUSE_HIGH,   GTK_ICON_SIZE_BUTTON,
                  stock_reflectivity_diffuse_high);
  add_stock_icon (STOCK_REFLECTIVITY_SPECULAR_LOW,   GTK_ICON_SIZE_BUTTON,
                  stock_reflectivity_specular_low);
  add_stock_icon (STOCK_REFLECTIVITY_SPECULAR_HIGH,  GTK_ICON_SIZE_BUTTON,
                  stock_reflectivity_specular_high);
  add_stock_icon (STOCK_REFLECTIVITY_HIGHLIGHT_LOW,  GTK_ICON_SIZE_BUTTON,
                  stock_reflectivity_highlight_low);
  add_stock_icon (STOCK_REFLECTIVITY_HIGHLIGHT_HIGH, GTK_ICON_SIZE_BUTTON,
                  stock_reflectivity_highlight_high);

  gtk_icon_factory_add_default (mapobject_icon_factory);

  gtk_stock_add_static (mapobject_stock_items,
                        G_N_ELEMENTS (mapobject_stock_items));

  initialized = TRUE;
}
