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

#include "gui-types.h"

#include "core/gimppattern.h"
#include "core/gimpcontext.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpitemfactory.h"

#include "patterns-commands.h"

#include "libgimp/gimpintl.h"


/*  public functions  */

void
patterns_menu_update (GtkItemFactory *factory,
                      gpointer        data)
{
  GimpContainerEditor *editor;
  GimpPattern         *pattern;
  gboolean             internal = FALSE;

  editor = GIMP_CONTAINER_EDITOR (data);

  pattern = gimp_context_get_pattern (editor->view->context);

  if (pattern)
    internal = GIMP_DATA (pattern)->internal;

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/Duplicate Pattern",
		 pattern && GIMP_DATA_GET_CLASS (pattern)->duplicate);
  SET_SENSITIVE ("/Edit Pattern...",
		 pattern && GIMP_DATA_FACTORY_VIEW (editor)->data_edit_func);
  SET_SENSITIVE ("/Delete Pattern...",
		 pattern && ! internal);

#undef SET_SENSITIVE
}
