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
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"

#include "actions/documents-commands.h"

#include "documents-menu.h"
#include "menus.h"

#include "gimp-intl.h"


GimpItemFactoryEntry documents_menu_entries[] =
{
  { { N_("/_Open Image"), "",
      documents_open_document_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    NULL,
    GIMP_HELP_DOCUMENT_OPEN, NULL },
  { { N_("/_Raise or Open Image"), "",
      documents_raise_or_open_document_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    NULL,
    GIMP_HELP_DOCUMENT_OPEN, NULL },
  { { N_("/File Open _Dialog"), "",
      documents_file_open_dialog_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    NULL,
    GIMP_HELP_DOCUMENT_OPEN, NULL },
  { { N_("/Remove _Entry"), NULL,
      documents_remove_document_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_REMOVE },
    NULL,
    GIMP_HELP_DOCUMENT_REMOVE, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Recreate _Preview"), "",
      documents_recreate_preview_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_REFRESH },
    NULL,
    GIMP_HELP_DOCUMENT_REFRESH, NULL },
  { { N_("/Reload _all Previews"), "",
      documents_reload_previews_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_REFRESH },
    NULL,
    GIMP_HELP_DOCUMENT_REFRESH, NULL },
  { { N_("/Remove Dangling E_ntries"), "",
      documents_delete_dangling_documents_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_REFRESH },
    NULL,
    GIMP_HELP_DOCUMENT_REFRESH, NULL },
};

gint n_documents_menu_entries = G_N_ELEMENTS (documents_menu_entries);


void
documents_menu_update (GtkItemFactory *factory,
                       gpointer        data)
{
  GimpContainerEditor *editor;
  GimpImagefile       *imagefile;

  editor = GIMP_CONTAINER_EDITOR (data);

  imagefile = gimp_context_get_imagefile (editor->view->context);

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/Open Image",              imagefile);
  SET_SENSITIVE ("/Raise or Open Image",     imagefile);
  SET_SENSITIVE ("/File Open Dialog",        TRUE);
  SET_SENSITIVE ("/Remove Entry",            imagefile);
  SET_SENSITIVE ("/Recreate Preview",        imagefile);
  SET_SENSITIVE ("/Reload all Previews",     imagefile);
  SET_SENSITIVE ("/Remove Dangling Entries", imagefile);

#undef SET_SENSITIVE
}
