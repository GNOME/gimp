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

#include "actions/buffers-commands.h"

#include "buffers-menu.h"

#include "gimp-intl.h"


GimpItemFactoryEntry buffers_menu_entries[] =
{
  { { N_("/_Paste Buffer"), "",
      buffers_paste_buffer_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_PASTE },
    NULL,
    GIMP_HELP_BUFFER_PASTE, NULL },
  { { N_("/Paste Buffer _Into"), NULL,
      buffers_paste_buffer_into_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_PASTE_INTO },
    NULL,
    GIMP_HELP_BUFFER_PASTE_INTO, NULL },
  { { N_("/Paste Buffer as _New"), NULL,
      buffers_paste_buffer_as_new_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_PASTE_AS_NEW },
    NULL,
    GIMP_HELP_BUFFER_PASTE_AS_NEW, NULL },
  { { N_("/_Delete Buffer"), "",
      buffers_delete_buffer_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    GIMP_HELP_BUFFER_DELETE, NULL }
};

gint n_buffers_menu_entries = G_N_ELEMENTS (buffers_menu_entries);


void
buffers_menu_update (GtkItemFactory *factory,
                     gpointer        data)
{
  GimpContainerEditor *editor;
  GimpBuffer          *buffer;

  editor = GIMP_CONTAINER_EDITOR (data);

  buffer = gimp_context_get_buffer (editor->view->context);

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/Paste Buffer",        buffer);
  SET_SENSITIVE ("/Paste Buffer Into",   buffer);
  SET_SENSITIVE ("/Paste Buffer as New", buffer);
  SET_SENSITIVE ("/Delete Buffer",       buffer);

#undef SET_SENSITIVE
}
