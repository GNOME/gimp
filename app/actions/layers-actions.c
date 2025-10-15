/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimplayer-floating-selection.h"
#include "core/gimplinklayer.h"

#include "text/gimptextlayer.h"

#include "path/gimpvectorlayer.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpactiongroup.h"
#include "widgets/gimpwidgets-utils.h"

#include "actions.h"
#include "image-commands.h"
#include "items-actions.h"
#include "layers-actions.h"
#include "layers-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry layers_actions[] =
{
  { "layers-edit", GIMP_ICON_EDIT,
    NC_("layers-action", "Default Edit Action"), NULL, { NULL },
    NC_("layers-action", "Activate the default edit action for this type of layer"),
    layers_edit_cmd_callback,
    GIMP_HELP_LAYER_EDIT },

  { "layers-edit-text", GIMP_ICON_EDIT,
    NC_("layers-action", "Edit Te_xt on canvas"), NULL, { NULL },
    NC_("layers-action", "Edit this text layer content on canvas"),
    layers_edit_text_cmd_callback,
    GIMP_HELP_LAYER_EDIT },

  { "layers-edit-vector", GIMP_ICON_TOOL_PATH,
    NC_("layers-action", "Path Tool"), NULL, { NULL },
    NC_("layers-action", "Activate the path tool on this vector layer's path"),
    layers_edit_vector_cmd_callback,
    GIMP_HELP_TOOL_PATH },

  { "layers-edit-attributes", GIMP_ICON_EDIT,
    NC_("layers-action", "_Edit Layer Attributes..."), NULL, { NULL },
    NC_("layers-action", "Edit the layer's name"),
    layers_edit_attributes_cmd_callback,
    GIMP_HELP_LAYER_EDIT },

  { "layers-new", GIMP_ICON_DOCUMENT_NEW,
    NC_("layers-action", "_New Layer..."), NULL, { "<primary><shift>N", NULL },
    NC_("layers-action", "Create a new layer and add it to the image"),
    layers_new_cmd_callback,
    GIMP_HELP_LAYER_NEW },

  { "layers-new-last-values", GIMP_ICON_DOCUMENT_NEW,
    NC_("layers-action", "_New Layer"), NULL, { NULL },
    NC_("layers-action", "Create new layers with last used values"),
    layers_new_last_vals_cmd_callback,
    GIMP_HELP_LAYER_NEW },

  { "layers-new-from-visible", NULL,
    NC_("layers-action", "New from _Visible"), NULL, { NULL },
    NC_("layers-action",
        "Create a new layer from what is visible in this image"),
    layers_new_from_visible_cmd_callback,
    GIMP_HELP_LAYER_NEW_FROM_VISIBLE },

  { "layers-new-group", GIMP_ICON_FOLDER_NEW,
    NC_("layers-action", "New Layer _Group"), NULL, { NULL },
    NC_("layers-action", "Create a new layer group and add it to the image"),
    layers_new_group_cmd_callback,
    GIMP_HELP_LAYER_NEW },

  { "layers-duplicate", GIMP_ICON_OBJECT_DUPLICATE,
    NC_("layers-action", "D_uplicate Layers"), NULL, { "<primary><shift>D", NULL },
    NC_("layers-action",
        "Create duplicates of selected layers and add them to the image"),
    layers_duplicate_cmd_callback,
    GIMP_HELP_LAYER_DUPLICATE },

  { "layers-delete", GIMP_ICON_EDIT_DELETE,
    NC_("layers-action", "_Delete Layers"), NULL, { NULL },
    NC_("layers-action", "Delete selected layers"),
    layers_delete_cmd_callback,
    GIMP_HELP_LAYER_DELETE },

  { "layers-raise", GIMP_ICON_GO_UP,
    NC_("layers-action", "_Raise Layers"), NULL, { NULL },
    NC_("layers-action", "Raise the selected layers one step in the layer stack"),
    layers_raise_cmd_callback,
    GIMP_HELP_LAYER_RAISE },

  { "layers-raise-to-top", GIMP_ICON_GO_TOP,
    NC_("layers-action", "Layers to _Top"), NULL, { NULL },
    NC_("layers-action", "Move the selected layers to the top of the layer stack"),
    layers_raise_to_top_cmd_callback,
    GIMP_HELP_LAYER_RAISE_TO_TOP },

  { "layers-lower", GIMP_ICON_GO_DOWN,
    NC_("layers-action", "_Lower Layers"), NULL, { NULL },
    NC_("layers-action", "Lower the selected layers one step in the layer stack"),
    layers_lower_cmd_callback,
    GIMP_HELP_LAYER_LOWER },

  { "layers-lower-to-bottom", GIMP_ICON_GO_BOTTOM,
    NC_("layers-action", "Layers to _Bottom"), NULL, { NULL },
    NC_("layers-action", "Move the selected layers to the bottom of the layer stack"),
    layers_lower_to_bottom_cmd_callback,
    GIMP_HELP_LAYER_LOWER_TO_BOTTOM },

  { "layers-anchor", GIMP_ICON_LAYER_ANCHOR,
    NC_("layers-action", "_Anchor Floating Layer or Mask"), NULL, { "<primary>H", NULL },
    NC_("layers-action", "Anchor the floating layer or mask"),
    layers_anchor_cmd_callback,
    GIMP_HELP_LAYER_ANCHOR },

  { "layers-merge-down", GIMP_ICON_LAYER_MERGE_DOWN,
    NC_("layers-action", "Merge Do_wn"), NULL, { NULL },
    NC_("layers-action", "Merge these layers with the first visible layer below each"),
    layers_merge_down_cmd_callback,
    GIMP_HELP_LAYER_MERGE_DOWN },

  /* this is the same as layers-merge-down, except it's sensitive even if
   * the layer can't be merged down
   */
  { "layers-merge-down-button", GIMP_ICON_LAYER_MERGE_DOWN,
    NC_("layers-action", "Merge Do_wn"), NULL, { NULL },
    NC_("layers-action", "Merge these layers with the first visible layer below each"),
    layers_merge_down_cmd_callback,
    GIMP_HELP_LAYER_MERGE_DOWN },

  { "layers-merge-group", NULL,
    NC_("layers-action", "Merge Layer Groups"), NULL, { NULL },
    NC_("layers-action", "Merge the layer groups' layers into one normal layer"),
    layers_merge_group_cmd_callback,
    GIMP_HELP_LAYER_MERGE_GROUP },

  { "layers-merge-layers", NULL,
    NC_("layers-action", "Merge _Visible Layers..."), NULL, { NULL },
    NC_("layers-action", "Merge all visible layers into one layer"),
    image_merge_layers_cmd_callback,
    GIMP_HELP_IMAGE_MERGE_LAYERS },

  { "layers-merge-layers-last-values", NULL,
    NC_("layers-action", "Merge _Visible Layers"), NULL, { NULL },
    NC_("layers-action", "Merge all visible layers with last used values"),
    image_merge_layers_last_vals_cmd_callback,
    GIMP_HELP_IMAGE_MERGE_LAYERS },

  { "layers-flatten-image", NULL,
    NC_("layers-action", "_Flatten Image"), NULL, { NULL },
    NC_("layers-action", "Merge all layers into one and remove transparency"),
    image_flatten_image_cmd_callback,
    GIMP_HELP_IMAGE_FLATTEN },

  { "layers-rasterize", GIMP_ICON_TOOL_TEXT,
    NC_("layers-action", "_Rasterize"), NULL, { NULL },
    NC_("layers-action", "Turn selected text, link or vector layers into raster layers"),
    layers_rasterize_cmd_callback,
    GIMP_HELP_LAYER_TEXT_DISCARD },

  { "layers-revert-rasterize", GIMP_ICON_TOOL_TEXT,
    NC_("layers-action", "_Revert Rasterize"), NULL, { NULL },
    NC_("layers-action", "Turn rasterized layers back into text, link or vector layers"),
    layers_revert_rasterize_cmd_callback,
    GIMP_HELP_LAYER_TEXT_DISCARD },

  { "layers-text-to-path", GIMP_ICON_TOOL_TEXT,
    NC_("layers-action", "Text to _Path"), NULL, { NULL },
    NC_("layers-action", "Create paths from text layers"),
    layers_text_to_path_cmd_callback,
    GIMP_HELP_LAYER_TEXT_TO_PATH },

  { "layers-text-along-path", GIMP_ICON_TOOL_TEXT,
    NC_("layers-action", "Text alon_g Path"), NULL, { NULL },
    NC_("layers-action", "Warp this layer's text along the current path"),
    layers_text_along_path_cmd_callback,
    GIMP_HELP_LAYER_TEXT_ALONG_PATH },

  { "layers-resize", GIMP_ICON_OBJECT_RESIZE,
    NC_("layers-action", "Layer B_oundary Size..."), NULL, { NULL },
    NC_("layers-action", "Adjust the layer dimensions"),
    layers_resize_cmd_callback,
    GIMP_HELP_LAYER_RESIZE },

  { "layers-resize-to-image", GIMP_ICON_LAYER_TO_IMAGESIZE,
    NC_("layers-action", "Layers to _Image Size"), NULL, { NULL },
    NC_("layers-action", "Resize the layers to the size of the image"),
    layers_resize_to_image_cmd_callback,
    GIMP_HELP_LAYER_RESIZE_TO_IMAGE },

  { "layers-scale", GIMP_ICON_OBJECT_SCALE,
    NC_("layers-action", "_Scale Layer..."), NULL, { NULL },
    NC_("layers-action", "Change the size of the layer content"),
    layers_scale_cmd_callback,
    GIMP_HELP_LAYER_SCALE },

  { "layers-crop-to-selection", GIMP_ICON_TOOL_CROP,
    NC_("layers-action", "_Resize Layers to Selection"), NULL, { NULL },
    NC_("layers-action", "Resize the layers to the extents of the selection"),
    layers_crop_to_selection_cmd_callback,
    GIMP_HELP_LAYER_RESIZE_TO_SELECTION },

  { "layers-crop-to-content", GIMP_ICON_TOOL_CROP,
    NC_("layers-action", "Crop Layers to C_ontent"), NULL, { NULL },
    NC_("layers-action", "Crop the layers to the extents of their content (remove empty borders from the layer)"),
    layers_crop_to_content_cmd_callback,
    GIMP_HELP_LAYER_CROP_TO_CONTENT },

  { "layers-mask-add", GIMP_ICON_LAYER_MASK,
    NC_("layers-action", "Add La_yer Masks..."), NULL, { NULL },
    NC_("layers-action",
        "Add masks to selected layers that allows non-destructive editing of transparency"),
    layers_mask_add_cmd_callback,
    GIMP_HELP_LAYER_MASK_ADD },

  /* this is the same as layers-mask-add, except it's sensitive even if
   * there is a mask on the layer
   */
  { "layers-mask-add-button", GIMP_ICON_LAYER_MASK,
    NC_("layers-action", "Add La_yer Masks..."), NULL, { NULL },
    NC_("layers-action",
        "Add masks to selected layers that allows non-destructive editing of transparency"),
    layers_mask_add_cmd_callback,
    GIMP_HELP_LAYER_MASK_ADD },

  { "layers-mask-add-last-values", GIMP_ICON_LAYER_MASK,
    NC_("layers-action", "Add La_yer Masks with Last Values"), NULL, { NULL },
    NC_("layers-action",
        "Add mask to selected layers with last used values"),
    layers_mask_add_last_vals_cmd_callback,
    GIMP_HELP_LAYER_MASK_ADD },

  { "layers-alpha-add", GIMP_ICON_TRANSPARENCY,
    NC_("layers-action", "Add Alpha C_hannel"), NULL, { NULL },
    NC_("layers-action", "Add transparency information to the layer"),
    layers_alpha_add_cmd_callback,
    GIMP_HELP_LAYER_ALPHA_ADD },

  { "layers-alpha-remove", NULL,
    NC_("layers-action", "_Remove Alpha Channel"), NULL, { NULL },
    NC_("layers-action", "Remove transparency information from the layer"),
    layers_alpha_remove_cmd_callback,
    GIMP_HELP_LAYER_ALPHA_REMOVE }
};

