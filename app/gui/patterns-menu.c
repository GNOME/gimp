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
#include "patterns-menu.h"
#include "menus.h"

#include "gimp-intl.h"


GimpItemFactoryEntry patterns_menu_entries[] =
{
#if 0
  /*  disabled because they are useless now  */
  { { N_("/_Edit Pattern..."), NULL,
      data_edit_data_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    GIMP_HELP_PATTERN_EDIT, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/_New Pattern"), "",
      data_new_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    GIMP_HELP_PATTERN_NEW, NULL },
  { { N_("/D_uplicate Pattern"), NULL,
      data_duplicate_data_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    GIMP_HELP_PATTERN_DUPLICATE, NULL },
#endif
  { { N_("/_Delete Pattern..."), "",
      data_delete_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    GIMP_HELP_PATTERN_DELETE, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/_Refresh Patterns"), "",
      data_refresh_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_REFRESH },
    NULL,
    GIMP_HELP_PATTERN_REFRESH, NULL }
};

gint n_patterns_menu_entries = G_N_ELEMENTS (patterns_menu_entries);


void
patterns_menu_update (GtkItemFactory *factory,
                      gpointer        user_data)
{
  GimpContainerEditor *editor;
  GimpPattern         *pattern;
  GimpData            *data = NULL;

  editor = GIMP_CONTAINER_EDITOR (user_data);

  pattern = gimp_context_get_pattern (editor->view->context);

  if (pattern)
    data = GIMP_DATA (pattern);

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

#if 0
  SET_SENSITIVE ("/Edit Pattern...",
		 pattern && GIMP_DATA_FACTORY_VIEW (editor)->data_edit_func);
  SET_SENSITIVE ("/Duplicate Pattern",
		 pattern && GIMP_DATA_GET_CLASS (data)->duplicate);
#endif
  SET_SENSITIVE ("/Delete Pattern...",
		 pattern && data->deletable);

#undef SET_SENSITIVE
}
