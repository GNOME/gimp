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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpitemfactory.h"

#include "brushes-commands.h"
#include "buffers-commands.h"
#include "channels-commands.h"
#include "colormap-editor-commands.h"
#include "data-commands.h"
#include "dialogs-commands.h"
#include "documents-commands.h"
#include "drawable-commands.h"
#include "edit-commands.h"
#include "file-commands.h"
#include "gradient-editor-commands.h"
#include "gradients-commands.h"
#include "help-commands.h"
#include "image-commands.h"
#include "layers-commands.h"
#include "menus.h"
#include "palette-editor-commands.h"
#include "palettes-commands.h"
#include "paths-dialog.h"
#include "patterns-commands.h"
#include "plug-in-commands.h"
#include "qmask-commands.h"
#include "select-commands.h"
#include "test-commands.h"
#include "tools-commands.h"
#include "vectors-commands.h"
#include "view-commands.h"

#include "gimprc.h"

#include "libgimp/gimpintl.h"


#define ENABLE_DEBUG_ENTRIES 1


/*  local function prototypes  */

static void    menus_last_opened_add        (GimpItemFactory *item_factory,
                                             Gimp            *gimp);
static void    menus_last_opened_update     (GimpContainer   *container,
                                             GimpImagefile   *unused,
                                             GimpItemFactory *item_factory);
static void    menus_last_opened_reorder    (GimpContainer   *container,
                                             GimpImagefile   *unused1,
                                             gint             unused2,
                                             GimpItemFactory *item_factory);
static void    menus_color_changed          (GimpContext     *context,
                                             const GimpRGB   *unused,
                                             GimpItemFactory *item_factory);
static void    menus_filters_subdirs_to_top (GtkMenu         *menu);
#ifdef ENABLE_DEBUG_ENTRIES
static void    menus_debug_recurse_menu       (GtkWidget       *menu,
                                               gint             depth,
                                               gchar           *path);
static void    menus_debug_cmd_callback       (GtkWidget       *widget,
                                               gpointer         data,
                                               guint            action);
static void    menus_mem_profile_cmd_callback (GtkWidget       *widget,
                                               gpointer         data,
                                               guint            action);
#endif  /*  ENABLE_DEBUG_ENTRIES  */


#define SEPARATOR(path) \
        { { (path), NULL, NULL, 0, "<Separator>" }, NULL, NULL, NULL }
#define BRANCH(path) \
        { { (path), NULL, NULL, 0, "<Branch>" }, NULL, NULL, NULL }


/*****  <Toolbox>  *****/