static const GimpToggleActionEntry layers_toggle_actions[] =
{
  { "layers-mask-edit", GIMP_ICON_EDIT,
    NC_("layers-action", "_Edit Layer Mask"), NULL, { NULL },
    NC_("layers-action", "Work on the layer mask"),
    layers_mask_edit_cmd_callback,
    FALSE,
    GIMP_HELP_LAYER_MASK_EDIT },

  { "layers-mask-show", GIMP_ICON_VISIBLE,
    NC_("layers-action", "S_how Layer Masks"), NULL, { NULL }, NULL,
    layers_mask_show_cmd_callback,
    FALSE,
    GIMP_HELP_LAYER_MASK_SHOW },

  { "layers-mask-disable", NULL,
    NC_("layers-action", "_Disable Layer Masks"), NULL, { NULL },
    NC_("layers-action", "Dismiss the effect of the layer masks"),
    layers_mask_disable_cmd_callback,
    FALSE,
    GIMP_HELP_LAYER_MASK_DISABLE },

  { "layers-visible", GIMP_ICON_VISIBLE,
    NC_("layers-action", "Toggle Layer _Visibility"), NULL, { NULL }, NULL,
    layers_visible_cmd_callback,
    FALSE,
    GIMP_HELP_LAYER_VISIBLE },

  { "layers-lock-content", GIMP_ICON_LOCK_CONTENT,
    NC_("layers-action", "L_ock Pixels of Layer"), NULL, { NULL }, NULL,
    layers_lock_content_cmd_callback,
    FALSE,
    GIMP_HELP_LAYER_LOCK_PIXELS },

  { "layers-lock-position", GIMP_ICON_LOCK_POSITION,
    NC_("layers-action", "L_ock Position of Layer"), NULL, { NULL }, NULL,
    layers_lock_position_cmd_callback,
    FALSE,
    GIMP_HELP_LAYER_LOCK_POSITION },

  { "layers-lock-alpha", GIMP_ICON_LOCK_ALPHA,
    NC_("layers-action", "Lock Alph_a Channel"), NULL, { NULL },
    NC_("layers-action",
        "Keep transparency information on this layer from being modified"),
    layers_lock_alpha_cmd_callback,
    FALSE,
    GIMP_HELP_LAYER_LOCK_ALPHA },
};

