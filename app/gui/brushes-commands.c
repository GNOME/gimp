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

#include "core/gimpbrushgenerated.h"
#include "core/gimpcontext.h"

#include "widgets/gimpcontainereditor.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpwidgets-utils.h"

#include "brushes-commands.h"
#include "menus.h"

#include "libgimp/gimpintl.h"


static void   brushes_menu_set_sensitivity (GimpContainerEditor *editor);


void
brushes_show_context_menu (GimpContainerEditor *editor)
{
  GtkItemFactory *item_factory;

  brushes_menu_set_sensitivity (editor);

  item_factory = menus_get_brushes_factory ();

  gimp_item_factory_popup_with_data (item_factory, editor);
}


static void
brushes_menu_set_sensitivity (GimpContainerEditor *editor)
{
  GimpBrush *brush;

  brush = gimp_context_get_brush (editor->view->context);

#define SET_SENSITIVE(menu,condition) \
        menus_set_sensitive ("<Brushes>/" menu, (condition) != 0)

  SET_SENSITIVE ("Duplicate Brush",
		 brush && GIMP_DATA_GET_CLASS (brush)->duplicate);
  SET_SENSITIVE ("Edit Brush...",
		 brush && GIMP_IS_BRUSH_GENERATED (brush));
  SET_SENSITIVE ("Delete Brush...",
		 brush);

#undef SET_SENSITIVE
}
