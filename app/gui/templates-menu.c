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

#include "widgets/gimpcontainereditor.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpitemfactory.h"

#include "templates-commands.h"
#include "templates-menu.h"
#include "menus.h"

#include "gimp-intl.h"


GimpItemFactoryEntry templates_menu_entries[] =
{
  { { N_("/New Template..."), "",
      templates_new_template_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL, NULL, NULL },
  { { N_("/Duplicate Template..."), "",
      templates_duplicate_template_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL, NULL, NULL },
  { { N_("/Edit Template..."), "",
      templates_edit_template_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL, NULL, NULL },
  { { N_("/Create Image from Template..."), "",
      templates_create_image_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_IMAGE },
    NULL, NULL, NULL },
  { { N_("/Delete Template..."), "",
      templates_delete_template_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL, NULL, NULL }
};

gint n_templates_menu_entries = G_N_ELEMENTS (templates_menu_entries);


void
templates_menu_update (GtkItemFactory *factory,
                       gpointer        data)
{
  GimpContainerEditor *editor;
  GimpTemplate        *template;

  editor = GIMP_CONTAINER_EDITOR (data);

  template = gimp_context_get_template (editor->view->context);

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/New Template...",               TRUE);
  SET_SENSITIVE ("/Duplicate Template...",         template);
  SET_SENSITIVE ("/Edit Template...",              template);
  SET_SENSITIVE ("/Create Image from Template...", template);
  SET_SENSITIVE ("/Delete Template...",            template);

#undef SET_SENSITIVE
}
