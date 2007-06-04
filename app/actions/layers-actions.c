/* GIMP - The GNU Image Manipulation Program
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

#include "actions-types.h"

#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimplist.h"

#include "text/gimptextlayer.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpactiongroup.h"

#include "actions.h"
#include "image-commands.h"
#include "layers-actions.h"
#include "layers-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry layers_actions[] =
{
  { "layers-popup", GIMP_STOCK_LAYERS,
    N_("Layers Menu"), NULL, NULL, NULL,
    GIMP_HELP_LAYER_DIALOG },

  { "layers-menu",              NULL,                      N_("_Layer")        },
  { "layers-stack-menu",        NULL,                      N_("Stac_k")        },
  { "layers-text-to-selection-menu", GIMP_STOCK_TOOL_TEXT, N_("Te_xt to Selection") },
  { "layers-mask-menu",         NULL,                      N_("_Mask")         },
  { "layers-transparency-menu", NULL,                      N_("Tr_ansparency") },
  { "layers-transform-menu",    NULL,                      N_("_Transform")    },
  { "layers-properties-menu",   GTK_STOCK_PROPERTIES,      N_("_Properties")   },
  { "layers-opacity-menu",      GIMP_STOCK_TRANSPARENCY,   N_("_Opacity")      },
  { "layers-mode-menu",         GIMP_STOCK_TOOL_PENCIL,    N_("Layer _Mode")   },

  { "layers-text-tool", GIMP_STOCK_TOOL_TEXT,
    N_("Te_xt Tool"), NULL,
    N_("Activate the text tool on this text layer"),
    G_CALLBACK (layers_text_tool_cmd_callback),
    GIMP_HELP_TOOL_TEXT },

  { "layers-edit-attributes", GTK_STOCK_EDIT,
    N_("_Edit Layer Attributes..."), NULL,
    N_("Edit the layer's name"),
    G_CALLBACK (layers_edit_attributes_cmd_callback),
    GIMP_HELP_LAYER_EDIT },

  { "layers-new", GTK_STOCK_NEW,
    N_("_New Layer..."), "<control><shift>N",
    N_("Create a new layer and add it to the image"),
    G_CALLBACK (layers_new_cmd_callback),
    GIMP_HELP_LAYER_NEW },

  { "layers-new-last-values", GTK_STOCK_NEW,
    N_("_New Layer"), "",
    N_("Create a new layer with last used values"),
    G_CALLBACK (layers_new_last_vals_cmd_callback),
    GIMP_HELP_LAYER_NEW },

  { "layers-duplicate", GIMP_STOCK_DUPLICATE,
    N_("D_uplicate Layer"), "<control><shift>D",
    N_("Create a duplicate of the layer and add it to the image"),
    G_CALLBACK (layers_duplicate_cmd_callback),
    GIMP_HELP_LAYER_DUPLICATE },

  { "layers-delete", GTK_STOCK_DELETE,
    N_("_Delete Layer"), "",
    N_("Delete this layer"),
    G_CALLBACK (layers_delete_cmd_callback),
    GIMP_HELP_LAYER_DELETE },

  { "layers-raise", GTK_STOCK_GO_UP,
    N_("_Raise Layer"), "",
    N_("Raise this layer one step in the layer stack"),
    G_CALLBACK (layers_raise_cmd_callback),
    GIMP_HELP_LAYER_RAISE },

  { "layers-raise-to-top", GTK_STOCK_GOTO_TOP,
    N_("Layer to _Top"), "",
    N_("Move this layer to the top of the layer stack"),
    G_CALLBACK (layers_raise_to_top_cmd_callback),
    GIMP_HELP_LAYER_RAISE_TO_TOP },

  { "layers-lower", GTK_STOCK_GO_DOWN,
    N_("_Lower Layer"), "",
    N_("Lower this layer one step in the layer stack"),
    G_CALLBACK (layers_lower_cmd_callback),
    GIMP_HELP_LAYER_LOWER },

  { "layers-lower-to-bottom", GTK_STOCK_GOTO_BOTTOM,
    N_("Layer to _Bottom"), "",
    N_("Move this layer to the bottom of the layer stack"),
    G_CALLBACK (layers_lower_to_bottom_cmd_callback),
    GIMP_HELP_LAYER_LOWER_TO_BOTTOM },

  { "layers-anchor", GIMP_STOCK_ANCHOR,
    N_("_Anchor Layer"), "<control>H",
    N_("Anchor the floating layer"),
    G_CALLBACK (layers_anchor_cmd_callback),
    GIMP_HELP_LAYER_ANCHOR },

  { "layers-merge-down", GIMP_STOCK_MERGE_DOWN,
    N_("Merge Do_wn"), NULL,
    N_("Merge this layer with the one below it"),
    G_CALLBACK (layers_merge_down_cmd_callback),
    GIMP_HELP_LAYER_MERGE_DOWN },

  { "layers-merge-layers", NULL,
    N_("Merge _Visible Layers..."), NULL,
    N_("Merge all visible layers into one layer"),
    G_CALLBACK (image_merge_layers_cmd_callback),
    GIMP_HELP_IMAGE_MERGE_LAYERS },

  { "layers-flatten-image", NULL,
    N_("_Flatten Image"), NULL,
    N_("Merge all layers into one and remove transparency"),
    G_CALLBACK (image_flatten_image_cmd_callback),
    GIMP_HELP_IMAGE_FLATTEN },

  { "layers-text-discard", GIMP_STOCK_TOOL_TEXT,
    N_("_Discard Text Information"), NULL,
    N_("Turn this text layer into a normal layer"),
    G_CALLBACK (layers_text_discard_cmd_callback),
    GIMP_HELP_LAYER_TEXT_DISCARD },

  { "layers-text-to-vectors", GIMP_STOCK_TOOL_TEXT,
    N_("Text to _Path"), NULL,
    N_("Create a path from this text layer"),
    G_CALLBACK (layers_text_to_vectors_cmd_callback),
    GIMP_HELP_LAYER_TEXT_TO_PATH },

  { "layers-text-along-vectors", GIMP_STOCK_TOOL_TEXT,
    N_("Text alon_g Path"), NULL,
    N_("Warp this layer's text along the current path"),
    G_CALLBACK (layers_text_along_vectors_cmd_callback),
    GIMP_HELP_LAYER_TEXT_ALONG_PATH },

  { "layers-resize", GIMP_STOCK_RESIZE,
    N_("Layer B_oundary Size..."), NULL,
    N_("Adjust the layer dimensions"),
    G_CALLBACK (layers_resize_cmd_callback),
    GIMP_HELP_LAYER_RESIZE },

  { "layers-resize-to-image", GIMP_STOCK_LAYER_TO_IMAGESIZE,
    N_("Layer to _Image Size"), NULL,
    N_("Resize the layer to the size of the image"),
    G_CALLBACK (layers_resize_to_image_cmd_callback),
    GIMP_HELP_LAYER_RESIZE_TO_IMAGE },

  { "layers-scale", GIMP_STOCK_SCALE,
    N_("_Scale Layer..."), NULL,
    N_("Change the size of the layer content"),
    G_CALLBACK (layers_scale_cmd_callback),
    GIMP_HELP_LAYER_SCALE },

  { "layers-crop", GIMP_STOCK_TOOL_CROP,
    N_("_Crop to Selection"), NULL,
    N_("Crop the layer to the extents of the selection"),
    G_CALLBACK (layers_crop_cmd_callback),
    GIMP_HELP_LAYER_CROP },

  { "layers-mask-add", GIMP_STOCK_LAYER_MASK,
    N_("Add La_yer Mask..."), NULL,
    N_("Add a mask that allows non-destructive editing of transparency"),
    G_CALLBACK (layers_mask_add_cmd_callback),
    GIMP_HELP_LAYER_MASK_ADD },

  { "layers-alpha-add", GIMP_STOCK_TRANSPARENCY,
    N_("Add Alpha C_hannel"), NULL,
    N_("Add transparency information to the layer"),
    G_CALLBACK (layers_alpha_add_cmd_callback),
    GIMP_HELP_LAYER_ALPHA_ADD },

  { "layers-alpha-remove", NULL,
    N_("_Remove Alpha Channel"), NULL,
    N_("Remove transparency information from the layer"),
    G_CALLBACK (layers_alpha_remove_cmd_callback),
    GIMP_HELP_LAYER_ALPHA_REMOVE }
};

static const GimpToggleActionEntry layers_toggle_actions[] =
{
  { "layers-lock-alpha", GIMP_STOCK_TRANSPARENCY,
    N_("Lock Alph_a Channel"), NULL,
    N_("Keep transparency information on this layer from being modified"),
    G_CALLBACK (layers_lock_alpha_cmd_callback),
    FALSE,
    GIMP_HELP_LAYER_LOCK_ALPHA },

  { "layers-mask-edit", GTK_STOCK_EDIT,
    N_("_Edit Layer Mask"), NULL,
    N_("Work on the layer mask"),
    G_CALLBACK (layers_mask_edit_cmd_callback),
    FALSE,
    GIMP_HELP_LAYER_MASK_EDIT },

  { "layers-mask-show", GIMP_STOCK_VISIBLE,
    N_("S_how Layer Mask"), NULL, NULL,
    G_CALLBACK (layers_mask_show_cmd_callback),
    FALSE,
    GIMP_HELP_LAYER_MASK_SHOW },

  { "layers-mask-disable", NULL,
    N_("_Disable Layer Mask"), NULL,
    N_("Dismiss the effect of the layer mask"),
    G_CALLBACK (layers_mask_disable_cmd_callback),
    FALSE,
    GIMP_HELP_LAYER_MASK_DISABLE }
};

static const GimpEnumActionEntry layers_mask_apply_actions[] =
{
  { "layers-mask-apply", NULL,
    N_("Apply Layer _Mask"), NULL,
    N_("Apply the effect of the layer mask and remove it"),
    GIMP_MASK_APPLY, FALSE,
    GIMP_HELP_LAYER_MASK_APPLY },

  { "layers-mask-delete", GTK_STOCK_DELETE,
    N_("Delete Layer Mas_k"), "",
    N_("Remove the layer mask and its effect"),
    GIMP_MASK_DISCARD, FALSE,
    GIMP_HELP_LAYER_MASK_DELETE }
};

static const GimpEnumActionEntry layers_mask_to_selection_actions[] =
{
  { "layers-mask-selection-replace", GIMP_STOCK_SELECTION_REPLACE,
    N_("_Mask to Selection"), NULL,
    N_("Replace the selection with the layer mask"),
    GIMP_CHANNEL_OP_REPLACE, FALSE,
    GIMP_HELP_LAYER_MASK_SELECTION_REPLACE },

  { "layers-mask-selection-add", GIMP_STOCK_SELECTION_ADD,
    N_("_Add to Selection"), NULL,
    N_("Add the layer mask to the current selection"),
    GIMP_CHANNEL_OP_ADD, FALSE,
    GIMP_HELP_LAYER_MASK_SELECTION_ADD },

  { "layers-mask-selection-subtract", GIMP_STOCK_SELECTION_SUBTRACT,
    N_("_Subtract from Selection"), NULL,
    N_("Subtract the layer mask from the current selection"),
    GIMP_CHANNEL_OP_SUBTRACT, FALSE,
    GIMP_HELP_LAYER_MASK_SELECTION_SUBTRACT },

  { "layers-mask-selection-intersect", GIMP_STOCK_SELECTION_INTERSECT,
    N_("_Intersect with Selection"), NULL,
    N_("Intersect the layer mask with the current selection"),
    GIMP_CHANNEL_OP_INTERSECT, FALSE,
    GIMP_HELP_LAYER_MASK_SELECTION_INTERSECT }
};

static const GimpEnumActionEntry layers_alpha_to_selection_actions[] =
{
  { "layers-alpha-selection-replace", GIMP_STOCK_SELECTION_REPLACE,
    N_("Al_pha to Selection"), NULL,
    N_("Replace the selection with the layer's alpha channel"),
    GIMP_CHANNEL_OP_REPLACE, FALSE,
    GIMP_HELP_LAYER_ALPHA_SELECTION_REPLACE },

  { "layers-alpha-selection-add", GIMP_STOCK_SELECTION_ADD,
    N_("A_dd to Selection"), NULL,
    N_("Add the layer's alpha channel to the current selection"),
    GIMP_CHANNEL_OP_ADD, FALSE,
    GIMP_HELP_LAYER_ALPHA_SELECTION_ADD },

  { "layers-alpha-selection-subtract", GIMP_STOCK_SELECTION_SUBTRACT,
    N_("_Subtract from Selection"), NULL,
    N_("Subtract the layer's alpha channel from the current selection"),
    GIMP_CHANNEL_OP_SUBTRACT, FALSE,
    GIMP_HELP_LAYER_ALPHA_SELECTION_SUBTRACT },

  { "layers-alpha-selection-intersect", GIMP_STOCK_SELECTION_INTERSECT,
    N_("_Intersect with Selection"), NULL,
    N_("Intersect the layer's alpha channel with the current selection"),
    GIMP_CHANNEL_OP_INTERSECT, FALSE,
    GIMP_HELP_LAYER_ALPHA_SELECTION_INTERSECT }
};

static const GimpEnumActionEntry layers_text_to_selection_actions[] =
{
  { "layers-text-selection-replace", GIMP_STOCK_SELECTION_REPLACE,
    N_("_Text to Selection"), NULL,
    N_("Replace the selection with the text layer's outline"),
    GIMP_CHANNEL_OP_REPLACE, FALSE,
    GIMP_HELP_LAYER_TEXT_SELECTION_REPLACE },

  { "layers-text-selection-add", GIMP_STOCK_SELECTION_ADD,
    N_("A_dd to Selection"), NULL,
    N_("Add the text layer's outline to the current selection"),
    GIMP_CHANNEL_OP_ADD, FALSE,
    GIMP_HELP_LAYER_TEXT_SELECTION_ADD },

  { "layers-text-selection-subtract", GIMP_STOCK_SELECTION_SUBTRACT,
    N_("_Subtract from Selection"), NULL,
    N_("Subtract the text layer's outline from the current selection"),
    GIMP_CHANNEL_OP_SUBTRACT, FALSE,
    GIMP_HELP_LAYER_TEXT_SELECTION_SUBTRACT },

  { "layers-text-selection-intersect", GIMP_STOCK_SELECTION_INTERSECT,
    N_("_Intersect with Selection"), NULL,
    N_("Intersect the text layer's outline with the current selection"),
    GIMP_CHANNEL_OP_INTERSECT, FALSE,
    GIMP_HELP_LAYER_TEXT_SELECTION_INTERSECT }
};

static const GimpEnumActionEntry layers_select_actions[] =
{
  { "layers-select-top", NULL,
    N_("Select _Top Layer"), "Home",
    N_("Select the topmost layer"),
    GIMP_ACTION_SELECT_FIRST, FALSE,
    GIMP_HELP_LAYER_TOP },

  { "layers-select-bottom", NULL,
    N_("Select _Bottom Layer"), "End",
    N_("Select the bottommost layer"),
    GIMP_ACTION_SELECT_LAST, FALSE,
    GIMP_HELP_LAYER_BOTTOM },

  { "layers-select-previous", NULL,
    N_("Select _Previous Layer"), "Prior",
    N_("Select the layer above the current layer"),
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    GIMP_HELP_LAYER_PREVIOUS },

  { "layers-select-next", NULL,
    N_("Select _Next Layer"), "Next",
    N_("Select the layer below the current layer"),
    GIMP_ACTION_SELECT_NEXT, FALSE,
    GIMP_HELP_LAYER_NEXT }
};

static const GimpEnumActionEntry layers_opacity_actions[] =
{
  { "layers-opacity-set", GIMP_STOCK_TRANSPARENCY,
    N_("Set Opacity"), NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    GIMP_HELP_LAYER_OPACITY },
  { "layers-opacity-transparent", GIMP_STOCK_TRANSPARENCY,
    "Completely Transparent", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    GIMP_HELP_LAYER_OPACITY },
  { "layers-opacity-opaque", GIMP_STOCK_TRANSPARENCY,
    "Completely Opaque", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    GIMP_HELP_LAYER_OPACITY },
  { "layers-opacity-decrease", GIMP_STOCK_TRANSPARENCY,
    "More Transparent", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    GIMP_HELP_LAYER_OPACITY },
  { "layers-opacity-increase", GIMP_STOCK_TRANSPARENCY,
    "More Opaque", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    GIMP_HELP_LAYER_OPACITY },
  { "layers-opacity-decrease-skip", GIMP_STOCK_TRANSPARENCY,
    "10% More Transparent", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    GIMP_HELP_LAYER_OPACITY },
  { "layers-opacity-increase-skip", GIMP_STOCK_TRANSPARENCY,
    "10% More Opaque", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    GIMP_HELP_LAYER_OPACITY }
};

static const GimpEnumActionEntry layers_mode_actions[] =
{
  { "layers-mode-first", GIMP_STOCK_TOOL_PENCIL,
    "First Layer Mode", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    GIMP_HELP_LAYER_MODE },
  { "layers-mode-last", GIMP_STOCK_TOOL_PENCIL,
    "Last Layer Mode", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    GIMP_HELP_LAYER_MODE },
  { "layers-mode-previous", GIMP_STOCK_TOOL_PENCIL,
    "Previous Layer Mode", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    GIMP_HELP_LAYER_MODE },
  { "layers-mode-next", GIMP_STOCK_TOOL_PENCIL,
    "Next Layer Mode", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    GIMP_HELP_LAYER_MODE }
};


void
layers_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 layers_actions,
                                 G_N_ELEMENTS (layers_actions));

  gimp_action_group_add_toggle_actions (group,
                                        layers_toggle_actions,
                                        G_N_ELEMENTS (layers_toggle_actions));

  gimp_action_group_add_enum_actions (group,
                                      layers_mask_apply_actions,
                                      G_N_ELEMENTS (layers_mask_apply_actions),
                                      G_CALLBACK (layers_mask_apply_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      layers_mask_to_selection_actions,
                                      G_N_ELEMENTS (layers_mask_to_selection_actions),
                                      G_CALLBACK (layers_mask_to_selection_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      layers_alpha_to_selection_actions,
                                      G_N_ELEMENTS (layers_alpha_to_selection_actions),
                                      G_CALLBACK (layers_alpha_to_selection_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      layers_text_to_selection_actions,
                                      G_N_ELEMENTS (layers_alpha_to_selection_actions),
                                      G_CALLBACK (layers_alpha_to_selection_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      layers_select_actions,
                                      G_N_ELEMENTS (layers_select_actions),
                                      G_CALLBACK (layers_select_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      layers_opacity_actions,
                                      G_N_ELEMENTS (layers_opacity_actions),
                                      G_CALLBACK (layers_opacity_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      layers_mode_actions,
                                      G_N_ELEMENTS (layers_mode_actions),
                                      G_CALLBACK (layers_mode_cmd_callback));
}

void
layers_actions_update (GimpActionGroup *group,
                       gpointer         data)
{
  GimpImage     *image     = action_data_get_image (data);
  GimpLayer     *layer      = NULL;
  GimpLayerMask *mask       = NULL;     /*  layer mask             */
  gboolean       fs         = FALSE;    /*  floating sel           */
  gboolean       ac         = FALSE;    /*  active channel         */
  gboolean       sel        = FALSE;
  gboolean       alpha      = FALSE;    /*  alpha channel present  */
  gboolean       indexed    = FALSE;    /*  is indexed             */
  gboolean       lock_alpha = FALSE;
  gboolean       text_layer = FALSE;
  GList         *next       = NULL;
  GList         *prev       = NULL;

  if (image)
    {
      fs      = (gimp_image_floating_sel (image) != NULL);
      ac      = (gimp_image_get_active_channel (image) != NULL);
      sel     = ! gimp_channel_is_empty (gimp_image_get_mask (image));
      indexed = (gimp_image_base_type (image) == GIMP_INDEXED);

      layer = gimp_image_get_active_layer (image);

      if (layer)
        {
          GList *list;

          mask       = gimp_layer_get_mask (layer);
          lock_alpha = gimp_layer_get_lock_alpha (layer);
          alpha      = gimp_drawable_has_alpha (GIMP_DRAWABLE (layer));

          list = g_list_find (GIMP_LIST (image->layers)->list, layer);

          if (list)
            {
              prev = g_list_previous (list);
              next = g_list_next (list);
            }

          if (layer)
            text_layer = gimp_drawable_is_text_layer (GIMP_DRAWABLE (layer));
        }
    }

