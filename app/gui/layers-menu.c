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
#include "core/gimplayer.h"
#include "core/gimplist.h"

#include "widgets/gimpitemfactory.h"
#include "widgets/gimpitemlistview.h"
#include "widgets/gimpitemtreeview.h"

#include "layers-commands.h"
#include "layers-menu.h"
#include "menus.h"

#include "libgimp/gimpintl.h"


GimpItemFactoryEntry layers_menu_entries[] =
{
  { { N_("/New Layer..."), "<control>N",
      layers_new_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    "dialogs/new_layer.html", NULL },

  { { N_("/Raise Layer"), "<control>F",
      layers_raise_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_UP },
    NULL,
    "stack/stack.html#raise_layer", NULL },
  { { N_("/Layer to Top"), "<control><shift>F",
      layers_raise_to_top_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GOTO_TOP },
    NULL,
    "stack/stack.html#later_to_top", NULL },
  { { N_("/Lower Layer"), "<control>B",
      layers_lower_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_DOWN },
    NULL,
    "stack/stack.html#lower_layer", NULL },
  { { N_("/Layer to Bottom"), "<control><shift>B",
      layers_lower_to_bottom_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GOTO_BOTTOM },
    NULL,
    "stack/stack.html#layer_to_bottom", NULL },

  { { N_("/Duplicate Layer"), "<control>C",
      layers_duplicate_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    "duplicate_layer.html", NULL },
  { { N_("/Anchor Layer"), "<control>H",
      layers_anchor_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_ANCHOR },
    NULL,
    "anchor_layer.html", NULL },
  { { N_("/Merge Down"), "<control><shift>M",
      layers_merge_down_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_MERGE_DOWN },
    NULL,
    "merge_down.html", NULL },
  { { N_("/Delete Layer"), "<control>X",
      layers_delete_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    "delete_layer.html", NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Layer Boundary Size..."), "<control>R",
      layers_resize_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_RESIZE },
    NULL,
    "dialogs/layer_boundary_size.html", NULL },
  { { N_("/Layer to Imagesize"), NULL,
      layers_resize_to_image_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_LAYER_TO_IMAGESIZE },
    NULL,
    "layer_to_image_size.html", NULL },
  { { N_("/Scale Layer..."), "<control>S",
      layers_scale_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SCALE },
    NULL,
    "dialogs/scale_layer.html", NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Add Layer Mask..."), NULL,
      layers_add_layer_mask_cmd_callback, 0 },
    NULL,
    "dialogs/add_layer_mask.html", NULL },
  { { N_("/Apply Layer Mask"), NULL,
      layers_apply_layer_mask_cmd_callback, 0 },
    NULL,
    "apply_mask.html", NULL },
  { { N_("/Delete Layer Mask"), NULL,
      layers_delete_layer_mask_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    "delete_mask.html", NULL },
  { { N_("/Mask to Selection"), NULL,
      layers_mask_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_REPLACE },
    NULL,
    "mask_to_selection.html", NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Add Alpha Channel"), NULL,
      layers_add_alpha_channel_cmd_callback, 0 },
    NULL,
    "add_alpha_channel.html", NULL },
  { { N_("/Alpha to Selection"), NULL,
      layers_alpha_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_REPLACE },
    NULL,
    "alpha_to_selection.html", NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Edit Layer Attributes..."), NULL,
      layers_edit_attributes_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    "dialogs/edit_layer_attributes.html", NULL }
};

gint n_layers_menu_entries = G_N_ELEMENTS (layers_menu_entries);


void
layers_menu_update (GtkItemFactory *factory,
                    gpointer        data)
{
  GimpImage *gimage;
  GimpLayer *layer;
  gboolean   fs         = FALSE;    /*  floating sel           */
  gboolean   ac         = FALSE;    /*  active channel         */
  gboolean   lm         = FALSE;    /*  layer mask             */
  gboolean   lp         = FALSE;    /*  layers present         */
  gboolean   alpha      = FALSE;    /*  alpha channel present  */
  gboolean   indexed    = FALSE;    /*  is indexed             */
  gboolean   next_alpha = FALSE;
  GList     *list;
  GList     *next       = NULL;
  GList     *prev       = NULL;

  if (GIMP_IS_ITEM_LIST_VIEW (data))
    gimage = GIMP_ITEM_LIST_VIEW (data)->gimage;
  else if (GIMP_IS_ITEM_TREE_VIEW (data))
    gimage = GIMP_ITEM_TREE_VIEW (data)->gimage;
  else
    return;

  layer = gimp_image_get_active_layer (gimage);

  if (layer)
    lm = (gimp_layer_get_mask (layer)) ? TRUE : FALSE;

  fs = (gimp_image_floating_sel (gimage) != NULL);
  ac = (gimp_image_get_active_channel (gimage) != NULL);

  alpha = layer && gimp_drawable_has_alpha (GIMP_DRAWABLE (layer));

  lp      = ! gimp_image_is_empty (gimage);
  indexed = (gimp_image_base_type (gimage) == GIMP_INDEXED);

  for (list = GIMP_LIST (gimage->layers)->list;
       list;
       list = g_list_next (list)) 
    {
      if (layer == (GimpLayer *) list->data)
	{
	  prev = g_list_previous (list);
	  next = g_list_next (list);
	  break;
	}
    }

  if (next)
    next_alpha = gimp_drawable_has_alpha (GIMP_DRAWABLE (next->data));
  else
    next_alpha = FALSE;

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/New Layer...", gimage);

  SET_SENSITIVE ("/Raise Layer",     !fs && !ac && gimage && lp && alpha && prev);
  SET_SENSITIVE ("/Layer to Top",    !fs && !ac && gimage && lp && alpha && prev);

  SET_SENSITIVE ("/Lower Layer",     !fs && !ac && gimage && lp && next && next_alpha);
  SET_SENSITIVE ("/Layer to Bottom", !fs && !ac && gimage && lp && next && next_alpha);

  SET_SENSITIVE ("/Duplicate Layer", !fs && !ac && gimage && lp);
  SET_SENSITIVE ("/Anchor Layer",     fs && !ac && gimage && lp);
  SET_SENSITIVE ("/Merge Down",      !fs && !ac && gimage && lp && next);
  SET_SENSITIVE ("/Delete Layer",    !ac && gimage && lp);

  SET_SENSITIVE ("/Layer Boundary Size...", !ac && gimage && lp);
  SET_SENSITIVE ("/Layer to Imagesize",     !ac && gimage && lp);
  SET_SENSITIVE ("/Scale Layer...",         !ac && gimage && lp);

  SET_SENSITIVE ("/Add Layer Mask...", !fs && !ac && gimage && !lm && lp && alpha && !indexed);
  SET_SENSITIVE ("/Apply Layer Mask",  !fs && !ac && gimage && lm && lp);
  SET_SENSITIVE ("/Delete Layer Mask", !fs && !ac && gimage && lm && lp);
  SET_SENSITIVE ("/Mask to Selection", !fs && !ac && gimage && lm && lp);

  SET_SENSITIVE ("/Add Alpha Channel",  !fs && !alpha);
  SET_SENSITIVE ("/Alpha to Selection", !fs && !ac && gimage && lp && alpha);

  SET_SENSITIVE ("/Edit Layer Attributes...", !fs && !ac && gimage && lp);

#undef SET_SENSITIVE
}
