/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpstock.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>
#include <gtk/gtkiconfactory.h>

#include "gimpstock.h"

#include "pixmaps/gimp-stock-pixbufs.h"

#include "libgimp/libgimp-intl.h"


static GtkIconFactory *gimp_stock_factory = NULL;


static GtkIconSet *
sized_with_same_fallback_icon_set_from_inline (const guchar *inline_data,
					       GtkIconSize   size)
{
  GtkIconSource *source;
  GtkIconSet    *set;
  GdkPixbuf     *pixbuf;

  source = gtk_icon_source_new ();

  gtk_icon_source_set_size (source, size);
  gtk_icon_source_set_size_wildcarded (source, FALSE);

  pixbuf = gdk_pixbuf_new_from_stream (-1, inline_data, FALSE, NULL);

  g_assert (pixbuf);

  gtk_icon_source_set_pixbuf (source, pixbuf);

  gdk_pixbuf_unref (pixbuf);

  set = gtk_icon_set_new ();

  gtk_icon_set_add_source (set, source);

  gtk_icon_source_set_size_wildcarded (source, TRUE);
  gtk_icon_set_add_source (set, source);

  gtk_icon_source_free (source);

  return set;
}

static void
add_sized_with_same_fallback (GtkIconFactory *factory,
			      const guchar   *inline_data,
			      GtkIconSize     size,
			      const gchar    *stock_id)
{
  GtkIconSet *set;
  
  set = sized_with_same_fallback_icon_set_from_inline (inline_data, size);
  
  gtk_icon_factory_add (factory, stock_id, set);

  gtk_icon_set_unref (set);
}

static GtkStockItem gimp_stock_items[] =
{
  { GIMP_STOCK_ANCHOR,       N_("Anchor"),            0, 0, "gimp-libgimp" },
  { GIMP_STOCK_DELETE,       N_("_Delete"),           0, 0, "gimp-libgimp" },
  { GIMP_STOCK_DUPLICATE,    N_("Duplicate"),         0, 0, "gimp-libgimp" },
  { GIMP_STOCK_EDIT,         N_("Edit"),              0, 0, "gimp-libgimp" },
  { GIMP_STOCK_LINKED,       N_("Linked"),            0, 0, "gimp-libgimp" },
  { GIMP_STOCK_LOWER,        N_("Lower"),             0, 0, "gimp-libgimp" },
  { GIMP_STOCK_NEW,          N_("New"),               0, 0, "gimp-libgimp" },
  { GIMP_STOCK_PASTE,        N_("Paste"),             0, 0, "gimp-libgimp" },
  { GIMP_STOCK_PASTE_AS_NEW, N_("Paste as New"),      0, 0, "gimp-libgimp" },
  { GIMP_STOCK_PASTE_INTO,   N_("Paste Into"),        0, 0, "gimp-libgimp" },
  { GIMP_STOCK_RAISE,        N_("Raise"),             0, 0, "gimp-libgimp" },
  { GIMP_STOCK_REFRESH,      N_("Refresh"),           0, 0, "gimp-libgimp" },
  { GIMP_STOCK_RESET,        N_("_Reset"),            0, 0, "gimp-libgimp" },
  { GIMP_STOCK_STROKE,       N_("_Stroke"),           0, 0, "gimp-libgimp" },
  { GIMP_STOCK_TO_PATH,      N_("Selection To Path"), 0, 0, "gimp-libgimp" },
  { GIMP_STOCK_TO_SELECTION, N_("To Sleection"),      0, 0, "gimp-libgimp" },
  { GIMP_STOCK_VISIBLE,      N_("Visible"),           0, 0, "gimp-libgimp" },
  { GIMP_STOCK_ZOOM_IN,      N_("Zoom In"),           0, 0, "gimp-libgimp" },
  { GIMP_STOCK_ZOOM_OUT,     N_("Zoom Out"),          0, 0, "gimp-libgimp" }
};

