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

#include "widgets/widgets-types.h"

#include "core/gimppattern.h"
#include "core/gimpcontext.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpwidgets-utils.h"

#include "patterns-commands.h"
#include "menus.h"

#include "libgimp/gimpintl.h"


static void   patterns_menu_set_sensitivity (GimpContainerEditor *editor);


void
patterns_show_context_menu (GimpContainerEditor *editor)
{
  GtkItemFactory *item_factory;
  gint            x, y;

  patterns_menu_set_sensitivity (editor);

  item_factory = menus_get_patterns_factory ();

  gimp_menu_position (GTK_MENU (item_factory->widget), &x, &y);

  gtk_item_factory_popup_with_data (item_factory,
				    editor,
				    NULL,
				    x, y,
				    3, 0);
}


static void
patterns_menu_set_sensitivity (GimpContainerEditor *editor)
{
  GimpPattern *pattern;

  pattern = gimp_context_get_pattern (editor->view->context);

#define SET_SENSITIVE(menu,condition) \
        menus_set_sensitive ("<Patterns>/" menu, (condition) != 0)

  SET_SENSITIVE ("Duplicate Pattern",
		 pattern && GIMP_DATA_CLASS (GTK_OBJECT (pattern)->klass)->duplicate);
  SET_SENSITIVE ("Edit Pattern...",
		 pattern && GIMP_DATA_FACTORY_VIEW (editor)->data_edit_func);
  SET_SENSITIVE ("Delete Pattern...",
		 pattern);

#undef SET_SENSITIVE
}
