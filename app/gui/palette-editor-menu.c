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

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimppaletteeditor.h"

#include "menus.h"
#include "palette-editor-commands.h"
#include "palette-editor-menu.h"

#include "gimp-intl.h"


GimpItemFactoryEntry palette_editor_menu_entries[] =
{
  { { N_("/_Edit Color..."), "",
      palette_editor_edit_color_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    GIMP_HELP_PALETTE_EDITOR_EDIT, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/New Color from _FG"), "",
      palette_editor_new_color_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    GIMP_HELP_PALETTE_EDITOR_NEW, NULL },
  { { N_("/New Color from _BG"), "",
      palette_editor_new_color_cmd_callback, TRUE,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    GIMP_HELP_PALETTE_EDITOR_NEW, NULL },
  { { N_("/_Delete Color"), "",
      palette_editor_delete_color_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    GIMP_HELP_PALETTE_EDITOR_DELETE, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Zoom _Out"), "",
      palette_editor_zoom_out_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_ZOOM_OUT },
    NULL,
    GIMP_HELP_PALETTE_EDITOR_ZOOM_OUT, NULL },
  { { N_("/Zoom _In"), "",
      palette_editor_zoom_in_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_ZOOM_IN },
    NULL,
    GIMP_HELP_PALETTE_EDITOR_ZOOM_IN, NULL },
  { { N_("/Zoom _All"), "",
      palette_editor_zoom_all_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_ZOOM_FIT },
    NULL,
    GIMP_HELP_PALETTE_EDITOR_ZOOM_ALL, NULL }
};

gint n_palette_editor_menu_entries = G_N_ELEMENTS (palette_editor_menu_entries);


void
palette_editor_menu_update (GtkItemFactory *factory,
                            gpointer        data)
{
  GimpPaletteEditor *editor;
  GimpDataEditor    *data_editor;
  gboolean           editable = FALSE;

  editor      = GIMP_PALETTE_EDITOR (data);
  data_editor = GIMP_DATA_EDITOR (data);

  if (data_editor->data && data_editor->data_editable)
    editable = TRUE;

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/Edit Color...",     editable && editor->color);
  SET_SENSITIVE ("/New Color from FG", editable);
  SET_SENSITIVE ("/New Color from BG", editable);
  SET_SENSITIVE ("/Delete Color",      editable && editor->color);

  SET_SENSITIVE ("/Zoom Out", data_editor->data);
  SET_SENSITIVE ("/Zoom In",  data_editor->data);
  SET_SENSITIVE ("/Zoom All", data_editor->data);

#undef SET_SENSITIVE
}
