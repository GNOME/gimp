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
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"
#include "core/gimpundostack.h"

#include "plug-in/plug-ins.h"

#include "text/gimptextlayer.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayoptions.h"
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
#include "vectors-commands.h"
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
    GIMP_HELP_FILE_NEW, NULL },
  { { N_("/File/_Open..."), "<control>O",
      file_open_cmd_callback, 1,
      "<StockItem>", GTK_STOCK_OPEN },
    NULL,
    GIMP_HELP_FILE_OPEN, NULL },

  /*  <Image>/File/Open Recent  */

  MENU_BRANCH (N_("/File/Open _Recent")),

  { { N_("/File/Open Recent/(None)"), NULL, NULL, 0 },
    NULL, NULL, NULL },

  MENU_SEPARATOR ("/File/Open Recent/---"),

  { { N_("/File/Open Recent/Document _History"), "foo",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    "gimp-document-list|gimp-document-grid",
    GIMP_HELP_DOCUMENT_DIALOG, NULL },

  MENU_SEPARATOR ("/File/---"),

  { { N_("/File/_Save"), "<control>S",
      file_save_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SAVE },
    NULL,
    GIMP_HELP_FILE_SAVE, NULL },
  { { N_("/File/Save _as..."), "<control><shift>S",
      file_save_as_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SAVE_AS },
    NULL,
    GIMP_HELP_FILE_SAVE_AS, NULL },
  { { N_("/File/Save a Cop_y..."), NULL,
      file_save_a_copy_cmd_callback, 0 },
    NULL,
    GIMP_HELP_FILE_SAVE_A_COPY, NULL },
  { { N_("/File/Save as _Template..."), NULL,
      file_save_template_cmd_callback, 0 },
    NULL,
    GIMP_HELP_FILE_SAVE_AS_TEMPLATE, NULL },
  { { N_("/File/Re_vert"), NULL,
      file_revert_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_REVERT_TO_SAVED },
    NULL,
    GIMP_HELP_FILE_REVERT, NULL },

  MENU_SEPARATOR ("/File/---"),

  { { N_( "/File/_Close"), "<control>W",
      file_close_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_CLOSE },
    NULL,
    GIMP_HELP_FILE_CLOSE, NULL },
  { { N_("/File/_Quit"), "<control>Q",
      file_quit_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_QUIT },
    NULL,
    GIMP_HELP_FILE_QUIT, NULL },

  MENU_SEPARATOR ("/File/---moved"),

  /*  <Image>/Edit  */

  MENU_BRANCH (N_("/_Edit")),

  { { N_("/Edit/_Undo"), "<control>Z",
      edit_undo_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_UNDO },
    NULL,
    GIMP_HELP_EDIT_UNDO, NULL },
  { { N_("/Edit/_Redo"), "<control>Y",
      edit_redo_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_REDO },
    NULL,
    GIMP_HELP_EDIT_REDO, NULL },
  { { N_("/Edit/Undo _History"), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_UNDO_HISTORY },
    "gimp-undo-history",
    GIMP_HELP_UNDO_DIALOG, NULL },

  MENU_SEPARATOR ("/Edit/---"),

  { { N_("/Edit/Cu_t"), "<control>X",
      edit_cut_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_CUT },
    NULL,
    GIMP_HELP_EDIT_CUT, NULL },
  { { N_("/Edit/_Copy"), "<control>C",
      edit_copy_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_COPY },
    NULL,
    GIMP_HELP_EDIT_COPY, NULL },
  { { N_("/Edit/_Paste"), "<control>V",
      edit_paste_cmd_callback, (guint) FALSE,
      "<StockItem>", GTK_STOCK_PASTE },
    NULL,
    GIMP_HELP_EDIT_PASTE, NULL },
  { { N_("/Edit/Paste _Into"), NULL,
      edit_paste_cmd_callback, (guint) TRUE,
      "<StockItem>", GIMP_STOCK_PASTE_INTO },
    NULL,
    GIMP_HELP_EDIT_PASTE_INTO, NULL },
  { { N_("/Edit/Paste as _New"), NULL,
      edit_paste_as_new_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_PASTE_AS_NEW },
    NULL,
    GIMP_HELP_EDIT_PASTE_AS_NEW, NULL },

  /*  <Image>/Edit/Buffer  */

  MENU_BRANCH (N_("/Edit/_Buffer")),

  { { N_("/Edit/Buffer/Cu_t Named..."), "<control><shift>X",
      edit_named_cut_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_CUT },
    NULL,
    GIMP_HELP_BUFFER_CUT, NULL },
  { { N_("/Edit/Buffer/_Copy Named..."), "<control><shift>C",
      edit_named_copy_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_COPY },
    NULL,
    GIMP_HELP_BUFFER_COPY, NULL },
  { { N_("/Edit/Buffer/_Paste Named..."), "<control><shift>V",
      edit_named_paste_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_PASTE },
    NULL,
    GIMP_HELP_BUFFER_PASTE, NULL },

  MENU_SEPARATOR ("/Edit/---"),

  { { N_("/Edit/Cl_ear"), "<control>K",
      edit_clear_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_CLEAR },
    NULL,
    GIMP_HELP_EDIT_CLEAR, NULL },
  { { N_("/Edit/Fill with _FG Color"), "<control>comma",
      edit_fill_cmd_callback, (guint) GIMP_FOREGROUND_FILL,
      "<StockItem>", GIMP_STOCK_TOOL_BUCKET_FILL },
    NULL,
    GIMP_HELP_EDIT_FILL_FG, NULL },
  { { N_("/Edit/Fill with B_G Color"), "<control>period",
      edit_fill_cmd_callback, (guint) GIMP_BACKGROUND_FILL,
      "<StockItem>", GIMP_STOCK_TOOL_BUCKET_FILL },
    NULL,
    GIMP_HELP_EDIT_FILL_BG, NULL },
  { { N_("/Edit/Fill with P_attern"), NULL,
      edit_fill_cmd_callback, (guint) GIMP_PATTERN_FILL,
      "<StockItem>", GIMP_STOCK_TOOL_BUCKET_FILL },
    NULL,
    GIMP_HELP_EDIT_FILL_PATTERN, NULL },
  { { N_("/Edit/_Stroke Selection..."), NULL,
      edit_stroke_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_STROKE },
    NULL,
    GIMP_HELP_SELECTION_STROKE, NULL },
  { { N_("/Edit/St_roke Path..."), NULL,
      vectors_stroke_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_PATH_STROKE },
    NULL,
    GIMP_HELP_PATH_STROKE, NULL },

  MENU_SEPARATOR ("/Edit/---"),

  /*  <Image>/Select  */

  MENU_BRANCH (N_("/_Select")),

  { { N_("/Select/_All"), "<control>A",
      select_all_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_ALL },
    NULL,
    GIMP_HELP_SELECTION_ALL, NULL },
  { { N_("/Select/_None"), "<control><shift>A",
      select_none_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_NONE },
    NULL,
    GIMP_HELP_SELECTION_NONE, NULL },
  { { N_("/Select/_Invert"), "<control>I",
      select_invert_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_INVERT },
    NULL,
    GIMP_HELP_SELECTION_INVERT, NULL },
  { { N_("/Select/_Float"), "<control><shift>L",
      select_float_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_FLOATING_SELECTION },
    NULL,
    GIMP_HELP_SELECTION_FLOAT, NULL },
  { { N_("/Select/_By Color"), "<shift>O",
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_BY_COLOR_SELECT },
    "gimp-by-color-select-tool",
    GIMP_HELP_TOOL_BY_COLOR_SELECT, NULL },

  MENU_SEPARATOR ("/Select/---"),

  { { N_("/Select/Fea_ther..."), NULL,
      select_feather_cmd_callback, 0 },
    NULL,
    GIMP_HELP_SELECTION_FEATHER, NULL },
  { { N_("/Select/_Sharpen"), NULL,
      select_sharpen_cmd_callback, 0 },
    NULL,
    GIMP_HELP_SELECTION_SHARPEN, NULL },
  { { N_("/Select/S_hrink..."), NULL,
      select_shrink_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_SHRINK },
    NULL,
    GIMP_HELP_SELECTION_SHRINK, NULL },
  { { N_("/Select/_Grow..."), NULL,
      select_grow_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_GROW },
    NULL,
    GIMP_HELP_SELECTION_GROW, NULL },
  { { N_("/Select/Bo_rder..."), NULL,
      select_border_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_BORDER },
    NULL,
    GIMP_HELP_SELECTION_BORDER, NULL },

  MENU_SEPARATOR ("/Select/---"),

  { { N_("/Select/Toggle _QuickMask"), "<shift>Q",
      select_toggle_quickmask_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_QMASK_ON },
    NULL,
    GIMP_HELP_QMASK_TOGGLE, NULL },
  { { N_("/Select/Save to _Channel"), NULL,
      select_save_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_TO_CHANNEL },
    NULL,
    GIMP_HELP_SELECTION_TO_CHANNEL, NULL },
  { { N_("/Select/To _Path"), NULL,
      vectors_selection_to_vectors_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_TO_PATH },
    NULL,
    GIMP_HELP_SELECTION_TO_PATH, NULL },

  /*  <Image>/View  */

  MENU_BRANCH (N_("/_View")),

  { { N_("/View/_New View"), "",
      view_new_view_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    GIMP_HELP_VIEW_NEW, NULL },
  { { N_("/View/_Dot for Dot"), NULL,
      view_dot_for_dot_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    GIMP_HELP_VIEW_DOT_FOR_DOT, NULL },

  /*  <Image>/View/Zoom  */

  MENU_BRANCH (N_("/View/_Zoom")),

  { { N_("/View/Zoom/Zoom _Out"), "minus",
      view_zoom_out_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_ZOOM_OUT },
    NULL,
    GIMP_HELP_VIEW_ZOOM_OUT, NULL },
  { { N_("/View/Zoom/Zoom _In"), "plus",
      view_zoom_in_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_ZOOM_IN },
    NULL,
    GIMP_HELP_VIEW_ZOOM_IN, NULL },
  { { N_("/View/Zoom/Zoom to _Fit Window"), "<control><shift>E",
      view_zoom_fit_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_ZOOM_FIT },
    NULL,
    GIMP_HELP_VIEW_ZOOM_FIT, NULL },

  MENU_SEPARATOR ("/View/Zoom/---"),

  { { N_("/View/Zoom/16:1"), NULL,
      view_zoom_cmd_callback, 1601, "<RadioItem>" },
    NULL,
    GIMP_HELP_VIEW_ZOOM_IN, NULL },
  { { N_("/View/Zoom/8:1"), NULL,
      view_zoom_cmd_callback, 801, "/View/Zoom/16:1" },
    NULL,
    GIMP_HELP_VIEW_ZOOM_IN, NULL },
  { { N_("/View/Zoom/4:1"), NULL,
      view_zoom_cmd_callback, 401, "/View/Zoom/16:1" },
    NULL,
    GIMP_HELP_VIEW_ZOOM_IN, NULL },
  { { N_("/View/Zoom/2:1"), NULL,
      view_zoom_cmd_callback, 201, "/View/Zoom/16:1" },
    NULL,
    GIMP_HELP_VIEW_ZOOM_IN, NULL },
  { { N_("/View/Zoom/1:1"), "1",
      view_zoom_cmd_callback, 101, "/View/Zoom/16:1" },
    NULL,
    GIMP_HELP_VIEW_ZOOM_100, NULL },
  { { N_("/View/Zoom/1:2"), NULL,
      view_zoom_cmd_callback, 102, "/View/Zoom/16:1" },
    NULL,
    GIMP_HELP_VIEW_ZOOM_OUT, NULL },
  { { N_("/View/Zoom/1:4"), NULL,
      view_zoom_cmd_callback, 104, "/View/Zoom/16:1" },
    NULL,
    GIMP_HELP_VIEW_ZOOM_OUT, NULL },
  { { N_("/View/Zoom/1:8"), NULL,
      view_zoom_cmd_callback, 108, "/View/Zoom/16:1" },
    NULL,
    GIMP_HELP_VIEW_ZOOM_OUT, NULL },
  { { N_("/View/Zoom/1:16"), NULL,
      view_zoom_cmd_callback, 116, "/View/Zoom/16:1" },
    NULL,
    GIMP_HELP_VIEW_ZOOM_OUT, NULL },

  MENU_SEPARATOR ("/View/Zoom/---"),

  { { "/View/Zoom/O_ther...", NULL,
      view_zoom_other_cmd_callback, 0, "/View/Zoom/16:1" },
    NULL,
    GIMP_HELP_VIEW_ZOOM_OTHER, NULL },

  MENU_SEPARATOR ("/View/---"),

  { { N_("/View/_Info Window"), "<control><shift>I",
      view_info_window_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_INFO },
    NULL,
    GIMP_HELP_INFO_DIALOG, NULL },
  { { N_("/View/Na_vigation Window"), "<control><shift>N",
      view_navigation_window_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_NAVIGATION },
    NULL,
    GIMP_HELP_NAVIGATION_DIALOG, NULL },
  { { N_("/View/Display _Filters..."), NULL,
      view_display_filters_cmd_callback, 0 },
    NULL,
    GIMP_HELP_DISPLAY_FILTER_DIALOG, NULL },

  MENU_SEPARATOR ("/View/---"),

  { { N_("/View/Show _Selection"), "<control>T",
      view_toggle_selection_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    GIMP_HELP_VIEW_SHOW_SELECTION, NULL },
  { { N_("/View/Show _Layer Boundary"), NULL,
      view_toggle_layer_boundary_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    GIMP_HELP_VIEW_SHOW_LAYER_BOUNDARY, NULL },
  { { N_("/View/Show _Guides"), "<control><shift>T",
      view_toggle_guides_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    GIMP_HELP_VIEW_SHOW_GUIDES, NULL },
  { { N_("/View/Sn_ap to Guides"), NULL,
      view_snap_to_guides_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    GIMP_HELP_VIEW_SNAP_TO_GUIDES, NULL },
  { { N_("/View/S_how Grid"), NULL,
      view_toggle_grid_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    GIMP_HELP_VIEW_SHOW_GRID, NULL },
  { { N_("/View/Sna_p to Grid"), NULL,
      view_snap_to_grid_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    GIMP_HELP_VIEW_SNAP_TO_GRID, NULL },

  MENU_SEPARATOR ("/View/---"),

  { { N_("/View/Show _Menubar"), NULL,
      view_toggle_menubar_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    GIMP_HELP_VIEW_SHOW_MENUBAR, NULL },
  { { N_("/View/Show R_ulers"), "<control><shift>R",
      view_toggle_rulers_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    GIMP_HELP_VIEW_SHOW_RULERS, NULL },
  { { N_("/View/Show Scroll_bars"), NULL,
      view_toggle_scrollbars_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    GIMP_HELP_VIEW_SHOW_SCROLLBARS, NULL },
  { { N_("/View/Show S_tatusbar"), NULL,
      view_toggle_statusbar_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    GIMP_HELP_VIEW_SHOW_STATUSBAR, NULL },

  MENU_SEPARATOR ("/View/---"),

  { { N_("/View/Shrink _Wrap"), "<control>E",
      view_shrink_wrap_cmd_callback, 0 },
    NULL,
    GIMP_HELP_VIEW_SHRINK_WRAP, NULL },

  { { N_("/View/Fullscr_een"), "F11",
      view_fullscreen_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    GIMP_HELP_VIEW_FULLSCREEN, NULL },

  { { N_("/View/Move to Screen..."), NULL,
      view_change_screen_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_MOVE_TO_SCREEN },
    NULL,
    GIMP_HELP_VIEW_CHANGE_SCREEN, NULL },

  /*  <Image>/Image  */

  MENU_BRANCH (N_("/_Image")),

  /*  <Image>/Image/Mode  */

  MENU_BRANCH (N_("/Image/_Mode")),

  { { N_("/Image/Mode/_RGB"), NULL,
      image_convert_rgb_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_CONVERT_RGB },
    NULL,
    GIMP_HELP_IMAGE_CONVERT_RGB, NULL },
  { { N_("/Image/Mode/_Grayscale"), NULL,
      image_convert_grayscale_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_CONVERT_GRAYSCALE },
    NULL,
    GIMP_HELP_IMAGE_CONVERT_GRAYSCALE, NULL },
  { { N_("/Image/Mode/_Indexed..."), NULL,
      image_convert_indexed_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_CONVERT_INDEXED },
    NULL,
    GIMP_HELP_IMAGE_CONVERT_INDEXED, NULL },

  /*  <Image>/Image/Transform  */

  MENU_BRANCH (N_("/Image/_Transform")),

  { { N_("/Image/Transform/Flip _Horizontally"), NULL,
      image_flip_cmd_callback, GIMP_ORIENTATION_HORIZONTAL,
      "<StockItem>", GIMP_STOCK_FLIP_HORIZONTAL },
    NULL,
    GIMP_HELP_IMAGE_FLIP_HORIZONTAL, NULL },
  { { N_("/Image/Transform/Flip _Vertically"), NULL,
      image_flip_cmd_callback, GIMP_ORIENTATION_VERTICAL,
      "<StockItem>", GIMP_STOCK_FLIP_VERTICAL },
    NULL,
    GIMP_HELP_IMAGE_FLIP_VERTICAL, NULL },

  MENU_SEPARATOR ("/Image/Transform/---"),

  /*  please use the degree symbol in the translation  */
  { { N_("/Image/Transform/Rotate 90 degrees _CW"), NULL,
      image_rotate_cmd_callback, GIMP_ROTATE_90,
      "<StockItem>", GIMP_STOCK_ROTATE_90 },
    NULL,
    GIMP_HELP_IMAGE_ROTATE_90, NULL },
  { { N_("/Image/Transform/Rotate 90 degrees CC_W"), NULL,
      image_rotate_cmd_callback, GIMP_ROTATE_270,
      "<StockItem>", GIMP_STOCK_ROTATE_270 },
    NULL,
    GIMP_HELP_IMAGE_ROTATE_270, NULL },
  { { N_("/Image/Transform/Rotate _180 degrees"), NULL,
      image_rotate_cmd_callback, GIMP_ROTATE_180,
      "<StockItem>", GIMP_STOCK_ROTATE_180 },
    NULL,
    GIMP_HELP_IMAGE_ROTATE_180, NULL },

  MENU_SEPARATOR ("/Image/Transform/---"),

  MENU_SEPARATOR ("/Image/---"),

  { { N_("/Image/Can_vas Size..."), NULL,
      image_resize_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_RESIZE },
    NULL,
    GIMP_HELP_IMAGE_RESIZE, NULL },
  { { N_("/Image/_Scale Image..."), NULL,
      image_scale_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SCALE },
    NULL,
    GIMP_HELP_IMAGE_SCALE, NULL },
  { { N_("/Image/_Crop Image"), NULL,
      image_crop_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_CROP },
    NULL,
    GIMP_HELP_IMAGE_CROP, NULL },
  { { N_("/Image/_Duplicate"), "<control>D",
      image_duplicate_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    GIMP_HELP_IMAGE_DUPLICATE, NULL },

  MENU_SEPARATOR ("/Image/---"),

  { { N_("/Image/Merge Visible _Layers..."), "<control>M",
      image_merge_layers_cmd_callback, 0 },
    NULL,
    GIMP_HELP_IMAGE_MERGE_LAYERS, NULL },
  { { N_("/Image/_Flatten Image"), NULL,
      image_flatten_image_cmd_callback, 0 },
    NULL,
    GIMP_HELP_IMAGE_FLATTEN, NULL },

  MENU_SEPARATOR ("/Image/---"),

  { { N_("/Image/Configure G_rid..."), NULL,
      image_configure_grid_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_GRID },
    NULL,
    GIMP_HELP_IMAGE_GRID, NULL },

  /*  <Image>/Layer  */

  MENU_BRANCH (N_("/_Layer")),

  { { N_("/Layer/_New Layer..."), "",
      layers_new_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    GIMP_HELP_LAYER_NEW, NULL },
  { { N_("/Layer/Du_plicate Layer"), NULL,
      layers_duplicate_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    GIMP_HELP_LAYER_DUPLICATE, NULL },
  { { N_("/Layer/Anchor _Layer"), "<control>H",
      layers_anchor_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_ANCHOR },
    NULL,
    GIMP_HELP_LAYER_ANCHOR, NULL },
  { { N_("/Layer/Me_rge Down"), "<control><shift>M",
      layers_merge_down_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_MERGE_DOWN },
    NULL,
    GIMP_HELP_LAYER_MERGE_DOWN, NULL },
  { { N_("/Layer/_Delete Layer"), NULL,
      layers_delete_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    GIMP_HELP_LAYER_DELETE, NULL },
  { { N_("/Layer/Discard _Text Information"), NULL,
      layers_text_discard_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_TEXT },
    NULL,
    GIMP_HELP_LAYER_TEXT_DISCARD, NULL },

  MENU_SEPARATOR ("/Layer/---"),

  /*  <Image>/Layer/Stack  */

  MENU_BRANCH (N_("/Layer/Stac_k")),

  { { N_("/Layer/Stack/Select _Previous Layer"), "Prior",
      layers_select_previous_cmd_callback, 0 },
    NULL,
    GIMP_HELP_LAYER_PREVIOUS, NULL },
  { { N_("/Layer/Stack/Select _Next Layer"), "Next",
      layers_select_next_cmd_callback, 0 },
    NULL,
    GIMP_HELP_LAYER_NEXT, NULL },
  { { N_("/Layer/Stack/Select _Top Layer"), "Home",
      layers_select_top_cmd_callback, 0 },
    NULL,
    GIMP_HELP_LAYER_TOP, NULL },
  { { N_("/Layer/Stack/Select _Bottom Layer"), "End",
      layers_select_bottom_cmd_callback, 0 },
    NULL,
    GIMP_HELP_LAYER_BOTTOM, NULL },

  MENU_SEPARATOR ("/Layer/Stack/---"),

  { { N_("/Layer/Stack/_Raise Layer"), "<shift>Prior",
      layers_raise_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_UP },
    NULL,
    GIMP_HELP_LAYER_RAISE, NULL },
  { { N_("/Layer/Stack/_Lower Layer"), "<shift>Next",
      layers_lower_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_DOWN },
    NULL,
    GIMP_HELP_LAYER_LOWER, NULL },
  { { N_("/Layer/Stack/Layer to T_op"), "<shift>Home",
      layers_raise_to_top_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GOTO_TOP },
    NULL,
    GIMP_HELP_LAYER_RAISE_TO_TOP, NULL },
  { { N_("/Layer/Stack/Layer to Botto_m"), "<Shift>End",
      layers_lower_to_bottom_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GOTO_BOTTOM },
    NULL,
    GIMP_HELP_LAYER_LOWER_TO_BOTTOM, NULL },

  /*  <Image>/Layer/Colors  */

  MENU_BRANCH (N_("/Layer/_Colors")),

  { { N_("/Layer/Colors/Color _Balance..."), NULL,
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_COLOR_BALANCE },
    "gimp-color-balance-tool",
    GIMP_HELP_TOOL_COLOR_BALANCE, NULL },
  { { N_("/Layer/Colors/Hue-_Saturation..."), NULL,
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_HUE_SATURATION },
    "gimp-hue-saturation-tool",
    GIMP_HELP_TOOL_HUE_SATURATION, NULL },
  { { N_("/Layer/Colors/Colori_ze..."), NULL,
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_COLORIZE },
    "gimp-colorize-tool",
    GIMP_HELP_TOOL_COLORIZE, NULL },
  { { N_("/Layer/Colors/B_rightness-Contrast..."), NULL,
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_BRIGHTNESS_CONTRAST },
    "gimp-brightness-contrast-tool",
    GIMP_HELP_TOOL_BRIGHTNESS_CONTRAST, NULL },
  { { N_("/Layer/Colors/_Threshold..."), NULL,
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_THRESHOLD },
    "gimp-threshold-tool",
    GIMP_HELP_TOOL_THRESHOLD, NULL },
  { { N_("/Layer/Colors/_Levels..."), NULL,
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_LEVELS },
    "gimp-levels-tool",
    GIMP_HELP_TOOL_LEVELS, NULL },
  { { N_("/Layer/Colors/_Curves..."), NULL,
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_CURVES },
    "gimp-curves-tool",
    GIMP_HELP_TOOL_CURVES, NULL },
  { { N_("/Layer/Colors/_Posterize..."), NULL,
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_POSTERIZE },
    "gimp-posterize-tool",
    GIMP_HELP_TOOL_POSTERIZE, NULL },

  MENU_SEPARATOR ("/Layer/Colors/---"),

  { { N_("/Layer/Colors/_Desaturate"), NULL,
      drawable_desaturate_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_CONVERT_GRAYSCALE },
    NULL,
    GIMP_HELP_LAYER_DESATURATE, NULL },
  { { N_("/Layer/Colors/In_vert"), NULL,
      drawable_invert_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_INVERT },
    NULL,
    GIMP_HELP_LAYER_INVERT, NULL },

  /*  <Image>/Layer/Colors/Auto  */

  MENU_BRANCH (N_("/Layer/Colors/_Auto")),

  { { N_("/Layer/Colors/Auto/_Equalize"), NULL,
      drawable_equalize_cmd_callback, 0 },
    NULL,
    GIMP_HELP_LAYER_EQUALIZE, NULL },

  MENU_SEPARATOR ("/Layer/Colors/---"),

  { { N_("/Layer/Colors/_Histogram"), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_HISTOGRAM },
    "gimp-histogram-editor",
    GIMP_HELP_HISTOGRAM_DIALOG, NULL },

  /*  <Image>/Layer/Mask  */

  MENU_BRANCH (N_("/Layer/_Mask")),

  { { N_("/Layer/Mask/_Add Layer Mask..."), NULL,
      layers_mask_add_cmd_callback, 0 },
    NULL,
    GIMP_HELP_LAYER_MASK_ADD, NULL },
  { { N_("/Layer/Mask/A_pply Layer Mask"), NULL,
      layers_mask_apply_cmd_callback, 0 },
    NULL,
    GIMP_HELP_LAYER_MASK_APPLY, NULL },
  { { N_("/Layer/Mask/_Delete Layer Mask"), NULL,
      layers_mask_delete_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    GIMP_HELP_LAYER_MASK_DELETE, NULL },

  MENU_SEPARATOR ("/Layer/Mask/---"),

  { { N_("/Layer/Mask/_Mask to Selection"), NULL,
      layers_mask_to_selection_cmd_callback, GIMP_CHANNEL_OP_REPLACE,
      "<StockItem>", GIMP_STOCK_SELECTION_REPLACE },
    NULL,
    GIMP_HELP_LAYER_MASK_SELECTION_REPLACE, NULL },
  { { N_("/Layer/Mask/_Add to Selection"), NULL,
      layers_mask_to_selection_cmd_callback, GIMP_CHANNEL_OP_ADD,
      "<StockItem>", GIMP_STOCK_SELECTION_ADD },
    NULL,
    GIMP_HELP_LAYER_MASK_SELECTION_ADD, NULL },
  { { N_("/Layer/Mask/_Subtract from Selection"), NULL,
      layers_mask_to_selection_cmd_callback, GIMP_CHANNEL_OP_SUBTRACT,
      "<StockItem>", GIMP_STOCK_SELECTION_SUBTRACT },
    NULL,
    GIMP_HELP_LAYER_MASK_SELECTION_SUBTRACT, NULL },
  { { N_("/Layer/Mask/_Intersect with Selection"), NULL,
      layers_mask_to_selection_cmd_callback, GIMP_CHANNEL_OP_INTERSECT,
      "<StockItem>", GIMP_STOCK_SELECTION_INTERSECT },
    NULL,
    GIMP_HELP_LAYER_MASK_SELECTION_INTERSECT, NULL },

  /*  <Image>/Layer/Transparency  */

  MENU_BRANCH (N_("/Layer/Tr_ansparency")),

  { { N_("/Layer/Transparency/_Add Alpha Channel"), NULL,
      layers_alpha_add_cmd_callback, 0,
       "<StockItem>", GIMP_STOCK_TRANSPARENCY },
    NULL,
    GIMP_HELP_LAYER_ALPHA_ADD, NULL },

  MENU_SEPARATOR ("/Layer/Transparency/---"),

  { { N_("/Layer/Transparency/Al_pha to Selection"), NULL,
      layers_alpha_to_selection_cmd_callback, GIMP_CHANNEL_OP_REPLACE,
      "<StockItem>", GIMP_STOCK_SELECTION_REPLACE },
    NULL,
    GIMP_HELP_LAYER_ALPHA_SELECTION_REPLACE, NULL },
  { { N_("/Layer/Transparency/A_dd to Selection"), NULL,
      layers_alpha_to_selection_cmd_callback, GIMP_CHANNEL_OP_ADD,
      "<StockItem>", GIMP_STOCK_SELECTION_ADD },
    NULL,
    GIMP_HELP_LAYER_ALPHA_SELECTION_ADD, NULL },
  { { N_("/Layer/Transparency/_Subtract from Selection"), NULL,
      layers_alpha_to_selection_cmd_callback, GIMP_CHANNEL_OP_SUBTRACT,
      "<StockItem>", GIMP_STOCK_SELECTION_SUBTRACT },
    NULL,
    GIMP_HELP_LAYER_ALPHA_SELECTION_SUBTRACT, NULL },
  { { N_("/Layer/Transparency/_Intersect with Selection"), NULL,
      layers_alpha_to_selection_cmd_callback, GIMP_CHANNEL_OP_INTERSECT,
      "<StockItem>", GIMP_STOCK_SELECTION_INTERSECT },
    NULL,
    GIMP_HELP_LAYER_ALPHA_SELECTION_INTERSECT, NULL },

  MENU_SEPARATOR ("/Layer/Transparency/---"),

  /*  <Image>/Layer/Transform  */

  MENU_BRANCH (N_("/Layer/_Transform")),

  { { N_("/Layer/Transform/Flip _Horizontally"), NULL,
      drawable_flip_cmd_callback, GIMP_ORIENTATION_HORIZONTAL,
      "<StockItem>", GIMP_STOCK_FLIP_HORIZONTAL },
    NULL,
    GIMP_HELP_LAYER_FLIP_HORIZONTAL, NULL },
  { { N_("/Layer/Transform/Flip _Vertically"), NULL,
      drawable_flip_cmd_callback, GIMP_ORIENTATION_VERTICAL,
      "<StockItem>", GIMP_STOCK_FLIP_VERTICAL },
    NULL,
    GIMP_HELP_LAYER_FLIP_VERTICAL, NULL },

  MENU_SEPARATOR ("/Layer/Transform/---"),

  { { N_("/Layer/Transform/Rotate 90 degrees _CW"), NULL,
      drawable_rotate_cmd_callback, GIMP_ROTATE_90,
      "<StockItem>", GIMP_STOCK_ROTATE_90 },
    NULL,
    GIMP_HELP_LAYER_ROTATE_90, NULL },
  { { N_("/Layer/Transform/Rotate 90 degrees CC_W"), NULL,
      drawable_rotate_cmd_callback, GIMP_ROTATE_270,
      "<StockItem>", GIMP_STOCK_ROTATE_270 },
    NULL,
    GIMP_HELP_LAYER_ROTATE_270, NULL },
  { { N_("/Layer/Transform/Rotate _180 degrees"), NULL,
      drawable_rotate_cmd_callback, GIMP_ROTATE_180,
      "<StockItem>", GIMP_STOCK_ROTATE_180 },
    NULL,
    GIMP_HELP_LAYER_ROTATE_180, NULL },
  { { N_("/Layer/Transform/_Arbitrary Rotation..."), NULL,
      tools_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_ROTATE },
    "gimp-rotate-tool",
    GIMP_HELP_TOOL_ROTATE, NULL },

  MENU_SEPARATOR ("/Layer/Transform/---"),

  { { N_("/Layer/Transform/_Offset..."), "<control><shift>O",
      drawable_offset_cmd_callback, 0 },
    NULL,
    GIMP_HELP_LAYER_OFFSET, NULL },

  MENU_SEPARATOR ("/Layer/---"),

  { { N_("/Layer/Layer _Boundary Size..."), NULL,
      layers_resize_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_RESIZE },
    NULL,
    GIMP_HELP_LAYER_RESIZE, NULL },
  { { N_("/Layer/Layer to _Image Size"), NULL,
      layers_resize_to_image_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_LAYER_TO_IMAGESIZE },
    NULL,
    GIMP_HELP_LAYER_RESIZE_TO_IMAGE, NULL },
  { { N_("/Layer/_Scale Layer..."), NULL,
      layers_scale_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SCALE },
    NULL,
    GIMP_HELP_LAYER_SCALE, NULL },
  { { N_("/Layer/Cr_op Layer"), NULL,
      layers_crop_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_CROP },
    NULL,
    GIMP_HELP_LAYER_CROP, NULL },

  MENU_SEPARATOR ("/Layer/---"),

  /*  <Image>/Tools  */

  MENU_BRANCH (N_("/_Tools")),

  { { N_("/Tools/Tool_box"), NULL,
      dialogs_show_toolbox_cmd_callback, 0 },
    NULL,
    GIMP_HELP_TOOLBOX, NULL },
  { { N_("/Tools/_Default Colors"), "D",
      tools_default_colors_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DEFAULT_COLORS },
    NULL,
    GIMP_HELP_TOOLBOX_DEFAULT_COLORS, NULL },
  { { N_("/Tools/S_wap Colors"), "X",
      tools_swap_colors_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SWAP_COLORS },
    NULL,
    GIMP_HELP_TOOLBOX_SWAP_COLORS, NULL },

  MENU_SEPARATOR ("/Tools/---"),

  MENU_BRANCH (N_("/Tools/_Selection Tools")),
  MENU_BRANCH (N_("/Tools/_Paint Tools")),
  MENU_BRANCH (N_("/Tools/_Transform Tools")),
  MENU_BRANCH (N_("/Tools/_Color Tools")),

  /*  <Image>/Dialogs  */

  MENU_BRANCH (N_("/_Dialogs")),

  MENU_BRANCH (N_("/Dialogs/Create New Doc_k")),

  { { N_("/Dialogs/Create New Dock/_Layers, Channels & Paths"), NULL,
      dialogs_create_lc_cmd_callback, 0 },
    NULL,
    NULL, NULL },
  { { N_("/Dialogs/Create New Dock/_Brushes, Patterns & Gradients"), NULL,
      dialogs_create_data_cmd_callback, 0 },
    NULL,
    NULL, NULL },
  { { N_("/Dialogs/Create New Dock/_Misc. Stuff"), NULL,
      dialogs_create_stuff_cmd_callback, 0 },
    NULL,
    NULL, NULL },

  { { N_("/Dialogs/Tool _Options"), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_OPTIONS },
    "gimp-tool-options",
    GIMP_HELP_TOOL_OPTIONS_DIALOG, NULL },
  { { N_("/Dialogs/_Device Status"), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DEVICE_STATUS },
    "gimp-device-status",
    GIMP_HELP_DEVICE_STATUS_DIALOG, NULL },

  MENU_SEPARATOR ("/Dialogs/---"),

  { { N_("/Dialogs/_Layers"), "<control>L",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_LAYERS },
    "gimp-layer-list",
    GIMP_HELP_LAYER_DIALOG, NULL },
  { { N_("/Dialogs/_Channels"), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_CHANNELS },
    "gimp-channel-list",
    GIMP_HELP_CHANNEL_DIALOG, NULL },
  { { N_("/Dialogs/_Paths"), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_PATHS },
    "gimp-vectors-list",
    GIMP_HELP_PATH_DIALOG, NULL },
  { { N_("/Dialogs/Inde_xed Palette"), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SELECT_COLOR },
    "gimp-indexed-palette",
    GIMP_HELP_INDEXED_PALETTE_DIALOG, NULL },
  { { N_("/Dialogs/Histogra_m"), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_HISTOGRAM },
    "gimp-histogram-editor",
    GIMP_HELP_HISTOGRAM_DIALOG, NULL },
  { { N_("/Dialogs/_Selection Editor"), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_RECT_SELECT },
    "gimp-selection-editor",
    GIMP_HELP_SELECTION_DIALOG, NULL },
  { { N_("/Dialogs/Na_vigation"), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_NAVIGATION },
    "gimp-navigation-view",
    GIMP_HELP_NAVIGATION_DIALOG, NULL },
  { { N_("/Dialogs/_Undo History"), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_UNDO_HISTORY },
    "gimp-undo-history",
    GIMP_HELP_UNDO_DIALOG, NULL },

  MENU_SEPARATOR ("/Dialogs/---"),

  { { N_("/Dialogs/Colo_rs"), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DEFAULT_COLORS },
    "gimp-color-editor",
    GIMP_HELP_COLOR_DIALOG, NULL },
  { { N_("/Dialogs/Brus_hes"), "<control><shift>B",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_PAINTBRUSH },
    "gimp-brush-grid|gimp-brush-list",
    GIMP_HELP_BRUSH_DIALOG, NULL },
  { { N_("/Dialogs/P_atterns"), "<control><shift>P",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_BUCKET_FILL },
    "gimp-pattern-grid|gimp-pattern-list",
    GIMP_HELP_PATTERN_DIALOG, NULL },
  { { N_("/Dialogs/_Gradients"), "<control>G",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_BLEND },
    "gimp-gradient-list|gimp-gradient-grid",
    GIMP_HELP_GRADIENT_DIALOG, NULL },
  { { N_("/Dialogs/Pal_ettes"), "<control>P",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SELECT_COLOR },
    "gimp-palette-list|gimp-palette-grid",
    GIMP_HELP_PALETTE_DIALOG, NULL },
  { { N_("/Dialogs/_Fonts"), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SELECT_FONT },
    "gimp-font-list|gimp-font-grid",
    GIMP_HELP_FONT_DIALOG, NULL },
  { { N_("/Dialogs/_Buffers"), "foo",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_PASTE },
    "gimp-buffer-list|gimp-buffer-grid",
    GIMP_HELP_BUFFER_DIALOG, NULL },

  MENU_SEPARATOR ("/Dialogs/---"),

  { { N_("/Dialogs/_Images"), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_IMAGES },
    "gimp-image-list|gimp-image-grid",
    GIMP_HELP_IMAGE_DIALOG, NULL },
  { { N_("/Dialogs/Document Histor_y"), "",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    "gimp-document-list|gimp-document-grid",
    GIMP_HELP_DOCUMENT_DIALOG, NULL },
  { { N_("/Dialogs/_Templates"), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TEMPLATE },
    "gimp-template-list|gimp-template-grid",
    GIMP_HELP_TEMPLATE_DIALOG, NULL },
  { { N_("/Dialogs/T_ools"), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOLS },
    "gimp-tool-list|gimp-tool-grid",
    GIMP_HELP_TOOLS_DIALOG, NULL },
  { { N_("/Dialogs/Error Co_nsole"), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_WARNING },
    "gimp-error-console",
    GIMP_HELP_ERRORS_DIALOG, NULL },


  /*  <Image>/Filters  */

  MENU_SEPARATOR ("/filters-separator"),
  MENU_BRANCH (N_("/Filte_rs")),

  { { N_("/Filters/Repeat Last"), "<control>F",
      plug_in_repeat_cmd_callback, (guint) FALSE,
      "<StockItem>", GTK_STOCK_EXECUTE },
    NULL,
    GIMP_HELP_FILTER_REPEAT, NULL },
  { { N_("/Filters/Re-Show Last"), "<control><shift>F",
      plug_in_repeat_cmd_callback, (guint) TRUE,
      "<StockItem>", GIMP_STOCK_RESHOW_FILTER },
    NULL,
    GIMP_HELP_FILTER_RESHOW, NULL },

  MENU_SEPARATOR ("/Filters/---"),

  MENU_BRANCH (N_("/Filters/_Blur")),
  MENU_BRANCH (N_("/Filters/_Colors")),
  MENU_BRANCH (N_("/Filters/Colors/Ma_p")),
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
  MENU_BRANCH (N_("/Filters/Render/_Clouds")),
  MENU_BRANCH (N_("/Filters/Render/_Nature")),
  MENU_BRANCH (N_("/Filters/Render/_Pattern")),
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
      gimp_item_factory_set_visible (GTK_ITEM_FACTORY (factory),
                                     "/filters-separator", FALSE);
    }

  menus_last_opened_add (factory);

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

            stock_id   = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool_info));
            identifier = gimp_object_get_name (GIMP_OBJECT (tool_info));

            entry.entry.path            = tool_info->menu_path;
            entry.entry.accelerator     = tool_info->menu_accel;
            entry.entry.callback        = tools_select_cmd_callback;
            entry.entry.callback_action = 0;
            entry.entry.item_type       = "<StockItem>";
            entry.entry.extra_data      = stock_id;
            entry.quark_string          = identifier;
            entry.help_id               = tool_info->help_id;
            entry.description           = NULL;

            gimp_item_factory_create_item (factory,
                                           &entry,
                                           NULL,
                                           factory->gimp, 2, FALSE);
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
  Gimp               *gimp          = NULL;
  GimpDisplay        *gdisp         = NULL;
  GimpDisplayShell   *shell         = NULL;
  GimpDisplayOptions *options       = NULL;
  GimpImage          *gimage        = NULL;
  GimpDrawable       *drawable      = NULL;
  GimpLayer          *layer         = NULL;
  GimpVectors        *vectors       = NULL;
  GimpImageType       drawable_type = -1;
  gboolean            is_rgb        = FALSE;
  gboolean            is_gray       = FALSE;
  gboolean            is_indexed    = FALSE;
  gboolean            fs            = FALSE;
  gboolean            aux           = FALSE;
  gboolean            lm            = FALSE;
  gboolean            lp            = FALSE;
  gboolean            sel           = FALSE;
  gboolean            alpha         = FALSE;
  gboolean            text_layer    = FALSE;
  gint                lind          = -1;
  gint                lnum          = -1;
  gboolean            fullscreen    = FALSE;
  gint                n_screens     = 1;

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
      sel = ! gimp_channel_is_empty (gimp_image_get_mask (gimage));

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

              text_layer = (GIMP_IS_TEXT_LAYER (layer) &&
                            GIMP_TEXT_LAYER (layer)->text);
	    }

	  lnum = gimp_container_num_children (gimage->layers);
	}

      vectors = gimp_image_get_active_vectors (gimage);

      fullscreen = gimp_display_shell_get_fullscreen (shell);

      options = fullscreen ? shell->fullscreen_options : shell->options;

      n_screens =
        gdk_display_get_n_screens (gtk_widget_get_display (GTK_WIDGET (shell)));
    }

#define SET_ACTIVE(menu,condition) \
        gimp_item_factory_set_active (item_factory, menu, (condition) != 0)
#define SET_VISIBLE(menu,condition) \
        gimp_item_factory_set_visible (item_factory, menu, (condition) != 0)
#define SET_LABEL(menu,label) \
        gimp_item_factory_set_label (item_factory, menu, (label))
#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (item_factory, menu, (condition) != 0)

  /*  File  */

  SET_SENSITIVE ("/File/Save",                gdisp && drawable);
  SET_SENSITIVE ("/File/Save as...",          gdisp && drawable);
  SET_SENSITIVE ("/File/Save a Copy...",      gdisp && drawable);
  SET_SENSITIVE ("/File/Save as Template...", gdisp);
  SET_SENSITIVE ("/File/Revert",              gdisp && GIMP_OBJECT (gimage)->name);
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

    SET_LABEL ("/Edit/Undo", undo_name ? undo_name : _("_Undo"));
    SET_LABEL ("/Edit/Redo", redo_name ? redo_name : _("_Redo"));

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
  SET_SENSITIVE ("/Edit/Fill with Pattern",     lp);
  SET_SENSITIVE ("/Edit/Stroke Selection...",   lp && sel);
  SET_SENSITIVE ("/Edit/Stroke Path...",        lp && vectors);

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
  SET_SENSITIVE ("/Select/Save to Channel",  sel && !fs);
  SET_SENSITIVE ("/Select/To Path",          sel && !fs);

  /*  View  */

  SET_SENSITIVE ("/View/New View", gdisp);

  SET_SENSITIVE ("/View/Dot for Dot", gdisp);
  SET_ACTIVE    ("/View/Dot for Dot", gdisp && shell->dot_for_dot);

  SET_SENSITIVE ("/View/Zoom/Zoom Out",           gdisp);
  SET_SENSITIVE ("/View/Zoom/Zoom In",            gdisp);
  SET_SENSITIVE ("/View/Zoom/Zoom to Fit Window", gdisp);

  SET_SENSITIVE ("/View/Zoom/16:1",     gdisp);
  SET_SENSITIVE ("/View/Zoom/8:1",      gdisp);
  SET_SENSITIVE ("/View/Zoom/4:1",      gdisp);
  SET_SENSITIVE ("/View/Zoom/2:1",      gdisp);
  SET_SENSITIVE ("/View/Zoom/1:1",      gdisp);
  SET_SENSITIVE ("/View/Zoom/1:2",      gdisp);
  SET_SENSITIVE ("/View/Zoom/1:4",      gdisp);
  SET_SENSITIVE ("/View/Zoom/1:8",      gdisp);
  SET_SENSITIVE ("/View/Zoom/1:16",     gdisp);
  SET_SENSITIVE ("/View/Zoom/Other...", gdisp);

  if (gdisp)
    image_menu_set_zoom (item_factory, shell);

  SET_SENSITIVE ("/View/Info Window",         gdisp);
  SET_SENSITIVE ("/View/Navigation Window",   gdisp);
  SET_SENSITIVE ("/View/Display Filters...",  gdisp);

  SET_SENSITIVE ("/View/Show Selection",      gdisp);
  SET_ACTIVE    ("/View/Show Selection",      gdisp && options->show_selection);
  SET_SENSITIVE ("/View/Show Layer Boundary", gdisp);
  SET_ACTIVE    ("/View/Show Layer Boundary", gdisp && options->show_layer_boundary);
  SET_ACTIVE    ("/View/Show Guides",         gdisp && options->show_guides);
  SET_ACTIVE    ("/View/Snap to Guides",      gdisp && shell->snap_to_guides);
  SET_ACTIVE    ("/View/Show Grid",           gdisp && options->show_grid);
  SET_ACTIVE    ("/View/Snap to Grid",        gdisp && shell->snap_to_grid);

  SET_SENSITIVE ("/View/Show Menubar",    gdisp);
  SET_ACTIVE    ("/View/Show Menubar",    gdisp && options->show_menubar);
  SET_SENSITIVE ("/View/Show Rulers",     gdisp);
  SET_ACTIVE    ("/View/Show Rulers",     gdisp && options->show_rulers);
  SET_SENSITIVE ("/View/Show Scrollbars", gdisp);
  SET_ACTIVE    ("/View/Show Scrollbars", gdisp && options->show_scrollbars);
  SET_SENSITIVE ("/View/Show Statusbar",  gdisp);
  SET_ACTIVE    ("/View/Show Statusbar",  gdisp && options->show_statusbar);

  SET_SENSITIVE ("/View/Shrink Wrap",       gdisp);
  SET_SENSITIVE ("/View/Fullscreen",        gdisp);
  SET_ACTIVE    ("/View/Fullscreen",        gdisp && fullscreen);
  SET_VISIBLE   ("/View/Move to Screen...", gdisp && n_screens > 1);

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
  SET_SENSITIVE ("/Image/Configure Grid...",       gdisp);

  /*  Layer  */

  SET_SENSITIVE ("/Layer/New Layer...",      gdisp);
  SET_SENSITIVE ("/Layer/Duplicate Layer",   lp && !fs && !aux);
  SET_SENSITIVE ("/Layer/Anchor Layer",      lp &&  fs && !aux);
  SET_SENSITIVE ("/Layer/Merge Down",        lp && !fs && !aux && lind < (lnum - 1));
  SET_SENSITIVE ("/Layer/Delete Layer",      lp && !aux);
  SET_VISIBLE   ("/Layer/Discard Text Information", text_layer && !aux);

  SET_SENSITIVE ("/Layer/Layer Boundary Size...", lp && !aux);
  SET_SENSITIVE ("/Layer/Layer to Image Size",    lp && !aux);
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
  SET_SENSITIVE ("/Layer/Colors/Histogram",              lp);

  SET_SENSITIVE ("/Layer/Mask/Add Layer Mask...",        lp && !fs && !aux && !lm && alpha);
  SET_SENSITIVE ("/Layer/Mask/Apply Layer Mask",         lm && !fs && !aux);
  SET_SENSITIVE ("/Layer/Mask/Delete Layer Mask",        lm && !fs && !aux);
  SET_SENSITIVE ("/Layer/Mask/Mask to Selection",        lm && !fs && !aux);
  SET_SENSITIVE ("/Layer/Mask/Add to Selection",         lm && !fs && !aux);
  SET_SENSITIVE ("/Layer/Mask/Subtract from Selection",  lm && !fs && !aux);
  SET_SENSITIVE ("/Layer/Mask/Intersect with Selection", lm && !fs && !aux);

  SET_SENSITIVE ("/Layer/Transparency/Add Alpha Channel",        lp && !aux && !fs && !lm && !alpha);
  SET_SENSITIVE ("/Layer/Transparency/Alpha to Selection",       lp && !aux);
  SET_SENSITIVE ("/Layer/Transparency/Add to Selection",         lp && !aux);
  SET_SENSITIVE ("/Layer/Transparency/Subtract from Selection",  lp && !aux);
  SET_SENSITIVE ("/Layer/Transparency/Intersect with Selection", lp && !aux);

  SET_SENSITIVE ("/Layer/Transform/Flip Horizontally",     lp);
  SET_SENSITIVE ("/Layer/Transform/Flip Vertically",       lp);
  SET_SENSITIVE ("/Layer/Transform/Rotate 90 degrees CW",  lp);
  SET_SENSITIVE ("/Layer/Transform/Rotate 90 degrees CCW", lp);
  SET_SENSITIVE ("/Image/Transform/Rotate 180 degrees",    lp);
  SET_SENSITIVE ("/Layer/Transform/Offset...",             lp);

#undef SET_ACTIVE
#undef SET_VISIBLE
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
