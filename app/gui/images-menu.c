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
#include "core/gimpimage.h"

#include "widgets/gimpcontainereditor.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpitemfactory.h"

#include "actions/images-commands.h"

#include "images-menu.h"

#include "gimp-intl.h"


GimpItemFactoryEntry images_menu_entries[] =
{
  { { N_("/_Raise Views"), "",
      images_raise_views_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GOTO_TOP },
    NULL, NULL, NULL },
  { { N_("/_New View"), "",
      images_new_view_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL, NULL, NULL },
  { { N_("/_Delete Image"), "",
      images_delete_image_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL, NULL, NULL }
};

gint n_images_menu_entries = G_N_ELEMENTS (images_menu_entries);


void
images_menu_update (GtkItemFactory *factory,
                    gpointer        data)
{
  GimpContainerEditor *editor;
  GimpImage           *image;

  editor = GIMP_CONTAINER_EDITOR (data);

  image = gimp_context_get_image (editor->view->context);

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/Raise Views",  image);
  SET_SENSITIVE ("/New View",     image);
  SET_SENSITIVE ("/Delete Image", image && image->disp_count == 0);

#undef SET_SENSITIVE
}
