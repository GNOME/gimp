/* The GIMP -- an image manipulation program
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

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "widgets/gimpitemfactory.h"
#include "widgets/gimppaletteeditor.h"

#include "palette-editor-commands.h"
#include "palette-editor-menu.h"

#include "libgimp/gimpintl.h"


GimpItemFactoryEntry palette_editor_menu_entries[] =
{
  { { N_("/New Color"), NULL,
      palette_editor_new_color_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    NULL, NULL },
  { { N_("/Edit Color..."), NULL,
      palette_editor_edit_color_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    NULL, NULL },
  { { N_("/Delete Color"), NULL,
      palette_editor_delete_color_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    NULL, NULL }
};

gint n_palette_editor_menu_entries = G_N_ELEMENTS (palette_editor_menu_entries);


void
palette_editor_menu_update (GtkItemFactory *factory,
                            gpointer        data)
{
  GimpPaletteEditor *editor;

  editor = GIMP_PALETTE_EDITOR (data);

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/New Color",     TRUE);
  SET_SENSITIVE ("/Edit Color...", editor->color);
  SET_SENSITIVE ("/Delete Color",  editor->color);

#undef SET_SENSITIVE
}