#define SET_VISIBLE(action,condition) \
        gimp_action_group_set_action_visible (group, action, (condition) != 0)
#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)
#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

  SET_VISIBLE   ("layers-text-tool",       text_layer && !ac);
  SET_SENSITIVE ("layers-edit-attributes", layer && !fs && !ac);

  SET_SENSITIVE ("layers-new",             image);
  SET_SENSITIVE ("layers-new-last-values", image);
  SET_SENSITIVE ("layers-duplicate",       layer && !fs && !ac);
  SET_SENSITIVE ("layers-delete",          layer && !ac);

  SET_SENSITIVE ("layers-select-top",      layer && !fs && !ac && prev);
  SET_SENSITIVE ("layers-select-bottom",   layer && !fs && !ac && next);
  SET_SENSITIVE ("layers-select-previous", layer && !fs && !ac && prev);
  SET_SENSITIVE ("layers-select-next",     layer && !fs && !ac && next);

  SET_SENSITIVE ("layers-raise",           layer && !fs && !ac && prev);
  SET_SENSITIVE ("layers-raise-to-top",    layer && !fs && !ac && prev);
  SET_SENSITIVE ("layers-lower",           layer && !fs && !ac && next);
  SET_SENSITIVE ("layers-lower-to-bottom", layer && !fs && !ac && next);

  SET_SENSITIVE ("layers-anchor",          layer &&  fs && !ac);
  SET_SENSITIVE ("layers-merge-down",      layer && !fs && !ac && next);
  SET_SENSITIVE ("layers-merge-layers",    layer && !fs && !ac);
  SET_SENSITIVE ("layers-flatten-image",   layer && !fs && !ac);

  SET_VISIBLE   ("layers-text-discard",             text_layer && !ac);
  SET_VISIBLE   ("layers-text-to-vectors",          text_layer && !ac);
  SET_VISIBLE   ("layers-text-along-vectors",       text_layer && !ac);
  SET_VISIBLE   ("layers-text-selection-replace",   text_layer && !ac);
  SET_VISIBLE   ("layers-text-selection-add",       text_layer && !ac);
  SET_VISIBLE   ("layers-text-selection-subtract",  text_layer && !ac);
  SET_VISIBLE   ("layers-text-selection-intersect", text_layer && !ac);

  SET_SENSITIVE ("layers-resize",          layer && !ac);
  SET_SENSITIVE ("layers-resize-to-image", layer && !ac);
  SET_SENSITIVE ("layers-scale",           layer && !ac);

  SET_SENSITIVE ("layers-crop",            layer && sel);

  SET_SENSITIVE ("layers-alpha-add",       layer && !fs && !alpha);
  SET_SENSITIVE ("layers-alpha-remove",    layer && !fs &&  alpha);

  SET_SENSITIVE ("layers-lock-alpha", layer);
  SET_ACTIVE    ("layers-lock-alpha", lock_alpha);

  SET_SENSITIVE ("layers-mask-add",    layer && !fs && !ac && !mask);
  SET_SENSITIVE ("layers-mask-apply",  layer && !fs && !ac &&  mask);
  SET_SENSITIVE ("layers-mask-delete", layer && !fs && !ac &&  mask);

  SET_SENSITIVE ("layers-mask-edit",    layer && !fs && !ac &&  mask);
  SET_SENSITIVE ("layers-mask-show",    layer && !fs && !ac &&  mask);
  SET_SENSITIVE ("layers-mask-disable", layer && !fs && !ac &&  mask);

  SET_ACTIVE ("layers-mask-edit",    mask && gimp_layer_mask_get_edit (mask));
  SET_ACTIVE ("layers-mask-show",    mask && gimp_layer_mask_get_show (mask));
  SET_ACTIVE ("layers-mask-disable", mask && !gimp_layer_mask_get_apply (mask));

  SET_SENSITIVE ("layers-mask-selection-replace",   layer && !fs && !ac && mask);
  SET_SENSITIVE ("layers-mask-selection-add",       layer && !fs && !ac && mask);
  SET_SENSITIVE ("layers-mask-selection-subtract",  layer && !fs && !ac && mask);
  SET_SENSITIVE ("layers-mask-selection-intersect", layer && !fs && !ac && mask);

  SET_SENSITIVE ("layers-alpha-selection-replace",   layer && !fs && !ac);
  SET_SENSITIVE ("layers-alpha-selection-add",       layer && !fs && !ac);
  SET_SENSITIVE ("layers-alpha-selection-subtract",  layer && !fs && !ac);
  SET_SENSITIVE ("layers-alpha-selection-intersect", layer && !fs && !ac);

#undef SET_VISIBLE
#undef SET_SENSITIVE
#undef SET_ACTIVE
}