static const GimpRadioActionEntry layers_blend_space_actions[] =
{
  { "layers-blend-space-auto", NULL,
    NC_("layers-action", "Auto"), NULL, { NULL },
    NC_("layers-action", "Layer Blend Space: Auto"),
    GIMP_LAYER_COLOR_SPACE_AUTO,
    NULL },

  { "layers-blend-space-rgb-linear", NULL,
    NC_("layers-action", "RGB (linear)"), NULL, { NULL },
    NC_("layers-action", "Layer Blend Space: RGB (linear)"),
    GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    NULL },

  { "layers-blend-space-rgb-non-linear", NULL,
    NC_("layers-action", "RGB (from color profile)"), NULL, { NULL },
    NC_("layers-action", "Layer Blend Space: RGB (from color profile)"),
    GIMP_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    NULL },

  { "layers-blend-space-rgb-perceptual", NULL,
    NC_("layers-action", "RGB (perceptual)"), NULL, { NULL },
    NC_("layers-action", "Layer Blend Space: RGB (perceptual)"),
    GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    NULL }
};

static const GimpRadioActionEntry layers_composite_space_actions[] =
{
  { "layers-composite-space-auto", NULL,
    NC_("layers-action", "Auto"), NULL, { NULL },
    NC_("layers-action", "Layer Composite Space: Auto"),
    GIMP_LAYER_COLOR_SPACE_AUTO,
    NULL },

  { "layers-composite-space-rgb-linear", NULL,
    NC_("layers-action", "RGB (linear)"), NULL, { NULL },
    NC_("layers-action", "Layer Composite Space: RGB (linear)"),
    GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
    NULL },

  { "layers-composite-space-rgb-non-linear", NULL,
    NC_("layers-action", "RGB (from color profile)"), NULL, { NULL },
    NC_("layers-action", "Layer Composite Space: RGB (from color profile)"),
    GIMP_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    NULL },

  { "layers-composite-space-rgb-perceptual", NULL,
    NC_("layers-action", "RGB (perceptual)"), NULL, { NULL },
    NC_("layers-action", "Layer Composite Space: RGB (perceptual)"),
    GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
    NULL }
};

static const GimpRadioActionEntry layers_composite_mode_actions[] =
{
  { "layers-composite-mode-auto", NULL,
    NC_("layers-action", "Auto"), NULL, { NULL },
    NC_("layers-action", "Layer Composite Mode: Auto"),
    GIMP_LAYER_COMPOSITE_AUTO,
    NULL },

  { "layers-composite-mode-union", NULL,
    NC_("layers-action", "Union"), NULL, { NULL },
    NC_("layers-action", "Layer Composite Mode: Union"),
    GIMP_LAYER_COMPOSITE_UNION,
    NULL },

  { "layers-composite-mode-clip-to-backdrop", NULL,
    NC_("layers-action", "Clip to Backdrop"), NULL, { NULL },
    NC_("layers-action", "Layer Composite Mode: Clip to Backdrop"),
    GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    NULL },

  { "layers-composite-mode-clip-to-layer", NULL,
    NC_("layers-action", "Clip to Layer"), NULL, { NULL },
    NC_("layers-action", "Layer Composite Mode: Clip to Layer"),
    GIMP_LAYER_COMPOSITE_CLIP_TO_LAYER,
    NULL },

  { "layers-composite-mode-intersection", NULL,
    NC_("layers-action", "Intersection"), NULL, { NULL },
    NC_("layers-action", "Layer Composite Mode: Intersection"),
    GIMP_LAYER_COMPOSITE_INTERSECTION,
    NULL }
};

static const GimpEnumActionEntry layers_color_tag_actions[] =
{
  { "layers-color-tag-none", GIMP_ICON_EDIT_CLEAR,
    NC_("layers-action", "None"), NULL, { NULL },
    NC_("layers-action", "Layer Color Tag: Clear"),
    GIMP_COLOR_TAG_NONE, FALSE,
    GIMP_HELP_LAYER_COLOR_TAG },

  { "layers-color-tag-blue", NULL,
    NC_("layers-action", "Blue"), NULL, { NULL },
    NC_("layers-action", "Layer Color Tag: Set to Blue"),
    GIMP_COLOR_TAG_BLUE, FALSE,
    GIMP_HELP_LAYER_COLOR_TAG },

  { "layers-color-tag-green", NULL,
    NC_("layers-action", "Green"), NULL, { NULL },
    NC_("layers-action", "Layer Color Tag: Set to Green"),
    GIMP_COLOR_TAG_GREEN, FALSE,
    GIMP_HELP_LAYER_COLOR_TAG },

  { "layers-color-tag-yellow", NULL,
    NC_("layers-action", "Yellow"), NULL, { NULL },
    NC_("layers-action", "Layer Color Tag: Set to Yellow"),
    GIMP_COLOR_TAG_YELLOW, FALSE,
    GIMP_HELP_LAYER_COLOR_TAG },

  { "layers-color-tag-orange", NULL,
    NC_("layers-action", "Orange"), NULL, { NULL },
    NC_("layers-action", "Layer Color Tag: Set to Orange"),
    GIMP_COLOR_TAG_ORANGE, FALSE,
    GIMP_HELP_LAYER_COLOR_TAG },

  { "layers-color-tag-brown", NULL,
    NC_("layers-action", "Brown"), NULL, { NULL },
    NC_("layers-action", "Layer Color Tag: Set to Brown"),
    GIMP_COLOR_TAG_BROWN, FALSE,
    GIMP_HELP_LAYER_COLOR_TAG },

  { "layers-color-tag-red", NULL,
    NC_("layers-action", "Red"), NULL, { NULL },
    NC_("layers-action", "Layer Color Tag: Set to Red"),
    GIMP_COLOR_TAG_RED, FALSE,
    GIMP_HELP_LAYER_COLOR_TAG },

  { "layers-color-tag-violet", NULL,
    NC_("layers-action", "Violet"), NULL, { NULL },
    NC_("layers-action", "Layer Color Tag: Set to Violet"),
    GIMP_COLOR_TAG_VIOLET, FALSE,
    GIMP_HELP_LAYER_COLOR_TAG },

  { "layers-color-tag-gray", NULL,
    NC_("layers-action", "Gray"), NULL, { NULL },
    NC_("layers-action", "Layer Color Tag: Set to Gray"),
    GIMP_COLOR_TAG_GRAY, FALSE,
    GIMP_HELP_LAYER_COLOR_TAG }
};

