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

#include "widgets/gimperrorconsole.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"

#include "actions/error-console-commands.h"

#include "error-console-menu.h"

#include "gimp-intl.h"


GimpItemFactoryEntry error_console_menu_entries[] =
{
  { { N_("/_Clear Errors"), "",
      error_console_clear_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_CLEAR },
    NULL,
    GIMP_HELP_ERRORS_CLEAR, NULL },

  { { "/---", NULL, NULL, 0, "<Separator>", NULL }, NULL, NULL, NULL },

  { { N_("/Save _All Errors to File..."), "",
      error_console_save_all_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SAVE_AS },
    NULL,
    GIMP_HELP_ERRORS_SAVE, NULL },
  { { N_("/Save _Selection to File..."), "",
      error_console_save_selection_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SAVE_AS },
    NULL,
    GIMP_HELP_ERRORS_SAVE, NULL }
};

gint n_error_console_menu_entries = G_N_ELEMENTS (error_console_menu_entries);


void
error_console_menu_update (GtkItemFactory *factory,
                           gpointer        data)
{
  GimpErrorConsole *console;
  gboolean          selection;

  console = GIMP_ERROR_CONSOLE (data);

  selection = gtk_text_buffer_get_selection_bounds (console->text_buffer,
                                                    NULL, NULL);

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/Clear Errors",               TRUE);
  SET_SENSITIVE ("/Save All Errors to File...", TRUE);
  SET_SENSITIVE ("/Save Selection to File...",  selection);

#undef SET_SENSITIVE
}
