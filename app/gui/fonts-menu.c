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

#include "text/gimpfont.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimpfontview.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"

#include "actions/fonts-commands.h"

#include "fonts-menu.h"

#include "gimp-intl.h"


GimpItemFactoryEntry fonts_menu_entries[] =
{
  { { N_("/_Rescan Font List"), "",
      fonts_refresh_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_REFRESH },
    NULL,
    GIMP_HELP_FONT_REFRESH, NULL }
};

gint n_fonts_menu_entries = G_N_ELEMENTS (fonts_menu_entries);


void
fonts_menu_update (GtkItemFactory *factory,
                   gpointer        data)
{
  GimpContainerEditor *editor;
  GimpFont            *font;

  editor = GIMP_CONTAINER_EDITOR (data);

  font = gimp_context_get_font (editor->view->context);

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/Rescan Font List", TRUE);

#undef SET_SENSITIVE
}
