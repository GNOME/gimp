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

#include "actions/data-commands.h"

#include "brushes-menu.h"
#include "menus.h"

#include "gimp-intl.h"


GimpItemFactoryEntry brushes_menu_entries[] =
{
  { { N_("/_Edit Brush..."), NULL,
      data_edit_data_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    GIMP_HELP_BRUSH_EDIT, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/_New Brush"), "",
      data_new_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    GIMP_HELP_BRUSH_NEW, NULL },
  { { N_("/D_uplicate Brush"), NULL,
      data_duplicate_data_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    GIMP_HELP_BRUSH_DUPLICATE, NULL },
  { { N_("/_Delete Brush"), "",
      data_delete_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    GIMP_HELP_BRUSH_DELETE, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/_Refresh Brushes"), "",
      data_refresh_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_REFRESH },
    NULL,
    GIMP_HELP_BRUSH_REFRESH, NULL }
};

gint n_brushes_menu_entries = G_N_ELEMENTS (brushes_menu_entries);


void
brushes_menu_update (GtkItemFactory *factory,
                     gpointer        user_data)
{
  GimpContainerEditor *editor;
  GimpBrush           *brush;
  GimpData            *data = NULL;

  editor = GIMP_CONTAINER_EDITOR (user_data);

  brush = gimp_context_get_brush (editor->view->context);

  if (brush)
    data = GIMP_DATA (brush);

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/Edit Brush...",
		 brush && GIMP_DATA_FACTORY_VIEW (editor)->data_edit_func);
  SET_SENSITIVE ("/Duplicate Brush",
		 brush && GIMP_DATA_GET_CLASS (data)->duplicate);
  SET_SENSITIVE ("/Delete Brush",
		 brush && data->deletable);

#undef SET_SENSITIVE
}
