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
#include "widgets/gimpitemfactory.h"

#include "brushes-commands.h"

#include "libgimp/gimpintl.h"


void
brushes_menu_update (GtkItemFactory *factory,
                     gpointer        data)
{
  GimpContainerEditor *editor;
  GimpBrush           *brush;

  editor = GIMP_CONTAINER_EDITOR (data);

  brush = gimp_context_get_brush (editor->view->context);

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/Duplicate Brush",
		 brush && GIMP_DATA_GET_CLASS (brush)->duplicate);
  SET_SENSITIVE ("/Edit Brush...",
		 brush && GIMP_IS_BRUSH_GENERATED (brush));
  SET_SENSITIVE ("/Delete Brush...",
		 brush);

#undef SET_SENSITIVE
}
