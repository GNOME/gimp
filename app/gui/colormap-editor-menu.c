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

#include "core/gimpimage.h"

#include "widgets/gimpcolormapeditor.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"

#include "actions/colormap-editor-commands.h"

#include "colormap-editor-menu.h"

#include "gimp-intl.h"


GimpItemFactoryEntry colormap_editor_menu_entries[] =
{
  { { N_("/_Edit Color..."), NULL,
      colormap_editor_edit_color_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    GIMP_HELP_INDEXED_PALETTE_EDIT, NULL },
  { { N_("/_Add Color from FG"), "",
      colormap_editor_add_color_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_ADD },
    NULL,
    GIMP_HELP_INDEXED_PALETTE_ADD, NULL },
  { { N_("/_Add Color from BG"), "",
      colormap_editor_add_color_cmd_callback, TRUE,
      "<StockItem>", GTK_STOCK_ADD },
    NULL,
    GIMP_HELP_INDEXED_PALETTE_ADD, NULL }
};

gint n_colormap_editor_menu_entries = G_N_ELEMENTS (colormap_editor_menu_entries);


void
colormap_editor_menu_update (GtkItemFactory *factory,
                             gpointer        data)
{
  GimpColormapEditor *editor;
  GimpImage          *gimage;
  gboolean            indexed    = FALSE;
  gint                num_colors = 0;

  editor = GIMP_COLORMAP_EDITOR (data);
  gimage = GIMP_IMAGE_EDITOR (editor)->gimage;

  if (gimage)
    {
      indexed    = gimp_image_base_type (gimage) == GIMP_INDEXED;
      num_colors = gimage->num_cols;
    }

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/Edit Color...",     gimage && indexed);
  SET_SENSITIVE ("/Add Color from FG", gimage && indexed && num_colors < 256);
  SET_SENSITIVE ("/Add Color from BG", gimage && indexed && num_colors < 256);

#undef SET_SENSITIVE
}
