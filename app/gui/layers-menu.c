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

#include "text/gimptextlayer.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimpitemtreeview.h"

#include "layers-commands.h"
#include "layers-menu.h"
#include "menus.h"

#include "gimp-intl.h"


GimpItemFactoryEntry layers_menu_entries[] =
{
  { { N_("/_Edit Layer Attributes..."), NULL,
      layers_edit_attributes_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    GIMP_HELP_LAYER_EDIT, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/_New Layer..."), "",
      layers_new_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    GIMP_HELP_LAYER_NEW, NULL },

  { { N_("/_Raise Layer"), "",
      layers_raise_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_UP },
    NULL,
    GIMP_HELP_LAYER_RAISE, NULL },
  { { N_("/Layer to _Top"), "",
      layers_raise_to_top_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GOTO_TOP },
    NULL,
    GIMP_HELP_LAYER_RAISE_TO_TOP, NULL },
  { { N_("/_Lower Layer"), "",
      layers_lower_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_DOWN },
    NULL,
    GIMP_HELP_LAYER_LOWER, NULL },
  { { N_("/Layer to _Bottom"), "",
      layers_lower_to_bottom_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GOTO_BOTTOM },
    NULL,
    GIMP_HELP_LAYER_LOWER_TO_BOTTOM, NULL },

  { { N_("/D_uplicate Layer"), NULL,
      layers_duplicate_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    GIMP_HELP_LAYER_DUPLICATE, NULL },
  { { N_("/_Anchor Layer"), NULL,
      layers_anchor_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_ANCHOR },
    NULL,
    GIMP_HELP_LAYER_ANCHOR, NULL },
  { { N_("/Merge Do_wn"), NULL,
      layers_merge_down_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_MERGE_DOWN },
    NULL,
    GIMP_HELP_LAYER_MERGE_DOWN, NULL },
  { { N_("/_Delete Layer"), "",
      layers_delete_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    GIMP_HELP_LAYER_DELETE, NULL },
  { { N_("/_Discard Text Information"), NULL,
      layers_text_discard_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_TEXT },
    NULL,
    GIMP_HELP_LAYER_TEXT_DISCARD, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Layer B_oundary Size..."), NULL,
      layers_resize_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_RESIZE },
    NULL,
    GIMP_HELP_LAYER_RESIZE, NULL },
  { { N_("/Layer to _Image Size"), NULL,
      layers_resize_to_image_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_LAYER_TO_IMAGESIZE },
    NULL,
    GIMP_HELP_LAYER_RESIZE_TO_IMAGE, NULL },
  { { N_("/_Scale Layer..."), NULL,
      layers_scale_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SCALE },
    NULL,
    GIMP_HELP_LAYER_SCALE, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Add La_yer Mask..."), NULL,
      layers_mask_add_cmd_callback, 0 },
    NULL,
    GIMP_HELP_LAYER_MASK_ADD, NULL },
  { { N_("/Apply Layer _Mask"), NULL,
      layers_mask_apply_cmd_callback, 0 },
    NULL,
    GIMP_HELP_LAYER_MASK_APPLY, NULL },
  { { N_("/Delete Layer Mas_k"), NULL,
      layers_mask_delete_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    GIMP_HELP_LAYER_MASK_DELETE , NULL },
  { { N_("/Mask to Sele_ction"), NULL,
      layers_mask_to_selection_cmd_callback, GIMP_CHANNEL_OP_REPLACE,
      "<StockItem>", GIMP_STOCK_SELECTION_REPLACE },
    NULL,
    GIMP_HELP_LAYER_MASK_SELECTION_REPLACE, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Add Alpha C_hannel"), NULL,
      layers_alpha_add_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TRANSPARENCY },
    NULL,
    GIMP_HELP_LAYER_ALPHA_ADD, NULL },
  { { N_("/Al_pha to Selection"), NULL,
      layers_alpha_to_selection_cmd_callback, GIMP_CHANNEL_OP_REPLACE,
      "<StockItem>", GIMP_STOCK_SELECTION_REPLACE },
    NULL,
    GIMP_HELP_LAYER_ALPHA_SELECTION_REPLACE, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Merge _Visible Layers..."), NULL,
      layers_merge_layers_cmd_callback, 0,
      NULL, NULL },
    NULL,
    GIMP_HELP_IMAGE_MERGE_LAYERS, NULL },
  { { N_("/_Flatten Image"), NULL,
      layers_flatten_image_cmd_callback, 0,
      NULL, NULL },
    NULL,
    GIMP_HELP_IMAGE_FLATTEN, NULL }
};

