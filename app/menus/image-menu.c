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

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimplayer.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "plug-in/plug-ins.h"

#include "widgets/gimpitemfactory.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-selection.h"

#include "dialogs-commands.h"
#include "drawable-commands.h"
#include "edit-commands.h"
#include "file-commands.h"
#include "image-commands.h"
#include "image-menu.h"
#include "layers-commands.h"
#include "menus.h"
#include "plug-in-commands.h"
#include "plug-in-menus.h"
#include "select-commands.h"
#include "tools-commands.h"
#include "view-commands.h"

#include "undo.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void   image_menu_foreground_changed (GimpContext     *context,
                                             const GimpRGB   *color,
                                             GimpItemFactory *item_factory);
static void   image_menu_background_changed (GimpContext     *context,
                                             const GimpRGB   *color,
                                             GimpItemFactory *item_factory);


GimpItemFactoryEntry image_menu_entries[] =
{
  { { "/tearoff1", NULL, gimp_item_factory_tearoff_callback, 0, "<Tearoff>" },
    NULL, NULL, NULL },

  /*  <Image>/File  */

  { { N_("/File/New..."), "<control>N",
      file_new_cmd_callback, 1,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    "file/dialogs/file_new.html", NULL },
  { { N_("/File/Open..."), "<control>O",
      file_open_cmd_callback, 1,
      "<StockItem>", GTK_STOCK_OPEN },
    NULL,
    "file/dialogs/file_open.html", NULL },

  /*  <Image>/File/Open Recent  */

  { { N_("/File/Open Recent/(None)"), NULL, NULL, 0 },
    NULL, NULL, NULL },

  MENU_SEPARATOR ("/File/---"),

  { { N_("/File/Save"), "<control>S",
      file_save_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SAVE },
    NULL,
    "file/dialogs/file_save.html", NULL },
  { { N_("/File/Save as..."), "<control><shift>S",
      file_save_as_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SAVE_AS },
    NULL,
    "file/dialogs/file_save.html", NULL },
  { { N_("/File/Save a Copy..."), NULL,
      file_save_a_copy_cmd_callback, 0 },
    NULL,
    "file/dialogs/file_save.html", NULL },
  { { N_("/File/Revert..."), NULL,
      file_revert_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_REVERT_TO_SAVED },
    NULL,
    "file/revert.html", NULL },

  MENU_SEPARATOR ("/File/---"),

  { { N_( "/File/Close"), "<control>W",
      file_close_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_CLOSE },
    NULL,
    "file/close.html", NULL },
  { { N_("/File/Quit"), "<control>Q",
      file_quit_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_QUIT },
    NULL,
    "file/quit.html", NULL },

  MENU_SEPARATOR ("/File/---moved"),

  /*  <Image>/Edit  */

  { { N_("/Edit/Undo"), "<control>Z",
      edit_undo_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_UNDO },
    NULL,
    "edit/undo.html", NULL },
  { { N_("/Edit/Redo"), "<control>R",
      edit_redo_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_REDO },
    NULL,
    "edit/redo.html", NULL },

  MENU_SEPARATOR ("/Edit/---"),

  { { N_("/Edit/Cut"), "<control>X",
      edit_cut_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_CUT },
    NULL,
    "edit/cut.html", NULL },
  { { N_("/Edit/Copy"), "<control>C",
      edit_copy_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_COPY },
    NULL,
    "edit/copy.html", NULL },
  { { N_("/Edit/Paste"), "<control>V",
      edit_paste_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_PASTE },
    NULL,
    "edit/paste.html", NULL },
  { { N_("/Edit/Paste Into"), NULL,
      edit_paste_into_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_PASTE_INTO },
    NULL,
    "edit/paste_into.html", NULL },
  { { N_("/Edit/Paste as New"), NULL,
      edit_paste_as_new_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_PASTE_AS_NEW },
    NULL,
    "edit/paste_as_new.html", NULL },

  /*  <Image>/Edit/Buffer  */

  { { N_("/Edit/Buffer/Cut Named..."), "<control><shift>X",
      edit_named_cut_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_CUT },
    NULL,
    "edit/dialogs/cut_named.html", NULL },
  { { N_("/Edit/Buffer/Copy Named..."), "<control><shift>C",
      edit_named_copy_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_COPY },
    NULL,
    "edit/dialogs/copy_named.html", NULL },
  { { N_("/Edit/Buffer/Paste Named..."), "<control><shift>V",
      edit_named_paste_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_PASTE },
    NULL,
    "edit/dialogs/paste_named.html", NULL },

  MENU_SEPARATOR ("/Edit/---"),

  { { N_("/Edit/Clear"), "<control>K",
      edit_clear_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_CLEAR },
    NULL,
    "edit/clear.html", NULL },
  { { N_("/Edit/Fill with FG Color"), "<control>comma",
      edit_fill_cmd_callback, (guint) GIMP_FOREGROUND_FILL,
      "<StockItem>", GIMP_STOCK_TOOL_BUCKET_FILL },
    NULL,
    "edit/fill.html", NULL },
  { { N_("/Edit/Fill with BG Color"), "<control>period",
      edit_fill_cmd_callback, (guint) GIMP_BACKGROUND_FILL,
      "<StockItem>", GIMP_STOCK_TOOL_BUCKET_FILL },
    NULL,
    "edit/fill.html", NULL },
  { { N_("/Edit/Stroke"), NULL,
      edit_stroke_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_STROKE },
    NULL,
    "edit/stroke.html", NULL },

  MENU_SEPARATOR ("/Edit/---"),

  /*  <Image>/Select  */
  
  { { N_("/Select/Invert"), "<control>I",
      select_invert_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_INVERT },
    NULL,
    "select/invert.html", NULL },
  { { N_("/Select/All"), "<control>A",
      select_all_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_ALL },
    NULL,
    "select/all.html", NULL },
  { { N_("/Select/None"), "<control><shift>A",
      select_none_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_NONE },
    NULL,
    "select/none.html", NULL },
  { { N_("/Select/Float"), "<control><shift>L",
      select_float_cmd_callback, 0 },
    NULL,
    "select/float.html", NULL },

  MENU_SEPARATOR ("/Select/---"),

  { { N_("/Select/Feather..."), "<control><shift>F",
      select_feather_cmd_callback, 0 },
    NULL,
    "select/dialogs/feather_selection.html", NULL },
  { { N_("/Select/Sharpen"), "<control><shift>H",
      select_sharpen_cmd_callback, 0 },
    NULL,
    "select/sharpen.html", NULL },
  { { N_("/Select/Shrink..."), NULL,
      select_shrink_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_SHRINK },
    NULL,
    "select/dialogs/shrink_selection.html", NULL },
  { { N_("/Select/Grow..."), NULL,
      select_grow_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_GROW },
    NULL,
    "select/dialogs/grow_selection.html", NULL },
  { { N_("/Select/Border..."), NULL,
      select_border_cmd_callback, 0 },
    NULL,
    "select/dialogs/border_selection.html", NULL },

  MENU_SEPARATOR ("/Select/---"),

  { { N_("/Select/Toggle QuickMask"), "<shift>Q",
      select_toggle_quickmask_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_QMASK_ON },
    NULL,
    "select/quickmask.html", NULL },
  { { N_("/Select/Save to Channel"), NULL,
      select_save_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_TO_CHANNEL },
    NULL,
    "select/save_to_channel.html", NULL },

  /*  <Image>/View  */

  { { N_("/View/Zoom In"), "equal",
      view_zoom_in_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_ZOOM_IN },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom Out"), "minus",
      view_zoom_out_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_ZOOM_OUT },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom to Fit Window"), "<control><shift>E",
      view_zoom_fit_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_ZOOM_FIT },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Shrink Wrap"), "<control>E",
      view_shrink_wrap_cmd_callback, 0 },
    NULL,
    "view/shrink_wrap.html", NULL },

  /*  <Image>/View/Zoom  */

  { { N_("/View/Zoom/16:1"), NULL,
      view_zoom_cmd_callback, 1601,
      "<StockItem>", GTK_STOCK_ZOOM_IN },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/8:1"), NULL,
      view_zoom_cmd_callback, 801,
      "<StockItem>", GTK_STOCK_ZOOM_IN },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/4:1"), NULL,
      view_zoom_cmd_callback, 401,
      "<StockItem>", GTK_STOCK_ZOOM_IN },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/2:1"), NULL,
      view_zoom_cmd_callback, 201,
      "<StockItem>", GTK_STOCK_ZOOM_IN },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/1:1"), "1",
      view_zoom_cmd_callback, 101,
      "<StockItem>", GTK_STOCK_ZOOM_100 },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/1:2"), NULL,
      view_zoom_cmd_callback, 102,
      "<StockItem>", GTK_STOCK_ZOOM_OUT },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/1:4"), NULL,
      view_zoom_cmd_callback, 104,
      "<StockItem>", GTK_STOCK_ZOOM_OUT },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/1:8"), NULL,
      view_zoom_cmd_callback, 108,
      "<StockItem>", GTK_STOCK_ZOOM_OUT },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/1:16"), NULL,
      view_zoom_cmd_callback, 116,
      "<StockItem>", GTK_STOCK_ZOOM_OUT },
    NULL,
    "view/zoom.html", NULL },

  { { N_("/View/Dot for Dot"), NULL,
      view_dot_for_dot_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/dot_for_dot.html", NULL },

  MENU_SEPARATOR ("/View/---"),

  { { N_("/View/Info Window..."), "<control><shift>I",
      view_info_window_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_INFO },
    NULL,
    "view/dialogs/info_window.html", NULL },
  { { N_("/View/Navigation Window..."), "<control><shift>N",
      view_navigation_window_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_NAVIGATION },
    NULL,
    "view/dialogs/navigation_window.html", NULL },
  { { N_("/View/Display Filters..."), NULL,
      view_display_filters_cmd_callback, 0 },
    NULL,
    "dialogs/display_filters/display_filters.html", NULL },

  MENU_SEPARATOR ("/View/---"),

  { { N_("/View/Show Selection"), "<control>T",
      view_toggle_selection_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/toggle_selection.html", NULL },
  { { N_("/View/Show Layer Boundary"), NULL,
      view_toggle_layer_boundary_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/toggle_selection.html", NULL },
  { { N_("/View/Show Guides"), "<control><shift>T",
      view_toggle_guides_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/toggle_guides.html", NULL },
  { { N_("/View/Snap to Guides"), NULL,
      view_snap_to_guides_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/snap_to_guides.html", NULL },

  MENU_SEPARATOR ("/View/---"),

  { { N_("/View/Show Menubar"), NULL,
      view_toggle_menubar_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/toggle_menubar.html", NULL },
  { { N_("/View/Show Rulers"), "<control><shift>R",
      view_toggle_rulers_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/toggle_rulers.html", NULL },
  { { N_("/View/Show Statusbar"), NULL,
      view_toggle_statusbar_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/toggle_statusbar.html", NULL },

  MENU_SEPARATOR ("/View/---"),

  { { N_("/View/New View"), "",
      view_new_view_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    "view/new_view.html", NULL },

  /*  <Image>/Image/Mode  */

  { { N_("/Image/Mode/RGB"), "<alt>R",
      image_convert_rgb_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_CONVERT_RGB },
    NULL,
    "image/mode/convert_to_rgb.html", NULL },
  { { N_("/Image/Mode/Grayscale"), "<alt>G",
      image_convert_grayscale_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_CONVERT_GRAYSCALE },
    NULL,
    "image/mode/convert_to_grayscale.html", NULL },
  { { N_("/Image/Mode/Indexed..."), "<alt>I",
      image_convert_indexed_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_CONVERT_INDEXED },
    NULL,
    "image/mode/dialogs/convert_to_indexed.html", NULL },

  MENU_SEPARATOR ("/Image/Mode/---"),

  /*  <Image>/Image/Transform  */

  MENU_BRANCH (N_("/Image/Transform")),

  MENU_SEPARATOR ("/Image/Transform/---"),

  MENU_SEPARATOR ("/Image/---"),

  { { N_("/Image/Canvas Size..."), NULL,
      image_resize_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_RESIZE },
    NULL,
    "image/dialogs/set_canvas_size.html", NULL },
  { { N_("/Image/Scale Image..."), NULL,
      image_scale_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SCALE },
    NULL,
    "image/dialogs/scale_image.html", NULL },
  { { N_("/Image/Crop Image"), NULL,
      image_crop_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_CROP },
    NULL,
    "image/dialogs/scale_image.html", NULL },
  { { N_("/Image/Duplicate"), "<control>D",
      image_duplicate_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    "image/duplicate.html", NULL },

  MENU_SEPARATOR ("/Image/---"),

  { { N_("/Image/Merge Visible Layers..."), "<control>M",
      image_merge_layers_cmd_callback, 0 },
    NULL,
    "layers/dialogs/merge_visible_layers.html", NULL },
  { { N_("/Image/Flatten Image"), NULL,
      image_flatten_image_cmd_callback, 0 },
    NULL,
    "layers/flatten_image.html", NULL },

  MENU_SEPARATOR ("/Image/---"),

  { { N_("/Image/Undo History..."), NULL,
      dialogs_create_toplevel_cmd_callback, 0 },
    "gimp-undo-history-dialog",
    "dialogs/undo_history.html", NULL },

 /*  <Image>/Layer  */

  /*  <Image>/Layer/Stack  */

  { { N_("/Layer/Stack/Previous Layer"), "Prior",
      layers_previous_cmd_callback, 0 },
    NULL,
    "layers/stack/stack.html#previous_layer", NULL },
  { { N_("/Layer/Stack/Next Layer"), "Next",
      layers_next_cmd_callback, 0 },
    NULL,
    "layers/stack/stack.html#next_layer", NULL },
  { { N_("/Layer/Stack/Raise Layer"), "<shift>Prior",
      layers_raise_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_UP },
    NULL,
    "layers/stack/stack.html#raise_layer", NULL },
  { { N_("/Layer/Stack/Lower Layer"), "<shift>Next",
      layers_lower_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_DOWN },
    NULL,
    "layers/stack/stack.html#lower_layer", NULL },
  { { N_("/Layer/Stack/Layer to Top"), "<control>Prior",
      layers_raise_to_top_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GOTO_TOP },
    NULL,
    "layers/stack/stack.html#layer_to_top", NULL },
  { { N_("/Layer/Stack/Layer to Bottom"), "<control>Next",
      layers_lower_to_bottom_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GOTO_BOTTOM },
    NULL,
    "layers/stack/stack.html#layer_to_bottom", NULL },

  MENU_SEPARATOR ("/Layer/Stack/---"),

  { { N_("/Layer/New Layer..."), "",
      layers_new_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    "layers/dialogs/new_layer.html", NULL },
  { { N_("/Layer/Duplicate Layer"), NULL,
      layers_duplicate_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    "layers/duplicate_layer.html", NULL },
  { { N_("/Layer/Anchor Layer"), "<control>H",
      layers_anchor_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_ANCHOR },
    NULL,
    "layers/anchor_layer.html", NULL },
  { { N_("/Layer/Merge Down"), "<control><shift>M",
      layers_merge_down_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_MERGE_DOWN },
    NULL,
    "layers/merge_visible_layers.html", NULL },
  { { N_("/Layer/Delete Layer"), NULL,
      layers_delete_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    "layers/delete_layer.html", NULL },

  MENU_SEPARATOR ("/Layer/---"),

  { { N_("/Layer/Layer Boundary Size..."), NULL,
      layers_resize_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_RESIZE },
    NULL,
    "layers/dialogs/layer_boundary_size.html", NULL },
  { { N_("/Layer/Layer to Imagesize"), NULL,
      layers_resize_to_image_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_LAYER_TO_IMAGESIZE },
    NULL,
    "layers/layer_to_image_size.html", NULL },
  { { N_("/Layer/Scale Layer..."), NULL,
      layers_scale_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SCALE },
    NULL,
    "layers/dialogs/scale_layer.html", NULL },
  { { N_("/Layer/Crop Layer"), NULL,
      layers_crop_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_CROP },
    NULL,
    "layers/dialogs/scale_layer.html", NULL },

  /*  <Image>/Layer/Transform  */

  MENU_SEPARATOR ("/Layer/Transform/---"),

  { { N_("/Layer/Transform/Offset..."), "<control><shift>O",
      drawable_offset_cmd_callback, 0 },
    NULL,
    "layers/dialogs/offset.html", NULL },

  MENU_SEPARATOR ("/Layer/---"),

  /*  <Image>/Layer/Colors  */

  MENU_SEPARATOR ("/Layer/Colors/---"),

  { { N_("/Layer/Colors/Desaturate"), NULL,
      drawable_desaturate_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_CONVERT_GRAYSCALE },
    NULL,
    "layers/colors/desaturate.html", NULL },
  { { N_("/Layer/Colors/Invert"), NULL,
      drawable_invert_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_INVERT },
    NULL,
    "layers/colors/invert.html", NULL },

  MENU_SEPARATOR ("/Layer/Colors/---"),

  /*  <Image>/Layer/Colors/Auto  */

  { { N_("/Layer/Colors/Auto/Equalize"), NULL,
      drawable_equalize_cmd_callback, 0 },
    NULL,
    "layers/colors/auto/equalize.html", NULL },

  MENU_SEPARATOR ("/Layer/Colors/---"),

  /*  <Image>/Layer/Mask  */

  { { N_("/Layer/Mask/Add Layer Mask..."), NULL,
      layers_add_layer_mask_cmd_callback, 0 },
    NULL,
    "layers/dialogs/add_layer_mask.html", NULL },
  { { N_("/Layer/Mask/Apply Layer Mask"), NULL,
      layers_apply_layer_mask_cmd_callback, 0 },
    NULL,
    "layers/apply_mask.html", NULL },
  { { N_("/Layer/Mask/Delete Layer Mask"), NULL,
      layers_delete_layer_mask_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    "layers/delete_mask.html", NULL },
  { { N_("/Layer/Mask/Mask to Selection"), NULL,
      layers_mask_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_REPLACE },
    NULL,
    "layers/mask_to_selection.html", NULL },

  /*  <Image>/Layer/Transparency  */

  { { N_("/Layer/Transparency/Add Alpha Channel"), NULL,
      layers_add_alpha_channel_cmd_callback, 0 },
    NULL,
    "layers/add_alpha_channel.html", NULL },
  { { N_("/Layer/Transparency/Alpha to Selection"), NULL,
      layers_alpha_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_REPLACE },
    NULL,
    "layers/alpha_to_selection.html", NULL },

  MENU_SEPARATOR ("/Layer/Transparency/---"),

  MENU_SEPARATOR ("/Layer/---"),

  /*  <Image>/Tools  */

  { { N_("/Tools/Toolbox"), NULL,
      dialogs_show_toolbox_cmd_callback, 0 },
    NULL,
    "toolbox/toolbox.html", NULL },
  { { N_("/Tools/Default Colors"), "D",
      tools_default_colors_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DEFAULT_COLORS },
    NULL,
    "toolbox/toolbox.html#default_colors", NULL },
  { { N_("/Tools/Swap Colors"), "X",
      tools_swap_colors_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SWAP_COLORS },
    NULL,
    "toolbox/toolbox.html#swap_colors", NULL },
  { { N_("/Tools/Swap Contexts"), "<alt>X",
      tools_swap_contexts_cmd_callback, 0 },
    NULL,
    "toolbox/toolbox.html#swap_colors", NULL },

  MENU_SEPARATOR ("/Tools/---"),

  MENU_BRANCH (N_("/Tools/Selection Tools")),
  MENU_BRANCH (N_("/Tools/Paint Tools")),
  MENU_BRANCH (N_("/Tools/Transform Tools")),

  /*  <Image>/Dialogs  */

  { { N_("/Dialogs/Layers, Channels & Paths..."), NULL,
      dialogs_create_lc_cmd_callback, 0 },
    NULL,
    "dialogs/layers_and_channels.html", NULL },
  { { N_("/Dialogs/Brushes, Patterns & Stuff..."), NULL,
      dialogs_create_stuff_cmd_callback, 0 },
    NULL,
    NULL, NULL },
  { { N_("/Dialogs/Tool Options..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_OPTIONS },
    "gimp-tool-options",
    "dialogs/tool_options.html", NULL },
  { { N_("/Dialogs/Device Status..."), NULL,
      dialogs_create_toplevel_cmd_callback, 0 },
    "gimp-device-status-dialog",
    "dialogs/device_status.html", NULL },

  MENU_SEPARATOR ("/Dialogs/---"),

  { { N_("/Dialogs/Layers..."), "<control>L",
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-layer-list",
    NULL, NULL },
  { { N_("/Dialogs/Channels..."), NULL,
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-channel-list",
    NULL, NULL },
  { { N_("/Dialogs/Paths..."), NULL,
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-vectors-list",
    NULL, NULL },
  { { N_("/Dialogs/Indexed Palette..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SELECT_COLOR },
    "gimp-indexed-palette",
    "file/dialogs/indexed_palette.html", NULL },
  { { N_("/Dialogs/Selection Editor..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_RECT_SELECT },
    "gimp-selection-editor",
    NULL, NULL },
  { { N_("/Dialogs/Navigation..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_NAVIGATION },
    "gimp-navigation-view",
    NULL, NULL },

  MENU_SEPARATOR ("/Dialogs/---"),

  { { N_("/Dialogs/Colors..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SELECT_COLOR },
    "gimp-color-editor",
    NULL, NULL },
  { { N_("/Dialogs/Brushes..."), "<control><shift>B",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_PAINTBRUSH },
    "gimp-brush-grid",
    "dialogs/brush_selection.html", NULL },
  { { N_("/Dialogs/Patterns..."), "<control><shift>P",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_BUCKET_FILL },
    "gimp-pattern-grid",
    "dialogs/pattern_selection.html", NULL },
  { { N_("/Dialogs/Gradients..."), "<control>G",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_BLEND },
    "gimp-gradient-list",
    "dialogs/gradient_selection.html", NULL },
  { { N_("/Dialogs/Palettes..."), "<control>P",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SELECT_COLOR },
    "gimp-palette-list",
    "dialogs/palette_selection.html", NULL },
  { { N_("/Dialogs/Buffers..."), "foo",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_PASTE },
    "gimp-buffer-list",
    NULL, NULL },

  MENU_SEPARATOR ("/Dialogs/---"),

  { { N_("/Dialogs/Images..."), NULL,
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-image-list",
    NULL, NULL },
  { { N_("/Dialogs/Document History..."), "foo",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    "gimp-document-history",
    "dialogs/document_index.html", NULL },
  { { N_("/Dialogs/Error Console..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_WARNING },
    "gimp-error-console",
    "dialogs/error_console.html", NULL },

  MENU_SEPARATOR ("/filters-separator"),

  /*  <Image>/Filters  */

  { { N_("/Filters/Repeat Last"), "<alt>F",
      plug_in_repeat_cmd_callback, (guint) FALSE,
      "<StockItem>", GTK_STOCK_EXECUTE },
    NULL,
    "filters/repeat_last.html", NULL },
  { { N_("/Filters/Re-Show Last"), "<alt><shift>F",
      plug_in_repeat_cmd_callback, (guint) TRUE,
      "<StockItem>", GIMP_STOCK_RESHOW_FILTER },
    NULL,
    "filters/reshow_last.html", NULL },

  MENU_SEPARATOR ("/Filters/---"),

  MENU_BRANCH (N_("/Filters/Blur")),
  MENU_BRANCH (N_("/Filters/Colors")),
  MENU_BRANCH (N_("/Filters/Noise")),
  MENU_BRANCH (N_("/Filters/Edge-Detect")),
  MENU_BRANCH (N_("/Filters/Enhance")),
  MENU_BRANCH (N_("/Filters/Generic")),

  MENU_SEPARATOR ("/Filters/---"),

  MENU_BRANCH (N_("/Filters/Glass Effects")),
  MENU_BRANCH (N_("/Filters/Light Effects")),
  MENU_BRANCH (N_("/Filters/Distorts")),
  MENU_BRANCH (N_("/Filters/Artistic")),
  MENU_BRANCH (N_("/Filters/Map")),
  MENU_BRANCH (N_("/Filters/Render")),
  MENU_BRANCH (N_("/Filters/Text")),
  MENU_BRANCH (N_("/Filters/Web")),

  MENU_SEPARATOR ("/Filters/---INSERT"),

  MENU_BRANCH (N_("/Filters/Animation")),
  MENU_BRANCH (N_("/Filters/Combine")),

  MENU_SEPARATOR ("/Filters/---"),

  MENU_BRANCH (N_("/Filters/Toys"))
};

gint n_image_menu_entries = G_N_ELEMENTS (image_menu_entries);


void
image_menu_setup (GimpItemFactory *factory)
{
  if (GTK_IS_MENU_BAR (GTK_ITEM_FACTORY (factory)->widget))
    {
      gimp_item_factory_set_visible (GTK_ITEM_FACTORY (factory),
                                     "/tearoff1", FALSE);
      gimp_item_factory_set_visible (GTK_ITEM_FACTORY (factory),
                                     "/filters-separator", FALSE);
    }

  menus_last_opened_add (factory, factory->gimp);

  /*  create tool menu items  */
  {
    static const gchar *color_tools[] = { "gimp-color-balance-tool",
                                          "gimp-hue-saturation-tool",
                                          "gimp-brightness-contrast-tool",
                                          "gimp-threshold-tool",
                                          "gimp-levels-tool",
                                          "gimp-curves-tool" };
    GtkWidget    *menu_item;
    GimpToolInfo *tool_info;
    GList        *list;
    gint          i;

    for (list = GIMP_LIST (factory->gimp->tool_info_list)->list;
         list;
         list = g_list_next (list))
      {
        tool_info = GIMP_TOOL_INFO (list->data);

        if (tool_info->menu_path)
          {
            GimpItemFactoryEntry entry;

            entry.entry.path            = tool_info->menu_path;
            entry.entry.accelerator     = tool_info->menu_accel;
            entry.entry.callback        = tools_select_cmd_callback;
            entry.entry.callback_action = 0;
            entry.entry.item_type       = "<StockItem>";
            entry.entry.extra_data      = tool_info->stock_id;
            entry.quark_string          = NULL;
            entry.help_page             = tool_info->help_data;
            entry.description           = NULL;

            gimp_item_factory_create_item (factory,
                                           &entry,
                                           NULL,
                                           tool_info, 2,
                                           TRUE, FALSE);
          }
      }

    /*  reorder <Image>/Image/Colors  */
    tool_info = (GimpToolInfo *)
      gimp_container_get_child_by_name (factory->gimp->tool_info_list,
                                        "gimp-posterize-tool");

    menu_item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (factory),
                                             tool_info->menu_path);
    if (menu_item && menu_item->parent)
      gtk_menu_reorder_child (GTK_MENU (menu_item->parent), menu_item, 4);

    for (i = 0; i < G_N_ELEMENTS (color_tools); i++)
      {
        tool_info = (GimpToolInfo *)
          gimp_container_get_child_by_name (factory->gimp->tool_info_list,
                                            color_tools[i]);

        menu_item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (factory),
                                                 tool_info->menu_path);
        if (menu_item && menu_item->parent)
          {
            gtk_menu_reorder_child (GTK_MENU (menu_item->parent),
                                    menu_item, i + 1);
          }
      }
  }

  {
    GimpContext *user_context;
    GimpRGB      fg;
    GimpRGB      bg;

    user_context = gimp_get_user_context (factory->gimp);

    g_signal_connect_object (user_context, "foreground_changed",
                             G_CALLBACK (image_menu_foreground_changed),
                             factory, 0);
    g_signal_connect_object (user_context, "background_changed",
                             G_CALLBACK (image_menu_background_changed),
                             factory, 0);

    gimp_context_get_foreground (user_context, &fg);
    gimp_context_get_background (user_context, &bg);

    image_menu_foreground_changed (user_context, &fg, factory);
    image_menu_background_changed (user_context, &bg, factory);
  }

  plug_in_make_menu (factory, proc_defs);

  {
    static gchar *rotate_plugins[]      = { "Rotate 90 degrees",
                                            "Rotate 180 degrees",
                                            "Rotate 270 degrees" };
    static gchar *image_file_entries[]  = { "---moved",
                                            "Close",
                                            "Quit" };
    static gchar *reorder_submenus[]    = { "/Video",
                                            "/Script-Fu" };
    static gchar *reorder_subsubmenus[] = { "/Filters" };

    GtkWidget       *menu_item;
    GtkWidget       *menu;
    GList           *list;
    gchar           *path;
    gint             i, pos;

    /*  Move all menu items under "<Image>/Filters" which are not submenus or
     *  separators to the top of the menu
     */
    pos = 3;
    menu_item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (factory),
                                             "/Filters/Filter all Layers...");
    if (menu_item && menu_item->parent && GTK_IS_MENU (menu_item->parent))
      {
        menu = menu_item->parent;

        for (list = g_list_nth (GTK_MENU_SHELL (menu)->children, pos); list;
             list = g_list_next (list))
          {
            menu_item = GTK_WIDGET (list->data);

            if (! GTK_MENU_ITEM (menu_item)->submenu &&
                GTK_IS_LABEL (GTK_BIN (menu_item)->child))
              {
                gtk_menu_reorder_child (GTK_MENU (menu_item->parent),
                                        menu_item, pos);
                list = g_list_nth (GTK_MENU_SHELL (menu)->children, pos);
                pos++;
              }
          }
      }

    /*  Reorder Rotate plugin menu entries */
    pos = 1;
    for (i = 0; i < G_N_ELEMENTS (rotate_plugins); i++)
      {
        path = g_strconcat ("/Image/Transform/", rotate_plugins[i], NULL);
        menu_item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (factory),
                                                 path);
        g_free (path);

        if (menu_item && menu_item->parent)
          {
            gtk_menu_reorder_child (GTK_MENU (menu_item->parent),
                                    menu_item, pos);
            pos++;
          }
      }

    pos = 1;
    for (i = 0; i < G_N_ELEMENTS (rotate_plugins); i++)
      {
        path = g_strconcat ("/Layer/Transform/", rotate_plugins[i], NULL);
        menu_item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (factory),
                                                 path);
        g_free (path);

        if (menu_item && menu_item->parent)
          {
            gtk_menu_reorder_child (GTK_MENU (menu_item->parent),
                                    menu_item, pos);
            pos++;
          }
      }

    /*  Reorder "<Image>/File"  */
    for (i = 0; i < G_N_ELEMENTS (image_file_entries); i++)
      {
        path = g_strconcat ("/File/", image_file_entries[i], NULL);
        menu_item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (factory),
                                                 path);
        g_free (path);

        if (menu_item && menu_item->parent)
          gtk_menu_reorder_child (GTK_MENU (menu_item->parent), menu_item, -1);
      }

    /*  Reorder menus where plugins registered submenus  */
    for (i = 0; i < G_N_ELEMENTS (reorder_submenus); i++)
      {
        menu = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (factory),
                                            reorder_submenus[i]);

        if (menu && GTK_IS_MENU (menu))
          menus_filters_subdirs_to_top (GTK_MENU (menu));
      }

    for (i = 0; i < G_N_ELEMENTS (reorder_subsubmenus); i++)
      {
        menu = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (factory),
                                            reorder_subsubmenus[i]);

        if (menu && GTK_IS_MENU (menu))
          {
            for (list = GTK_MENU_SHELL (menu)->children; list;
                 list = g_list_next (list))
              {
                GtkMenuItem *menu_item;

                menu_item = GTK_MENU_ITEM (list->data);

                if (menu_item->submenu)
                  menus_filters_subdirs_to_top (GTK_MENU (menu_item->submenu));
              }
          }
      }

    /*  Move all submenus which registered after "<Image>/Filters/Toys"
     *  before the separator after "<Image>/Filters/Web"
     */
    menu_item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (factory),
                                             "/Filters/---INSERT");

    if (menu_item && menu_item->parent && GTK_IS_MENU (menu_item->parent))
      {
        menu = menu_item->parent;
        pos = g_list_index (GTK_MENU_SHELL (menu)->children, menu_item);

        menu_item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (factory),
                                                 "/Filters/Toys");

        if (menu_item && GTK_IS_MENU (menu_item))
          {
            GList *list;
            gint index = 1;

            for (list = GTK_MENU_SHELL (menu)->children; list;
                 list = g_list_next (list))
              {
                if (GTK_MENU_ITEM (list->data)->submenu == menu_item)
                  break;

                index++;
              }

            while ((menu_item = g_list_nth_data (GTK_MENU_SHELL (menu)->children,
                                                 index)))
              {
                gtk_menu_reorder_child (GTK_MENU (menu), menu_item, pos);

                pos++;
                index++;
              }
          }
      }
  }
}

void
image_menu_update (GtkItemFactory *item_factory,
                   gpointer        data)
{
  Gimp             *gimp          = NULL;
  GimpDisplayShell *shell         = NULL;
  GimpDisplay      *gdisp         = NULL;
  GimpImage        *gimage        = NULL;
  GimpDrawable     *drawable      = NULL;
  GimpLayer        *layer         = NULL;
  GimpImageType     drawable_type = -1;
  GimpRGB           fg;
  GimpRGB           bg;
  gboolean          is_rgb        = FALSE;
  gboolean          is_gray       = FALSE;
  gboolean          is_indexed    = FALSE;
  gboolean          fs            = FALSE;
  gboolean          aux           = FALSE;
  gboolean          lm            = FALSE;
  gboolean          lp            = FALSE;
  gboolean          sel           = FALSE;
  gboolean          alpha         = FALSE;
  gint              lind          = -1;
  gint              lnum          = -1;

  gimp = GIMP_ITEM_FACTORY (item_factory)->gimp;

  if (data)
    {
      shell = GIMP_DISPLAY_SHELL (data);
      gdisp = shell->gdisp;
    }

  if (gdisp)
    {
      GimpImageBaseType base_type;

      gimage = gdisp->gimage;

      base_type = gimp_image_base_type (gimage);

      is_rgb     = (base_type == GIMP_RGB);
      is_gray    = (base_type == GIMP_GRAY);
      is_indexed = (base_type == GIMP_INDEXED);

      fs  = (gimp_image_floating_sel (gimage) != NULL);
      aux = (gimp_image_get_active_channel (gimage) != NULL);
      lp  = ! gimp_image_is_empty (gimage);
      sel = ! gimp_image_mask_is_empty (gimage);

      drawable = gimp_image_active_drawable (gimage);
      if (drawable)
	drawable_type = gimp_drawable_type (drawable);

      if (lp)
	{
	  layer = gimp_image_get_active_layer (gimage);

	  if (layer)
	    {
	      lm    = gimp_layer_get_mask (layer) ? TRUE : FALSE;
	      alpha = gimp_drawable_has_alpha (GIMP_DRAWABLE (layer));
	      lind  = gimp_image_get_layer_index (gimage, layer);
	    }

	  lnum = gimp_container_num_children (gimage->layers);
	}
    }

  gimp_context_get_foreground (gimp_get_user_context (gimp), &fg);
  gimp_context_get_background (gimp_get_user_context (gimp), &bg);

#define SET_ACTIVE(menu,condition) \
        gimp_item_factory_set_active (item_factory, menu, (condition) != 0)
#define SET_LABEL(menu,label) \
        gimp_item_factory_set_label (item_factory, menu, (label))
#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (item_factory, menu, (condition) != 0)

  /*  File  */

  SET_SENSITIVE ("/File/Save",           gdisp && drawable);
  SET_SENSITIVE ("/File/Save as...",     gdisp && drawable);
  SET_SENSITIVE ("/File/Save a Copy...", gdisp && drawable);
  SET_SENSITIVE ("/File/Revert...",      gdisp && GIMP_OBJECT (gimage)->name);
  SET_SENSITIVE ("/File/Close",          gdisp);

  /*  Edit  */

  {
    gchar *undo_name = NULL;
    gchar *redo_name = NULL;

    if (gdisp && gimp_image_undo_is_enabled (gimage))
      {
        undo_name = (gchar *) undo_get_undo_name (gimage);
        redo_name = (gchar *) undo_get_redo_name (gimage);
      }

    if (undo_name)
      undo_name = g_strdup_printf (_("Undo %s"), gettext (undo_name));

    if (redo_name)
      redo_name = g_strdup_printf (_("Redo %s"), gettext (redo_name));

    SET_LABEL ("/Edit/Undo", undo_name ? undo_name : _("Undo"));
    SET_LABEL ("/Edit/Redo", redo_name ? redo_name : _("Redo"));

    SET_SENSITIVE ("/Edit/Undo", undo_name);
    SET_SENSITIVE ("/Edit/Redo", redo_name);

    g_free (undo_name);
    g_free (redo_name);
  }

  SET_SENSITIVE ("/Edit/Cut",                   lp);
  SET_SENSITIVE ("/Edit/Copy",                  lp);
  SET_SENSITIVE ("/Edit/Paste",                 gdisp && gimp->global_buffer);
  SET_SENSITIVE ("/Edit/Paste Into",            gdisp && gimp->global_buffer);
  SET_SENSITIVE ("/Edit/Paste as New",          gimp->global_buffer);
  SET_SENSITIVE ("/Edit/Buffer/Cut Named...",   lp);
  SET_SENSITIVE ("/Edit/Buffer/Copy Named...",  lp);
  SET_SENSITIVE ("/Edit/Buffer/Paste Named...", lp);
  SET_SENSITIVE ("/Edit/Clear",                 lp);
  SET_SENSITIVE ("/Edit/Fill with FG Color",    lp);
  SET_SENSITIVE ("/Edit/Fill with BG Color",    lp);
  SET_SENSITIVE ("/Edit/Stroke",                lp && sel);

  /*  Select  */

  SET_SENSITIVE ("/Select/Invert",           lp && sel);
  SET_SENSITIVE ("/Select/All",              lp);
  SET_SENSITIVE ("/Select/None",             lp && sel);
  SET_SENSITIVE ("/Select/Float",            lp && sel);
  SET_SENSITIVE ("/Select/Feather...",       lp && sel);
  SET_SENSITIVE ("/Select/Sharpen",          lp && sel);
  SET_SENSITIVE ("/Select/Shrink...",        lp && sel);
  SET_SENSITIVE ("/Select/Grow...",          lp && sel);
  SET_SENSITIVE ("/Select/Border...",        lp && sel);
  SET_SENSITIVE ("/Select/Toggle QuickMask", gdisp);
  SET_SENSITIVE ("/Select/Save to Channel",  lp && sel && !fs);

  /*  View  */

  SET_SENSITIVE ("/View/Zoom In",            gdisp);
  SET_SENSITIVE ("/View/Zoom Out",           gdisp);
  SET_SENSITIVE ("/View/Zoom to Fit Window", gdisp);

  SET_SENSITIVE ("/View/Zoom/16:1", gdisp);
  SET_SENSITIVE ("/View/Zoom/8:1",  gdisp);
  SET_SENSITIVE ("/View/Zoom/4:1",  gdisp);
  SET_SENSITIVE ("/View/Zoom/2:1",  gdisp);
  SET_SENSITIVE ("/View/Zoom/1:1",  gdisp);
  SET_SENSITIVE ("/View/Zoom/1:2",  gdisp);
  SET_SENSITIVE ("/View/Zoom/1:4",  gdisp);
  SET_SENSITIVE ("/View/Zoom/1:8",  gdisp);
  SET_SENSITIVE ("/View/Zoom/1:16", gdisp);

  SET_SENSITIVE ("/View/Dot for Dot", gdisp);
  SET_ACTIVE    ("/View/Dot for Dot", gdisp && shell->dot_for_dot);

  SET_SENSITIVE ("/View/Info Window...",       gdisp);
  SET_SENSITIVE ("/View/Navigation Window...", gdisp);
  SET_SENSITIVE ("/View/Display Filters...",   gdisp);

  SET_SENSITIVE ("/View/Show Selection",      gdisp);
  SET_ACTIVE    ("/View/Show Selection",      gdisp && ! shell->select->hidden);
  SET_SENSITIVE ("/View/Show Layer Boundary", gdisp);
  SET_ACTIVE    ("/View/Show Layer Boundary", gdisp && ! shell->select->layer_hidden);

  SET_SENSITIVE ("/View/Show Guides",  gdisp);
  SET_ACTIVE    ("/View/Show Guides",  gdisp && gdisp->draw_guides);
  SET_SENSITIVE ("/View/Snap to Guides", gdisp);
  SET_ACTIVE    ("/View/Snap to Guides", gdisp && gdisp->snap_to_guides);

  SET_SENSITIVE ("/View/Show Menubar", gdisp);
  SET_ACTIVE    ("/View/Show Menubar",
                 gdisp &&
		 GTK_WIDGET_VISIBLE (GTK_ITEM_FACTORY (shell->menubar_factory)->widget));

  SET_SENSITIVE ("/View/Show Rulers", gdisp);
  SET_ACTIVE    ("/View/Show Rulers",
                 gdisp && GTK_WIDGET_VISIBLE (shell->hrule));

  SET_SENSITIVE ("/View/Show Statusbar", gdisp);
  SET_ACTIVE    ("/View/Show Statusbar",
                 gdisp && GTK_WIDGET_VISIBLE (shell->statusbar));

  SET_SENSITIVE ("/View/New View",    gdisp);
  SET_SENSITIVE ("/View/Shrink Wrap", gdisp);

  /*  Image  */

  SET_SENSITIVE ("/Image/Mode/RGB",        gdisp && ! is_rgb);
  SET_SENSITIVE ("/Image/Mode/Grayscale",  gdisp && ! is_gray);
  SET_SENSITIVE ("/Image/Mode/Indexed...", gdisp && ! is_indexed);

  SET_SENSITIVE ("/Image/Canvas Size...",          gdisp);
  SET_SENSITIVE ("/Image/Scale Image...",          gdisp);
  SET_SENSITIVE ("/Image/Crop Image",              gdisp && sel);
  SET_SENSITIVE ("/Image/Duplicate",               gdisp);
  SET_SENSITIVE ("/Image/Merge Visible Layers...", gdisp && !fs && !aux && lp);
  SET_SENSITIVE ("/Image/Flatten Image",           gdisp && !fs && !aux && lp);
  SET_SENSITIVE ("/Image/Undo History...",         gdisp);

  /*  Layer  */

  SET_SENSITIVE ("/Layer/Stack/Previous Layer",
                 lp && !fs && !aux && lind > 0);
  SET_SENSITIVE ("/Layer/Stack/Next Layer",
                 lp && !fs && !aux && lind < (lnum - 1));
  SET_SENSITIVE ("/Layer/Stack/Raise Layer",
                 lp && !fs && !aux && alpha && lind > 0);
  SET_SENSITIVE ("/Layer/Stack/Lower Layer",
                 lp && !fs && !aux && alpha && lind < (lnum - 1));
  SET_SENSITIVE ("/Layer/Stack/Layer to Top",
                 lp && !fs && !aux && alpha && lind > 0);
  SET_SENSITIVE ("/Layer/Stack/Layer to Bottom",
                 lp && !fs && !aux && alpha && lind < (lnum - 1));

  SET_SENSITIVE ("/Layer/New Layer...",    gdisp);
  SET_SENSITIVE ("/Layer/Duplicate Layer", lp && !fs && !aux);
  SET_SENSITIVE ("/Layer/Anchor Layer",    lp &&  fs && !aux);
  SET_SENSITIVE ("/Layer/Merge Down",      lp && !fs && !aux);
  SET_SENSITIVE ("/Layer/Delete Layer",    lp && !aux);

  SET_SENSITIVE ("/Layer/Layer Boundary Size...", lp && !aux);
  SET_SENSITIVE ("/Layer/Layer to Imagesize",     lp && !aux);
  SET_SENSITIVE ("/Layer/Scale Layer...",         lp && !aux);
  SET_SENSITIVE ("/Layer/Crop Layer",             lp && !aux && sel);

  SET_SENSITIVE ("/Layer/Transform/Offset...", lp);

  SET_SENSITIVE ("/Layer/Colors/Color Balance...",       lp &&   is_rgb);
  SET_SENSITIVE ("/Layer/Colors/Hue-Saturation...",      lp &&   is_rgb);
  SET_SENSITIVE ("/Layer/Colors/Brightness-Contrast...", lp && ! is_indexed);
  SET_SENSITIVE ("/Layer/Colors/Threshold...",           lp && ! is_indexed);
  SET_SENSITIVE ("/Layer/Colors/Levels...",              lp && ! is_indexed);
  SET_SENSITIVE ("/Layer/Colors/Curves...",              lp && ! is_indexed);
  SET_SENSITIVE ("/Layer/Colors/Desaturate",             lp &&   is_rgb);
  SET_SENSITIVE ("/Layer/Colors/Posterize...",           lp && ! is_indexed);
  SET_SENSITIVE ("/Layer/Colors/Invert",                 lp && ! is_indexed);
  SET_SENSITIVE ("/Layer/Colors/Auto/Equalize",          lp && ! is_indexed);
  SET_SENSITIVE ("/Layer/Colors/Histogram...",           lp);

  SET_SENSITIVE ("/Layer/Mask/Add Layer Mask...", lp && !aux && !lm && alpha && ! is_indexed);
  SET_SENSITIVE ("/Layer/Mask/Apply Layer Mask",  lp && !aux && lm);
  SET_SENSITIVE ("/Layer/Mask/Delete Layer Mask", lp && !aux && lm);
  SET_SENSITIVE ("/Layer/Mask/Mask to Selection", lp && !aux && lm);

  SET_SENSITIVE ("/Layer/Transparency/Alpha to Selection", lp && !aux && alpha);
  SET_SENSITIVE ("/Layer/Transparency/Add Alpha Channel",  lp && !aux && !fs && !lm && !alpha);

#undef SET_ACTIVE
#undef SET_LABEL
#undef SET_SENSITIVE

  plug_in_menus_update (GIMP_ITEM_FACTORY (item_factory), drawable_type);
}


/*  private functions  */

static void
image_menu_foreground_changed (GimpContext     *context,
                               const GimpRGB   *color,
                               GimpItemFactory *item_factory)
{
  gimp_item_factory_set_color (GTK_ITEM_FACTORY (item_factory),
                               "/Edit/Fill with FG Color", color, FALSE);
}

static void
image_menu_background_changed (GimpContext     *context,
                               const GimpRGB   *color,
                               GimpItemFactory *item_factory)
{
  gimp_item_factory_set_color (GTK_ITEM_FACTORY (item_factory),
                               "/Edit/Fill with BG Color", color, FALSE);
}