static GimpItemFactoryEntry toolbox_entries[] =
{
  /*  <Toolbox>/File  */

  BRANCH (N_("/_File")),

  { { N_("/File/New..."), "<control>N",
      file_new_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    "file/dialogs/file_new.html", NULL },
  { { N_("/File/Open..."), "<control>O",
      file_open_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    NULL,
    "file/dialogs/file_open.html", NULL },

  /*  <Toolbox>/File/Open Recent  */

  { { N_("/File/Open Recent/(None)"), NULL, NULL, 0 },
    NULL, NULL, NULL },

  /*  <Toolbox>/File/Acquire  */

  BRANCH (N_("/File/Acquire")),

  SEPARATOR ("/File/---"),

  { { N_("/File/Preferences..."), NULL,
      dialogs_create_toplevel_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_PREFERENCES },
    "gimp-preferences-dialog",
    "file/dialogs/preferences/preferences.html", NULL },

  /*  <Toolbox>/File/Dialogs  */

  { { N_("/File/Dialogs/Layers, Channels & Paths..."), "<control>L",
      dialogs_create_lc_cmd_callback, 0 },
    NULL,
    "file/dialogs/layers_and_channels.html", NULL },
  { { N_("/File/Dialogs/Brushes, Patterns & Stuff..."), NULL,
      dialogs_create_stuff_cmd_callback, 0 },
    NULL,
    NULL, NULL },
  { { N_("/File/Dialogs/Tool Options..."), "<control><shift>T",
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-tool-options",
    "file/dialogs/tool_options.html", NULL },
  { { N_("/File/Dialogs/Device Status..."), NULL,
      dialogs_create_toplevel_cmd_callback, 0 },
    "gimp-device-status-dialog",
    "file/dialogs/device_status.html", NULL },

  SEPARATOR ("/File/Dialogs/---"),

  { { N_("/File/Dialogs/Brushes..."), "<control><shift>B",
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-brush-grid",
    "file/dialogs/brush_selection.html", NULL },
  { { N_("/File/Dialogs/Patterns..."), "<control><shift>P",
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-pattern-grid",
    "file/dialogs/pattern_selection.html", NULL },
  { { N_("/File/Dialogs/Gradients..."), "<control>G",
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-gradient-list",
    "file/dialogs/gradient_selection.html", NULL },
  { { N_("/File/Dialogs/Palettes..."), "<control>P",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SELECT_COLOR },
    "gimp-palette-list",
    "file/dialogs/palette_selection.html", NULL },
  { { N_("/File/Dialogs/Indexed Palette..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SELECT_COLOR },
    "gimp-indexed-palette",
    "file/dialogs/indexed_palette.html", NULL },
  { { N_("/File/Dialogs/Buffers..."), NULL,
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-buffer-list",
    NULL, NULL },
  { { N_("/File/Dialogs/Images..."), NULL,
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-image-list",
    NULL, NULL },

  SEPARATOR ("/File/Dialogs/---"),

  { { N_("/File/Dialogs/Document History..."), NULL,
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-document-history",
    "file/dialogs/document_index.html", NULL },
  { { N_("/File/Dialogs/Error Console..."), NULL,
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-error-console",
    "file/dialogs/error_console.html", NULL },

#ifdef ENABLE_DEBUG_ENTRIES
  { { "/File/Debug/Test Dialogs/Multi List...", NULL,
      test_multi_container_list_view_cmd_callback, 0 },
    NULL, NULL, NULL },
  { { "/File/Debug/Test Dialogs/Multi Grid...", NULL,
      test_multi_container_grid_view_cmd_callback, 0 },
    NULL, NULL, NULL },

  { { "/File/Debug/Mem Profile", NULL,
      menus_mem_profile_cmd_callback, 0 },
    NULL, NULL, NULL },
  { { "/File/Debug/Dump Items", NULL,
      menus_debug_cmd_callback, 0 },
    NULL, NULL, NULL },
#endif

  SEPARATOR ("/File/---"),

  { { N_("/File/Quit"), "<control>Q",
      file_quit_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_QUIT },
    NULL,
    "file/quit.html", NULL },

  /*  <Toolbox>/Xtns  */

  BRANCH (N_("/_Xtns")),

  { { N_("/Xtns/Module Browser..."), NULL,
      dialogs_create_toplevel_cmd_callback, 0 },
    "gimp-module-browser-dialog",
    "dialogs/module_browser.html", NULL },

  SEPARATOR ("/Xtns/---"),

  /*  <Toolbox>/Help  */

  BRANCH (N_("/_Help")),

  { { N_("/Help/Help..."), "F1",
      help_help_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_HELP },
    NULL,
    "help/dialogs/help.html", NULL },
  { { N_("/Help/Context Help..."), "<shift>F1",
      help_context_help_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_HELP },
    NULL,
    "help/context_help.html", NULL },
  { { N_("/Help/Tip of the Day..."), NULL,
      dialogs_create_toplevel_cmd_callback, 0 },
    "gimp-tips-dialog",
    "help/dialogs/tip_of_the_day.html", NULL },
  { { N_("/Help/About..."), NULL,
      dialogs_create_toplevel_cmd_callback, 0 },
    "gimp-about-dialog",
    "help/dialogs/about.html", NULL }
};


/*****  <Image>  *****/

static GimpItemFactoryEntry image_entries[] =
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
      file_open_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    NULL,
    "file/dialogs/file_open.html", NULL },

  /*  <Image>/File/Open Recent  */

  { { N_("/File/Open Recent/(None)"), NULL, NULL, 0 },
    NULL, NULL, NULL },

  SEPARATOR ("/File/---"),

  { { N_("/File/Save"), "<control>S",
      file_save_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SAVE },
    NULL,
    "file/dialogs/file_save.html", NULL },
  { { N_("/File/Save as..."), NULL,
      file_save_as_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SAVE_AS },
    NULL,
    "file/dialogs/file_save.html", NULL },
  { { N_("/File/Save a Copy as..."), NULL,
      file_save_a_copy_as_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SAVE_AS },
    NULL,
    "file/dialogs/file_save.html", NULL },
  { { N_("/File/Revert..."), NULL,
      file_revert_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_REVERT_TO_SAVED },
    NULL,
    "file/revert.html", NULL },

  SEPARATOR ("/File/---"),

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

  SEPARATOR ("/File/---moved"),

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

  SEPARATOR ("/Edit/---"),

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
      "<StockItem>", GTK_STOCK_PASTE },
    NULL,
    "edit/paste_into.html", NULL },
  { { N_("/Edit/Paste as New"), NULL,
      edit_paste_as_new_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_PASTE },
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

  SEPARATOR ("/Edit/---"),

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

  SEPARATOR ("/Edit/---"),

  /*  <Image>/Select  */
  
  { { N_("/Select/Invert"), "<control>I",
      select_invert_cmd_callback, 0 },
    NULL,
    "select/invert.html", NULL },
  { { N_("/Select/All"), "<control>A",
      select_all_cmd_callback, 0 },
    NULL,
    "select/all.html", NULL },
  { { N_("/Select/None"), "<control><shift>A",
      select_none_cmd_callback, 0 },
    NULL,
    "select/none.html", NULL },
  { { N_("/Select/Float"), "<control><shift>L",
      select_float_cmd_callback, 0 },
    NULL,
    "select/float.html", NULL },

  SEPARATOR ("/Select/---"),

  { { N_("/Select/Feather..."), "<control><shift>F",
      select_feather_cmd_callback, 0 },
    NULL,
    "select/dialogs/feather_selection.html", NULL },
  { { N_("/Select/Sharpen"), "<control><shift>H",
      select_sharpen_cmd_callback, 0 },
    NULL,
    "select/sharpen.html", NULL },
  { { N_("/Select/Shrink..."), NULL,
      select_shrink_cmd_callback, 0 },
    NULL,
    "select/dialogs/shrink_selection.html", NULL },
  { { N_("/Select/Grow..."), NULL,
      select_grow_cmd_callback, 0 },
    NULL,
    "select/dialogs/grow_selection.html", NULL },
  { { N_("/Select/Border..."), "<control><shift>B",
      select_border_cmd_callback, 0 },
    NULL,
    "select/dialogs/border_selection.html", NULL },

  SEPARATOR ("/Select/---"),

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

  SEPARATOR ("/View/---"),

  { { N_("/View/Info Window..."), "<control><shift>I",
      view_info_window_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_HELP },
    NULL,
    "view/dialogs/info_window.html", NULL },
  { { N_("/View/Navigation Window..."), "<control><shift>N",
      view_navigation_window_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_MOVE },
    NULL,
    "view/dialogs/navigation_window.html", NULL },
  { { N_("/View/Display Filters..."), NULL,
      view_display_filters_cmd_callback, 0 },
    NULL,
    "dialogs/display_filters/display_filters.html", NULL },

  SEPARATOR ("/View/---"),

  { { N_("/View/Toggle Selection"), "<control>T",
      view_toggle_selection_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/toggle_selection.html", NULL },
  { { N_("/View/Toggle Layer Boundary"), NULL,
      view_toggle_layer_boundary_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/toggle_selection.html", NULL },
  { { N_("/View/Toggle Guides"), "<control><shift>T",
      view_toggle_guides_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/toggle_guides.html", NULL },
  { { N_("/View/Snap to Guides"), NULL,
      view_snap_to_guides_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/snap_to_guides.html", NULL },

  SEPARATOR ("/View/---"),

  { { N_("/View/Toggle Rulers"), "<control><shift>R",
      view_toggle_rulers_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/toggle_rulers.html", NULL },
  { { N_("/View/Toggle Statusbar"), "<control><shift>S",
      view_toggle_statusbar_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    "view/toggle_statusbar.html", NULL },

  SEPARATOR ("/View/---"),

  { { N_("/View/New View"), NULL,
      view_new_view_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    "view/new_view.html", NULL },
  { { N_("/View/Shrink Wrap"), "<control>E",
      view_shrink_wrap_cmd_callback, 0 },
    NULL,
    "view/shrink_wrap.html", NULL },

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

  SEPARATOR ("/Image/Mode/---"),

  /*  <Image>/Image/Transform  */

  BRANCH (N_("/Image/Transform")),

  SEPARATOR ("/Image/Transform/---"),

  SEPARATOR ("/Image/---"),

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

  SEPARATOR ("/Image/---"),

  { { N_("/Image/Merge Visible Layers..."), "<control>M",
      image_merge_layers_cmd_callback, 0 },
    NULL,
    "layers/dialogs/merge_visible_layers.html", NULL },
  { { N_("/Image/Flatten Image"), NULL,
      image_flatten_image_cmd_callback, 0 },
    NULL,
    "layers/flatten_image.html", NULL },

  SEPARATOR ("/Image/---"),

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

  SEPARATOR ("/Layer/Stack/---"),

  { { N_("/Layer/New Layer..."), NULL,
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

  SEPARATOR ("/Layer/---"),

  { { N_("/Layer/Layer Boundary Size..."), NULL,
      layers_resize_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_RESIZE },
    NULL,
    "layers/dialogs/layer_boundary_size.html", NULL },
  { { N_("/Layer/Layer to Imagesize"), NULL,
      layers_resize_to_image_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_RESIZE },
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

  SEPARATOR ("/Layer/Transform/---"),

  { { N_("/Layer/Transform/Offset..."), "<control><shift>O",
      drawable_offset_cmd_callback, 0 },
    NULL,
    "layers/dialogs/offset.html", NULL },

  SEPARATOR ("/Layer/---"),

  /*  <Image>/Layer/Colors  */

  SEPARATOR ("/Layer/Colors/---"),

  { { N_("/Layer/Colors/Desaturate"), NULL,
      drawable_desaturate_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_CONVERT_GRAYSCALE },
    NULL,
    "layers/colors/desaturate.html", NULL },
  { { N_("/Layer/Colors/Invert"), NULL,
      drawable_invert_cmd_callback, 0 },
    NULL,
    "layers/colors/invert.html", NULL },

  SEPARATOR ("/Layer/Colors/---"),

  /*  <Image>/Layer/Colors/Auto  */

  { { N_("/Layer/Colors/Auto/Equalize"), NULL,
      drawable_equalize_cmd_callback, 0 },
    NULL,
    "layers/colors/auto/equalize.html", NULL },

  SEPARATOR ("/Layer/Colors/---"),

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

  /*  <Image>/Layer/Alpha  */

  { { N_("/Layer/Alpha/Add Alpha Channel"), NULL,
      layers_add_alpha_channel_cmd_callback, 0 },
    NULL,
    "layers/add_alpha_channel.html", NULL },
  { { N_("/Layer/Alpha/Alpha to Selection"), NULL,
      layers_alpha_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_REPLACE },
    NULL,
    "layers/alpha_to_selection.html", NULL },

  SEPARATOR ("/Layer/Alpha/---"),

  SEPARATOR ("/Layer/---"),

  /*  <Image>/Tools  */

  { { N_("/Tools/Toolbox"), NULL,
      dialogs_show_toolbox_cmd_callback, 0 },
    NULL,
    "toolbox/toolbox.html", NULL },
  { { N_("/Tools/Default Colors"), "D",
      tools_default_colors_cmd_callback, 0 },
    NULL,
    "toolbox/toolbox.html#default_colors", NULL },
  { { N_("/Tools/Swap Colors"), "X",
      tools_swap_colors_cmd_callback, 0 },
    NULL,
    "toolbox/toolbox.html#swap_colors", NULL },
  { { N_("/Tools/Swap Contexts"), "<shift>X",
      tools_swap_contexts_cmd_callback, 0 },
    NULL,
    "toolbox/toolbox.html#swap_colors", NULL },

  SEPARATOR ("/Tools/---"),

  BRANCH (N_("/Tools/Selection Tools")),
  BRANCH (N_("/Tools/Paint Tools")),
  BRANCH (N_("/Tools/Transform Tools")),

  /*  <Image>/Dialogs  */

  { { N_("/Dialogs/Layers, Channels & Paths..."), "<control>L",
      dialogs_create_lc_cmd_callback, 0 },
    NULL,
    "dialogs/layers_and_channels.html", NULL },
  { { N_("/Dialogs/Brushes, Patterns & Stuff..."), NULL,
      dialogs_create_stuff_cmd_callback, 0 },
    NULL,
    NULL, NULL },
  { { N_("/Dialogs/Tool Options..."), NULL,
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-tool-options",
    "dialogs/tool_options.html", NULL },
  { { N_("/Dialogs/Device Status..."), NULL,
      dialogs_create_toplevel_cmd_callback, 0 },
    "gimp-device-status-dialog",
    "dialogs/device_status.html", NULL },

  SEPARATOR ("/Dialogs/---"),

  { { N_("/Dialogs/Brushes..."), "<control><shift>B",
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-brush-grid",
    "dialogs/brush_selection.html", NULL },
  { { N_("/Dialogs/Patterns..."), "<control><shift>P",
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-pattern-grid",
    "dialogs/pattern_selection.html", NULL },
  { { N_("/Dialogs/Gradients..."), "<control>G",
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-gradient-list",
    "dialogs/gradient_selection.html", NULL },
  { { N_("/Dialogs/Palettes..."), "<control>P",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SELECT_COLOR },
    "gimp-palette-list",
    "dialogs/palette_selection.html", NULL },
  { { N_("/Dialogs/Indexed Palette..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SELECT_COLOR },
    "gimp-indexed-palette",
    "dialogs/indexed_palette.html", NULL },
  { { N_("/Dialogs/Buffers..."), NULL,
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-buffer-list",
    NULL, NULL },
  { { N_("/Dialogs/Images..."), NULL,
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-image-list",
    NULL, NULL },

  SEPARATOR ("/Dialogs/---"),

  { { N_("/Dialogs/Document History..."), NULL,
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-document-history",
    "dialogs/document_index.html", NULL },
  { { N_("/Dialogs/Error Console..."), NULL,
      dialogs_create_dockable_cmd_callback, 0 },
    "gimp-error-console",
    "dialogs/error_console.html", NULL },

  SEPARATOR ("/---"),

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

  SEPARATOR ("/Filters/---"),

  BRANCH (N_("/Filters/Blur")),
  BRANCH (N_("/Filters/Colors")),
  BRANCH (N_("/Filters/Noise")),
  BRANCH (N_("/Filters/Edge-Detect")),
  BRANCH (N_("/Filters/Enhance")),
  BRANCH (N_("/Filters/Generic")),

  SEPARATOR ("/Filters/---"),

  BRANCH (N_("/Filters/Glass Effects")),
  BRANCH (N_("/Filters/Light Effects")),
  BRANCH (N_("/Filters/Distorts")),
  BRANCH (N_("/Filters/Artistic")),
  BRANCH (N_("/Filters/Map")),
  BRANCH (N_("/Filters/Render")),
  BRANCH (N_("/Filters/Text")),
  BRANCH (N_("/Filters/Web")),

  SEPARATOR ("/Filters/---INSERT"),

  BRANCH (N_("/Filters/Animation")),
  BRANCH (N_("/Filters/Combine")),

  SEPARATOR ("/Filters/---"),

  BRANCH (N_("/Filters/Toys"))
};


/*****  <Load>  *****/

static GimpItemFactoryEntry load_entries[] =
{
  { { N_("/Automatic"), NULL,
      file_open_by_extension_cmd_callback, 0 },
    NULL,
    "open_by_extension.html", NULL },

  SEPARATOR ("/---")
};

  
/*****  <Save>  *****/

static GimpItemFactoryEntry save_entries[] =
{
  { { N_("/By Extension"), NULL,
      file_save_by_extension_cmd_callback, 0 },
    NULL,
    "save_by_extension.html", NULL },

  SEPARATOR ("/---")
};


/*****  <Layers>  *****/

static GimpItemFactoryEntry layers_entries[] =
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

  SEPARATOR ("/---"),

  { { N_("/Layer Boundary Size..."), "<control>R",
      layers_resize_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_RESIZE },
    NULL,
    "dialogs/layer_boundary_size.html", NULL },
  { { N_("/Layer to Imagesize"), NULL,
      layers_resize_to_image_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_RESIZE },
    NULL,
    "layer_to_image_size.html", NULL },
  { { N_("/Scale Layer..."), "<control>S",
      layers_scale_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SCALE },
    NULL,
    "dialogs/scale_layer.html", NULL },

  SEPARATOR ("/---"),

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

  SEPARATOR ("/---"),

  { { N_("/Add Alpha Channel"), NULL,
      layers_add_alpha_channel_cmd_callback, 0 },
    NULL,
    "add_alpha_channel.html", NULL },
  { { N_("/Alpha to Selection"), NULL,
      layers_alpha_select_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_REPLACE },
    NULL,
    "alpha_to_selection.html", NULL },

  SEPARATOR ("/---"),

  { { N_("/Edit Layer Attributes..."), NULL,
      layers_edit_attributes_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    "dialogs/edit_layer_attributes.html", NULL }
};


/*****  <Channels>  *****/

static GimpItemFactoryEntry channels_entries[] =
{
  { { N_("/New Channel..."), "<control>N",
      channels_new_channel_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    "dialogs/new_channel.html", NULL },
  { { N_("/Raise Channel"), "<control>F",
      channels_raise_channel_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_UP },
    NULL,
    "raise_channel.html", NULL },
  { { N_("/Lower Channel"), "<control>B",
      channels_lower_channel_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_DOWN },
    NULL,
    "lower_channel.html", NULL },
  { { N_("/Duplicate Channel"), "<control>C",
      channels_duplicate_channel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    "duplicate_channel.html", NULL },

  SEPARATOR ("/---"),

  { { N_("/Channel to Selection"), "<control>S",
      channels_channel_to_sel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_REPLACE },
    NULL,
    "channel_to_selection.html", NULL },
  { { N_("/Add to Selection"), NULL,
      channels_add_channel_to_sel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_ADD },
    NULL,
    "channel_to_selection.html#add", NULL },
  { { N_("/Subtract from Selection"), NULL,
      channels_sub_channel_from_sel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_SUBTRACT },
    NULL,
    "channel_to_selection.html#subtract", NULL },
  { { N_("/Intersect with Selection"), NULL,
      channels_intersect_channel_with_sel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_INTERSECT },
    NULL,
    "channel_to_selection.html#intersect", NULL },

  SEPARATOR ("/---"),

  { { N_("/Delete Channel"), "<control>X",
      channels_delete_channel_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    "delete_channel.html", NULL },

  SEPARATOR ("/---"),

  { { N_("/Edit Channel Attributes..."), NULL,
      channels_edit_channel_attributes_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    "dialogs/edit_channel_attributes.html", NULL }
};


/*****  <Vectors>  *****/

static GimpItemFactoryEntry vectors_entries[] =
{
  { { N_("/New Path..."), "<control>N",
      vectors_new_vectors_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    "new_path.html", NULL },
  { { N_("/Raise Path"), "<control>F",
      vectors_raise_vectors_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_UP },
    NULL,
    "raise_path.html", NULL },
  { { N_("/Lower Path"), "<control>B",
      vectors_lower_vectors_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_GO_DOWN },
    NULL,
    "lower_path.html", NULL },
  { { N_("/Duplicate Path"), "<control>U",
      vectors_duplicate_vectors_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    "duplicate_path.html", NULL },

  SEPARATOR ("/---"),

  { { N_("/Path to Selection"), "<control>S",
      vectors_vectors_to_sel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_REPLACE },
    NULL,
    "path_to_selection.html", NULL },
  { { N_("/Add to Selection"), NULL,
      vectors_add_vectors_to_sel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_ADD },
    NULL,
    "path_to_selection.html#add", NULL },
  { { N_("/Subtract from Selection"), NULL,
      vectors_sub_vectors_from_sel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_SUBTRACT },
    NULL,
    "path_to_selection.html#subtract", NULL },
  { { N_("/Intersect with Selection"), NULL,
      vectors_intersect_vectors_with_sel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_INTERSECT },
    NULL,
    "path_to_selection.html#intersect", NULL },

  { { N_("/Selection to Path"), "<control>P",
      vectors_sel_to_vectors_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_TO_PATH },
    NULL,
    "filters/sel2path.html", NULL },
  { { N_("/Stroke Path"), "<control>T",
      vectors_stroke_vectors_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_PATH_STROKE },
    NULL,
    "stroke_path.html", NULL },

  SEPARATOR ("/---"),

  { { N_("/Copy Path"), "<control>C",
      vectors_copy_vectors_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_COPY },
    NULL,
    "copy_path.html", NULL },
  { { N_("/Paste Path"), "<control>V",
      vectors_paste_vectors_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_PASTE },
    NULL,
    "paste_path.html", NULL },
  { { N_("/Import Path..."), "<control>I",
      vectors_import_vectors_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    NULL,
    "dialogs/import_path.html", NULL },
  { { N_("/Export Path..."), "<control>E",
      vectors_export_vectors_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SAVE },
    NULL,
    "dialogs/export_path.html", NULL },

  SEPARATOR ("/---"),

  { { N_("/Delete Path"), "<control>X",
      vectors_delete_vectors_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    "delete_path.html", NULL },

  SEPARATOR ("/---"),

  { { N_("/Path Tool"), NULL,
      vectors_vectors_tool_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_PATH },
    NULL,
    "tools/path_tool.html", NULL },
  { { N_("/Edit Path Attributes..."), NULL,
      vectors_edit_vectors_attributes_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    "dialogs/edit_path_attributes.html", NULL }
};


/*****  <Paths>  *****/

static GimpItemFactoryEntry paths_entries[] =
{
  { { N_("/New Path"), "<control>N",
      paths_dialog_new_path_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    "new_path.html", NULL },
  { { N_("/Duplicate Path"), "<control>U",
      paths_dialog_dup_path_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    "duplicate_path.html", NULL },
  { { N_("/Path to Selection"), "<control>S",
      paths_dialog_path_to_sel_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_REPLACE },
    NULL,
    "path_to_selection.html", NULL },
  { { N_("/Selection to Path"), "<control>P",
      paths_dialog_sel_to_path_callback, 0,
      "<StockItem>", GIMP_STOCK_SELECTION_TO_PATH },
    NULL,
    "filters/sel2path.html", NULL },
  { { N_("/Stroke Path"), "<control>T",
      paths_dialog_stroke_path_callback, 0,
      "<StockItem>", GIMP_STOCK_PATH_STROKE },
    NULL,
    "stroke_path.html", NULL },
  { { N_("/Delete Path"), "<control>X",
      paths_dialog_delete_path_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    "delete_path.html", NULL },

  SEPARATOR ("/---"),

  { { N_("/Copy Path"), "<control>C",
      paths_dialog_copy_path_callback, 0,
      "<StockItem>", GTK_STOCK_COPY },
    NULL,
    "copy_path.html", NULL },
  { { N_("/Paste Path"), "<control>V",
      paths_dialog_paste_path_callback, 0,
      "<StockItem>", GTK_STOCK_PASTE },
    NULL,
    "paste_path.html", NULL },
  { { N_("/Import Path..."), "<control>I",
      paths_dialog_import_path_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    NULL,
    "dialogs/import_path.html", NULL },
  { { N_("/Export Path..."), "<control>E",
      paths_dialog_export_path_callback, 0,
      "<StockItem>", GTK_STOCK_SAVE },
    NULL,
    "dialogs/export_path.html", NULL },

  SEPARATOR ("/---"),

  { { N_("/Edit Path Attributes..."), NULL,
      paths_dialog_edit_path_attributes_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    "dialogs/edit_path_attributes.html", NULL }
};


/*****  <Dialogs>  *****/

#define ADD_TAB(path,id,type,stock_id) \
  { { (path), NULL, dialogs_add_tab_cmd_callback, 0, (type), (stock_id) }, \
    (id), NULL, NULL }
#define PREVIEW_SIZE(path,size) \
  { { (path), NULL, dialogs_preview_size_cmd_callback, \
      (size), "/Preview Size/Tiny" }, NULL, NULL, NULL }

static GimpItemFactoryEntry dialogs_entries[] =
{
  { { N_("/Select Tab"), NULL, NULL, 0 },
    NULL,
    NULL, NULL },


  ADD_TAB (N_("/Add Tab/Layers..."),           "gimp-layer-list", NULL, NULL),
  ADD_TAB (N_("/Add Tab/Channels..."),         "gimp-channel-list", NULL, NULL),
  ADD_TAB (N_("/Add Tab/Paths..."),            "gimp-vectors-list", NULL, NULL),
  ADD_TAB (N_("/Add Tab/Old Paths..."),        "gimp-path-list", NULL, NULL),
  ADD_TAB (N_("/Add Tab/Document History..."), "gimp-document-history", NULL, NULL),
  ADD_TAB (N_("/Add Tab/Error Console..."),    "gimp-error-console", NULL, NULL),

  SEPARATOR ("/Add Tab/---"),

  ADD_TAB (N_("/Add Tab/Brushes..."),         "gimp-brush-grid", NULL, NULL),
  ADD_TAB (N_("/Add Tab/Patterns..."),        "gimp-pattern-grid", NULL, NULL),
  ADD_TAB (N_("/Add Tab/Gradients..."),       "gimp-gradient-list", NULL, NULL),
  ADD_TAB (N_("/Add Tab/Palettes..."),        "gimp-palette-list",
           "<StockItem>",                     GTK_STOCK_SELECT_COLOR),
  ADD_TAB (N_("/Add Tab/Indexed Palette..."), "gimp-indexed-palette",
           "<StockItem>",                     GTK_STOCK_SELECT_COLOR),
  ADD_TAB (N_("/Add Tab/Buffers..."),         "gimp-buffer-list", NULL, NULL),
  ADD_TAB (N_("/Add Tab/Images..."),          "gimp-image-list", NULL, NULL),

  SEPARATOR ("/Add Tab/---"),

  ADD_TAB (N_("/Add Tab/Tools..."), "gimp-tool-list", NULL, NULL),

  SEPARATOR ("/---"),

  { { N_("/Remove Tab"), NULL,
      dialogs_remove_tab_cmd_callback, 0 },
    NULL,
    NULL, NULL },

  { { N_("/Preview Size/Tiny"), NULL,
      dialogs_preview_size_cmd_callback,
      GIMP_PREVIEW_SIZE_TINY, "<RadioItem>" },
    NULL,
    NULL, NULL },

  PREVIEW_SIZE (N_("/Preview Size/Extra Small"), GIMP_PREVIEW_SIZE_EXTRA_SMALL),
  PREVIEW_SIZE (N_("/Preview Size/Small"),       GIMP_PREVIEW_SIZE_SMALL),
  PREVIEW_SIZE (N_("/Preview Size/Medium"),      GIMP_PREVIEW_SIZE_MEDIUM),
  PREVIEW_SIZE (N_("/Preview Size/Large"),       GIMP_PREVIEW_SIZE_LARGE),
  PREVIEW_SIZE (N_("/Preview Size/Extra Large"), GIMP_PREVIEW_SIZE_EXTRA_LARGE),
  PREVIEW_SIZE (N_("/Preview Size/Huge"),        GIMP_PREVIEW_SIZE_HUGE),
  PREVIEW_SIZE (N_("/Preview Size/Enormous"),    GIMP_PREVIEW_SIZE_ENORMOUS),
  PREVIEW_SIZE (N_("/Preview Size/Gigantic"),    GIMP_PREVIEW_SIZE_GIGANTIC),

  { { N_("/View as List"), NULL,
      dialogs_toggle_view_cmd_callback, GIMP_VIEW_TYPE_LIST, "<RadioItem>" },
    NULL,
    NULL, NULL },
  { { N_("/View as Grid"), NULL,
      dialogs_toggle_view_cmd_callback, GIMP_VIEW_TYPE_GRID, "/View as List" },
    NULL,
    NULL, NULL },

  SEPARATOR ("/image-menu-separator"),

  { { N_("/Show Image Menu"), NULL,
      dialogs_toggle_image_menu_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    NULL, NULL },
  { { N_("/Auto Follow Active Image"), NULL,
      dialogs_toggle_auto_cmd_callback, 0, "<ToggleItem>" },
    NULL,
    NULL, NULL }

};

#undef ADD_TAB
#undef PREVIEW_SIZE


/*****  <Brushes>  *****/

static GimpItemFactoryEntry brushes_entries[] =
{
  { { N_("/New Brush"), NULL,
      data_new_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    NULL, NULL },
  { { N_("/Duplicate Brush"), NULL,
      data_duplicate_data_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    NULL, NULL },
  { { N_("/Edit Brush..."), NULL,
      data_edit_data_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    NULL, NULL },
  { { N_("/Delete Brush..."), NULL,
      data_delete_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    NULL, NULL },

  SEPARATOR ("/---"),

  { { N_("/Refresh Brushes"), NULL,
      data_refresh_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_REFRESH },
    NULL,
    NULL, NULL }
};


/*****  <Patterns>  *****/

static GimpItemFactoryEntry patterns_entries[] =
{
  { { N_("/New Pattern"), NULL,
      data_new_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    NULL, NULL },
  { { N_("/Duplicate Pattern"), NULL,
      data_duplicate_data_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    NULL, NULL },
  { { N_("/Edit Pattern..."), NULL,
      data_edit_data_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    NULL, NULL },
  { { N_("/Delete Pattern..."), NULL,
      data_delete_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    NULL, NULL },

  SEPARATOR ("/---"),

  { { N_("/Refresh Patterns"), NULL,
      data_refresh_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_REFRESH },
    NULL,
    NULL, NULL }
};


/*****  <GradientEditor>  *****/

#define LOAD_LEFT_FROM(num,magic) \
  { { "/Load Left Color From/" num, NULL, \
      gradient_editor_load_left_cmd_callback, (magic) }, NULL, NULL, NULL }
#define SAVE_LEFT_TO(num,magic) \
  { { "/Save Left Color To/" num, NULL, \
      gradient_editor_save_left_cmd_callback, (magic) }, NULL, NULL, NULL }
#define LOAD_RIGHT_FROM(num,magic) \
  { { "/Load Right Color From/" num, NULL, \
      gradient_editor_load_right_cmd_callback, (magic) }, NULL, NULL, NULL }
#define SAVE_RIGHT_TO(num,magic) \
  { { "/Save Right Color To/" num, NULL, \
      gradient_editor_save_right_cmd_callback, (magic) }, NULL, NULL, NULL }

static GimpItemFactoryEntry gradient_editor_entries[] =
{
  { { N_("/Left Endpoint's Color..."), NULL,
      gradient_editor_left_color_cmd_callback, 0 },
    NULL,
    NULL, NULL },

  { { N_("/Load Left Color From/Left Neighbor's Right Endpoint"), NULL,
      gradient_editor_load_left_cmd_callback, 0 },
    NULL,
    NULL, NULL },
  { { N_("/Load Left Color From/Right Endpoint"), NULL,
      gradient_editor_load_left_cmd_callback, 1 },
    NULL,
    NULL, NULL },
  { { N_("/Load Left Color From/FG Color"), NULL,
      gradient_editor_load_left_cmd_callback, 2 },
    NULL,
    NULL, NULL },
  { { N_("/Load Left Color From/BG Color"), NULL,
      gradient_editor_load_left_cmd_callback, 3 },
    NULL,
    NULL, NULL },

  SEPARATOR ("/Load Left Color From/---"),

  LOAD_LEFT_FROM ("01", 4),
  LOAD_LEFT_FROM ("02", 5),
  LOAD_LEFT_FROM ("03", 6),
  LOAD_LEFT_FROM ("04", 7),
  LOAD_LEFT_FROM ("05", 8),
  LOAD_LEFT_FROM ("06", 9),
  LOAD_LEFT_FROM ("07", 10),
  LOAD_LEFT_FROM ("08", 11),
  LOAD_LEFT_FROM ("09", 12),
  LOAD_LEFT_FROM ("10", 13),

  BRANCH (N_("/Save Left Color To")),

  SAVE_LEFT_TO ("01", 0),
  SAVE_LEFT_TO ("02", 1),
  SAVE_LEFT_TO ("03", 2),
  SAVE_LEFT_TO ("04", 3),
  SAVE_LEFT_TO ("05", 4),
  SAVE_LEFT_TO ("06", 5),
  SAVE_LEFT_TO ("07", 6),
  SAVE_LEFT_TO ("08", 7),
  SAVE_LEFT_TO ("09", 8),
  SAVE_LEFT_TO ("10", 9),

  SEPARATOR ("/---"),

  { { N_("/Right Endpoint's Color..."), NULL,
      gradient_editor_right_color_cmd_callback, 0 },
    NULL,
    NULL, NULL },

  { { N_("/Load Right Color From/Right Neighbor's Left Endpoint"), NULL,
      gradient_editor_load_right_cmd_callback, 0 },
    NULL,
    NULL, NULL },
  { { N_("/Load Right Color From/Left Endpoint"), NULL,
      gradient_editor_load_right_cmd_callback, 1 },
    NULL,
    NULL, NULL },
  { { N_("/Load Right Color From/FG Color"), NULL,
      gradient_editor_load_right_cmd_callback, 2 },
    NULL,
    NULL, NULL },
  { { N_("/Load Right Color From/BG Color"), NULL,
      gradient_editor_load_right_cmd_callback, 3 },
    NULL,
    NULL, NULL },

  SEPARATOR ("/Load Right Color From/---"),

  LOAD_RIGHT_FROM ("01", 4),
  LOAD_RIGHT_FROM ("02", 5),
  LOAD_RIGHT_FROM ("03", 6),
  LOAD_RIGHT_FROM ("04", 7),
  LOAD_RIGHT_FROM ("05", 8),
  LOAD_RIGHT_FROM ("06", 9),
  LOAD_RIGHT_FROM ("07", 10),
  LOAD_RIGHT_FROM ("08", 11),
  LOAD_RIGHT_FROM ("09", 12),
  LOAD_RIGHT_FROM ("10", 13),

  BRANCH (N_("/Save Right Color To")),

  SAVE_RIGHT_TO ("01", 0),
  SAVE_RIGHT_TO ("02", 1),
  SAVE_RIGHT_TO ("03", 2),
  SAVE_RIGHT_TO ("04", 3),
  SAVE_RIGHT_TO ("05", 4),
  SAVE_RIGHT_TO ("06", 5),
  SAVE_RIGHT_TO ("07", 6),
  SAVE_RIGHT_TO ("08", 7),
  SAVE_RIGHT_TO ("09", 8),
  SAVE_RIGHT_TO ("10", 9),

  SEPARATOR ("/---"),

  { { N_("/blendingfunction/Linear"), NULL,
      gradient_editor_blending_func_cmd_callback,
      GIMP_GRAD_LINEAR, "<RadioItem>" },
    NULL,
    NULL, NULL },
  { { N_("/blendingfunction/Curved"), NULL,
      gradient_editor_blending_func_cmd_callback,
      GIMP_GRAD_CURVED, "/blendingfunction/Linear" },
    NULL,
    NULL, NULL },
  { { N_("/blendingfunction/Sinusodial"), NULL,
      gradient_editor_blending_func_cmd_callback,
      GIMP_GRAD_SINE, "/blendingfunction/Linear" },
    NULL,
    NULL, NULL },
  { { N_("/blendingfunction/Spherical (increasing)"), NULL,
      gradient_editor_blending_func_cmd_callback,
      GIMP_GRAD_SPHERE_INCREASING, "/blendingfunction/Linear" },
    NULL,
    NULL, NULL },
  { { N_("/blendingfunction/Spherical (decreasing)"), NULL,
      gradient_editor_blending_func_cmd_callback,
      GIMP_GRAD_SPHERE_DECREASING, "/blendingfunction/Linear" },
    NULL,
    NULL, NULL },
  { { N_("/blendingfunction/(Varies)"), NULL, NULL,
      0, "/blendingfunction/Linear" },
    NULL,
    NULL, NULL },

  { { N_("/coloringtype/RGB"), NULL,
      gradient_editor_coloring_type_cmd_callback,
      GIMP_GRAD_RGB, "<RadioItem>" },
    NULL,
    NULL, NULL },
  { { N_("/coloringtype/HSV (counter-clockwise hue)"), NULL,
      gradient_editor_coloring_type_cmd_callback,
      GIMP_GRAD_HSV_CCW, "/coloringtype/RGB" },
    NULL,
    NULL, NULL },
  { { N_("/coloringtype/HSV (clockwise hue)"), NULL,
      gradient_editor_coloring_type_cmd_callback,
      GIMP_GRAD_HSV_CW, "/coloringtype/RGB" },
    NULL,
    NULL, NULL },
  { { N_("/coloringtype/(Varies)"), NULL, NULL,
      0, "/coloringtype/RGB" },
    NULL,
    NULL, NULL },

  SEPARATOR ("/---"),

  { { "/flip", "F",
      gradient_editor_flip_cmd_callback, 0 },
    NULL,
    NULL, NULL },
  { { "/replicate", "R",
      gradient_editor_replicate_cmd_callback, 0 },
    NULL,
    NULL, NULL },
  { { "/splitmidpoint", "S",
      gradient_editor_split_midpoint_cmd_callback, 0 },
    NULL,
    NULL, NULL },
  { { "/splituniformly", "U",
      gradient_editor_split_uniformly_cmd_callback, 0 },
    NULL,
    NULL, NULL },
  { { "/delete", "D",
      gradient_editor_delete_cmd_callback, 0 },
    NULL,
    NULL, NULL },
  { { "/recenter", "C",
      gradient_editor_recenter_cmd_callback, 0 },
    NULL,
    NULL, NULL },
  { { "/redistribute", "<control>C",
      gradient_editor_redistribute_cmd_callback, 0 },
    NULL,
    NULL, NULL },

  SEPARATOR ("/---"),

  { { N_("/Blend Endpoints' Colors"), "B",
      gradient_editor_blend_color_cmd_callback, 0 },
    NULL,
    NULL, NULL },
  { { N_("/Blend Endpoints' Opacity"), "<control>B",
      gradient_editor_blend_opacity_cmd_callback, 0 },
    NULL,
    NULL, NULL },
};

#undef LOAD_LEFT_FROM
#undef SAVE_LEFT_TO
#undef LOAD_RIGHT_FROM
#undef SAVE_RIGHT_TO


/*****  <Gradients>  *****/

static GimpItemFactoryEntry gradients_entries[] =
{
  { { N_("/New Gradient"), NULL,
      data_new_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    NULL, NULL },
  { { N_("/Duplicate Gradient"), NULL,
      data_duplicate_data_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    NULL, NULL },
  { { N_("/Edit Gradient..."), NULL,
      data_edit_data_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    NULL, NULL },
  { { N_("/Delete Gradient..."), NULL,
      data_delete_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    NULL, NULL },

  SEPARATOR ("/---"),

  { { N_("/Refresh Gradients"), NULL,
      data_refresh_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_REFRESH },
    NULL,
    NULL, NULL },

  SEPARATOR ("/---"),

  { { N_("/Save as POV-Ray..."), NULL,
      gradients_save_as_pov_ray_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SAVE_AS },
    NULL,
    NULL, NULL }
};


/*****  <PaletteEditor>  *****/

static GimpItemFactoryEntry palette_editor_entries[] =
{
  { { N_("/New Color"), NULL,
      palette_editor_new_color_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    NULL, NULL },
  { { N_("/Edit Color..."), NULL,
      palette_editor_edit_color_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    NULL, NULL },
  { { N_("/Delete Color"), NULL,
      palette_editor_delete_color_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    NULL, NULL }
};


/*****  <Palettes>  *****/

static GimpItemFactoryEntry palettes_entries[] =
{
  { { N_("/New Palette"), NULL,
      data_new_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    NULL, NULL },
  { { N_("/Duplicate Palette"), NULL,
      data_duplicate_data_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DUPLICATE },
    NULL,
    NULL, NULL },
  { { N_("/Edit Palette..."), NULL,
      data_edit_data_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    NULL, NULL },
  { { N_("/Delete Palette..."), NULL,
      data_delete_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    NULL, NULL },

  SEPARATOR ("/---"),

  { { N_("/Refresh Palettes"), NULL,
      data_refresh_data_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_REFRESH },
    NULL,
    NULL, NULL },

  SEPARATOR ("/---"),

  { { N_("/Import Palette..."), NULL,
      palettes_import_palette_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_CONVERT },
    NULL,
    NULL, NULL },
  { { N_("/Merge Palettes..."), NULL,
      palettes_merge_palettes_cmd_callback, 0 },
    NULL,
    NULL, NULL }
};


/*****  <Buffers>  *****/

static GimpItemFactoryEntry buffers_entries[] =
{
  { { N_("/Paste Buffer"), NULL,
      buffers_paste_buffer_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_PASTE },
    NULL,
    NULL, NULL },
  { { N_("/Paste Buffer Into"), NULL,
      buffers_paste_buffer_into_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_PASTE_INTO },
    NULL,
    NULL, NULL },
  { { N_("/Paste Buffer as New"), NULL,
      buffers_paste_buffer_as_new_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_PASTE_AS_NEW },
    NULL,
    NULL, NULL },
  { { N_("/Delete Buffer"), NULL,
      buffers_delete_buffer_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    NULL, NULL }
};


/*****  <ColormapEditor>  *****/

static GimpItemFactoryEntry colormap_editor_entries[] =
{
  { { N_("/Add Color"), NULL,
      colormap_editor_add_color_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    NULL, NULL },
  { { N_("/Edit Color..."), NULL,
      colormap_editor_edit_color_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_EDIT },
    NULL,
    NULL, NULL },
};


/*****  <Documents>  *****/

static GimpItemFactoryEntry documents_entries[] =
{
  { { N_("/Open Image"), NULL,
      documents_open_document_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    NULL,
    NULL, NULL },
  { { N_("/Raise or Open Image"), NULL,
      documents_raise_or_open_document_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    NULL,
    NULL, NULL },
  { { N_("/File Open Dialog..."), NULL,
      documents_file_open_dialog_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    NULL,
    NULL, NULL },
  { { N_("/Remove Entry"), NULL,
      documents_delete_document_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_DELETE },
    NULL,
    NULL, NULL },

  SEPARATOR ("/---"),

  { { N_("/Refresh History"), NULL,
      documents_refresh_documents_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_REFRESH },
    NULL,
    NULL, NULL }
};


/*****  <QMask>  *****/

static GimpItemFactoryEntry qmask_entries[] =
{
  { { N_("/QMask Active"), NULL,
      qmask_toggle_cmd_callback, 0, "<ToggleItem>" },
    NULL, NULL, NULL },

  SEPARATOR ("/---"),

  { { N_("/Mask Selected Areas"), NULL,
      qmask_invert_cmd_callback, FALSE, "<RadioItem>" },
    NULL, NULL, NULL },
  { { N_("/Mask Unselected Areas"), NULL,
      qmask_invert_cmd_callback, TRUE, "/Mask Selected Areas" },
    NULL, NULL, NULL },

  SEPARATOR ("/---"),

  { { N_("/Configure Color and Opacity..."), NULL,
      qmask_configure_cmd_callback, 0 },
    NULL, NULL, NULL }
};


static gboolean  menus_initialized = FALSE;
static GList    *all_factories     = NULL;


/*  public functions  */

void
menus_init (Gimp *gimp)
{
  GimpItemFactory *toolbox_factory;
  GimpItemFactory *image_factory;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (menus_initialized == FALSE);

  menus_initialized = TRUE;

#define ADD_FACTORY(f) (all_factories = g_list_append (all_factories, (f)));

  toolbox_factory = gimp_item_factory_new (gimp,
                                           GTK_TYPE_MENU_BAR,
                                           "<Toolbox>", "toolbox",
                                           NULL,
                                           G_N_ELEMENTS (toolbox_entries),
                                           toolbox_entries,
                                           gimp,
                                           TRUE);
  menus_last_opened_add (toolbox_factory, gimp);
  ADD_FACTORY (toolbox_factory);

  image_factory = gimp_item_factory_new (gimp,
                                         GTK_TYPE_MENU,
                                         "<Image>", "image",
                                         NULL,
                                         G_N_ELEMENTS (image_entries),
                                         image_entries,
                                         gimp,
                                         TRUE);
  menus_last_opened_add (image_factory, gimp);
  ADD_FACTORY (image_factory);

  ADD_FACTORY (gimp_item_factory_new (gimp,
                                      GTK_TYPE_MENU,
                                      "<Load>", "open",
                                      NULL,
                                      G_N_ELEMENTS (load_entries),
                                      load_entries,
                                      gimp,
                                      FALSE));
  ADD_FACTORY (gimp_item_factory_new (gimp,
                                      GTK_TYPE_MENU,
                                      "<Save>", "save",
                                      NULL,
                                      G_N_ELEMENTS (save_entries),
                                      save_entries,
                                      gimp,
                                      FALSE));
  ADD_FACTORY (gimp_item_factory_new (gimp,
                                      GTK_TYPE_MENU,
                                      "<Layers>", "layers",
                                      layers_menu_update,
                                      G_N_ELEMENTS (layers_entries),
                                      layers_entries,
                                      gimp,
                                      FALSE));
  ADD_FACTORY (gimp_item_factory_new (gimp,
                                      GTK_TYPE_MENU,
                                      "<Channels>", "channels",
                                      channels_menu_update,
                                      G_N_ELEMENTS (channels_entries),
                                      channels_entries,
                                      gimp,
                                      FALSE));
  ADD_FACTORY (gimp_item_factory_new (gimp,
                                      GTK_TYPE_MENU,
                                      "<Vectors>", "vectors",
                                      vectors_menu_update,
                                      G_N_ELEMENTS (vectors_entries),
                                      vectors_entries,
                                      gimp,
                                      FALSE));
  ADD_FACTORY (gimp_item_factory_new (gimp,
                                      GTK_TYPE_MENU,
                                      "<Paths>", "paths",
                                      NULL,
                                      G_N_ELEMENTS (paths_entries),
                                      paths_entries,
                                      gimp,
                                      FALSE));
  ADD_FACTORY (gimp_item_factory_new (gimp,
                                      GTK_TYPE_MENU,
                                      "<Dialogs>", "dialogs",
                                      dialogs_menu_update,
                                      G_N_ELEMENTS (dialogs_entries),
                                      dialogs_entries,
                                      gimp,
                                      FALSE));
  ADD_FACTORY (gimp_item_factory_new (gimp,
                                      GTK_TYPE_MENU,
                                      "<Brushes>", "brushes",
                                      brushes_menu_update,
                                      G_N_ELEMENTS (brushes_entries),
                                      brushes_entries,
                                      gimp,
                                      FALSE));
  ADD_FACTORY (gimp_item_factory_new (gimp,
                                      GTK_TYPE_MENU,
                                      "<Patterns>", "patterns",
                                      patterns_menu_update,
                                      G_N_ELEMENTS (patterns_entries),
                                      patterns_entries,
                                      gimp,
                                      FALSE));
  ADD_FACTORY (gimp_item_factory_new (gimp,
                                      GTK_TYPE_MENU,
                                      "<GradientEditor>", "gradient_editor",
                                      gradient_editor_menu_update,
                                      G_N_ELEMENTS (gradient_editor_entries),
                                      gradient_editor_entries,
                                      gimp,
                                      FALSE));
  ADD_FACTORY (gimp_item_factory_new (gimp,
                                      GTK_TYPE_MENU,
                                      "<Gradients>", "gradients",
                                      gradients_menu_update,
                                      G_N_ELEMENTS (gradients_entries),
                                      gradients_entries,
                                      gimp,
                                      FALSE));
  ADD_FACTORY (gimp_item_factory_new (gimp,
                                      GTK_TYPE_MENU,
                                      "<PaletteEditor>", "palette_editor",
                                      palette_editor_menu_update,
                                      G_N_ELEMENTS (palette_editor_entries),
                                      palette_editor_entries,
                                      gimp,
                                      FALSE));
  ADD_FACTORY (gimp_item_factory_new (gimp,
                                      GTK_TYPE_MENU,
                                      "<Palettes>", "palettes",
                                      palettes_menu_update,
                                      G_N_ELEMENTS (palettes_entries),
                                      palettes_entries,
                                      gimp,
                                      FALSE));
  ADD_FACTORY (gimp_item_factory_new (gimp,
                                      GTK_TYPE_MENU,
                                      "<Buffers>", "buffers",
                                      buffers_menu_update,
                                      G_N_ELEMENTS (buffers_entries),
                                      buffers_entries,
                                      gimp,
                                      FALSE));
  ADD_FACTORY (gimp_item_factory_new (gimp,
                                      GTK_TYPE_MENU,
                                      "<Documents>", "documents",
                                      documents_menu_update,
                                      G_N_ELEMENTS (documents_entries),
                                      documents_entries,
                                      gimp,
                                      FALSE));
  ADD_FACTORY (gimp_item_factory_new (gimp,
                                      GTK_TYPE_MENU,
                                      "<ColormapEditor>", "colormap_editor",
                                      colormap_editor_menu_update,
                                      G_N_ELEMENTS (colormap_editor_entries),
                                      colormap_editor_entries,
                                      gimp,
                                      FALSE));
  ADD_FACTORY (gimp_item_factory_new (gimp,
                                      GTK_TYPE_MENU,
                                      "<QMask>", "qmask",
                                      qmask_menu_update,
                                      G_N_ELEMENTS (qmask_entries),
                                      qmask_entries,
                                      gimp,
                                      FALSE));

#undef ADD_FACTORY

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

    for (list = GIMP_LIST (gimp->tool_info_list)->list;
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

            gimp_item_factory_create_item (image_factory,
                                           &entry,
                                           tool_info,
                                           2,
                                           TRUE, FALSE);
          }
      }

    /*  reorder <Image>/Image/Colors  */
    tool_info = (GimpToolInfo *)
      gimp_container_get_child_by_name (gimp->tool_info_list,
                                        "gimp-posterize-tool");

    menu_item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (image_factory),
                                             tool_info->menu_path);
    if (menu_item && menu_item->parent)
      gtk_menu_reorder_child (GTK_MENU (menu_item->parent), menu_item, 4);

    for (i = 0; i < G_N_ELEMENTS (color_tools); i++)
      {
        tool_info = (GimpToolInfo *)
          gimp_container_get_child_by_name (gimp->tool_info_list,
                                            color_tools[i]);

        menu_item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (image_factory),
                                                 tool_info->menu_path);
        if (menu_item && menu_item->parent)
          {
            gtk_menu_reorder_child (GTK_MENU (menu_item->parent),
                                    menu_item, i + 1);
          }
      }
  }

  {
    gchar *filename;

    filename = gimp_personal_rc_file ("menurc");
    gtk_accel_map_load (filename);
    g_free (filename);
  }

  {
    GimpContext *user_context;

    user_context = gimp_get_user_context (gimp);

    g_signal_connect (G_OBJECT (user_context), "foreground_changed",
                      G_CALLBACK (menus_color_changed),
                      image_factory);
    g_signal_connect (G_OBJECT (user_context), "background_changed",
                      G_CALLBACK (menus_color_changed),
                      image_factory);

    menus_color_changed (user_context, NULL, image_factory);
  }
}

void
menus_exit (Gimp *gimp)
{
  GimpItemFactory *item_factory;
  gchar           *filename;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  item_factory = gimp_item_factory_from_path ("<Toolbox>");
  g_signal_handlers_disconnect_by_func (G_OBJECT (gimp->documents),
                                        menus_last_opened_update,
                                        item_factory);
  g_signal_handlers_disconnect_by_func (G_OBJECT (gimp->documents),
                                        menus_last_opened_reorder,
                                        item_factory);

  item_factory = gimp_item_factory_from_path ("<Image>");
  g_signal_handlers_disconnect_by_func (G_OBJECT (gimp->documents),
                                        menus_last_opened_update,
                                        item_factory);
  g_signal_handlers_disconnect_by_func (G_OBJECT (gimp->documents),
                                        menus_last_opened_reorder,
                                        item_factory);

  filename = gimp_personal_rc_file ("menurc");
  gtk_accel_map_save (filename);
  g_free (filename);

  g_list_foreach (all_factories, (GFunc) g_object_unref, NULL);
  g_list_free (all_factories);
  all_factories = NULL;
}

void
menus_restore (Gimp *gimp)
{
  static gchar *rotate_plugins[]      = { "Rotate 90 degrees",
                                          "Rotate 180 degrees",
                                          "Rotate 270 degrees" };
  static gchar *image_file_entries[]  = { "---moved",
                                          "Close",
                                          "Quit" };
  static gchar *reorder_submenus[]    = { "<Image>/Video",
                                          "<Image>/Script-Fu" };
  static gchar *reorder_subsubmenus[] = { "<Image>/Filters",
					  "<Toolbox>/Xtns" };

  GimpItemFactory *item_factory;
  GtkWidget       *menu_item;
  GtkWidget       *menu;
  GList           *list;
  gchar           *path;
  gint             i, pos;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  /*  Move all menu items under "<Toolbox>/Xtns" which are not submenus or
   *  separators to the top of the menu
   */
  pos = 1;
  item_factory = gimp_item_factory_from_path ("<Toolbox>");
  menu_item    = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (item_factory),
                                              "/Xtns/Module Browser...");
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

  /*  Move all menu items under "<Image>/Filters" which are not submenus or
   *  separators to the top of the menu
   */
  pos = 3;
  item_factory = gimp_item_factory_from_path ("<Image>");
  menu_item    = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (item_factory),
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
      menu_item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (item_factory),
                                               path);
      g_free (path);

      if (menu_item && menu_item->parent)
        {
          gtk_menu_reorder_child (GTK_MENU (menu_item->parent), menu_item, pos);
          pos++;
        }
    }

  pos = 1;
  for (i = 0; i < G_N_ELEMENTS (rotate_plugins); i++)
    {
      path = g_strconcat ("/Layer/Transform/", rotate_plugins[i], NULL);
      menu_item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (item_factory),
                                               path);
      g_free (path);

      if (menu_item && menu_item->parent)
        {
          gtk_menu_reorder_child (GTK_MENU (menu_item->parent), menu_item, pos);
          pos++;
        }
    }

  /*  Reorder "<Image>/File"  */
  for (i = 0; i < G_N_ELEMENTS (image_file_entries); i++)
    {
      path = g_strconcat ("/File/", image_file_entries[i], NULL);
      menu_item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (item_factory),
                                               path);
      g_free (path);

      if (menu_item && menu_item->parent)
        gtk_menu_reorder_child (GTK_MENU (menu_item->parent), menu_item, -1);
    }

  /*  Reorder menus where plugins registered submenus  */
  for (i = 0; i < G_N_ELEMENTS (reorder_submenus); i++)
    {
      item_factory = gimp_item_factory_from_path (reorder_submenus[i]);

      menu = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (item_factory),
					  reorder_submenus[i]);

      if (menu && GTK_IS_MENU (menu))
	{
	  menus_filters_subdirs_to_top (GTK_MENU (menu));
	}
    }

  for (i = 0; i < G_N_ELEMENTS (reorder_subsubmenus); i++)
    {
      item_factory = gimp_item_factory_from_path (reorder_subsubmenus[i]);

      menu = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (item_factory),
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
  item_factory = gimp_item_factory_from_path ("<Image>");
  menu_item    = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (item_factory),
                                              "/Filters/---INSERT");

  if (menu_item && menu_item->parent && GTK_IS_MENU (menu_item->parent))
    {
      menu = menu_item->parent;
      pos = g_list_index (GTK_MENU_SHELL (menu)->children, menu_item);

      menu_item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (item_factory),
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


/*  private functions  */

static void
menus_last_opened_add (GimpItemFactory *item_factory,
                       Gimp            *gimp)
{
  GimpItemFactoryEntry *last_opened_entries;
  gint                  i;

  last_opened_entries = g_new (GimpItemFactoryEntry,
                               gimprc.last_opened_size);

  for (i = 0; i < gimprc.last_opened_size; i++)
    {
      last_opened_entries[i].entry.path = 
        g_strdup_printf ("/File/Open Recent/%02d", i + 1);

      if (i < 9)
        last_opened_entries[i].entry.accelerator =
          g_strdup_printf ("<control>%d", i + 1);
      else
        last_opened_entries[i].entry.accelerator = "";

      last_opened_entries[i].entry.callback        = file_last_opened_cmd_callback;
      last_opened_entries[i].entry.callback_action = i;
      last_opened_entries[i].entry.item_type       = "<StockItem>";
      last_opened_entries[i].entry.extra_data      = GTK_STOCK_OPEN;
      last_opened_entries[i].quark_string          = NULL;
      last_opened_entries[i].help_page             = "file/last_opened.html";
      last_opened_entries[i].description           = NULL;
    }

  gimp_item_factory_create_items (item_factory,
                                  gimprc.last_opened_size, last_opened_entries,
                                  gimp, 2, TRUE, FALSE);

  for (i = 0; i < gimprc.last_opened_size; i++)
    {
      gimp_item_factory_set_visible (GTK_ITEM_FACTORY (item_factory),
                                     last_opened_entries[i].entry.path,
                                     FALSE);
    }

  gimp_item_factory_set_sensitive (GTK_ITEM_FACTORY (item_factory),
                                   "/File/Open Recent/(None)",
                                   FALSE);

  for (i = 0; i < gimprc.last_opened_size; i++)
    {
      g_free (last_opened_entries[i].entry.path);
      g_free (last_opened_entries[i].entry.accelerator);
    }

  g_free (last_opened_entries);

  g_signal_connect (G_OBJECT (gimp->documents), "add",
                    G_CALLBACK (menus_last_opened_update),
                    item_factory);
  g_signal_connect (G_OBJECT (gimp->documents), "remove",
                    G_CALLBACK (menus_last_opened_update),
                    item_factory);
  g_signal_connect (G_OBJECT (gimp->documents), "reorder",
                    G_CALLBACK (menus_last_opened_reorder),
                    item_factory);

  menus_last_opened_update (gimp->documents, NULL, item_factory);
}

static void
menus_last_opened_update (GimpContainer   *container,
                          GimpImagefile   *unused,
                          GimpItemFactory *item_factory)
{
  GtkWidget *widget;
  gint       num_documents;
  gint       i;

  num_documents = gimp_container_num_children (container);

  gimp_item_factory_set_visible (GTK_ITEM_FACTORY (item_factory),
                                 "/File/Open Recent/(None)",
                                 num_documents == 0);

  for (i = 0; i < gimprc.last_opened_size; i++)
    {
      gchar *path_str;

      path_str = g_strdup_printf ("/File/Open Recent/%02d", i + 1);

      widget = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (item_factory),
                                            path_str);

      g_free (path_str);

      if (i < num_documents)
        {
          GimpImagefile *imagefile;

          imagefile = (GimpImagefile *)
            gimp_container_get_child_by_index (container, i);

          if (g_object_get_data (G_OBJECT (widget), "gimp-imagefile") !=
              (gpointer) imagefile)
            {
              gchar *basename;

              basename = g_path_get_basename (GIMP_OBJECT (imagefile)->name);

              gtk_label_set_text (GTK_LABEL (GTK_BIN (widget)->child),
                                  basename);

              g_free (basename);

              gimp_help_set_help_data (widget,
                                       GIMP_OBJECT (imagefile)->name,
                                       NULL);

              g_object_set_data (G_OBJECT (widget), "gimp-imagefile", imagefile);
              gtk_widget_show (widget);
            }
        }
      else
        {
          g_object_set_data (G_OBJECT (widget), "gimp-imagefile", NULL);
          gtk_widget_hide (widget);
        }
    }
}

static void
menus_last_opened_reorder (GimpContainer   *container,
                           GimpImagefile   *unused1,
                           gint             unused2,
                           GimpItemFactory *item_factory)
{
  menus_last_opened_update (container, unused1, item_factory);
}

static void
menus_color_changed (GimpContext     *context,
                     const GimpRGB   *unused,
                     GimpItemFactory *item_factory)
{
  GimpRGB fg;
  GimpRGB bg;

  gimp_context_get_foreground (context, &fg);
  gimp_context_get_background (context, &bg);

  gimp_item_factory_set_color (GTK_ITEM_FACTORY (item_factory),
                               "/Edit/Fill with FG Color", &fg, FALSE);
  gimp_item_factory_set_color (GTK_ITEM_FACTORY (item_factory),
                               "/Edit/Fill with BG Color", &bg, FALSE);
}

static void
menus_filters_subdirs_to_top (GtkMenu *menu)
{
  GtkMenuItem *menu_item;
  GList       *list;
  gboolean     submenus_passed = FALSE;
  gint         pos;
  gint         items;

  pos   = 1;
  items = 0;

  for (list = GTK_MENU_SHELL (menu)->children; list; list = g_list_next (list))
    {
      menu_item = GTK_MENU_ITEM (list->data);
      items++;
      
      if (menu_item->submenu)
	{
	  if (submenus_passed)
	    {
	      menus_filters_subdirs_to_top (GTK_MENU (menu_item->submenu));
	      gtk_menu_reorder_child (menu, GTK_WIDGET (menu_item), pos++);
	    }
	}
      else
	{
	  submenus_passed = TRUE;
	}
    }

  if (pos > 1 && items > pos)
    {
      GtkWidget *separator;

      separator = gtk_menu_item_new ();
      gtk_menu_shell_insert (GTK_MENU_SHELL (menu), separator, pos);
      gtk_widget_show (separator);
    }
}

#ifdef ENABLE_DEBUG_ENTRIES

static void
menus_debug_recurse_menu (GtkWidget *menu,
			  gint       depth,
			  gchar     *path)
{
  GtkItemFactory *item_factory;
  GtkWidget      *menu_item;
  GList          *list;
  const gchar    *label;
  gchar          *help_page;
  gchar          *help_path;
  gchar          *factory_path;
  gchar          *hash;
  gchar          *full_path;
  gchar          *format_str;

  for (list = GTK_MENU_SHELL (menu)->children; list; list = g_list_next (list))
    {
      menu_item = GTK_WIDGET (list->data);
      
      if (GTK_IS_LABEL (GTK_BIN (menu_item)->child))
	{
	  label = gtk_label_get_text (GTK_LABEL (GTK_BIN (menu_item)->child));
	  full_path = g_strconcat (path, "/", label, NULL);

	  item_factory = GTK_ITEM_FACTORY (gimp_item_factory_from_path (path));
          help_page    = g_object_get_data (G_OBJECT (menu_item), "help_page");

	  if (item_factory)
	    {
	      factory_path = g_object_get_data (G_OBJECT (item_factory),
                                                "factory_path");

              if (factory_path)
                {
                  help_page = g_build_filename (factory_path, help_page, NULL);
                }
              else
                {
                  help_page = g_strdup (help_page);
                }
	    }
	  else
	    {
	      help_page = g_strdup (help_page);
	    }

	  if (help_page)
	    {
	      help_path = g_build_filename (gimp_data_directory (),
                                            "help", "C", help_page, NULL);

	      if ((hash = strchr (help_path, '#')) != NULL)
		*hash = '\0';

	      if (g_file_test (help_path, G_FILE_TEST_EXISTS))
		{
		  g_free (help_path);
		  help_path = g_strconcat ("! ", help_page, NULL);
		  g_free (help_page);
		  help_page = help_path;
		}
	      else
		{
		  g_free (help_path);
		}
	    }

	  format_str = g_strdup_printf ("%%%ds%%%ds %%-20s %%s\n", 
					depth * 2, depth * 2 - 40);
	  g_print (format_str, 
		   "", label, "", help_page ? help_page : "");
	  g_free (format_str);
	  g_free (help_page);

	  if (GTK_MENU_ITEM (menu_item)->submenu)
	    menus_debug_recurse_menu (GTK_MENU_ITEM (menu_item)->submenu, 
				      depth + 1, full_path);

	  g_free (full_path);
	}
    }
}

static void
menus_debug_cmd_callback (GtkWidget *widget,
			  gpointer   data,
			  guint      action)
{
#if 0
  gint                  n_factories = 7;
  GtkItemFactory       *factories[7];
  GimpItemFactoryEntry *entries[7];

  GtkWidget *menu_item;
  gint       i;

  factories[0] = GTK_ITEM_FACTORY (toolbox_factory);
  factories[1] = GTK_ITEM_FACTORY (image_factory);
  factories[2] = GTK_ITEM_FACTORY (layers_factory);
  factories[3] = GTK_ITEM_FACTORY (channels_factory);
  factories[4] = GTK_ITEM_FACTORY (paths_factory);
  factories[5] = GTK_ITEM_FACTORY (load_factory);
  factories[6] = GTK_ITEM_FACTORY (save_factory);

  entries[0] = toolbox_entries;
  entries[1] = image_entries;
  entries[2] = layers_entries;
  entries[3] = channels_entries;
  entries[4] = paths_entries;
  entries[5] = load_entries;
  entries[6] = save_entries;
  
  /*  toolbox needs special treatment  */
  g_print ("%s\n", factories[0]->path);

  menu_item = gtk_item_factory_get_item (factories[0], "/File");
  if (menu_item && menu_item->parent && GTK_IS_MENU_BAR (menu_item->parent))
    menus_debug_recurse_menu (menu_item->parent, 1, factories[0]->path);

  g_print ("\n");

  for (i = 1; i < n_factories; i++)
    {
      g_print ("%s\n", factories[i]->path);

      menu_item = gtk_item_factory_get_item (factories[i], entries[i][0].entry.path);
      if (menu_item && menu_item->parent && GTK_IS_MENU (menu_item->parent))
	menus_debug_recurse_menu (menu_item->parent, 1, factories[i]->path);

      g_print ("\n");
    }
#endif
}

static void
menus_mem_profile_cmd_callback (GtkWidget *widget,
                                gpointer   data,
                                guint      action)
{
  extern gboolean gimp_debug_memsize;

  gimp_debug_memsize = TRUE;

  gimp_object_get_memsize (GIMP_OBJECT (data));

  gimp_debug_memsize = FALSE;
}

#endif  /*  ENABLE_DEBUG_ENTRIES  */