static const GimpEnumActionEntry layers_mask_apply_actions[] =
{
  { "layers-mask-apply", NULL,
    NC_("layers-action", "Apply Layer _Masks"), NULL, { NULL },
    NC_("layers-action", "Apply the effect of the layer masks and remove them"),
    GIMP_MASK_APPLY, FALSE,
    GIMP_HELP_LAYER_MASK_APPLY },

  { "layers-mask-delete", GIMP_ICON_EDIT_DELETE,
    NC_("layers-action", "Delete Layer Mas_ks"), NULL, { NULL },
    NC_("layers-action", "Remove layer masks and their effect"),
    GIMP_MASK_DISCARD, FALSE,
    GIMP_HELP_LAYER_MASK_DELETE }
};

static const GimpEnumActionEntry layers_mask_to_selection_actions[] =
{
  { "layers-mask-selection-replace", GIMP_ICON_SELECTION_REPLACE,
    NC_("layers-action", "_Masks to Selection"), NULL, { NULL },
    NC_("layers-action", "Replace the selection with the layer masks"),
    GIMP_CHANNEL_OP_REPLACE, FALSE,
    GIMP_HELP_LAYER_MASK_SELECTION_REPLACE },

  { "layers-mask-selection-add", GIMP_ICON_SELECTION_ADD,
    NC_("layers-action", "_Add Masks to Selection"), NULL, { NULL },
    NC_("layers-action", "Add the layer masks to the current selection"),
    GIMP_CHANNEL_OP_ADD, FALSE,
    GIMP_HELP_LAYER_MASK_SELECTION_ADD },

  { "layers-mask-selection-subtract", GIMP_ICON_SELECTION_SUBTRACT,
    NC_("layers-action", "_Subtract Masks from Selection"), NULL, { NULL },
    NC_("layers-action", "Subtract the layer masks from the current selection"),
    GIMP_CHANNEL_OP_SUBTRACT, FALSE,
    GIMP_HELP_LAYER_MASK_SELECTION_SUBTRACT },

  { "layers-mask-selection-intersect", GIMP_ICON_SELECTION_INTERSECT,
    NC_("layers-action", "_Intersect Masks with Selection"), NULL, { NULL },
    NC_("layers-action", "Intersect the layer masks with the current selection"),
    GIMP_CHANNEL_OP_INTERSECT, FALSE,
    GIMP_HELP_LAYER_MASK_SELECTION_INTERSECT }
};

static const GimpEnumActionEntry layers_alpha_to_selection_actions[] =
{
  { "layers-alpha-selection-replace", GIMP_ICON_SELECTION_REPLACE,
    NC_("layers-action", "Al_pha to Selection"), NULL, { NULL },
    NC_("layers-action",
        "Replace the selection with the layer's alpha channel"),
    GIMP_CHANNEL_OP_REPLACE, FALSE,
    GIMP_HELP_LAYER_ALPHA_SELECTION_REPLACE },

  { "layers-alpha-selection-add", GIMP_ICON_SELECTION_ADD,
    NC_("layers-action", "A_dd Alpha to Selection"), NULL, { NULL },
    NC_("layers-action",
        "Add the layer's alpha channel to the current selection"),
    GIMP_CHANNEL_OP_ADD, FALSE,
    GIMP_HELP_LAYER_ALPHA_SELECTION_ADD },

  { "layers-alpha-selection-subtract", GIMP_ICON_SELECTION_SUBTRACT,
    NC_("layers-action", "_Subtract Alpha from Selection"), NULL, { NULL },
    NC_("layers-action",
        "Subtract the layer's alpha channel from the current selection"),
    GIMP_CHANNEL_OP_SUBTRACT, FALSE,
    GIMP_HELP_LAYER_ALPHA_SELECTION_SUBTRACT },

  { "layers-alpha-selection-intersect", GIMP_ICON_SELECTION_INTERSECT,
    NC_("layers-action", "_Intersect Alpha with Selection"), NULL, { NULL },
    NC_("layers-action",
        "Intersect the layer's alpha channel with the current selection"),
    GIMP_CHANNEL_OP_INTERSECT, FALSE,
    GIMP_HELP_LAYER_ALPHA_SELECTION_INTERSECT }
};

static const GimpEnumActionEntry layers_select_actions[] =
{
  { "layers-select-top", NULL,
    NC_("layers-action", "Select _Top Layer"), NULL, { "Home", NULL },
    NC_("layers-action", "Select the topmost layer"),
    GIMP_ACTION_SELECT_FIRST, FALSE,
    GIMP_HELP_LAYER_TOP },

  { "layers-select-bottom", NULL,
    NC_("layers-action", "Select _Bottom Layer"), NULL, { "End", NULL },
    NC_("layers-action", "Select the bottommost layer"),
    GIMP_ACTION_SELECT_LAST, FALSE,
    GIMP_HELP_LAYER_BOTTOM },

  { "layers-select-previous", NULL,
    NC_("layers-action", "Select _Previous Layers"), NULL, { "Prior", NULL },
    NC_("layers-action",
        "Select the layers above each currently selected layer. "
        "Layers will not be selected outside their current group level."),
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    GIMP_HELP_LAYER_PREVIOUS },

  { "layers-select-next", NULL,
    NC_("layers-action", "Select _Next Layers"), NULL, { "Next", NULL },
    NC_("layers-action",
        "Select the layers below each currently selected layer. "
        "Layers will not be selected outside their current group level."),
    GIMP_ACTION_SELECT_NEXT, FALSE,
    GIMP_HELP_LAYER_NEXT },

  { "layers-select-flattened-previous", NULL,
    NC_("layers-action", "Select Previous Layers (flattened view)"), NULL, { NULL },
    NC_("layers-action", "Select the layers above each currently selected layer"),
    GIMP_ACTION_SELECT_FLAT_PREVIOUS, FALSE,
    GIMP_HELP_LAYER_PREVIOUS },

  { "layers-select-flattened-next", NULL,
    NC_("layers-action", "Select Next Layers (flattened view)"), NULL, { NULL },
    NC_("layers-action", "Select the layers below each currently selected layer"),
    GIMP_ACTION_SELECT_FLAT_NEXT, FALSE,
    GIMP_HELP_LAYER_NEXT }
};

