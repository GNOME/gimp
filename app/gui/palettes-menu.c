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

#include "core/gimpcontext.h"
#include "core/gimpdata.h"

#include "widgets/gimpcontainereditor.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"

#include "data-commands.h"
#include "palettes-commands.h"
#include "palettes-menu.h"
#include "menus.h"

#include "gimp-intl.h"


GimpItemFactoryEntry palettes_menu_entries[] =
{
  { { N_("/_Edit Palette..."), NULL,
      data_edit_data_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    GIMP_HELP_PALETTE_EDIT, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/_New Palette"), "",
      data_new_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    GIMP_HELP_PALETTE_NEW, NULL },
  { { N_("/_Import Palette..."), "",
      palettes_import_palette_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_CONVERT },
    NULL,
    GIMP_HELP_PALETTE_IMPORT, NULL },
  { { N_("/D_uplicate Palette"), NULL,
      data_duplicate_data_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    GIMP_HELP_PALETTE_DUPLICATE, NULL },
  { { N_("/_Merge Palettes..."), NULL,
      palettes_merge_palettes_cmd_callback, 0 },
    NULL,
    GIMP_HELP_PALETTE_MERGE, NULL },
  { { N_("/_Delete Palette"), "",
      data_delete_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    GIMP_HELP_PALETTE_DELETE, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/_Refresh Palettes"), "",
      data_refresh_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_REFRESH },
    NULL,
    GIMP_HELP_PALETTE_REFRESH, NULL }
};

gint n_palettes_menu_entries = G_N_ELEMENTS (palettes_menu_entries);


void
palettes_menu_update (GtkItemFactory *factory,
                      gpointer        user_data)
{
  GimpContainerEditor *editor;
  GimpPalette         *palette;
  GimpData            *data = NULL;

  editor = GIMP_CONTAINER_EDITOR (user_data);

  palette = gimp_context_get_palette (editor->view->context);

  if (palette)
    data = GIMP_DATA (palette);

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/Edit Palette...",
		 palette && GIMP_DATA_FACTORY_VIEW (editor)->data_edit_func);
  SET_SENSITIVE ("/Duplicate Palette",
		 palette && GIMP_DATA_GET_CLASS (data)->duplicate);
  SET_SENSITIVE ("/Merge Palettes...",
		 FALSE); /* FIXME palette && GIMP_IS_CONTAINER_LIST_VIEW (editor->view)); */
  SET_SENSITIVE ("/Delete Palette",
		 palette && data->deletable);

#undef SET_SENSITIVE
}
