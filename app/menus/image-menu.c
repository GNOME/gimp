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

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimplayer.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"
#include "core/gimpundostack.h"

#include "plug-in/plug-ins.h"

#include "widgets/gimpitemfactory.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
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

#include "gimp-intl.h"


/*  local function prototypes  */

static void   image_menu_foreground_changed (GimpContext      *context,
                                             const GimpRGB    *color,
                                             GimpItemFactory  *item_factory);
static void   image_menu_background_changed (GimpContext      *context,
                                             const GimpRGB    *color,
                                             GimpItemFactory  *item_factory);
static void   image_menu_set_zoom           (GtkItemFactory   *item_factory,
                                             GimpDisplayShell *shell);


GimpItemFactoryEntry image_menu_entries[] =
{
  { { "/tearoff", NULL, gimp_item_factory_tearoff_callback, 0, "<Tearoff>" },
    NULL, NULL, NULL },

  /*  <Image>/File  */

  MENU_BRANCH (N_("/_File")),

  { { N_("/File/_New..."), "<control>N",
      file_new_cmd_callback, 1,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    "file/dialogs/file_new.html", NULL },
  { { N_("/File/_Open..."), "<control>O",
      file_open_cmd_callback, 1,
      "<StockItem>", GTK_STOCK_OPEN },
    NULL,
    "file/dialogs/file_open.html", NULL },

  /*  <Image>/File/Open Recent  */

  MENU_BRANCH (N_("/File/Open _Recent")),

  { { N_("/File/Open Recent/(None)"), NULL, NULL, 0 },
    NULL, NULL, NULL },

  MENU_SEPARATOR ("/File/Open Recent/---"),

  { { N_("/File/Open Recent/Document _History..."), "foo",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    "gimp-document-list",
    "dialogs/document_index.html", NULL },

  MENU_SEPARATOR ("/File/---"),

  { { N_("/File/_Save"), "<control>S",
      file_save_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SAVE },
    NULL,
    "file/dialogs/file_save.html", NULL },
  { { N_("/File/Save _as..."), "<control><shift>S",
      file_save_as_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SAVE_AS },
    NULL,
    "file/dialogs/file_save.html", NULL },
  { { N_("/File/Sa_ve a Copy..."), NULL,
      file_save_a_copy_cmd_callback, 0 },
    NULL,
    "file/dialogs/file_save.html", NULL },
  { { N_("/File/Save as _Template..."), NULL,
      file_save_template_cmd_callback, 0 },
    NULL,
    "file/dialogs/file_save.html", NULL },
  { { N_("/File/Re_vert..."), NULL,
      file_revert_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_REVERT_TO_SAVED },
    NULL,
    "file/revert.html", NULL },

  MENU_SEPARATOR ("/File/---"),

  { { N_( "/File/_Close"), "<control>W",
      file_close_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_CLOSE },
    NULL,
    "file/close.html", NULL },
  { { N_("/File/_Quit"), "<control>Q",
      file_quit_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_QUIT },
    NULL,
    "file/quit.html", NULL },

  MENU_SEPARATOR ("/File/---moved"),

  /*  <Image>/Edit  */

  MENU_BRANCH (N_("/_Edit")),

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

  { { N_("/Edit/Cu_t"), "<control>X",
      edit_cut_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_CUT },
    NULL,
    "edit/cut.html", NULL },
  { { N_("/Edit/_Copy"), "<control>C",
      edit_copy_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_COPY },
    NULL,
    "edit/copy.html", NULL },
  { { N_("/Edit/_Paste"), "<control>V",
      edit_paste_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_PASTE },
    NULL,
    "edit/paste.html", NULL },
  { { N_("/Edit/Paste _Into"), NULL,
      edit_paste_into_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_PASTE_INTO },
    NULL,
    "edit/paste_into.html", NULL },
  { { N_("/Edit/Paste as _New"), NULL,
      edit_paste_as_new_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_PASTE_AS_NEW },
    NULL,
    "edit/paste_as_new.html", NULL },

  /*  <Image>/Edit/Buffer  */

  MENU_BRANCH (N_("/Edit/_Buffer")),

  { { N_("/Edit/Buffer/Cu_t Named..."), "<control><shift>X",
      edit_named_cut_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_CUT },
    NULL,
    "edit/dialogs/cut_named.html", NULL },
  { { N_("/Edit/Buffer/_Copy Named..."), "<control><shift>C",
      edit_named_copy_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_COPY },
    NULL,
    "edit/dialogs/copy_named.html", NULL },
  { { N_("/Edit/Buffer/_Paste Named..."), "<control><shift>V",
      edit_named_paste_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_PASTE },
    NULL,
    "edit/dialogs/paste_named.html", NULL },

  MENU_SEPARATOR ("/Edit/---"),

  { { N_("/Edit/Cl_ear"), "<control>K",
      edit_clear_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_CLEAR },
    NULL,
    "edit/clear.html", NULL },
  { { N_("/Edit/Fill with _FG Color"), "<control>comma",
      edit_fill_cmd_callback, (guint) GIMP_FG_BUCKET_FILL,
      "<StockItem>", GIMP_STOCK_TOOL_BUCKET_FILL },
    NULL,
    "edit/fill.html", NULL },
  { { N_("/Edit/Fill with B_G Color"), "<control>period",
      edit_fill_cmd_callback, (guint) GIMP_BG_BUCKET_FILL,
      "<StockItem>", GIMP_STOCK_TOOL_BUCKET_FILL },
    NULL,
    "edit/fill.html", NULL },
  { { N_("/Edit/Fill with Pattern"), NULL,
      edit_fill_cmd_callback, (guint) GIMP_PATTERN_BUCKET_FILL,
      "<StockItem>", GIMP_STOCK_TOOL_BUCKET_FILL },
    NULL,
    "edit/fill.html", NULL },
  { { N_("/Edit/_Stroke"), NULL,
      edit_stroke_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_STROKE },
    NULL,
    "edit/stroke.html", NULL },

  MENU_SEPARATOR ("/Edit/---"),

  /*  <Image>/Select  */
  
  MENU_BRANCH (N_("/_Select")),

  { { N_("/Select/_All"), "<control>A",
      select_all_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_ALL },
    NULL,
    "select/all.html", NULL },
  { { N_("/Select/_None"), "<control><shift>A",
      select_none_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_NONE },
    NULL,
    "select/none.html", NULL },
  { { N_("/Select/_Invert"), "<control>I",
      select_invert_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_INVERT },
    NULL,
    "select/invert.html", NULL },
  { { N_("/Select/_Float"), "<control><shift>L",
      select_float_cmd_callback, 0 },
    NULL,
    "select/float.html", NULL },
  { { N_("/Select/_By Color"), "<shift>O",
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_BY_COLOR_SELECT },
    "gimp-by-color-select-tool",
    "tools/by_color_select.html", NULL },

  MENU_SEPARATOR ("/Select/---"),

  { { N_("/Select/Fea_ther..."), NULL,
      select_feather_cmd_callback, 0 },
    NULL,
    "select/dialogs/feather_selection.html", NULL },
  { { N_("/Select/_Sharpen"), NULL,
      select_sharpen_cmd_callback, 0 },
    NULL,
    "select/sharpen.html", NULL },
  { { N_("/Select/S_hrink..."), NULL,
      select_shrink_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_SHRINK },
    NULL,
    "select/dialogs/shrink_selection.html", NULL },
  { { N_("/Select/_Grow..."), NULL,
      select_grow_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_GROW },
    NULL,
    "select/dialogs/grow_selection.html", NULL },
  { { N_("/Select/Bo_rder..."), NULL,
      select_border_cmd_callback, 0 },
    NULL,
    "select/dialogs/border_selection.html", NULL },

  MENU_SEPARATOR ("/Select/---"),

  { { N_("/Select/Toggle _QuickMask"), "<shift>Q",
      select_toggle_quickmask_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_QMASK_ON },
    NULL,
    "select/quickmask.html", NULL },
  { { N_("/Select/Save to _Channel"), NULL,
      select_save_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_TO_CHANNEL },
    NULL,
    "select/save_to_channel.html", NULL },

  /*  <Image>/View  */

  MENU_BRANCH (N_("/_View")),

  { { N_("/View/_New View"), "",
      view_new_view_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    "view/new_view.html", NULL },
  { { N_("/View/_Dot for Dot"), NULL,
      view_dot_for_dot_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/dot_for_dot.html", NULL },

  /*  <Image>/View/Zoom  */

  MENU_BRANCH (N_("/View/_Zoom")),

  { { N_("/View/Zoom/Zoom _Out"), "minus",
      view_zoom_out_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_ZOOM_OUT },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/Zoom _In"), "plus",
      view_zoom_in_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_ZOOM_IN },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/Zoom to _Fit Window"), "<control><shift>E",
      view_zoom_fit_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_ZOOM_FIT },
    NULL,
    "view/zoom.html", NULL },

  MENU_SEPARATOR ("/View/Zoom/---"),

  { { N_("/View/Zoom/16:1"), NULL,
      view_zoom_cmd_callback, 1601, "<RadioItem>" },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/8:1"), NULL,
      view_zoom_cmd_callback, 801, "/View/Zoom/16:1" },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/4:1"), NULL,
      view_zoom_cmd_callback, 401, "/View/Zoom/16:1" },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/2:1"), NULL,
      view_zoom_cmd_callback, 201, "/View/Zoom/16:1" },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/1:1"), "1",
      view_zoom_cmd_callback, 101, "/View/Zoom/16:1" },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/1:2"), NULL,
      view_zoom_cmd_callback, 102, "/View/Zoom/16:1" },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/1:4"), NULL,
      view_zoom_cmd_callback, 104, "/View/Zoom/16:1" },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/1:8"), NULL,
      view_zoom_cmd_callback, 108, "/View/Zoom/16:1" },
    NULL,
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/1:16"), NULL,
      view_zoom_cmd_callback, 116, "/View/Zoom/16:1" },
    NULL,
    "view/zoom.html", NULL },

  MENU_SEPARATOR ("/View/Zoom/---"),

  { { "/View/Zoom/O_ther...", NULL,
      view_zoom_other_cmd_callback, 0, "/View/Zoom/16:1" },
    NULL,
    "view/zoom.html", NULL },

  MENU_SEPARATOR ("/View/---"),

  { { N_("/View/_Info Window..."), "<control><shift>I",
      view_info_window_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_INFO },
    NULL,
    "view/dialogs/info_window.html", NULL },
  { { N_("/View/Na_vigation Window..."), "<control><shift>N",
      view_navigation_window_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_NAVIGATION },
    NULL,
    "view/dialogs/navigation_window.html", NULL },
  { { N_("/View/Display _Filters..."), NULL,
      view_display_filters_cmd_callback, 0 },
    NULL,
    "dialogs/display_filters/display_filters.html", NULL },

  MENU_SEPARATOR ("/View/---"),

  { { N_("/View/Show _Selection"), "<control>T",
      view_toggle_selection_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/toggle_selection.html", NULL },
  { { N_("/View/Show _Layer Boundary"), NULL,
      view_toggle_layer_boundary_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/toggle_selection.html", NULL },
  { { N_("/View/Show _Guides"), "<control><shift>T",
      view_toggle_guides_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/toggle_guides.html", NULL },
  { { N_("/View/Sn_ap to Guides"), NULL,
      view_snap_to_guides_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/snap_to_guides.html", NULL },

  MENU_SEPARATOR ("/View/---"),

  { { N_("/View/Configure G_rid..."), NULL,
      view_configure_grid_cmd_callback, 0, NULL },
    NULL,
    "view/configure_grid.html", NULL },
  { { N_("/View/S_how Grid"), NULL,
      view_toggle_grid_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/toggle_grid.html", NULL },
  { { N_("/View/Sna_p to Grid"), NULL,
      view_snap_to_grid_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/snap_to_grid.html", NULL },

  MENU_SEPARATOR ("/View/---"),

  { { N_("/View/Show _Menubar"), NULL,
      view_toggle_menubar_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/toggle_menubar.html", NULL },
  { { N_("/View/Show R_ulers"), "<control><shift>R",
      view_toggle_rulers_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/toggle_rulers.html", NULL },
  { { N_("/View/Show Scroll_bars"), NULL,
      view_toggle_scrollbars_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/toggle_rulers.html", NULL },
  { { N_("/View/Show S_tatusbar"), NULL,
      view_toggle_statusbar_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/toggle_statusbar.html", NULL },

  MENU_SEPARATOR ("/View/---"),

  { { N_("/View/Shrink _Wrap"), "<control>E",
      view_shrink_wrap_cmd_callback, 0 },
    NULL,
    "view/shrink_wrap.html", NULL },

  { { N_("/View/Fullscr_een"), "F11",
      view_fullscreen_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/fullscreen.html", NULL },

  /*  <Image>/Image  */

  MENU_BRANCH (N_("/_Image")),

  /*  <Image>/Image/Mode  */

  MENU_BRANCH (N_("/Image/_Mode")),

  { { N_("/Image/Mode/_RGB"), NULL,
      image_convert_rgb_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_CONVERT_RGB },
    NULL,
    "image/mode/convert_to_rgb.html", NULL },
  { { N_("/Image/Mode/_Grayscale"), NULL,
      image_convert_grayscale_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_CONVERT_GRAYSCALE },
    NULL,
    "image/mode/convert_to_grayscale.html", NULL },
  { { N_("/Image/Mode/_Indexed..."), NULL,
      image_convert_indexed_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_CONVERT_INDEXED },
    NULL,
    "image/mode/dialogs/convert_to_indexed.html", NULL },

  /*  <Image>/Image/Transform  */

  MENU_BRANCH (N_("/Image/_Transform")),

  { { N_("/Image/Transform/Flip _Horizontally"), NULL,
      image_flip_cmd_callback, GIMP_ORIENTATION_HORIZONTAL,
      "<StockItem>", GIMP_STOCK_FLIP_HORIZONTAL },
    NULL,
    "image/dialogs/flip_image.html", NULL },
  { { N_("/Image/Transform/Flip _Vertically"), NULL,
      image_flip_cmd_callback, GIMP_ORIENTATION_VERTICAL,
      "<StockItem>", GIMP_STOCK_FLIP_VERTICAL },
    NULL,
    "image/dialogs/flip_image.html", NULL },

  MENU_SEPARATOR ("/Image/Transform/---"),

  /*  please use the degree symbol in the translation  */
  { { N_("/Image/Transform/Rotate 90 degrees _CW"), NULL,
      image_rotate_cmd_callback, GIMP_ROTATE_90,
      "<StockItem>", GIMP_STOCK_ROTATE_90 },
    NULL,
    "image/dialogs/rotate_image.html", NULL },
  { { N_("/Image/Transform/Rotate 90 degrees CC_W"), NULL,
      image_rotate_cmd_callback, GIMP_ROTATE_270,
      "<StockItem>", GIMP_STOCK_ROTATE_270 },
    NULL,
    "image/dialogs/rotate_image.html", NULL },
  { { N_("/Image/Transform/Rotate _180 degrees"), NULL,
      image_rotate_cmd_callback, GIMP_ROTATE_180,
      "<StockItem>", GIMP_STOCK_ROTATE_180 },
    NULL,
    "image/dialogs/rotate_image.html", NULL },

  MENU_SEPARATOR ("/Image/Transform/---"),

  MENU_SEPARATOR ("/Image/---"),

  { { N_("/Image/Can_vas Size..."), NULL,
      image_resize_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_RESIZE },
    NULL,
    "image/dialogs/set_canvas_size.html", NULL },
  { { N_("/Image/_Scale Image..."), NULL,
      image_scale_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SCALE },
    NULL,
    "image/dialogs/scale_image.html", NULL },
  { { N_("/Image/_Crop Image"), NULL,
      image_crop_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_CROP },
    NULL,
    "image/dialogs/crop_image.html", NULL },
  { { N_("/Image/_Duplicate"), "<control>D",
      image_duplicate_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    "image/duplicate.html", NULL },

  MENU_SEPARATOR ("/Image/---"),

  { { N_("/Image/_Merge Visible Layers..."), "<control>M",
      image_merge_layers_cmd_callback, 0 },
    NULL,
    "layers/dialogs/merge_visible_layers.html", NULL },
  { { N_("/Image/_Flatten Image"), NULL,
      image_flatten_image_cmd_callback, 0 },
    NULL,
    "layers/flatten_image.html", NULL },

  /*  <Image>/Layer  */

  MENU_BRANCH (N_("/_Layer")),

  { { N_("/Layer/_New Layer..."), "",
      layers_new_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    "layers/dialogs/new_layer.html", NULL },
  { { N_("/Layer/Du_plicate Layer"), NULL,
      layers_duplicate_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    "layers/duplicate_layer.html", NULL },
  { { N_("/Layer/Anchor _Layer"), "<control>H",
      layers_anchor_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_ANCHOR },
    NULL,
    "layers/anchor_layer.html", NULL },
  { { N_("/Layer/Me_rge Down"), "<control><shift>M",
      layers_merge_down_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_MERGE_DOWN },
    NULL,
    "layers/merge_visible_layers.html", NULL },
  { { N_("/Layer/_Delete Layer"), NULL,
      layers_delete_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    "layers/delete_layer.html", NULL },

  MENU_SEPARATOR ("/Layer/---"),

  /*  <Image>/Layer/Stack  */

  MENU_BRANCH (N_("/Layer/Stac_k")),

  { { N_("/Layer/Stack/Select _Previous Layer"), "Prior",
      layers_select_previous_cmd_callback, 0 },
    NULL,
    "layers/stack/stack.html#previous_layer", NULL },
  { { N_("/Layer/Stack/Select _Next Layer"), "Next",
      layers_select_next_cmd_callback, 0 },
    NULL,
    "layers/stack/stack.html#next_layer", NULL },
  { { N_("/Layer/Stack/Select _Top Layer"), "Home",
      layers_select_top_cmd_callback, 0 },
    NULL,
    "layers/stack/stack.html#previous_layer", NULL },
  { { N_("/Layer/Stack/Select _Bottom Layer"), "End",
      layers_select_bottom_cmd_callback, 0 },
    NULL,
    "layers/stack/stack.html#next_layer", NULL },

  MENU_SEPARATOR ("/Layer/Stack/---"),

  { { N_("/Layer/Stack/_Raise Layer"), "<shift>Prior",
      layers_raise_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_UP },
    NULL,
    "layers/stack/stack.html#raise_layer", NULL },
  { { N_("/Layer/Stack/_Lower Layer"), "<shift>Next",
      layers_lower_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_DOWN },
    NULL,
    "layers/stack/stack.html#lower_layer", NULL },
  { { N_("/Layer/Stack/Layer to T_op"), "<shift>Home",
      layers_raise_to_top_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GOTO_TOP },
    NULL,
    "layers/stack/stack.html#layer_to_top", NULL },
  { { N_("/Layer/Stack/Layer to Botto_m"), "<Shift>End",
      layers_lower_to_bottom_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GOTO_BOTTOM },
    NULL,
    "layers/stack/stack.html#layer_to_bottom", NULL },

  /*  <Image>/Layer/Colors  */

  MENU_BRANCH (N_("/Layer/_Colors")),

  { { N_("/Layer/Colors/Color _Balance..."), NULL,
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_COLOR_BALANCE },
    "gimp-color-balance-tool",
    "tools/color_balance.html", NULL },    
  { { N_("/Layer/Colors/Hue-_Saturation..."), NULL,
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_HUE_SATURATION },
    "gimp-hue-saturation-tool",
    "tools/hue_saturation.html", NULL },    
  { { N_("/Layer/Colors/Colori_ze..."), NULL,
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_COLORIZE },
    "gimp-colorize-tool",
    "tools/colorize.html", NULL },    
  { { N_("/Layer/Colors/B_rightness-Contrast..."), NULL,
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_BRIGHTNESS_CONTRAST },
    "gimp-brightness-contrast-tool",
    "tools/brightness-contrast.html", NULL },    
  { { N_("/Layer/Colors/_Threshold..."), NULL,
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_THRESHOLD },
    "gimp-threshold-tool",
    "tools/threshold.html", NULL },    
  { { N_("/Layer/Colors/_Levels..."), NULL,
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_LEVELS },
    "gimp-levels-tool",
    "tools/levels.html", NULL },    
  { { N_("/Layer/Colors/_Curves..."), NULL,
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_CURVES },
    "gimp-curves-tool",
    "tools/curves.html", NULL },    
  { { N_("/Layer/Colors/_Posterize..."), NULL,
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_POSTERIZE },
    "gimp-posterize-tool",
    "tools/posterize.html", NULL },    

  MENU_SEPARATOR ("/Layer/Colors/---"),

  { { N_("/Layer/Colors/_Desaturate"), NULL,
      drawable_desaturate_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_CONVERT_GRAYSCALE },
    NULL,
    "layers/colors/desaturate.html", NULL },
  { { N_("/Layer/Colors/In_vert"), NULL,
      drawable_invert_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_INVERT },
    NULL,
    "layers/colors/invert.html", NULL },

  /*  <Image>/Layer/Colors/Auto  */

  MENU_BRANCH (N_("/Layer/Colors/_Auto")),

  { { N_("/Layer/Colors/Auto/_Equalize"), NULL,
      drawable_equalize_cmd_callback, 0 },
    NULL,
    "layers/colors/auto/equalize.html", NULL },

  MENU_SEPARATOR ("/Layer/Colors/---"),

  { { N_("/Layer/Colors/_Histogram..."), NULL,
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_HISTOGRAM },
    "gimp-histogram-tool",
    "tools/histogram.html", NULL },    

  /*  <Image>/Layer/Mask  */

  MENU_BRANCH (N_("/Layer/_Mask")),

  { { N_("/Layer/Mask/_Add Layer Mask..."), NULL,
      layers_add_layer_mask_cmd_callback, 0 },
    NULL,
    "layers/dialogs/add_layer_mask.html", NULL },
  { { N_("/Layer/Mask/A_pply Layer Mask"), NULL,
      layers_apply_layer_mask_cmd_callback, 0 },
    NULL,
    "layers/apply_mask.html", NULL },
  { { N_("/Layer/Mask/_Delete Layer Mask"), NULL,
      layers_delete_layer_mask_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    "layers/delete_mask.html", NULL },
  { { N_("/Layer/Mask/_Mask to Selection"), NULL,
      layers_mask_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_REPLACE },
    NULL,
    "layers/mask_to_selection.html", NULL },

  /*  <Image>/Layer/Transparency  */

  MENU_BRANCH (N_("/Layer/Tr_ansparency")),

  { { N_("/Layer/Transparency/_Add Alpha Channel"), NULL,
      layers_add_alpha_channel_cmd_callback, 0 },
    NULL,
    "layers/add_alpha_channel.html", NULL },
  { { N_("/Layer/Transparency/Al_pha to Selection"), NULL,
      layers_alpha_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_REPLACE },
    NULL,
    "layers/alpha_to_selection.html", NULL },

  MENU_SEPARATOR ("/Layer/Transparency/---"),

  /*  <Image>/Layer/Transform  */

  MENU_BRANCH (N_("/Layer/_Transform")),

  { { N_("/Layer/Transform/Flip _Horizontally"), NULL,
      drawable_flip_cmd_callback, GIMP_ORIENTATION_HORIZONTAL,
      "<StockItem>", GIMP_STOCK_FLIP_HORIZONTAL },
    NULL,
    "layers/flip_layer.html", NULL },
  { { N_("/Layer/Transform/Flip _Vertically"), NULL,
      drawable_flip_cmd_callback, GIMP_ORIENTATION_VERTICAL,
      "<StockItem>", GIMP_STOCK_FLIP_VERTICAL },
    NULL,
    "layers/flip_layer.html", NULL },

  MENU_SEPARATOR ("/Layer/Transform/---"),

  { { N_("/Layer/Transform/Rotate 90 degrees _CW"), NULL,
      drawable_rotate_cmd_callback, GIMP_ROTATE_90,
      "<StockItem>", GIMP_STOCK_ROTATE_90 },
    NULL,
    "layers/rotate_layer.html", NULL },
  { { N_("/Layer/Transform/Rotate 90 degrees CC_W"), NULL,
      drawable_rotate_cmd_callback, GIMP_ROTATE_270,
      "<StockItem>", GIMP_STOCK_ROTATE_270 },
    NULL,
    "layers/rotate_layer.html", NULL },
  { { N_("/Layer/Transform/Rotate _180 degrees"), NULL,
      drawable_rotate_cmd_callback, GIMP_ROTATE_180,
      "<StockItem>", GIMP_STOCK_ROTATE_180 },
    NULL,
    "layers/rotate_layer.html", NULL },    
  { { N_("/Layer/Transform/_Arbitrary Rotation..."), NULL,
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_ROTATE },
    "gimp-rotate-tool",
    "layers/rotate_layer.html", NULL },    

  MENU_SEPARATOR ("/Layer/Transform/---"),

  { { N_("/Layer/Transform/_Offset..."), "<control><shift>O",
      drawable_offset_cmd_callback, 0 },
    NULL,
    "layers/dialogs/offset.html", NULL },

  MENU_SEPARATOR ("/Layer/---"),

  { { N_("/Layer/Layer _Boundary Size..."), NULL,
      layers_resize_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_RESIZE },
    NULL,
    "layers/dialogs/layer_boundary_size.html", NULL },
  { { N_("/Layer/Layer to _Imagesize"), NULL,
      layers_resize_to_image_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_LAYER_TO_IMAGESIZE },
    NULL,
    "layers/layer_to_image_size.html", NULL },
  { { N_("/Layer/_Scale Layer..."), NULL,
      layers_scale_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SCALE },
    NULL,
    "layers/dialogs/scale_layer.html", NULL },
  { { N_("/Layer/Cr_op Layer"), NULL,
      layers_crop_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_CROP },
    NULL,
    "layers/dialogs/scale_layer.html", NULL },

  MENU_SEPARATOR ("/Layer/---"),

  /*  <Image>/Tools  */

  MENU_BRANCH (N_("/_Tools")),

  { { N_("/Tools/Tool_box"), NULL,
      dialogs_show_toolbox_cmd_callback, 0 },
    NULL,
    "toolbox/toolbox.html", NULL },
  { { N_("/Tools/_Default Colors"), "D",
      tools_default_colors_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DEFAULT_COLORS },
    NULL,
    "toolbox/toolbox.html#default_colors", NULL },
  { { N_("/Tools/S_wap Colors"), "X",
      tools_swap_colors_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SWAP_COLORS },
    NULL,
    "toolbox/toolbox.html#swap_colors", NULL },

  MENU_SEPARATOR ("/Tools/---"),

  MENU_BRANCH (N_("/Tools/_Selection Tools")),
  MENU_BRANCH (N_("/Tools/_Paint Tools")),
  MENU_BRANCH (N_("/Tools/_Transform Tools")),
  MENU_BRANCH (N_("/Tools/_Color Tools")),

  /*  <Image>/Dialogs  */

  MENU_BRANCH (N_("/_Dialogs")),

  MENU_BRANCH (N_("/Dialogs/Create New Doc_k")),

  { { N_("/Dialogs/Create New Dock/_Layers, Channels & Paths..."), NULL,
      dialogs_create_lc_cmd_callback, 0 },
    NULL,
    "dialogs/layers_and_channels.html", NULL },
  { { N_("/Dialogs/Create New Dock/_Brushes, Patterns & Gradients..."), NULL,
      dialogs_create_data_cmd_callback, 0 },
    NULL,
    NULL, NULL },
  { { N_("/Dialogs/Create New Dock/_Misc. Stuff..."), NULL,
      dialogs_create_stuff_cmd_callback, 0 },
    NULL,
    NULL, NULL },

  { { N_("/Dialogs/Tool _Options..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_OPTIONS },
    "gimp-tool-options",
    "dialogs/tool_options.html", NULL },
  { { N_("/Dialogs/_Device Status..."), NULL,
      dialogs_create_toplevel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DEVICE_STATUS },
    "gimp-device-status-dialog",
    "dialogs/device_status.html", NULL },

  MENU_SEPARATOR ("/Dialogs/---"),

  { { N_("/Dialogs/_Layers..."), "<control>L",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_LAYERS },
    "gimp-layer-list",
    NULL, NULL },
  { { N_("/Dialogs/_Channels..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_CHANNELS },
    "gimp-channel-list",
    NULL, NULL },
  { { N_("/Dialogs/_Paths..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_PATHS },
    "gimp-vectors-list",
    NULL, NULL },
  { { N_("/Dialogs/_Indexed Palette..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SELECT_COLOR },
    "gimp-indexed-palette",
    "file/dialogs/indexed_palette.html", NULL },
  { { N_("/Dialogs/_Selection Editor..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_RECT_SELECT },
    "gimp-selection-editor",
    NULL, NULL },
  { { N_("/Dialogs/Na_vigation..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_NAVIGATION },
    "gimp-navigation-view",
    NULL, NULL },
  { { N_("/Dialogs/_Undo History..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_UNDO },
    "gimp-undo-history",
    NULL, NULL },

  MENU_SEPARATOR ("/Dialogs/---"),

  { { N_("/Dialogs/Colo_rs..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SELECT_COLOR },
    "gimp-color-editor",
    NULL, NULL },
  { { N_("/Dialogs/Brus_hes..."), "<control><shift>B",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_PAINTBRUSH },
    "gimp-brush-grid",
    "dialogs/brush_selection.html", NULL },
  { { N_("/Dialogs/P_atterns..."), "<control><shift>P",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_BUCKET_FILL },
    "gimp-pattern-grid",
    "dialogs/pattern_selection.html", NULL },
  { { N_("/Dialogs/_Gradients..."), "<control>G",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_BLEND },
    "gimp-gradient-list",
    "dialogs/gradient_selection.html", NULL },
  { { N_("/Dialogs/Pal_ettes..."), "<control>P",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SELECT_COLOR },
    "gimp-palette-list",
    "dialogs/palette_selection.html", NULL },
  { { N_("/Dialogs/_Fonts..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SELECT_FONT },
    "gimp-font-list",
    "dialogs/font_selection.html", NULL },
  { { N_("/Dialogs/_Buffers..."), "foo",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_PASTE },
    "gimp-buffer-list",
    NULL, NULL },

  MENU_SEPARATOR ("/Dialogs/---"),

  { { N_("/Dialogs/I_mages..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_IMAGES },
    "gimp-image-list",
    NULL, NULL },
  { { N_("/Dialogs/Document Histor_y..."), "",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    "gimp-document-list",
    "dialogs/document_index.html", NULL },
  { { N_("/Dialogs/_Templates..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TEMPLATE },
    "gimp-template-list",
    "dialogs/templates.html", NULL },
  { { N_("/Dialogs/Error Co_nsole..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_WARNING },
    "gimp-error-console",
    "dialogs/error_console.html", NULL },


  /*  <Image>/Filters  */

  MENU_SEPARATOR ("/filters-separator"),
  MENU_BRANCH (N_("/Filte_rs")),

  { { N_("/Filters/Repeat Last"), "<control>F",
      plug_in_repeat_cmd_callback, (guint) FALSE,
      "<StockItem>", GTK_STOCK_EXECUTE },
    NULL,
    "filters/repeat_last.html", NULL },
  { { N_("/Filters/Re-Show Last"), "<control><shift>F",
      plug_in_repeat_cmd_callback, (guint) TRUE,
      "<StockItem>", GIMP_STOCK_RESHOW_FILTER },
    NULL,
    "filters/reshow_last.html", NULL },

  MENU_SEPARATOR ("/Filters/---"),

  MENU_BRANCH (N_("/Filters/_Blur")),
  MENU_BRANCH (N_("/Filters/_Colors")),
  MENU_BRANCH (N_("/Filters/_Noise")),
  MENU_BRANCH (N_("/Filters/Edge-De_tect")),
  MENU_BRANCH (N_("/Filters/En_hance")),
  MENU_BRANCH (N_("/Filters/_Generic")),

  MENU_SEPARATOR ("/Filters/---"),

  MENU_BRANCH (N_("/Filters/Gla_ss Effects")),
  MENU_BRANCH (N_("/Filters/_Light Effects")),
  MENU_BRANCH (N_("/Filters/_Distorts")),
  MENU_BRANCH (N_("/Filters/_Artistic")),
  MENU_BRANCH (N_("/Filters/_Map")),
  MENU_BRANCH (N_("/Filters/_Render")),
  MENU_BRANCH (N_("/Filters/_Web")),

  MENU_SEPARATOR ("/Filters/web-separator"),

  MENU_BRANCH (N_("/Filters/An_imation")),
  MENU_BRANCH (N_("/Filters/C_ombine")),

  MENU_SEPARATOR ("/Filters/---"),

  MENU_BRANCH (N_("/Filters/To_ys"))
};

gint n_image_menu_entries = G_N_ELEMENTS (image_menu_entries);


void
image_menu_setup (GimpItemFactory *factory)
{
  if (GTK_IS_MENU_BAR (GTK_ITEM_FACTORY (factory)->widget))
    {
      if (GIMP_GUI_CONFIG (factory->gimp->config)->tearoff_menus)
        {
          gimp_item_factory_set_visible (GTK_ITEM_FACTORY (factory),
                                         "/tearoff", FALSE);
        }

      gimp_item_factory_set_visible (GTK_ITEM_FACTORY (factory),
                                     "/filters-separator", FALSE);
    }

  menus_last_opened_add (factory, factory->gimp);

  /*  create tool menu items  */
  {
    GimpToolInfo *tool_info;
    GList        *list;

    for (list = GIMP_LIST (factory->gimp->tool_info_list)->list;
         list;
         list = g_list_next (list))
      {
        tool_info = GIMP_TOOL_INFO (list->data);

        if (tool_info->menu_path)
          {
            GimpItemFactoryEntry  entry;
            const gchar          *stock_id;
            const gchar          *identifier;

            stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool_info));
            identifier = gimp_object_get_name (GIMP_OBJECT (tool_info));

            entry.entry.path            = tool_info->menu_path;
            entry.entry.accelerator     = tool_info->menu_accel;
            entry.entry.callback        = tools_select_cmd_callback;
            entry.entry.callback_action = 0;
            entry.entry.item_type       = "<StockItem>";
            entry.entry.extra_data      = stock_id;
            entry.quark_string          = identifier;
            entry.help_page             = tool_info->help_data;
            entry.description           = NULL;

            gimp_item_factory_create_item (factory,
                                           &entry,
                                           NULL,
                                           factory->gimp, 2,
                                           TRUE, FALSE);
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

  plug_in_menus_create (factory, factory->gimp->plug_in_proc_defs);

  {
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

        for (list = g_list_nth (GTK_MENU_SHELL (menu)->children, pos);
             list;
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
                                             "/Filters/web-separator");

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
  Gimp                       *gimp          = NULL;
  GimpDisplayShell           *shell         = NULL;
  GimpDisplay                *gdisp         = NULL;
  GimpImage                  *gimage        = NULL;
  GimpDrawable               *drawable      = NULL;
  GimpLayer                  *layer         = NULL;
  GimpImageType               drawable_type = -1;
  GimpRGB                     fg;
  GimpRGB                     bg;
  gboolean                    is_rgb        = FALSE;
  gboolean                    is_gray       = FALSE;
  gboolean                    is_indexed    = FALSE;
  gboolean                    fs            = FALSE;
  gboolean                    aux           = FALSE;
  gboolean                    lm            = FALSE;
  gboolean                    lp            = FALSE;
  gboolean                    sel           = FALSE;
  gboolean                    alpha         = FALSE;
  gint                        lind          = -1;
  gint                        lnum          = -1;
  gboolean                    fullscreen    = FALSE;
  GimpDisplayShellVisibility *visibility    = NULL;
  
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

      fullscreen = gimp_display_shell_get_fullscreen (shell);

      if (fullscreen)
        visibility = &shell->fullscreen_visibility;
      else
        visibility = &shell->visibility;
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

  SET_SENSITIVE ("/File/Save",                gdisp && drawable);
  SET_SENSITIVE ("/File/Save as...",          gdisp && drawable);
  SET_SENSITIVE ("/File/Save a Copy...",      gdisp && drawable);
  SET_SENSITIVE ("/File/Save as Template...", gdisp);
  SET_SENSITIVE ("/File/Revert...",           gdisp && GIMP_OBJECT (gimage)->name);
  SET_SENSITIVE ("/File/Close",               gdisp);

  /*  Edit  */

  {
    gchar *undo_name = NULL;
    gchar *redo_name = NULL;

    if (gdisp && gimp_image_undo_is_enabled (gimage))
      {
        GimpUndo *undo;
        GimpUndo *redo;

        undo = gimp_undo_stack_peek (gimage->undo_stack);
        redo = gimp_undo_stack_peek (gimage->redo_stack);

        if (undo)
          undo_name =
            g_strdup_printf (_("_Undo %s"),
                             gimp_object_get_name (GIMP_OBJECT (undo)));

        if (redo)
          redo_name =
            g_strdup_printf (_("_Redo %s"),
                             gimp_object_get_name (GIMP_OBJECT (redo)));
      }

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

  SET_SENSITIVE ("/Select/All",              lp);
  SET_SENSITIVE ("/Select/None",             lp && sel);
  SET_SENSITIVE ("/Select/Invert",           lp && sel);
  SET_SENSITIVE ("/Select/Float",            lp && sel);
  SET_SENSITIVE ("/Select/By Color",         lp);

  SET_SENSITIVE ("/Select/Feather...",       lp && sel);
  SET_SENSITIVE ("/Select/Sharpen",          lp && sel);
  SET_SENSITIVE ("/Select/Shrink...",        lp && sel);
  SET_SENSITIVE ("/Select/Grow...",          lp && sel);
  SET_SENSITIVE ("/Select/Border...",        lp && sel);

  SET_SENSITIVE ("/Select/Toggle QuickMask", gdisp);
  SET_SENSITIVE ("/Select/Save to Channel",  lp && sel && !fs);

  /*  View  */

  SET_SENSITIVE ("/View/New View",   gdisp);

  SET_SENSITIVE ("/View/Dot for Dot", gdisp);
  SET_ACTIVE    ("/View/Dot for Dot", gdisp && shell->dot_for_dot);

  SET_SENSITIVE ("/View/Zoom/Zoom Out",           gdisp);
  SET_SENSITIVE ("/View/Zoom/Zoom In",            gdisp);
  SET_SENSITIVE ("/View/Zoom/Zoom to Fit Window", gdisp);

  SET_SENSITIVE ("/View/Zoom/16:1", gdisp);
  SET_SENSITIVE ("/View/Zoom/8:1",  gdisp);
  SET_SENSITIVE ("/View/Zoom/4:1",  gdisp);
  SET_SENSITIVE ("/View/Zoom/2:1",  gdisp);
  SET_SENSITIVE ("/View/Zoom/1:1",  gdisp);
  SET_SENSITIVE ("/View/Zoom/1:2",  gdisp);
  SET_SENSITIVE ("/View/Zoom/1:4",  gdisp);
  SET_SENSITIVE ("/View/Zoom/1:8",  gdisp);
  SET_SENSITIVE ("/View/Zoom/1:16", gdisp);

  SET_SENSITIVE ("/View/Zoom/Other...", gdisp);

  if (gdisp)
    image_menu_set_zoom (item_factory, shell);

  SET_SENSITIVE ("/View/Info Window...",       gdisp);
  SET_SENSITIVE ("/View/Navigation Window...", gdisp);
  SET_SENSITIVE ("/View/Display Filters...",   gdisp);

  SET_SENSITIVE ("/View/Show Selection",      gdisp);
  SET_ACTIVE    ("/View/Show Selection",      gdisp && visibility->selection);
  SET_SENSITIVE ("/View/Show Layer Boundary", gdisp);
  SET_ACTIVE    ("/View/Show Layer Boundary", gdisp && visibility->active_layer);
  SET_SENSITIVE ("/View/Show Guides",         gdisp);
  SET_ACTIVE    ("/View/Show Guides",         gdisp && visibility->guides);
  SET_SENSITIVE ("/View/Snap to Guides",      gdisp);
  SET_ACTIVE    ("/View/Snap to Guides",      gdisp && shell->snap_to_guides);

  SET_SENSITIVE ("/View/Show Grid",    gdisp);
  SET_ACTIVE    ("/View/Show Grid",    gdisp && visibility->grid);
  SET_SENSITIVE ("/View/Snap to Grid", gdisp);
  SET_ACTIVE    ("/View/Snap to Grid", gdisp && shell->snap_to_grid);

  SET_SENSITIVE ("/View/Show Menubar",    gdisp);
  SET_ACTIVE    ("/View/Show Menubar",    gdisp && visibility->menubar);
  SET_SENSITIVE ("/View/Show Rulers",     gdisp);
  SET_ACTIVE    ("/View/Show Rulers",     gdisp && visibility->rulers);
  SET_SENSITIVE ("/View/Show Scrollbars", gdisp);
  SET_ACTIVE    ("/View/Show Scrollbars", gdisp && visibility->scrollbars);
  SET_SENSITIVE ("/View/Show Statusbar",  gdisp);
  SET_ACTIVE    ("/View/Show Statusbar",  gdisp && visibility->statusbar);

  SET_SENSITIVE ("/View/Shrink Wrap", gdisp);

  SET_SENSITIVE ("/View/Fullscreen", gdisp);
  SET_ACTIVE    ("/View/Fullscreen", gdisp && fullscreen);

  /*  Image  */

  SET_SENSITIVE ("/Image/Mode/RGB",        gdisp && ! is_rgb);
  SET_SENSITIVE ("/Image/Mode/Grayscale",  gdisp && ! is_gray);
  SET_SENSITIVE ("/Image/Mode/Indexed...", gdisp && ! is_indexed);

  SET_SENSITIVE ("/Image/Transform/Flip Horizontally",     gdisp);
  SET_SENSITIVE ("/Image/Transform/Flip Vertically",       gdisp);
  SET_SENSITIVE ("/Image/Transform/Rotate 90 degrees CW",  gdisp);
  SET_SENSITIVE ("/Image/Transform/Rotate 90 degrees CCW", gdisp);
  SET_SENSITIVE ("/Image/Transform/Rotate 180 degrees",    gdisp);

  SET_SENSITIVE ("/Image/Canvas Size...",          gdisp);
  SET_SENSITIVE ("/Image/Scale Image...",          gdisp);
  SET_SENSITIVE ("/Image/Crop Image",              gdisp && sel);
  SET_SENSITIVE ("/Image/Duplicate",               gdisp);
  SET_SENSITIVE ("/Image/Merge Visible Layers...", gdisp && !fs && !aux && lp);
  SET_SENSITIVE ("/Image/Flatten Image",           gdisp && !fs && !aux && lp);

  /*  Layer  */

  SET_SENSITIVE ("/Layer/New Layer...",    gdisp);
  SET_SENSITIVE ("/Layer/Duplicate Layer", lp && !fs && !aux);
  SET_SENSITIVE ("/Layer/Anchor Layer",    lp &&  fs && !aux);
  SET_SENSITIVE ("/Layer/Merge Down",      lp && !fs && !aux);
  SET_SENSITIVE ("/Layer/Delete Layer",    lp && !aux);

  SET_SENSITIVE ("/Layer/Layer Boundary Size...", lp && !aux);
  SET_SENSITIVE ("/Layer/Layer to Imagesize",     lp && !aux);
  SET_SENSITIVE ("/Layer/Scale Layer...",         lp && !aux);
  SET_SENSITIVE ("/Layer/Crop Layer",             lp && !aux && sel);

  SET_SENSITIVE ("/Layer/Stack/Select Previous Layer",
                 lp && !fs && !aux && lind > 0);
  SET_SENSITIVE ("/Layer/Stack/Select Next Layer",
                 lp && !fs && !aux && lind < (lnum - 1));
  SET_SENSITIVE ("/Layer/Stack/Select Top Layer",
                 lp && !fs && !aux && lind > 0);
  SET_SENSITIVE ("/Layer/Stack/Select Bottom Layer",
                 lp && !fs && !aux && lind < (lnum - 1));

  SET_SENSITIVE ("/Layer/Stack/Raise Layer",
                 lp && !fs && !aux && alpha && lind > 0);
  SET_SENSITIVE ("/Layer/Stack/Lower Layer",
                 lp && !fs && !aux && alpha && lind < (lnum - 1));
  SET_SENSITIVE ("/Layer/Stack/Layer to Top",
                 lp && !fs && !aux && alpha && lind > 0);
  SET_SENSITIVE ("/Layer/Stack/Layer to Bottom",
                 lp && !fs && !aux && alpha && lind < (lnum - 1));

  SET_SENSITIVE ("/Layer/Colors/Color Balance...",       lp &&   is_rgb);
  SET_SENSITIVE ("/Layer/Colors/Hue-Saturation...",      lp &&   is_rgb);
  SET_SENSITIVE ("/Layer/Colors/Colorize...",            lp &&   is_rgb);
  SET_SENSITIVE ("/Layer/Colors/Brightness-Contrast...", lp && ! is_indexed);
  SET_SENSITIVE ("/Layer/Colors/Threshold...",           lp && ! is_indexed);
  SET_SENSITIVE ("/Layer/Colors/Levels...",              lp && ! is_indexed);
  SET_SENSITIVE ("/Layer/Colors/Curves...",              lp && ! is_indexed);
  SET_SENSITIVE ("/Layer/Colors/Posterize...",           lp && ! is_indexed);
  SET_SENSITIVE ("/Layer/Colors/Desaturate",             lp &&   is_rgb);
  SET_SENSITIVE ("/Layer/Colors/Invert",                 lp && ! is_indexed);
  SET_SENSITIVE ("/Layer/Colors/Auto/Equalize",          lp && ! is_indexed);
  SET_SENSITIVE ("/Layer/Colors/Histogram...",           lp);

  SET_SENSITIVE ("/Layer/Mask/Add Layer Mask...", lp && !fs && !aux && !lm && alpha);
  SET_SENSITIVE ("/Layer/Mask/Apply Layer Mask",  lp && !fs && !aux && lm);
  SET_SENSITIVE ("/Layer/Mask/Delete Layer Mask", lp && !fs && !aux && lm);
  SET_SENSITIVE ("/Layer/Mask/Mask to Selection", lp && !fs && !aux && lm);

  SET_SENSITIVE ("/Layer/Transparency/Add Alpha Channel",  lp && !aux && !fs && !lm && !alpha);
  SET_SENSITIVE ("/Layer/Transparency/Alpha to Selection", lp && !aux && alpha);

  SET_SENSITIVE ("/Layer/Transform/Flip Horizontally",     lp);
  SET_SENSITIVE ("/Layer/Transform/Flip Vertically",       lp);
  SET_SENSITIVE ("/Layer/Transform/Rotate 90 degrees CW",  lp);
  SET_SENSITIVE ("/Layer/Transform/Rotate 90 degrees CCW", lp);
  SET_SENSITIVE ("/Image/Transform/Rotate 180 degrees",    lp);
  SET_SENSITIVE ("/Layer/Transform/Offset...",             lp);

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

static void
image_menu_set_zoom (GtkItemFactory   *item_factory,
                     GimpDisplayShell *shell)
{
  const gchar *menu = NULL;
  guint        scalesrc;
  guint        scaledest;
  gchar       *label;

  scalesrc  = SCALESRC (shell);
  scaledest = SCALEDEST (shell);
 
  if (scaledest == 1)
    {
      switch (scalesrc)
        {
        case  1:  menu = "/View/Zoom/1:1";   break;
        case  2:  menu = "/View/Zoom/1:2";   break;
        case  4:  menu = "/View/Zoom/1:4";   break;
        case  8:  menu = "/View/Zoom/1:8";   break;
        case 16:  menu = "/View/Zoom/1:16";  break;
        }
    }
  else if (scalesrc == 1)
    {
      switch (scaledest)
        {
        case  2:  menu = "/View/Zoom/2:1";   break;
        case  4:  menu = "/View/Zoom/4:1";   break;
        case  8:  menu = "/View/Zoom/8:1";   break;
        case 16:  menu = "/View/Zoom/16:1";  break;
        }
    }

  if (!menu)
    {
      menu = "/View/Zoom/Other...";

      label = g_strdup_printf (_("Other (%d:%d) ..."), scaledest, scalesrc);
      gimp_item_factory_set_label (item_factory, menu, label);
      g_free (label);      

      shell->other_scale = shell->scale;
    }

  gimp_item_factory_set_active (item_factory, menu, TRUE);

  label = g_strdup_printf (_("_Zoom (%d:%d)"), scaledest, scalesrc);
  gimp_item_factory_set_label (item_factory, "/View/Zoom", label);
  g_free (label);

  /*  flag as dirty  */
  shell->other_scale |= (1 << 30);
}