static const GimpEnumActionEntry layers_opacity_actions[] =
{
  { "layers-opacity-set", GIMP_ICON_TRANSPARENCY,
    NC_("layers-action", "Layer Opacity: Set"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    GIMP_HELP_LAYER_OPACITY },
  { "layers-opacity-transparent", GIMP_ICON_TRANSPARENCY,
    NC_("layers-action", "Layer Opacity: Make Completely Transparent"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    GIMP_HELP_LAYER_OPACITY },
  { "layers-opacity-opaque", GIMP_ICON_TRANSPARENCY,
    NC_("layers-action", "Layer Opacity: Make Completely Opaque"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    GIMP_HELP_LAYER_OPACITY },
  { "layers-opacity-decrease", GIMP_ICON_TRANSPARENCY,
    NC_("layers-action", "Layer Opacity: Make More Transparent"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    GIMP_HELP_LAYER_OPACITY },
  { "layers-opacity-increase", GIMP_ICON_TRANSPARENCY,
    NC_("layers-action", "Layer Opacity: Make More Opaque"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    GIMP_HELP_LAYER_OPACITY },
  { "layers-opacity-decrease-skip", GIMP_ICON_TRANSPARENCY,
    NC_("layers-action", "Layer Opacity: Make 10% More Transparent"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    GIMP_HELP_LAYER_OPACITY },
  { "layers-opacity-increase-skip", GIMP_ICON_TRANSPARENCY,
    NC_("layers-action", "Layer Opacity: Make 10% More Opaque"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    GIMP_HELP_LAYER_OPACITY }
};

static const GimpEnumActionEntry layers_mode_actions[] =
{
  { "layers-mode-first", GIMP_ICON_TOOL_PENCIL,
    NC_("layers-action", "Layer Mode: Select First"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    GIMP_HELP_LAYER_MODE },
  { "layers-mode-last", GIMP_ICON_TOOL_PENCIL,
    NC_("layers-action", "Layer Mode: Select Last"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    GIMP_HELP_LAYER_MODE },
  { "layers-mode-previous", GIMP_ICON_TOOL_PENCIL,
    NC_("layers-action", "Layer Mode: Select Previous"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    GIMP_HELP_LAYER_MODE },
  { "layers-mode-next", GIMP_ICON_TOOL_PENCIL,
    NC_("layers-action", "Layer Mode: Select Next"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    GIMP_HELP_LAYER_MODE }
};

/**
 * layers_actions_fix_tooltip:
 * @group:
 * @action:
 * @modifiers:
 *
 * Make layer alpha to selection click-shortcuts discoverable, at
 * least in theory.
 **/
static void
layers_actions_fix_tooltip (GimpActionGroup *group,
                            const gchar     *action,
                            GdkModifierType  modifiers)
{
  const gchar *old_hint;
  gchar       *new_hint;

  old_hint = gimp_action_group_get_action_tooltip (group,
                                                   action);
  new_hint = g_strconcat (old_hint,
                          "\n",
                          /* Will be followed with e.g. "Shift-Click
                             on thumbnail"
                           */
                          _("Shortcut: "),
                          gimp_get_mod_string (modifiers),
                          /* Will be prepended with a modifier key
                             string, e.g. "Shift"
                           */
                          _("-Click on thumbnail in Layers dockable"),
                          NULL);

  gimp_action_group_set_action_tooltip (group, action, new_hint);
  g_free (new_hint);
}

void
layers_actions_setup (GimpActionGroup *group)
{
  GdkDisplay      *display = gdk_display_get_default ();
  GdkModifierType  extend_mask;
  GdkModifierType  modify_mask;

  extend_mask =
    gdk_keymap_get_modifier_mask (gdk_keymap_get_for_display (display),
                                  GDK_MODIFIER_INTENT_EXTEND_SELECTION);
  modify_mask =
    gdk_keymap_get_modifier_mask (gdk_keymap_get_for_display (display),
                                  GDK_MODIFIER_INTENT_MODIFY_SELECTION);

  gimp_action_group_add_actions (group, "layers-action",
                                 layers_actions,
                                 G_N_ELEMENTS (layers_actions));

  gimp_action_group_add_toggle_actions (group, "layers-action",
                                        layers_toggle_actions,
                                        G_N_ELEMENTS (layers_toggle_actions));

  gimp_action_group_add_radio_actions (group, "layers-action",
                                       layers_blend_space_actions,
                                       G_N_ELEMENTS (layers_blend_space_actions),
                                       NULL, 0,
                                       layers_blend_space_cmd_callback);

  gimp_action_group_add_radio_actions (group, "layers-action",
                                       layers_composite_space_actions,
                                       G_N_ELEMENTS (layers_composite_space_actions),
                                       NULL, 0,
                                       layers_composite_space_cmd_callback);

  gimp_action_group_add_radio_actions (group, "layers-action",
                                       layers_composite_mode_actions,
                                       G_N_ELEMENTS (layers_composite_mode_actions),
                                       NULL, 0,
                                       layers_composite_mode_cmd_callback);

  gimp_action_group_add_enum_actions (group, "layers-action",
                                      layers_color_tag_actions,
                                      G_N_ELEMENTS (layers_color_tag_actions),
                                      layers_color_tag_cmd_callback);

  gimp_action_group_add_enum_actions (group, "layers-action",
                                      layers_mask_apply_actions,
                                      G_N_ELEMENTS (layers_mask_apply_actions),
                                      layers_mask_apply_cmd_callback);

  gimp_action_group_add_enum_actions (group, "layers-action",
                                      layers_mask_to_selection_actions,
                                      G_N_ELEMENTS (layers_mask_to_selection_actions),
                                      layers_mask_to_selection_cmd_callback);

  gimp_action_group_add_enum_actions (group, "layers-action",
                                      layers_alpha_to_selection_actions,
                                      G_N_ELEMENTS (layers_alpha_to_selection_actions),
                                      layers_alpha_to_selection_cmd_callback);

  layers_actions_fix_tooltip (group, "layers-alpha-selection-replace",
                              GDK_MOD1_MASK);
  layers_actions_fix_tooltip (group, "layers-alpha-selection-add",
                              extend_mask | GDK_MOD1_MASK);
  layers_actions_fix_tooltip (group, "layers-alpha-selection-subtract",
                              modify_mask | GDK_MOD1_MASK);
  layers_actions_fix_tooltip (group, "layers-alpha-selection-intersect",
                              extend_mask | modify_mask | GDK_MOD1_MASK);

  gimp_action_group_add_enum_actions (group, "layers-action",
                                      layers_select_actions,
                                      G_N_ELEMENTS (layers_select_actions),
                                      layers_select_cmd_callback);

  gimp_action_group_add_enum_actions (group, "layers-action",
                                      layers_opacity_actions,
                                      G_N_ELEMENTS (layers_opacity_actions),
                                      layers_opacity_cmd_callback);

  gimp_action_group_add_enum_actions (group, "layers-action",
                                      layers_mode_actions,
                                      G_N_ELEMENTS (layers_mode_actions),
                                      layers_mode_cmd_callback);

  items_actions_setup (group, "layers");
}

void
layers_actions_update (GimpActionGroup *group,
                       gpointer         data)
{
  GimpImage     *image              = action_data_get_image (data);
  GList         *layers             = NULL;
  GList         *iter               = NULL;
  GimpLayer     *layer              = NULL;
  gboolean       fs                 = FALSE;    /*  floating sel           */
  gboolean       ac                 = FALSE;    /*  Has selected channels  */
  gboolean       sel                = FALSE;
  gboolean       indexed            = FALSE;    /*  is indexed             */
  gboolean       lock_alpha         = TRUE;
  gboolean       can_lock_alpha     = FALSE;
  gboolean       has_rasterizable   = FALSE;
  gboolean       has_rasterized     = FALSE;
  gboolean       text_layer         = FALSE;
  gboolean       vector_layer       = FALSE;
  gboolean       bs_mutable         = FALSE; /* At least 1 selected layers' blend space is mutable.     */
  gboolean       cs_mutable         = FALSE; /* At least 1 selected layers' composite space is mutable. */
  gboolean       cm_mutable         = FALSE; /* At least 1 selected layers' composite mode is mutable.  */
  gboolean       next_mode          = TRUE;
  gboolean       prev_mode          = TRUE;
  gboolean       last_mode          = FALSE;
  gboolean       first_mode         = FALSE;

  gboolean       first_selected     = FALSE; /* First layer is selected  */
  gboolean       last_selected      = FALSE; /* Last layer is selected   */

  gboolean       have_masks         = FALSE; /* At least 1 selected layer has a mask.             */
  gboolean       have_no_masks      = FALSE; /* At least 1 selected layer has no mask.            */
  gboolean       have_groups        = FALSE; /* At least 1 selected layer is a group.             */
  gboolean       have_no_groups     = FALSE; /* At least 1 selected layer is not a group.         */
  gboolean       have_writable      = FALSE; /* At least 1 selected layer has no contents lock.   */
  gboolean       have_prev          = FALSE; /* At least 1 selected layer has a previous sibling. */
  gboolean       have_next          = FALSE; /* At least 1 selected layer has a next sibling.     */
  gboolean       have_alpha         = FALSE; /* At least 1 selected layer has an alpha channel.   */
  gboolean       have_no_alpha      = FALSE; /* At least 1 selected layer has no alpha channel.   */

  gboolean       all_visible        = TRUE;
  gboolean       all_next_visible   = TRUE;
  gboolean       any_mask_shown     = FALSE;
  gboolean       any_mask_disabled  = FALSE;
  gboolean       all_writable       = TRUE;
  gboolean       all_movable        = TRUE;

  gint           n_selected_layers  = 0;
  gint           n_layers           = 0;
  gint           n_text_layers      = 0;

  if (image)
    {
      fs      = (gimp_image_get_floating_selection (image) != NULL);
      ac      = (gimp_image_get_selected_channels (image) != NULL);
      sel     = ! gimp_channel_is_empty (gimp_image_get_mask (image));
      indexed = (gimp_image_get_base_type (image) == GIMP_INDEXED);

      layers            = gimp_image_get_selected_layers (image);
      n_selected_layers = g_list_length (layers);
      n_layers          = gimp_image_get_n_layers (image);

      for (iter = layers; iter; iter = iter->next)
        {
          GimpLayerMode *modes;
          GimpLayerMode  mode;
          GList         *layer_list;
          GList         *iter2;
          gint           n_modes;
          gint           i = 0;

          /* have_masks and have_no_masks are not opposite. 3 cases are
           * possible: all layers have masks, none have masks, or some
           * have masks, and some none.
           */
          if (gimp_layer_get_mask (iter->data))
            {
              have_masks = TRUE;

              if (gimp_layer_get_show_mask (iter->data))
                any_mask_shown = TRUE;
              if (! gimp_layer_get_apply_mask (iter->data))
                any_mask_disabled = TRUE;
            }
          else
            {
              have_no_masks = TRUE;
            }

          if (gimp_viewable_get_children (GIMP_VIEWABLE (iter->data)))
            have_groups = TRUE;
          else
            have_no_groups = TRUE;

          if (! gimp_item_is_content_locked (GIMP_ITEM (iter->data), NULL))
            have_writable = TRUE;
          else
            all_writable  = FALSE;

          if (gimp_item_is_position_locked (GIMP_ITEM (iter->data), NULL))
            all_movable = FALSE;

          if (gimp_layer_can_lock_alpha (iter->data))
            {
              if (! gimp_layer_get_lock_alpha (iter->data))
                lock_alpha = FALSE;
              can_lock_alpha = TRUE;
            }

          mode = gimp_layer_get_mode (iter->data);
          modes = gimp_layer_mode_get_context_array (mode,
                                                     GIMP_LAYER_MODE_CONTEXT_LAYER,
                                                     &n_modes);
          while (i < (n_modes - 1) && modes[i] != mode)
            i++;
          g_free (modes);
          if (i >= n_modes - 1)
            next_mode = FALSE;
          else
            last_mode = TRUE;
          if (i <= 0)
            prev_mode = FALSE;
          else
            first_mode = TRUE;

          layer_list = gimp_item_get_container_iter (GIMP_ITEM (iter->data));
          iter2 = g_list_find (layer_list, iter->data);

          if (iter2)
            {
              GList *next_visible;

              if (gimp_item_get_index (iter2->data) == 0)
                first_selected = TRUE;
              if (gimp_item_get_index (iter2->data) == n_layers - 1)
                last_selected = TRUE;

              if (g_list_previous (iter2))
                have_prev = TRUE;

              if (g_list_next (iter2))
                have_next = TRUE;

              for (next_visible = g_list_next (iter2);
                   next_visible;
                   next_visible = g_list_next (next_visible))
                {
                  if (gimp_item_get_visible (next_visible->data))
                    {
                      /*  "next_visible" is actually "next_visible" and
                       *  "writable" and "not group"
                       */
                      if (gimp_item_is_content_locked (next_visible->data, NULL) ||
                          gimp_viewable_get_children (next_visible->data))
                        next_visible = NULL;

                      break;
                    }
                }

              if (! next_visible)
                all_next_visible = FALSE;
            }

          if (gimp_layer_mode_is_blend_space_mutable (mode))
            bs_mutable = TRUE;
          if (gimp_layer_mode_is_composite_space_mutable (mode))
            cs_mutable = TRUE;
          if (gimp_layer_mode_is_composite_mode_mutable (mode))
            cm_mutable = TRUE;

          if (! gimp_item_get_visible (iter->data))
            all_visible = FALSE;

          if (gimp_drawable_has_alpha (iter->data))
            have_alpha    = TRUE;
          else
            have_no_alpha = TRUE;

          if (GIMP_IS_TEXT_LAYER (iter->data))
            n_text_layers++;

          has_rasterizable = has_rasterizable || gimp_item_is_rasterizable (iter->data);
          has_rasterized = has_rasterized || gimp_item_is_rasterized (iter->data);
        }

      if (n_selected_layers == 1)
        {
          /* Special unique layer case. */
          const gchar *action = NULL;

          layer  = layers->data;
          switch (gimp_layer_get_blend_space (layer))
            {
            case GIMP_LAYER_COLOR_SPACE_AUTO:
              action = "layers-blend-space-auto"; break;
            case GIMP_LAYER_COLOR_SPACE_RGB_LINEAR:
              action = "layers-blend-space-rgb-linear"; break;
            case GIMP_LAYER_COLOR_SPACE_RGB_NON_LINEAR:
              action = "layers-blend-space-rgb-non-linear"; break;
            case GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL:
              action = "layers-blend-space-rgb-perceptual"; break;
            default:
              action = NULL; break; /* can't happen */
            }

          if (action)
            gimp_action_group_set_action_active (group, action, TRUE);

          switch (gimp_layer_get_composite_space (layer))
            {
            case GIMP_LAYER_COLOR_SPACE_AUTO:
              action = "layers-composite-space-auto"; break;
            case GIMP_LAYER_COLOR_SPACE_RGB_LINEAR:
              action = "layers-composite-space-rgb-linear"; break;
            case GIMP_LAYER_COLOR_SPACE_RGB_NON_LINEAR:
              action = "layers-composite-space-rgb-non-linear"; break;
            case GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL:
              action = "layers-composite-space-rgb-perceptual"; break;
            default:
              action = NULL; break; /* can't happen */
            }

          if (action)
            gimp_action_group_set_action_active (group, action, TRUE);

          switch (gimp_layer_get_composite_mode (layer))
            {
            case GIMP_LAYER_COMPOSITE_AUTO:
              action = "layers-composite-mode-auto"; break;
            case GIMP_LAYER_COMPOSITE_UNION:
              action = "layers-composite-mode-union"; break;
            case GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP:
              action = "layers-composite-mode-clip-to-backdrop"; break;
            case GIMP_LAYER_COMPOSITE_CLIP_TO_LAYER:
              action = "layers-composite-mode-clip-to-layer"; break;
            case GIMP_LAYER_COMPOSITE_INTERSECTION:
              action = "layers-composite-mode-intersection"; break;
            }

          gimp_action_group_set_action_active (group, action, TRUE);

          text_layer   = gimp_item_is_text_layer (GIMP_ITEM (layer));
          vector_layer = gimp_item_is_vector_layer (GIMP_ITEM (layer));
        }
    }

#define SET_VISIBLE(action,condition) \
        gimp_action_group_set_action_visible (group, action, (condition) != 0)
#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)
#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)
#define SET_LABEL(action,label) \
        gimp_action_group_set_action_label (group, action, label)

  SET_SENSITIVE ("layers-edit",             !ac && ((layer && !fs) || text_layer));
  SET_VISIBLE   ("layers-edit-text",        text_layer && !ac);
  SET_SENSITIVE ("layers-edit-text",        text_layer && !ac);
  SET_VISIBLE   ("layers-edit-vector",      vector_layer && !ac);
  SET_SENSITIVE ("layers-edit-attributes",  layer && !fs && !ac);

  if (layer && gimp_layer_is_floating_sel (layer))
    {
      SET_LABEL ("layers-new",             C_("layers-action", "To _New Layer"));
      SET_LABEL ("layers-new-last-values", C_("layers-action", "To _New Layer"));
    }
  else
    {
      SET_LABEL ("layers-new",             C_("layers-action", "_New Layer..."));
      SET_LABEL ("layers-new-last-values", C_("layers-action", "_New Layer"));
    }

  SET_SENSITIVE ("layers-new",              image);
  SET_SENSITIVE ("layers-new-last-values",  image);
  SET_SENSITIVE ("layers-new-from-visible", image);
  SET_SENSITIVE ("layers-new-group",        image && !indexed && !fs);
  SET_SENSITIVE ("layers-duplicate",        n_selected_layers > 0 && !fs && !ac);
  SET_SENSITIVE ("layers-delete",           n_selected_layers > 0 && !ac);

  SET_SENSITIVE ("layers-mode-first",       n_selected_layers > 0 && !ac && first_mode);
  SET_SENSITIVE ("layers-mode-last",        n_selected_layers > 0 && !ac && last_mode);
  SET_SENSITIVE ("layers-mode-previous",    n_selected_layers > 0 && !ac && prev_mode);
  SET_SENSITIVE ("layers-mode-next",        n_selected_layers > 0 && !ac && next_mode);

  SET_SENSITIVE ("layers-select-top",       n_layers > 0 && !fs && (n_selected_layers == 0 || have_prev));
  SET_SENSITIVE ("layers-select-bottom",    n_layers > 0 && !fs && (n_selected_layers == 0 || have_next));
  SET_SENSITIVE ("layers-select-previous",  n_selected_layers > 0 && !fs && !ac && have_prev);
  SET_SENSITIVE ("layers-select-next",      n_selected_layers > 0 && !fs && !ac && have_next);

  SET_SENSITIVE ("layers-raise",            n_selected_layers > 0 && !fs && !ac && have_prev && !first_selected);
  SET_SENSITIVE ("layers-raise-to-top",     n_selected_layers > 0 && !fs && !ac && have_prev);
  SET_SENSITIVE ("layers-lower",            n_selected_layers > 0 && !fs && !ac && have_next && !last_selected);
  SET_SENSITIVE ("layers-lower-to-bottom",  n_selected_layers > 0 && !fs && !ac && have_next);

  SET_VISIBLE   ("layers-anchor",            fs && !ac);
  SET_VISIBLE   ("layers-merge-down",        !fs);
  SET_SENSITIVE ("layers-merge-down",        n_selected_layers > 0 && !fs && !ac && all_visible && all_next_visible);
  SET_VISIBLE   ("layers-merge-down-button", !fs);
  SET_SENSITIVE ("layers-merge-down-button", n_selected_layers > 0 && !fs && !ac);
  SET_VISIBLE   ("layers-merge-group",       have_groups);
  SET_SENSITIVE ("layers-merge-group",       n_selected_layers && !fs && !ac && have_groups);
  SET_SENSITIVE ("layers-merge-layers",      n_selected_layers > 0 && !fs && !ac);
  SET_SENSITIVE ("layers-flatten-image",     !fs && !ac);

  SET_VISIBLE   ("layers-rasterize",         has_rasterizable);
  SET_VISIBLE   ("layers-revert-rasterize",  has_rasterized);

  SET_VISIBLE   ("layers-text-to-path",      n_text_layers > 0 && !ac);
  SET_VISIBLE   ("layers-text-along-path",   text_layer && !ac);

  SET_SENSITIVE ("layers-resize",          n_selected_layers == 1 && all_writable && all_movable && !ac);
  SET_SENSITIVE ("layers-resize-to-image", all_writable && all_movable && !ac);
  SET_SENSITIVE ("layers-scale",           n_selected_layers == 1 && all_writable && all_movable && !ac);

  SET_SENSITIVE ("layers-crop-to-selection", all_writable && all_movable && sel);
  SET_SENSITIVE ("layers-crop-to-content",   all_writable && all_movable);

  SET_SENSITIVE ("layers-alpha-add",    all_writable && have_no_groups && !fs && have_no_alpha);
  SET_SENSITIVE ("layers-alpha-remove", all_writable && have_no_groups && !fs && have_alpha);

  SET_SENSITIVE ("layers-lock-alpha", can_lock_alpha);
  SET_ACTIVE    ("layers-lock-alpha", lock_alpha);

  SET_SENSITIVE ("layers-blend-space-auto",           n_selected_layers && bs_mutable);
  SET_SENSITIVE ("layers-blend-space-rgb-linear",     n_selected_layers && bs_mutable);
  SET_SENSITIVE ("layers-blend-space-rgb-non-linear", n_selected_layers && bs_mutable);
  SET_SENSITIVE ("layers-blend-space-rgb-perceptual", n_selected_layers && bs_mutable);

  SET_SENSITIVE ("layers-composite-space-auto",           n_selected_layers && cs_mutable);
  SET_SENSITIVE ("layers-composite-space-rgb-linear",     n_selected_layers && cs_mutable);
  SET_SENSITIVE ("layers-composite-space-rgb-non-linear", n_selected_layers && cs_mutable);
  SET_SENSITIVE ("layers-composite-space-rgb-perceptual", n_selected_layers && cs_mutable);

  SET_SENSITIVE ("layers-composite-mode-auto",             n_selected_layers && cm_mutable);
  SET_SENSITIVE ("layers-composite-mode-union",            n_selected_layers && cm_mutable);
  SET_SENSITIVE ("layers-composite-mode-clip-to-backdrop", n_selected_layers && cm_mutable);
  SET_SENSITIVE ("layers-composite-mode-clip-to-layer",    n_selected_layers && cm_mutable);
  SET_SENSITIVE ("layers-composite-mode-intersection",     n_selected_layers && cm_mutable);

  SET_SENSITIVE ("layers-mask-add",             n_selected_layers > 0 && !fs && !ac && have_no_masks);
  SET_SENSITIVE ("layers-mask-add-button",      n_selected_layers > 0 && !fs && !ac);
  SET_SENSITIVE ("layers-mask-add-last-values", n_selected_layers > 0 && !fs && !ac && have_no_masks);

  SET_SENSITIVE ("layers-mask-apply",  have_writable && !fs && !ac && have_masks && have_no_groups);
  SET_SENSITIVE ("layers-mask-delete", n_selected_layers > 0 && !fs && !ac && have_masks);

  SET_SENSITIVE ("layers-mask-edit",    n_selected_layers == 1 && !fs && !ac && have_masks);
  SET_SENSITIVE ("layers-mask-show",    n_selected_layers > 0 && !fs && !ac && have_masks);
  SET_SENSITIVE ("layers-mask-disable", n_selected_layers > 0 && !fs && !ac && have_masks);

  SET_ACTIVE ("layers-mask-edit",    n_selected_layers == 1 && have_masks && gimp_layer_get_edit_mask (layers->data));
  SET_ACTIVE ("layers-mask-show",    any_mask_shown);
  SET_ACTIVE ("layers-mask-disable", any_mask_disabled);

  SET_SENSITIVE ("layers-mask-selection-replace",   n_selected_layers && !fs && !ac && have_masks);
  SET_SENSITIVE ("layers-mask-selection-add",       n_selected_layers && !fs && !ac && have_masks);
  SET_SENSITIVE ("layers-mask-selection-subtract",  n_selected_layers && !fs && !ac && have_masks);
  SET_SENSITIVE ("layers-mask-selection-intersect", n_selected_layers && !fs && !ac && have_masks);

  SET_SENSITIVE ("layers-alpha-selection-replace",   n_selected_layers > 0 && !fs && !ac);
  SET_SENSITIVE ("layers-alpha-selection-add",       n_selected_layers > 0 && !fs && !ac);
  SET_SENSITIVE ("layers-alpha-selection-subtract",  n_selected_layers > 0 && !fs && !ac);
  SET_SENSITIVE ("layers-alpha-selection-intersect", n_selected_layers > 0 && !fs && !ac);

#undef SET_VISIBLE
#undef SET_SENSITIVE
#undef SET_ACTIVE
#undef SET_LABEL

  items_actions_update (group, "layers", layers);
}