gint n_layers_menu_entries = G_N_ELEMENTS (layers_menu_entries);


void
layers_menu_update (GtkItemFactory *factory,
                    gpointer        data)
{
  GimpImage *gimage;
  GimpLayer *layer      = NULL;
  gboolean   fs         = FALSE;    /*  floating sel           */
  gboolean   ac         = FALSE;    /*  active channel         */
  gboolean   lm         = FALSE;    /*  layer mask             */
  gboolean   alpha      = FALSE;    /*  alpha channel present  */
  gboolean   indexed    = FALSE;    /*  is indexed             */
  gboolean   next_alpha = FALSE;
  gboolean   text_layer = FALSE;
  GList     *next       = NULL;
  GList     *prev       = NULL;

  gimage = GIMP_ITEM_TREE_VIEW (data)->gimage;

  if (gimage)
    {
      GList *list;

      layer = gimp_image_get_active_layer (gimage);

      if (layer)
        lm = (gimp_layer_get_mask (layer)) ? TRUE : FALSE;

      fs = (gimp_image_floating_sel (gimage) != NULL);
      ac = (gimp_image_get_active_channel (gimage) != NULL);

      alpha = layer && gimp_drawable_has_alpha (GIMP_DRAWABLE (layer));

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

      text_layer = (layer &&
                    GIMP_IS_TEXT_LAYER (layer) &&
                    GIMP_TEXT_LAYER (layer)->text);
    }

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)
#define SET_VISIBLE(menu,condition) \
        gimp_item_factory_set_visible (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/Edit Layer Attributes...", layer && !fs && !ac);

  SET_SENSITIVE ("/New Layer...",    gimage);

  SET_SENSITIVE ("/Raise Layer",     layer && !fs && !ac && alpha && prev);
  SET_SENSITIVE ("/Layer to Top",    layer && !fs && !ac && alpha && prev);

  SET_SENSITIVE ("/Lower Layer",     layer && !fs && !ac && next && next_alpha);
  SET_SENSITIVE ("/Layer to Bottom", layer && !fs && !ac && next && next_alpha);

  SET_SENSITIVE ("/Duplicate Layer", layer && !fs && !ac);
  SET_SENSITIVE ("/Anchor Layer",    layer &&  fs && !ac);
  SET_SENSITIVE ("/Merge Down",      layer && !fs && !ac && next);
  SET_SENSITIVE ("/Delete Layer",    layer && !ac);
  SET_VISIBLE   ("/Discard Text Information", text_layer && !ac);

  SET_SENSITIVE ("/Layer Boundary Size...", layer && !ac);
  SET_SENSITIVE ("/Layer to Image Size",    layer && !ac);
  SET_SENSITIVE ("/Scale Layer...",         layer && !ac);

  SET_SENSITIVE ("/Add Layer Mask...", layer && !fs && !ac && !lm && alpha);
  SET_SENSITIVE ("/Apply Layer Mask",  layer && !fs && !ac &&  lm);
  SET_SENSITIVE ("/Delete Layer Mask", layer && !fs && !ac &&  lm);
  SET_SENSITIVE ("/Mask to Selection", layer && !fs && !ac &&  lm);

  SET_SENSITIVE ("/Add Alpha Channel",  layer && !fs && !alpha);
  SET_SENSITIVE ("/Alpha to Selection", layer && !fs && !ac && alpha);

  SET_SENSITIVE ("/Merge Visible Layers...", layer && !fs && !ac);
  SET_SENSITIVE ("/Flatten Image",           layer && !fs && !ac);

#undef SET_SENSITIVE
}
