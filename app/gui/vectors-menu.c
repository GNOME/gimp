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
#include "core/gimpimage-mask.h"
#include "core/gimplist.h"

#include "widgets/gimpitemfactory.h"
#include "widgets/gimpitemtreeview.h"

#include "vectors-commands.h"
#include "vectors-menu.h"
#include "menus.h"

#include "gimp-intl.h"


GimpItemFactoryEntry vectors_menu_entries[] =
{
  { { N_("/_New Path..."), "<control>N",
      vectors_new_vectors_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    "new_path.html", NULL },
  { { N_("/_Raise Path"), "<control>F",
      vectors_raise_vectors_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_UP },
    NULL,
    "raise_path.html", NULL },
  { { N_("/_Lower Path"), "<control>B",
      vectors_lower_vectors_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_DOWN },
    NULL,
    "lower_path.html", NULL },
  { { N_("/D_uplicate Path"), "<control>U",
      vectors_duplicate_vectors_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    "duplicate_path.html", NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Path to Sele_ction"), "<control>S",
      vectors_vectors_to_sel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_REPLACE },
    NULL,
    "path_to_selection.html", NULL },
  { { N_("/_Add to Selection"), NULL,
      vectors_add_vectors_to_sel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_ADD },
    NULL,
    "path_to_selection.html#add", NULL },
  { { N_("/_Subtract from Selection"), NULL,
      vectors_sub_vectors_from_sel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_SUBTRACT },
    NULL,
    "path_to_selection.html#subtract", NULL },
  { { N_("/_Intersect with Selection"), NULL,
      vectors_intersect_vectors_with_sel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_INTERSECT },
    NULL,
    "path_to_selection.html#intersect", NULL },

  { { N_("/Selecti_on to Path"), "<control>P",
      vectors_sel_to_vectors_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_TO_PATH },
    NULL,
    "filters/sel2path.html", NULL },
  { { N_("/Stro_ke Path"), "<control>T",
      vectors_stroke_vectors_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_PATH_STROKE },
    NULL,
    "stroke_path.html", NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Co_py Path"), "<control>C",
      vectors_copy_vectors_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_COPY },
    NULL,
    "copy_path.html", NULL },

  { { N_("/Paste Pat_h"), "<control>V",
      vectors_paste_vectors_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_PASTE },
    NULL,
    "paste_path.html", NULL },
  { { N_("/I_mport Path..."), "<control>I",
      vectors_import_vectors_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    NULL,
    "dialogs/import_path.html", NULL },
  { { N_("/E_xport Path..."), "<control>E",
      vectors_export_vectors_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SAVE },
    NULL,
    "dialogs/export_path.html", NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/_Delete Path"), "<control>X",
      vectors_delete_vectors_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    "delete_path.html", NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Path _Tool"), NULL,
      vectors_vectors_tool_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_PATH },
    NULL,
    "tools/path_tool.html", NULL },
  { { N_("/_Edit Path Attributes..."), NULL,
      vectors_edit_vectors_attributes_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    "dialogs/edit_path_attributes.html", NULL }
};

gint n_vectors_menu_entries = G_N_ELEMENTS (vectors_menu_entries);


void
vectors_menu_update (GtkItemFactory *factory,
                     gpointer        data)
{
  GimpImage   *gimage;
  GimpVectors *vectors    = NULL;
  gboolean     mask_empty = TRUE;
  gboolean     global_buf = FALSE;
  GList       *next       = NULL;
  GList       *prev       = NULL;

  gimage = GIMP_ITEM_TREE_VIEW (data)->gimage;

  if (gimage)
    {
      GList *list;

      vectors = gimp_image_get_active_vectors (gimage);

      mask_empty = gimp_image_mask_is_empty (gimage);

      global_buf = FALSE;

      for (list = GIMP_LIST (gimage->vectors)->list;
           list;
           list = g_list_next (list))
        {
          if (vectors == (GimpVectors *) list->data)
            {
              prev = g_list_previous (list);
              next = g_list_next (list);
              break;
            }
        }
    }

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/New Path...",              gimage);
  SET_SENSITIVE ("/Raise Path",               vectors && prev);
  SET_SENSITIVE ("/Lower Path",               vectors && next);
  SET_SENSITIVE ("/Duplicate Path",           vectors);
  SET_SENSITIVE ("/Path to Selection",        vectors);
  SET_SENSITIVE ("/Add to Selection",         vectors);
  SET_SENSITIVE ("/Subtract from Selection",  vectors);
  SET_SENSITIVE ("/Intersect with Selection", vectors);
  SET_SENSITIVE ("/Selection to Path",        ! mask_empty);
  SET_SENSITIVE ("/Stroke Path",              vectors);
  SET_SENSITIVE ("/Delete Path",              vectors);
  SET_SENSITIVE ("/Copy Path",                vectors);
  SET_SENSITIVE ("/Paste Path",               global_buf);
  SET_SENSITIVE ("/Import Path...",           gimage);
  SET_SENSITIVE ("/Export Path...",           vectors);
  SET_SENSITIVE ("/Path Tool",                vectors);
  SET_SENSITIVE ("/Edit Path Attributes...",  vectors);

#undef SET_SENSITIVE
}