void
gimp_stock_init (void)
{
  static gboolean initialized = FALSE;

  if (initialized)
    return;

  gimp_stock_factory = gtk_icon_factory_new ();

  add_sized_with_same_fallback (gimp_stock_factory,   stock_anchor_button,
				GTK_ICON_SIZE_BUTTON, GIMP_STOCK_ANCHOR);
  add_sized_with_same_fallback (gimp_stock_factory,   stock_delete_button,
				GTK_ICON_SIZE_BUTTON, GIMP_STOCK_DELETE);
  add_sized_with_same_fallback (gimp_stock_factory,   stock_duplicate_button,
				GTK_ICON_SIZE_BUTTON, GIMP_STOCK_DUPLICATE);
  add_sized_with_same_fallback (gimp_stock_factory,   stock_edit_button,
				GTK_ICON_SIZE_BUTTON, GIMP_STOCK_EDIT);
  add_sized_with_same_fallback (gimp_stock_factory,   stock_linked_button,
				GTK_ICON_SIZE_BUTTON, GIMP_STOCK_LINKED);
  add_sized_with_same_fallback (gimp_stock_factory,   stock_lower_button,
				GTK_ICON_SIZE_BUTTON, GIMP_STOCK_LOWER);
  add_sized_with_same_fallback (gimp_stock_factory,   stock_new_button,
				GTK_ICON_SIZE_BUTTON, GIMP_STOCK_NEW);
  add_sized_with_same_fallback (gimp_stock_factory,   stock_paste_button,
				GTK_ICON_SIZE_BUTTON, GIMP_STOCK_PASTE);
  add_sized_with_same_fallback (gimp_stock_factory,   stock_paste_as_new_button,
				GTK_ICON_SIZE_BUTTON, GIMP_STOCK_PASTE_AS_NEW);
  add_sized_with_same_fallback (gimp_stock_factory,   stock_paste_into_button,
				GTK_ICON_SIZE_BUTTON, GIMP_STOCK_PASTE_INTO);
  add_sized_with_same_fallback (gimp_stock_factory,   stock_raise_button,
				GTK_ICON_SIZE_BUTTON, GIMP_STOCK_RAISE);
  add_sized_with_same_fallback (gimp_stock_factory,   stock_refresh_button,
				GTK_ICON_SIZE_BUTTON, GIMP_STOCK_REFRESH);
  add_sized_with_same_fallback (gimp_stock_factory,   stock_refresh_button,
				GTK_ICON_SIZE_BUTTON, GIMP_STOCK_RESET);
  add_sized_with_same_fallback (gimp_stock_factory,   stock_stroke_button,
				GTK_ICON_SIZE_BUTTON, GIMP_STOCK_STROKE);
  add_sized_with_same_fallback (gimp_stock_factory,   stock_to_path_button,
				GTK_ICON_SIZE_BUTTON, GIMP_STOCK_TO_PATH);
  add_sized_with_same_fallback (gimp_stock_factory,   stock_to_selection_button,
				GTK_ICON_SIZE_BUTTON, GIMP_STOCK_TO_SELECTION);
  add_sized_with_same_fallback (gimp_stock_factory,   stock_eye_button,
				GTK_ICON_SIZE_BUTTON, GIMP_STOCK_VISIBLE);
  add_sized_with_same_fallback (gimp_stock_factory,   stock_zoom_in_button,
				GTK_ICON_SIZE_BUTTON, GIMP_STOCK_ZOOM_IN);
  add_sized_with_same_fallback (gimp_stock_factory,   stock_zoom_out_button,
				GTK_ICON_SIZE_BUTTON, GIMP_STOCK_ZOOM_OUT);

  gtk_icon_factory_add_default (gimp_stock_factory);

  gtk_stock_add_static (gimp_stock_items, G_N_ELEMENTS (gimp_stock_items));

  initialized = TRUE;
}
