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

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimpitemtreeview.h"

#include "actions/vectors-commands.h"

#include "vectors-menu.h"
#include "menus.h"

#include "gimp-intl.h"


GimpItemFactoryEntry vectors_menu_entries[] =
{
  { { N_("/Path _Tool"), NULL,
      vectors_vectors_tool_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_PATH },
    NULL,
    GIMP_HELP_TOOL_VECTORS, NULL },
  { { N_("/_Edit Path Attributes..."), NULL,
      vectors_edit_attributes_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    GIMP_HELP_PATH_EDIT, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/_New Path..."), "",
      vectors_new_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    GIMP_HELP_PATH_NEW, NULL },
  { { N_("/_Raise Path"), "",
      vectors_raise_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_UP },
    NULL,
    GIMP_HELP_PATH_RAISE, NULL },
  { { N_("/_Lower Path"), "",
      vectors_lower_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_DOWN },
    NULL,
    GIMP_HELP_PATH_LOWER, NULL },
  { { N_("/D_uplicate Path"), NULL,
      vectors_duplicate_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    GIMP_HELP_PATH_DUPLICATE, NULL },
  { { N_("/_Delete Path"), "",
      vectors_delete_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    GIMP_HELP_PATH_DELETE, NULL },
  { { N_("/Merge _Visible Paths"), "",
      vectors_merge_visible_cmd_callback, 0,
      NULL, NULL },
    NULL,
    GIMP_HELP_PATH_MERGE_VISIBLE, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Path to Sele_ction"), NULL,
      vectors_to_selection_cmd_callback, GIMP_CHANNEL_OP_REPLACE,
      "<StockItem>", GIMP_STOCK_SELECTION_REPLACE },
    NULL,
    GIMP_HELP_PATH_SELECTION_REPLACE, NULL },
  { { N_("/_Add to Selection"), NULL,
      vectors_to_selection_cmd_callback, GIMP_CHANNEL_OP_ADD,
      "<StockItem>", GIMP_STOCK_SELECTION_ADD },
    NULL,
    GIMP_HELP_PATH_SELECTION_ADD, NULL },
  { { N_("/_Subtract from Selection"), NULL,
      vectors_to_selection_cmd_callback, GIMP_CHANNEL_OP_SUBTRACT,
      "<StockItem>", GIMP_STOCK_SELECTION_SUBTRACT },
    NULL,
    GIMP_HELP_PATH_SELECTION_SUBTRACT, NULL },
  { { N_("/_Intersect with Selection"), NULL,
      vectors_to_selection_cmd_callback, GIMP_CHANNEL_OP_INTERSECT,
      "<StockItem>", GIMP_STOCK_SELECTION_INTERSECT },
    NULL,
    GIMP_HELP_PATH_SELECTION_INTERSECT, NULL },

  { { N_("/Selecti_on to Path"), NULL,
      vectors_selection_to_vectors_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_TO_PATH },
    NULL,
    GIMP_HELP_SELECTION_TO_PATH, NULL },
  { { N_("/Stro_ke Path..."), NULL,
      vectors_stroke_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_PATH_STROKE },
    NULL,
    GIMP_HELP_PATH_STROKE, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Co_py Path"), "",
      vectors_copy_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_COPY },
    NULL,
    GIMP_HELP_PATH_COPY, NULL },

  { { N_("/Paste Pat_h"), "",
      vectors_paste_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_PASTE },
    NULL,
    GIMP_HELP_PATH_PASTE, NULL },
  { { N_("/I_mport Path..."), "",
      vectors_import_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    NULL,
    GIMP_HELP_PATH_IMPORT, NULL },
  { { N_("/E_xport Path..."), "",
      vectors_export_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SAVE },
    NULL,
    GIMP_HELP_PATH_EXPORT, NULL }
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

      mask_empty = gimp_channel_is_empty (gimp_image_get_mask (gimage));

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

  SET_SENSITIVE ("/Path Tool",                vectors);
  SET_SENSITIVE ("/Edit Path Attributes...",  vectors);

  SET_SENSITIVE ("/New Path...",              gimage);
  SET_SENSITIVE ("/Raise Path",               vectors && prev);
  SET_SENSITIVE ("/Lower Path",               vectors && next);
  SET_SENSITIVE ("/Duplicate Path",           vectors);
  SET_SENSITIVE ("/Delete Path",              vectors);

  SET_SENSITIVE ("/Path to Selection",        vectors);
  SET_SENSITIVE ("/Add to Selection",         vectors);
  SET_SENSITIVE ("/Subtract from Selection",  vectors);
  SET_SENSITIVE ("/Intersect with Selection", vectors);

  SET_SENSITIVE ("/Selection to Path",        ! mask_empty);
  SET_SENSITIVE ("/Stroke Path...",           vectors);

  SET_SENSITIVE ("/Copy Path",                vectors);
  SET_SENSITIVE ("/Paste Path",               global_buf);
  SET_SENSITIVE ("/Import Path...",           gimage);
  SET_SENSITIVE ("/Export Path...",           vectors);

#undef SET_SENSITIVE
}
