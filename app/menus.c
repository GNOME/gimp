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
#include <stdlib.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>

#include "appenv.h"
#include "channels_dialog.h"
#include "colormaps.h"
#include "commands.h"
#include "dialog_handler.h"
#include "fileops.h"
#include "gimprc.h"
#include "interface.h"
#include "layers_dialog.h"
#include "menus.h"
#include "paths_dialog.h"
#include "paint_funcs.h"
#include "procedural_db.h"
#include "scale.h"
#include "tools.h"
#include "gdisplay.h"
#include "docindex.h"

#include "config.h"
#include "libgimp/gimpintl.h"
#include "libgimp/gimpenv.h"

#define MRU_MENU_ENTRY_SIZE (strlen (_("/File/MRU00 ")) + 1)
#define MRU_MENU_ACCEL_SIZE sizeof ("<control>0")

static void   menus_create_item    (GtkItemFactory       *item_factory,
				    GimpItemFactoryEntry *entry,
				    gpointer              callback_data,
				    guint                 callback_type);
static void   menus_create_items   (GtkItemFactory       *item_factory,
				    guint                 n_entries,
				    GimpItemFactoryEntry *entries,
				    gpointer              callback_data,
				    guint                 callback_type);
static void   menus_init           (void);
static gchar *menu_translate       (const gchar          *path,
				    gpointer              data);
static void   tearoff_cmd_callback (GtkWidget            *widget,
				    gpointer              callback_data,
				    guint                 callback_action);

static void   help_debug_cmd_callback (GtkWidget *widget,
				       gpointer   callback_data,
				       guint      callback_action);

static gchar* G_GNUC_UNUSED dummyMenus[] =
{
  N_("/File/MRU00 "),
  N_("/File/Dialogs"),
  N_("/View/Zoom"),
  N_("/Stack")
};

static GSList *last_opened_raw_filenames = NULL;

/*****  <Toolbox>  *****/

static GimpItemFactoryEntry toolbox_entries[] =
{
  /*  <Toolbox>/File  */

  { { N_("/File/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/File/New..."), "<control>N", file_new_cmd_callback, 0 },
    "file/dialogs/file_new.html", NULL },
  { { N_("/File/Open..."), "<control>O", file_open_cmd_callback, 0 },
    "file/open/dialogs/file_open.html", NULL },

  /*  <Toolbox>/File/Acquire  */

  { { N_("/File/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/File/Acquire/tearoff1"), NULL, NULL, 0, "<Tearoff>" },
    NULL, NULL },

  { { N_("/File/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/File/Preferences..."), NULL, file_pref_cmd_callback, 0 },
    "file/dialogs/preferences/preferences.html", NULL },

  /*  <Toolbox>/File/Dialogs  */

  { { N_("/File/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/File/Dialogs/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/File/Dialogs/Layers & Channels..."), "<control>L", dialogs_lc_cmd_callback, 0 },
    "file/dialogs/layers_and_channels.html", NULL },
  { { N_("/File/Dialogs/Tool Options..."), "<control><shift>T", dialogs_tool_options_cmd_callback, 0 },
    "file/dialogs/tool_options.html", NULL },

  { { N_("/File/Dialogs/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/File/Dialogs/Brushes..."), "<control><shift>B", dialogs_brushes_cmd_callback, 0 },
    "file/dialogs/brush_selection.html", NULL },
  { { N_("/File/Dialogs/Patterns..."), "<control><shift>P", dialogs_patterns_cmd_callback, 0 },
    "file/dialogs/pattern_selection.html", NULL },
  { { N_("/File/Dialogs/Gradients..."), "<control>G", dialogs_gradients_cmd_callback, 0 },
    "file/dialogs/gradient_selection.html", NULL },
  { { N_("/File/Dialogs/Palette..."), "<control>P", dialogs_palette_cmd_callback, 0 },
    "file/dialogs/palette_selection.html", NULL },
  { { N_("/File/Dialogs/Indexed Palette..."), NULL, dialogs_indexed_palette_cmd_callback, 0 },
    "file/dialogs/indexed_palette.html", NULL },

  { { N_("/File/Dialogs/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/File/Dialogs/Input Devices..."), NULL, dialogs_input_devices_cmd_callback, 0 },
    "file/dialogs/input_devices.html", NULL },
  { { N_("/File/Dialogs/Device Status..."), NULL, dialogs_device_status_cmd_callback, 0 },
    "file/dialogs/device_status.html", NULL },

  { { N_("/File/Dialogs/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/File/Dialogs/Document Index..."), NULL, raise_idea_callback, 0 },
    "file/dialogs/document_index.html", NULL },
  { { N_("/File/Dialogs/Error Console..."), NULL, dialogs_error_console_cmd_callback, 0 },
    "file/dialogs/error_console.html", NULL },
  { { N_("/File/Dialogs/Display Filters..."), NULL, dialogs_display_filters_cmd_callback, 0 },
    "file/dialogs/display_filters/display_filters.html", NULL },

  { { N_("/File/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/File/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },

  /*  <Toolbox>/Xtns  */

  { { N_("/Xtns/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/Xtns/Module Browser..."), NULL, dialogs_module_browser_cmd_callback, 0 },
    "dialogs/module_browser.html", NULL },

  { { N_("/Xtns/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },

  /*  <Toolbox>/Help  */

  { { N_("/Help"), NULL, NULL, 0, "<LastBranch>" },
    NULL, NULL },
  { { N_("/Help/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/Help/Help..."), "F1", help_help_cmd_callback, 0 },
    "help/dialogs/help.html", NULL },
  { { N_("/Help/Context Help..."), "<shift>F1", help_context_help_cmd_callback, 0 },
    "help/context_help.html", NULL },
  { { N_("/Help/Tip of the day..."), NULL, help_tips_cmd_callback, 0 },
    "help/dialogs/tip_of_the_day.html", NULL },
  { { N_("/Help/About..."), NULL, help_about_cmd_callback, 0 },
    "help/dialogs/about.html", NULL },
  { { N_("/Help/Dump Items (Debug)"), NULL, help_debug_cmd_callback, 0 },
    NULL, NULL }
};
static guint n_toolbox_entries = (sizeof (toolbox_entries) /
				  sizeof (toolbox_entries[0]));
static GtkItemFactory *toolbox_factory = NULL;

static GimpItemFactoryEntry file_menu_separator =
{ { N_("/File/---"), NULL, NULL, 0, "<Separator>" },
  NULL, NULL };

static GimpItemFactoryEntry toolbox_end =
{ { N_("/File/Quit"), "<control>Q", file_quit_cmd_callback, 0 },
  "file/quit.html", NULL };

/*****  <Image>  *****/

static GimpItemFactoryEntry image_entries[] =
{
  { { N_("/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },

  /*  <Image>/File  */

  { { N_("/File/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/File/New..."), "<control>N", file_new_cmd_callback, 1 },
    "file/dialogs/file_new.html", NULL },
  { { N_("/File/Open..."), "<control>O", file_open_cmd_callback, 0 },
    "file/open/dialogs/file_open.html", NULL },
  { { N_("/File/Save"), "<control>S", file_save_cmd_callback, 0 },
    "file/save/dialogs/file_save.html", NULL },
  { { N_("/File/Save as..."), NULL, file_save_as_cmd_callback, 0 },
    "file/save/dialogs/file_save.html", NULL },
  { { N_("/File/Revert"), NULL, file_revert_cmd_callback, 0 },
    "file/revert.html", NULL },

  { { N_("/File/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_( "/File/Close"), "<control>W", file_close_cmd_callback, 0 },
    "file/close.html", NULL },
  { { N_("/File/Quit"), "<control>Q", file_quit_cmd_callback, 0 },
    "file/quit.html", NULL },

  { { N_("/File/---moved"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },

  /*  <Image>/Edit  */

  { { N_("/Edit/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/Edit/Undo"), "<control>Z", edit_undo_cmd_callback, 0 },
    "edit/undo.html", NULL },
  { { N_("/Edit/Redo"), "<control>R", edit_redo_cmd_callback, 0 },
    "edit/redo.html", NULL },

  { { N_("/Edit/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/Edit/Cut"), "<control>X", edit_cut_cmd_callback, 0 },
    "edit/cut.html", NULL },
  { { N_("/Edit/Copy"), "<control>C", edit_copy_cmd_callback, 0 },
    "edit/copy.html", NULL },
  { { N_("/Edit/Paste"), "<control>V", edit_paste_cmd_callback, 0 },
    "edit/paste.html", NULL },
  { { N_("/Edit/Paste Into"), NULL, edit_paste_into_cmd_callback, 0 },
    "edit/paste_into.html", NULL },
  { { N_("/Edit/Paste As New"), NULL, edit_paste_as_new_cmd_callback, 0 },
    "edit/paste_as_new.html", NULL },

  /*  <Image>/Edit/Buffer  */

  { { N_("/Edit/Buffer/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/Edit/Buffer/Cut Named..."), "<control><shift>X", edit_named_cut_cmd_callback, 0 },
    "edit/dialogs/cut_named.html", NULL },
  { { N_("/Edit/Buffer/Copy Named..."), "<control><shift>C", edit_named_copy_cmd_callback, 0 },
    "edit/dialogs/copy_named.html", NULL },
  { { N_("/Edit/Buffer/Paste Named..."), "<control><shift>V", edit_named_paste_cmd_callback, 0 },
    "edit/dialogs/paste_named.html", NULL },

  { { N_("/Edit/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/Edit/Clear"), "<control>K", edit_clear_cmd_callback, 0 },
    "edit/clear.html", NULL },
  { { N_("/Edit/Fill"), "<control>period", edit_fill_cmd_callback, 0 },
    "edit/fill.html", NULL },
  { { N_("/Edit/Stroke"), NULL, edit_stroke_cmd_callback, 0 },
    "edit/stroke.html", NULL },

  { { N_("/Edit/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },

  /*  <Image>/Select  */
  
  { { N_("/Select/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/Select/Invert"), "<control>I", select_invert_cmd_callback, 0 },
    "select/invert.html", NULL },
  { { N_("/Select/All"), "<control>A", select_all_cmd_callback, 0 },
    "select/all.html", NULL },
  { { N_("/Select/None"), "<control><shift>A", select_none_cmd_callback, 0 },
    "select/none.html", NULL },
  { { N_("/Select/Float"), "<control><shift>L", select_float_cmd_callback, 0 },
    "select/float.html", NULL },

  { { N_("/Select/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/Select/Feather..."), "<control><shift>F", select_feather_cmd_callback, 0 },
    "select/dialogs/feather_selection.html", NULL },
  { { N_("/Select/Sharpen"), "<control><shift>H", select_sharpen_cmd_callback, 0 },
    "select/sharpen.html", NULL },
  { { N_("/Select/Shrink..."), NULL, select_shrink_cmd_callback, 0 },
    "select/dialogs/shrink_selection.html", NULL },
  { { N_("/Select/Grow..."), NULL, select_grow_cmd_callback, 0 },
    "select/dialogs/grow_selection.html", NULL },
  { { N_("/Select/Border..."), "<control><shift>B", select_border_cmd_callback, 0 },
    "select/dialogs/border_selection.html", NULL },

  { { N_("/Select/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/Select/Save To Channel"), NULL, select_save_cmd_callback, 0 },
    "select/save_to_channel.html", NULL },

  /*  <Image>/View  */

  { { N_("/View/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/View/Zoom In"), "equal", view_zoomin_cmd_callback, 0 },
    "view/zoom.html", NULL },
  { { N_("/View/Zoom Out"), "minus", view_zoomout_cmd_callback, 0 },
    "view/zoom.html", NULL },

  /*  <Image>/View/Zoom  */

  { { N_("/View/Zoom/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/View/Zoom/16:1"), NULL, view_zoom_16_1_cmd_callback, 0 },
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/8:1"), NULL, view_zoom_8_1_cmd_callback, 0 },
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/4:1"), NULL, view_zoom_4_1_cmd_callback, 0 },
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/2:1"), NULL, view_zoom_2_1_cmd_callback, 0 },
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/1:1"), "1", view_zoom_1_1_cmd_callback, 0 },
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/1:2"), NULL, view_zoom_1_2_cmd_callback, 0 },
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/1:4"), NULL, view_zoom_1_4_cmd_callback, 0 },
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/1:8"), NULL, view_zoom_1_8_cmd_callback, 0 },
    "view/zoom.html", NULL },
  { { N_("/View/Zoom/1:16"), NULL, view_zoom_1_16_cmd_callback, 0 },
    "view/zoom.html", NULL },

  { { N_("/View/Dot for dot"), NULL, view_dot_for_dot_cmd_callback, 0, "<ToggleItem>" },
    "view/dot_for_dot.html", NULL },

  { { N_("/View/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/View/Info Window..."), "<control><shift>I", view_info_window_cmd_callback, 0 },
    "view/dialogs/info_window.html", NULL },
  { { N_("/View/Nav. Window..."), "<control><shift>N", view_nav_window_cmd_callback, 0 },
    "view/dialogs/navigation_window.html", NULL },
  { { N_("/View/Undo history..."), NULL, view_undo_history_cmd_callback, 0},
    "view/dialogs/undo_history.html", NULL },

  { { N_("/View/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/View/Toggle Selection"), "<control>T", view_toggle_selection_cmd_callback, 0, "<ToggleItem>" },
    "view/toggle_selection.html", NULL },
  { { N_("/View/Toggle Rulers"), "<control><shift>R", view_toggle_rulers_cmd_callback, 0, "<ToggleItem>" },
    "view/toggle_rulers.html", NULL },
  { { N_("/View/Toggle Statusbar"), "<control><shift>S", view_toggle_statusbar_cmd_callback, 0, "<ToggleItem>" },
    "view/toggle_statusbar.html", NULL },
  { { N_("/View/Toggle Guides"), "<control><shift>T", view_toggle_guides_cmd_callback, 0, "<ToggleItem>" },
    "view/toggle_guides.html", NULL },
  { { N_("/View/Snap To Guides"), NULL, view_snap_to_guides_cmd_callback, 0, "<ToggleItem>" },
    "view/snap_to_guides.html", NULL },

  { { N_("/View/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/View/New View"), NULL, view_new_view_cmd_callback, 0 },
    "view/new_view.html", NULL },
  { { N_("/View/Shrink Wrap"), "<control>E", view_shrink_wrap_cmd_callback, 0 },
    "view/shrink_wrap.html", NULL },

  { { N_("/View/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },

  /*  <Image>/Image  */

  { { N_("/Image/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },

  /*  <Image>/Image/Mode  */

  { { N_("/Image/Mode/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/Image/Mode/RGB"), "<alt>R", image_convert_rgb_cmd_callback, 0 },
    "image/mode/convert_to_rgb.html", NULL },
  { { N_("/Image/Mode/Grayscale"), "<alt>G", image_convert_grayscale_cmd_callback, 0 },
    "image/mode/convert_to_grayscale.html", NULL },
  { { N_("/Image/Mode/Indexed..."), "<alt>I", image_convert_indexed_cmd_callback, 0 },
    "image/mode/dialogs/convert_to_indexed.html", NULL },

  { { N_("/Image/Mode/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },

  /*  <Image>/Image/Colors  */

  { { N_("/Image/Colors/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },

  { { N_("/Image/Colors/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/Image/Colors/Desaturate"), NULL, image_desaturate_cmd_callback, 0 },
    "image/colors/desaturate.html", NULL },
  { { N_("/Image/Colors/Invert"), NULL, image_invert_cmd_callback, 0 },
    "image/colors/invert.html", NULL },

  { { N_("/Image/Colors/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },

  /*  <Image>/Image/Colors/Auto  */

  { { N_("/Image/Colors/Auto/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/Image/Colors/Auto/Equalize"), NULL, image_equalize_cmd_callback, 0 },
    "image/colors/auto/equalize.html", NULL },

  { { N_("/Image/Colors/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },

  /*  <Image>/Image/Alpha  */

  { { N_("/Image/Alpha/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/Image/Alpha/Add Alpha Channel"), NULL, layers_add_alpha_channel_cmd_callback, 0 },
    "layers/add_alpha_channel.html", NULL },

  /*  <Image>/Image/Transforms  */

  { { N_("/Image/Transforms/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/Image/Transforms/Offset..."), "<control><shift>O", image_offset_cmd_callback, 0 },
    "image/transforms/dialogs/offset.html", NULL },
  { { N_("/Image/Transforms/Rotate/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/Image/Transforms/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },

  { { N_("/Image/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/Image/Canvas Size..."), NULL, image_resize_cmd_callback, 0 },
    "image/dialogs/canvas_size.html", NULL },
  { { N_("/Image/Scale Image..."), NULL, image_scale_cmd_callback, 0 },
    "image/dialogs/scale_image.html", NULL },
  { { N_("/Image/Duplicate"), "<control>D", image_duplicate_cmd_callback, 0 },
    "image/duplicate.html", NULL },

  { { N_("/Image/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },

  /*  <Image>/Layers  */

  { { N_("/Layers/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/Layers/Layers & Channels..."), "<control>L", dialogs_lc_cmd_callback, 0 },
    "dialogs/layers_and_channels.html", NULL },

  /*  <Image>/Layers/Stack  */

  { { N_("/Layers/Stack/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/Layers/Stack/Previous Layer"), "Prior", layers_previous_cmd_callback, 0 },
    "layers/stack/stack.html#previous_layer", NULL },
  { { N_("/Layers/Stack/Next Layer"), "Next", layers_next_cmd_callback, 0 },
    "layers/stack/stack.html#next_layer", NULL },
  { { N_("/Layers/Stack/Raise Layer"), "<shift>Prior", layers_raise_cmd_callback, 0 },
    "layers/stack/stack.html#raise_layer", NULL },
  { { N_("/Layers/Stack/Lower Layer"), "<shift>Next", layers_lower_cmd_callback, 0 },
    "layers/stack/stack.html#lower_layer", NULL },
  { { N_("/Layers/Stack/Layer to Top"), "<control>Prior", layers_raise_to_top_cmd_callback, 0 },
    "layers/stack/stack.html#layer_to_top", NULL },
  { { N_("/Layers/Stack/Layer to Bottom"), "<control>Next", layers_lower_to_bottom_cmd_callback, 0 },
    "layers/stack/stack.html#layer_to_bottom", NULL },
  { { N_("/Layers/Stack/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },

  /*  <Image>/Layers/Rotate  */

  { { N_("/Layers/Rotate/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },

  { { N_("/Layers/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/Layers/Anchor Layer"), "<control>H", layers_anchor_cmd_callback, 0 },
    "layers/anchor_layer.html", NULL },
  { { N_("/Layers/Merge Visible Layers..."), "<control>M", layers_merge_cmd_callback, 0 },
    "layers/dialogs/merge_visible_layers.html", NULL },
  { { N_("/Layers/Flatten Image"), NULL, layers_flatten_cmd_callback, 0 },
    "layers/flatten_image.html", NULL },

  { { N_("/Layers/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/Layers/Mask To Selection"), NULL, layers_mask_select_cmd_callback, 0 },
    "layers/mask_to_selection.html", NULL },

  { { N_("/Layers/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/Layers/Add Alpha Channel"), NULL, layers_add_alpha_channel_cmd_callback, 0 },
    "layers/add_alpha_channel.html", NULL },
  { { N_("/Layers/Alpha To Selection"), NULL, layers_alpha_select_cmd_callback, 0 },
    "layers/alpha_to_selection.html", NULL },

  { { N_("/Layers/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },

  /*  <Image>/Tools  */

  { { N_("/Tools/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/Tools/Toolbox"), NULL, toolbox_raise_callback, 0 },
    "toolbox/toolbox.html", NULL },
  { { N_("/Tools/Default Colors"), "D", tools_default_colors_cmd_callback, 0 },
    "toolbox/toolbox.html#default_colors", NULL },
  { { N_("/Tools/Swap Colors"), "X", tools_swap_colors_cmd_callback, 0 },
    "toolbox/toolbox.html#swap_colors", NULL },

  { { N_("/Tools/---"), NULL, NULL, 0, "<Separator>" },  
    NULL, NULL },

  /*  <Image>/Filters  */

  { { N_("/Filters/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/Filters/Repeat last"), "<alt>F", filters_repeat_cmd_callback, 0x0 },
    "filters/repeat_last.html", NULL },
  { { N_("/Filters/Re-show last"), "<alt><shift>F", filters_repeat_cmd_callback, 0x1 },
    "filters/reshow_last.html", NULL },

  { { N_("/Filters/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },

  /*  <Image>/Script-Fu  */

  { { N_("/Script-Fu/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },

  /*  <Image>/Dialogs  */

  { { N_("/Dialogs/tearoff1"), NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
    NULL, NULL },
  { { N_("/Dialogs/Layers & Channels..."), "<control>L", dialogs_lc_cmd_callback, 0 },
    "dialogs/layers_and_channels.html", NULL },
  { { N_("/Dialogs/Tool Options..."), NULL, dialogs_tool_options_cmd_callback, 0 },
    "dialogs/tool_options.html", NULL },

  { { N_("/Dialogs/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/Dialogs/Brushes..."), "<control><shift>B", dialogs_brushes_cmd_callback, 0 },
    "dialogs/brush_selection.html", NULL },
  { { N_("/Dialogs/Patterns..."), "<control><shift>P", dialogs_patterns_cmd_callback, 0 },
    "dialogs/pattern_selection.html", NULL },
  { { N_("/Dialogs/Gradients..."), "<control>G", dialogs_gradients_cmd_callback, 0 },
    "dialogs/gradient_selection.html", NULL },
  { { N_("/Dialogs/Palette..."), "<control>P", dialogs_palette_cmd_callback, 0 },
    "dialogs/palette_selection.html", NULL },
  { { N_("/Dialogs/Indexed Palette..."), NULL, dialogs_indexed_palette_cmd_callback, 0 },
    "dialogs/indexed_palette.html", NULL },

  { { N_("/Dialogs/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/Dialogs/Input Devices..."), NULL, dialogs_input_devices_cmd_callback, 0 },
    "dialogs/input_devices.html", NULL },
  { { N_("/Dialogs/Device Status..."), NULL, dialogs_device_status_cmd_callback, 0 },
    "dialogs/device_status.html", NULL },

  { { N_("/Dialogs/---"), NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/Dialogs/Document Index..."), NULL, raise_idea_callback, 0 },
    "dialogs/document_index.html", NULL },
  { { N_("/Dialogs/Error Console..."), NULL, dialogs_error_console_cmd_callback, 0 },
    "dialogs/error_console.html", NULL },
  { { N_("/Dialogs/Display Filters..."), NULL, dialogs_display_filters_cmd_callback, 0 },
    "dialogs/display_filters/display_filters.html", NULL },
};
static guint n_image_entries = (sizeof (image_entries) /
				sizeof (image_entries[0]));
static GtkItemFactory *image_factory = NULL;

/*****  <Load>  *****/

static GimpItemFactoryEntry load_entries[] =
{
  { { N_("/Automatic"), NULL, file_load_by_extension_callback, 0 },
    "open_by_extension.html", NULL },

  { { "/---", NULL, NULL, 0, "<Separator>" },
    NULL, NULL }
};
static guint n_load_entries = (sizeof (load_entries) /
			       sizeof (load_entries[0]));
static GtkItemFactory *load_factory = NULL;
  
/*****  <Save>  *****/

static GimpItemFactoryEntry save_entries[] =
{
  { { N_("/By extension"), NULL, file_save_by_extension_callback, 0 },
    "save_by_extension.html", NULL },

  { { "/---", NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
};
static guint n_save_entries = (sizeof (save_entries) /
			       sizeof (save_entries[0]));
static GtkItemFactory *save_factory = NULL;

/*****  <Layers>  *****/

static GimpItemFactoryEntry layers_entries[] =
{
  { { N_("/New Layer..."), "<control>N", layers_dialog_new_layer_callback, 0 },
    "dialogs/new_layer.html", NULL },

  /*  <Layers>/Stack  */

  { { N_("/Stack/Previous Layer"), "Prior", layers_dialog_previous_layer_callback, 0 },
    "stack/stack.html#previous_layer", NULL },
  { { N_("/Stack/Next Layer"), "Next", layers_dialog_next_layer_callback, 0 },
    "stack/stack.html#next_layer", NULL },
  { { N_("/Stack/Raise Layer"), "<shift>Prior", layers_dialog_raise_layer_callback, 0 },
    "stack/stack.html#raise_layer", NULL },
  { { N_("/Stack/Lower Layer"), "<shift>Next", layers_dialog_lower_layer_callback, 0 },
    "stack/stack.html#lower_layer", NULL },
  { { N_("/Stack/Layer to Top"), "<control>Prior", layers_dialog_raise_layer_to_top_callback, 0 },
    "stack/stack.html#later_to_top", NULL },
  { { N_("/Stack/Layer to Bottom"), "<control>Next", layers_dialog_lower_layer_to_bottom_callback, 0 },
    "stack/stack.html#layer_to_bottom", NULL },

  { { N_("/Duplicate Layer"), "<control>C", layers_dialog_duplicate_layer_callback, 0 },
    "duplicate_layer.html", NULL },
  { { N_("/Anchor Layer"), "<control>H", layers_dialog_anchor_layer_callback, 0 },
    "anchor_layer.html", NULL },
  { { N_("/Delete Layer"), "<control>X", layers_dialog_delete_layer_callback, 0 },
    "delete_layer.html", NULL },

  { { "/---", NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/Layer Boundary Size..."), "<control>R", layers_dialog_resize_layer_callback, 0 },
    "dialogs/resize_layer.html", NULL },
  { { N_("/Scale Layer..."), "<control>S", layers_dialog_scale_layer_callback, 0 },
    "dialogs/scale_layer.html", NULL },

  { { "/---", NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/Merge Visible Layers..."), "<control>M", layers_dialog_merge_layers_callback, 0 },
    "dialogs/merge_visible_layers.html", NULL },
  { { N_("/Merge Down"), "<control><shift>M", layers_dialog_merge_down_callback, 0 },
    "merge_down.html", NULL },
  { { N_("/Flatten Image"), NULL, layers_dialog_flatten_image_callback, 0 },
    "flatten_image.html", NULL },

  { { "/---", NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/Add Layer Mask..."), NULL, layers_dialog_add_layer_mask_callback, 0 },
    "dialogs/add_mask.html", NULL },
  { { N_("/Apply Layer Mask..."), NULL, layers_dialog_apply_layer_mask_callback, 0 },
    "dialogs/apply_mask.html", NULL },
  { { N_("/Mask to Selection"), NULL, layers_dialog_mask_select_callback, 0 },
    "mask_to_selection.html", NULL },

  { { "/---", NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/Add Alpha Channel"), NULL, layers_dialog_add_alpha_channel_callback, 0 },
    "add_alpha_channel.html", NULL },
  { { N_("/Alpha to Selection"), NULL, layers_dialog_alpha_select_callback, 0 },
    "alpha_to_selection.html", NULL }
};
static guint n_layers_entries = (sizeof (layers_entries) /
				 sizeof (layers_entries[0]));
static GtkItemFactory *layers_factory = NULL;

/*****  <Channels>  *****/

static GimpItemFactoryEntry channels_entries[] =
{
  { { N_("/New Channel..."), "<control>N", channels_dialog_new_channel_callback, 0 },
    "dialogs/new_channel.html", NULL },
  { { N_("/Raise Channel"), "<control>F", channels_dialog_raise_channel_callback, 0 },
    "raise_channel.html", NULL },
  { { N_("/Lower Channel"), "<control>B", channels_dialog_lower_channel_callback, 0 },
    "lower_channel.html", NULL },
  { { N_("/Duplicate Channel"), "<control>C", channels_dialog_duplicate_channel_callback, 0 },
    "duplicate_channel.html", NULL },

  { { "/---", NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/Channel to Selection"), "<control>S", channels_dialog_channel_to_sel_callback, 0 },
    "channel_to_selection.html", NULL },
  { { N_("/Add to Selection"), NULL, channels_dialog_add_channel_to_sel_callback, 0 },
    "channel_to_selection.html#add", NULL },
  { { N_("/Subtract From Selection"), NULL, channels_dialog_sub_channel_from_sel_callback, 0 },
    "channel_to_selection.html#subtract", NULL },
  { { N_("/Intersect With Selection"), NULL, channels_dialog_sub_channel_from_sel_callback, 0 },
    "channel_to_selection.html#intersect", NULL },

  { { "/---", NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/Delete Channel"), "<control>X", channels_dialog_delete_channel_callback, 0 },
    "delete_channel.html", NULL }
};
static guint n_channels_entries = (sizeof (channels_entries) /
				   sizeof (channels_entries[0]));
static GtkItemFactory *channels_factory = NULL;

/*****  <Paths>  *****/

static GimpItemFactoryEntry paths_entries[] =
{
  { { N_("/New Path"), "<control>N", paths_dialog_new_path_callback, 0 },
    "new_path.html", NULL },
  { { N_("/Duplicate Path"), "<control>U", paths_dialog_dup_path_callback, 0 },
    "duplicate_path.html", NULL },
  { { N_("/Path to Selection"), "<control>S", paths_dialog_path_to_sel_callback, 0 },
    "path_to_selection.html", NULL },
  { { N_("/Selection to Path"), "<control>P", paths_dialog_sel_to_path_callback, 0 },
    "selection_to_path.html", NULL },
  { { N_("/Stroke Path"), "<control>T", paths_dialog_stroke_path_callback, 0 },
    "stroke_path.html", NULL },
  { { N_("/Delete Path"), "<control>X", paths_dialog_delete_path_callback, 0 },
    "delete_path.html", NULL },

  { { "/---", NULL, NULL, 0, "<Separator>" },
    NULL, NULL },
  { { N_("/Copy Path"), "<control>C", paths_dialog_copy_path_callback, 0 },
    "copy_path.html", NULL },
  { { N_("/Paste Path"), "<control>V", paths_dialog_paste_path_callback, 0 },
    "paste_path.html", NULL },
  { { N_("/Import Path..."), "<control>I", paths_dialog_import_path_callback, 0 },
    "dialogs/import_path.html", NULL },
  { { N_("/Export Path..."), "<control>E", paths_dialog_export_path_callback, 0 },
    "dialogs/export_path.html", NULL }
};
static guint n_paths_entries = (sizeof (paths_entries) /
				sizeof (paths_entries[0]));
static GtkItemFactory *paths_factory = NULL;

static gboolean initialize = TRUE;

void
menus_get_toolbox_menubar (GtkWidget     **menubar,
			   GtkAccelGroup **accel_group)
{
  if (initialize)
    menus_init ();
  
  if (menubar)
    *menubar = toolbox_factory->widget;
  if (accel_group)
    *accel_group = toolbox_factory->accel_group;
}

void
menus_get_image_menu (GtkWidget     **menu,
		      GtkAccelGroup **accel_group)
{
  if (initialize)
    menus_init ();

  if (menu)
    *menu = image_factory->widget;
  if (accel_group)
    *accel_group = image_factory->accel_group;
}

void
menus_get_load_menu (GtkWidget     **menu,
		     GtkAccelGroup **accel_group)
{
  if (initialize)
    menus_init ();

  if (menu)
    *menu = load_factory->widget;
  if (accel_group)
    *accel_group = load_factory->accel_group;
}

void
menus_get_save_menu (GtkWidget     **menu,
		     GtkAccelGroup **accel_group)
{
  if (initialize)
    menus_init ();

  if (menu)
    *menu = save_factory->widget;
  if (accel_group)
    *accel_group = save_factory->accel_group;
}

void
menus_get_layers_menu (GtkWidget     **menu,
		       GtkAccelGroup **accel_group)
{
  if (initialize)
    menus_init ();

  if (menu)
    *menu = layers_factory->widget;
  if (accel_group)
    *accel_group = layers_factory->accel_group;
}

void
menus_get_channels_menu (GtkWidget     **menu,
			 GtkAccelGroup **accel_group)
{
  if (initialize)
    menus_init ();

  if (menu)
    *menu = channels_factory->widget;
  if (accel_group)
    *accel_group = channels_factory->accel_group;
}

void
menus_get_paths_menu (GtkWidget     **menu,
		      GtkAccelGroup **accel_group)
{
  if (initialize)
    menus_init ();

  if (menu)
    *menu = paths_factory->widget;
  if (accel_group)
    *accel_group = paths_factory->accel_group;
}

void
menus_create_item_from_full_path (GimpItemFactoryEntry *entry,
				  gpointer              callback_data)
{
  GtkItemFactory *ifactory;
  GtkWidget *menu_item;
  gboolean redo_image_menu = FALSE;
  GString *tearoff_path;
  gchar *path;

  if (initialize)
    menus_init ();

  tearoff_path = g_string_new ("");

  if (! strncmp (entry->entry.path, "<Image>", 7))
    {
      gchar *p;
	
      p = strchr (entry->entry.path + 8, '/');
      while (p)
	{
	  g_string_assign (tearoff_path, entry->entry.path + 7);
	  g_string_truncate (tearoff_path,
			     p - entry->entry.path + 1 - 7);
	  g_string_append (tearoff_path, "tearoff1");
	    
	  if (! gtk_item_factory_get_widget (image_factory,
					     tearoff_path->str))
	    {
	      GimpItemFactoryEntry tearoff_entry =
	      { { tearoff_path->str, NULL, tearoff_cmd_callback, 0, "<Tearoff>" },
		NULL, NULL };

	      menus_create_item (image_factory, &tearoff_entry, NULL, 2);
	    }

	  p = strchr (p + 1, '/');
	} 

      redo_image_menu = TRUE;
    }

  g_string_free (tearoff_path, TRUE);

  path = entry->entry.path;
  ifactory = gtk_item_factory_from_path (path);

  if (!ifactory)
    {
      g_warning ("menus_create_item_from_full_path(): "
		 "entry refers to unknown item factory: \"%s\"", path);
      return;
    }

  while (*path != '>')
    path++;
  path++;

  entry->entry.path = path;

  menus_create_item (ifactory, entry, callback_data, 2);

  if (redo_image_menu)
    {
      menu_item = gtk_item_factory_get_widget (image_factory, 
	  				       "<Image>/File/---moved");
      if (menu_item && menu_item->parent)
	gtk_menu_reorder_child (GTK_MENU (menu_item->parent), menu_item, -1);

      menu_item = gtk_item_factory_get_widget (image_factory,
	  				       "<Image>/File/Close");
      if (menu_item && menu_item->parent)
	gtk_menu_reorder_child (GTK_MENU (menu_item->parent), menu_item, -1);

      menu_item = gtk_item_factory_get_widget (image_factory,
	  				       "<Image>/File/Quit");
      if (menu_item && menu_item->parent)
	gtk_menu_reorder_child (GTK_MENU (menu_item->parent), menu_item, -1);
    }
}

void
menus_reorder_plugins (void)
{
  static gchar *xtns_plugins[] = { "/Xtns/DB Browser...",
				   "/Xtns/PDB Explorer",
				   "/Xtns/Plugin Details...",
				   "/Xtns/Parasite Editor" };
  static gint n_xtns_plugins = (sizeof (xtns_plugins) /
				sizeof (xtns_plugins[0]));

  GtkWidget *menu_item;
  gint i, pos;

  pos = 2;

  for (i = 0; i < n_xtns_plugins; i++)
    {
      menu_item = gtk_item_factory_get_widget (toolbox_factory,
					       xtns_plugins[i]);
      if (menu_item && menu_item->parent)
	{
	  gtk_menu_reorder_child (GTK_MENU (menu_item->parent), menu_item, pos);
	  pos++;
	}
    }

  menu_item = gtk_item_factory_get_widget (image_factory,
					   "/Filters/Filter all Layers...");
  if (menu_item && menu_item->parent)
    gtk_menu_reorder_child (GTK_MENU (menu_item->parent), menu_item, 3);
}

static void
menus_tools_create (ToolInfo *tool_info)
{
  GimpItemFactoryEntry entry;

  entry.entry.path            = tool_info->menu_path;
  entry.entry.accelerator     = tool_info->menu_accel;
  entry.entry.callback        = tools_select_cmd_callback;
  entry.entry.callback_action = tool_info->tool_id;
  entry.entry.item_type       = NULL;
  entry.help_page             = tool_info->private_tip;
  entry.description           = NULL;

  menus_create_item (image_factory, &entry, (gpointer) tool_info, 2);
}

void
menus_set_sensitive (gchar    *path,
		     gboolean  sensitive)
{
  GtkItemFactory *ifactory;
  GtkWidget *widget = NULL;

  if (initialize)
    menus_init ();

  ifactory = gtk_item_factory_from_path (path);

  if (ifactory)
    {
      widget = gtk_item_factory_get_widget (ifactory, path);

      gtk_widget_set_sensitive (widget, sensitive);
    }

  if (!ifactory || !widget)
    g_warning ("Unable to set sensitivity for menu which doesn't exist:\n%s",
	       path);
}

/* The following function will enhance our localesystem because
   we don't need to have our menuentries twice in our catalog */ 

void
menus_set_sensitive_glue (gchar    *prepath,
			  gchar    *path,
			  gboolean  sensitive)
{
  gchar *menupath;

  menupath = g_strdup_printf ("%s%s", prepath, path);
  menus_set_sensitive (menupath, sensitive);
  g_free (menupath); 
}

static void
menus_set_state (gchar    *path,
		 gboolean  state)
{
  GtkItemFactory *ifactory;
  GtkWidget *widget = NULL;

  if (initialize)
    menus_init ();

  ifactory = gtk_item_factory_from_path (path);

  if (ifactory)
    {
      widget = gtk_item_factory_get_widget (ifactory, path);

      if (widget && GTK_IS_CHECK_MENU_ITEM (widget))
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), state);
      else
	widget = NULL;
    }
  if (!ifactory || !widget)
    g_warning ("Unable to set state for menu which doesn't exist:\n%s\n",
	       path);
}

void
menus_set_state_glue (gchar    *prepath,
		      gchar    *path,
		      gboolean  state)
{
  gchar *menupath;

  menupath = g_strdup_printf ("%s%s", prepath, path);
  menus_set_state (menupath, state);
  g_free (menupath); 
}

void
menus_destroy (gchar *path)
{
  if (initialize)
    menus_init ();

  gtk_item_factories_path_delete (NULL, path);
}

void
menus_quit (void)
{
  gchar *filename;

  filename = gimp_personal_rc_file ("menurc");
  gtk_item_factory_dump_rc (filename, NULL, TRUE);
  g_free (filename);

  if (!initialize)
    {
      gtk_object_unref (GTK_OBJECT (toolbox_factory));
      gtk_object_unref (GTK_OBJECT (image_factory));
      gtk_object_unref (GTK_OBJECT (load_factory));
      gtk_object_unref (GTK_OBJECT (save_factory));
      gtk_object_unref (GTK_OBJECT (layers_factory));
      gtk_object_unref (GTK_OBJECT (channels_factory));
      gtk_object_unref (GTK_OBJECT (paths_factory));
    }
}

static void
menus_last_opened_cmd_callback (GtkWidget *widget,
                                gpointer   callback_data,
                                guint      num)
{
  gchar *filename, *raw_filename;
  guint num_entries;

  num_entries = g_slist_length (last_opened_raw_filenames); 
  if (num >= num_entries)
    return;

  raw_filename = ((GString *) g_slist_nth_data (last_opened_raw_filenames, num))->str;
  filename = g_basename (raw_filename);

  if (!file_open (raw_filename, raw_filename))
    g_message (_("Error opening file: %s\n"), raw_filename);
}

static void
menus_last_opened_update_labels (void)
{
  GSList    *filename_slist;
  GString   *entry_filename, *path;
  GtkWidget *widget;
  gint	     i;
  guint      num_entries;

  entry_filename = g_string_new ("");
  path = g_string_new ("");

  filename_slist = last_opened_raw_filenames;
  num_entries = g_slist_length (last_opened_raw_filenames); 

  for (i = 1; i <= num_entries; i++)
    {
      g_string_sprintf (entry_filename, "%d. %s", i,
			g_basename (((GString *) filename_slist->data)->str));

      g_string_sprintf (path, N_("/File/MRU%02d"), i);

      widget = gtk_item_factory_get_widget (toolbox_factory, path->str);
      if (widget != NULL) {
	gtk_widget_show (widget);

	gtk_label_set (GTK_LABEL (GTK_BIN (widget)->child),
		       entry_filename->str);
      }
      filename_slist = filename_slist->next;
    }

  g_string_free (entry_filename, TRUE);
  g_string_free (path, TRUE);
}

void
menus_last_opened_add (gchar *filename)
{
  GString   *raw_filename;
  GSList    *item;
  GtkWidget *widget;
  guint      num_entries;

  /* ignore the add if we've already got the filename on the list */
  item = last_opened_raw_filenames;
  while (item)
    {
      raw_filename = item->data;
      if (!strcmp (raw_filename->str, filename))
	return;
      item = g_slist_next (item);
    }

  num_entries = g_slist_length (last_opened_raw_filenames);

  if (num_entries == last_opened_size)
    {
      g_slist_remove_link (last_opened_raw_filenames, 
			   g_slist_last (last_opened_raw_filenames));
    }

  raw_filename = g_string_new (filename);
  last_opened_raw_filenames = g_slist_prepend (last_opened_raw_filenames,
					       raw_filename);

  if (num_entries == 0)
    {
      widget = gtk_item_factory_get_widget (toolbox_factory,
					    file_menu_separator.entry.path);
      gtk_widget_show (widget);
    }

  menus_last_opened_update_labels ();
}

static void
menus_init_mru (void)
{
  gchar		        *paths, *accelerators;
  gint		         i;
  GimpItemFactoryEntry  *last_opened_entries;
  GtkWidget	        *widget;
  
  last_opened_entries = g_new (GimpItemFactoryEntry, last_opened_size);

  paths = g_new (gchar, last_opened_size * MRU_MENU_ENTRY_SIZE);
  accelerators = g_new (gchar, 9 * MRU_MENU_ACCEL_SIZE);

  for (i = 0; i < last_opened_size; i++)
    {
      gchar *path, *accelerator;

      path = &paths[i * MRU_MENU_ENTRY_SIZE];
      if (i < 9)
        accelerator = &accelerators[i * MRU_MENU_ACCEL_SIZE];
      else
        accelerator = NULL;
    
      last_opened_entries[i].entry.path = path;
      last_opened_entries[i].entry.accelerator = accelerator;
      last_opened_entries[i].entry.callback =
	(GtkItemFactoryCallback) menus_last_opened_cmd_callback;
      last_opened_entries[i].entry.callback_action = i;
      last_opened_entries[i].entry.item_type = NULL;
      last_opened_entries[i].help_page = "file/last_opened.html";
      last_opened_entries[i].description = NULL;

      g_snprintf (path, MRU_MENU_ENTRY_SIZE, N_("/File/MRU%02d"), i + 1);
      if (accelerator != NULL)
	g_snprintf (accelerator, MRU_MENU_ACCEL_SIZE, "<control>%d", i + 1);
    }

  menus_create_items (toolbox_factory, last_opened_size,
		      last_opened_entries, NULL, 2);
  
  for (i=0; i < last_opened_size; i++)
    {
      widget = gtk_item_factory_get_widget (toolbox_factory,
					    last_opened_entries[i].entry.path);
      gtk_widget_hide (widget);
    }

  widget = gtk_item_factory_get_widget (toolbox_factory,
					file_menu_separator.entry.path);
  gtk_widget_hide (widget);

  g_free (paths);
  g_free (accelerators);
  g_free (last_opened_entries);
}

/*  This function gets called while browsing a menu created
 *  by a GtkItemFactory
 */
static gint
menus_item_key_press (GtkWidget   *widget,
		      GdkEventKey *kevent,
		      gpointer     data)
{
  GtkItemFactory *item_factory = NULL;
  GtkWidget      *active_menu_item = NULL;
  gchar *help_path = NULL;
  gchar *help_page = NULL;

  if (kevent->keyval != GDK_F1)
    return FALSE;

  gtk_signal_emit_stop_by_name (GTK_OBJECT (widget),
				"key_press_event");

  item_factory = (GtkItemFactory *) data;
  help_path = (gchar *) gtk_object_get_data (GTK_OBJECT (item_factory),
					     "help_path");

  active_menu_item = GTK_MENU_SHELL (widget)->active_menu_item;
  if (active_menu_item)
    help_page = (gchar *) gtk_object_get_data (GTK_OBJECT (active_menu_item),
					       "help_page");
  if (! help_page)
    help_page = "index.html";

  if (help_path && help_page)
    {
      gchar *help_string;

      help_string = g_strdup_printf ("%s/%s", help_path, help_page);
      gimp_help (help_string);
      g_free (help_string);
    }
  else
    {
      gimp_help (NULL);
    }

  return TRUE;
}

/*  set up the callback to catch the "F1" key  */
static void
menus_item_realize (GtkWidget *widget,
		    gpointer   data)
{
  if (GTK_IS_MENU_SHELL (widget->parent))
    {
      if (! gtk_object_get_data (GTK_OBJECT (widget->parent),
				 "menus_key_press_connected"))
	{
	  gtk_signal_connect (GTK_OBJECT (widget->parent), "key_press_event",
			      GTK_SIGNAL_FUNC (menus_item_key_press),
			      (gpointer) data);

	  gtk_object_set_data (GTK_OBJECT (widget->parent),
			       "menus_key_press_connected",
			       (gpointer) TRUE);
	}
    }
}

static void
menus_create_item (GtkItemFactory       *item_factory,
		   GimpItemFactoryEntry *entry,
		   gpointer              callback_data,
		   guint                 callback_type)
{
  GtkWidget *item;

  gtk_item_factory_create_item (item_factory,
				(GtkItemFactoryEntry *) entry,
				callback_data,
				callback_type);

  item = gtk_item_factory_get_item (item_factory,
				    ((GtkItemFactoryEntry *) entry)->path);

  if (item)
    {
      gtk_signal_connect_after (GTK_OBJECT (item), "realize",
				GTK_SIGNAL_FUNC (menus_item_realize),
				(gpointer) item_factory);

      gtk_object_set_data (GTK_OBJECT (item), "help_page",
			   (gpointer) entry->help_page);
    }
}

static void
menus_create_items (GtkItemFactory       *item_factory,
		    guint                 n_entries,
		    GimpItemFactoryEntry *entries,
		    gpointer              callback_data,
		    guint                 callback_type)
{
  gint i;

  for (i = 0; i < n_entries; i++)
    {
      menus_create_item (item_factory,
			 entries + i,
			 callback_data,
			 callback_type);
    }
}

static void
menus_init_toolbox (void)
{
  toolbox_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<Toolbox>", NULL);
  gtk_object_set_data (GTK_OBJECT (toolbox_factory), "help_path",
		       (gpointer) "toolbox");
  gtk_item_factory_set_translate_func (toolbox_factory, menu_translate,
	  			       NULL, NULL);
  menus_create_items (toolbox_factory, n_toolbox_entries,
		      toolbox_entries, NULL, 2);
  menus_init_mru ();

  menus_create_items (toolbox_factory, 1,
		      &file_menu_separator, NULL, 2);

  menus_create_items (toolbox_factory, 1,
		      &toolbox_end, NULL, 2);
}

static void
menus_init (void)
{
  gint i;

  if (initialize)
    {
      GtkWidget *menu_item;
      gchar *filename;

      initialize = FALSE;

      menus_init_toolbox ();

      image_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<Image>", NULL);
      gtk_object_set_data (GTK_OBJECT (image_factory), "help_path",
			   (gpointer) "image");
      gtk_item_factory_set_translate_func (image_factory,
	  				   menu_translate,
	  				   NULL, NULL);
      menus_create_items (image_factory,
			  n_image_entries,
			  image_entries,
			  NULL, 2);

      load_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<Load>", NULL);
      gtk_object_set_data (GTK_OBJECT (load_factory), "help_path",
			   (gpointer) "open");
      gtk_item_factory_set_translate_func (load_factory,
	  				   menu_translate,
	  				   NULL, NULL);
      menus_create_items (load_factory,
			  n_load_entries,
			  load_entries,
			  NULL, 2);

      save_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<Save>", NULL);
      gtk_object_set_data (GTK_OBJECT (save_factory), "help_path",
			   (gpointer) "save");
      gtk_item_factory_set_translate_func (save_factory,
	  				   menu_translate,
	  				   NULL, NULL);
      menus_create_items (save_factory,
			  n_save_entries,
			  save_entries,
			  NULL, 2);

      layers_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<Layers>", NULL);
      gtk_object_set_data (GTK_OBJECT (layers_factory), "help_path",
			   (gpointer) "layers");
      gtk_item_factory_set_translate_func (layers_factory,
	  				   menu_translate,
	  				   NULL, NULL);
      menus_create_items (layers_factory,
			  n_layers_entries,
			  layers_entries,
			  NULL, 2);

      channels_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<Channels>", NULL);
      gtk_object_set_data (GTK_OBJECT (channels_factory), "help_path",
			   (gpointer) "channels");
      gtk_item_factory_set_translate_func (channels_factory,
	  				   menu_translate,
	  				   NULL, NULL);
      menus_create_items (channels_factory,
			  n_channels_entries,
			  channels_entries,
			  NULL, 2);

      paths_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<Paths>", NULL);
      gtk_object_set_data (GTK_OBJECT (paths_factory), "help_path",
			   (gpointer) "paths");
      gtk_item_factory_set_translate_func (paths_factory,
	  				   menu_translate,
	  				   NULL, NULL);
      menus_create_items (paths_factory,
			  n_paths_entries,
			  paths_entries,
			  NULL, 2);

      for (i = 0; i < num_tools; i++)
	{
	  /* FIXME this need to use access functions to check a flag */
	  if (tool_info[i].menu_path)
	    menus_tools_create (&tool_info[i]);
	}

      menu_item = gtk_item_factory_get_widget (image_factory,
					       tool_info[POSTERIZE].menu_path);
      if (menu_item && menu_item->parent)
	gtk_menu_reorder_child (GTK_MENU (menu_item->parent), menu_item, 3);

      {
	static ToolType color_tools[] = { COLOR_BALANCE,
					  HUE_SATURATION,
					  BRIGHTNESS_CONTRAST,
					  THRESHOLD,
					  LEVELS,
					  CURVES };
	static gint n_color_tools = (sizeof (color_tools) /
				     sizeof (color_tools[0]));
	gint i, pos;

	pos = 1;

	for (i = 0; i < n_color_tools; i++)
	  {
	    menu_item =
	      gtk_item_factory_get_widget (image_factory,
					   tool_info[color_tools[i]].menu_path);
	    if (menu_item && menu_item->parent)
	      {
		gtk_menu_reorder_child (GTK_MENU (menu_item->parent),
					menu_item, pos);
		pos++;
	      }
	  }
      }

      filename = gimp_personal_rc_file ("menurc");
      gtk_item_factory_parse_rc (filename);
      g_free (filename);
    }
}

static gchar *
menu_translate (const gchar *path,
    		gpointer     data)
{
  static gchar menupath[256];
  gchar *retval;

  retval = gettext (path);
  if (!strcmp (path, retval))
    {
      strcpy (menupath, path);
      strncat (menupath, "/tearoff1", sizeof (menupath) - strlen (menupath) - 1);
      retval = gettext (menupath);
      if (strcmp (menupath, retval))
        {
          strcpy (menupath, retval);
          *(strrchr(menupath, '/')) = '\0';
          return menupath;
        }
      else
        {
          strcpy (menupath, "<Image>");
          strncat (menupath, path, sizeof (menupath) - sizeof ("<Image>"));
          retval = dgettext ("gimp-std-plugins", menupath) + strlen ("<Image>");
          if (!strcmp (path, retval))
            {
              strcpy (menupath, "<Toolbox>");
              strncat (menupath, path, sizeof (menupath) - sizeof ("<Toolbox>"));
              retval = dgettext ("gimp-std-plugins", menupath) + strlen ("<Toolbox>");
            }
        }
    }

  return retval;
}

static gint
tearoff_delete_cb (GtkWidget *widget, 
		   GdkEvent  *event,
		   gpointer   data)
{
  /* Unregister if dialog is deleted as well */
  dialog_unregister ((GtkWidget *) data);
  return TRUE; 
}

static void   
tearoff_cmd_callback (GtkWidget *widget,
		      gpointer   callback_data,
		      guint      callback_action)
{
  if (GTK_IS_TEAROFF_MENU_ITEM (widget))
    {
      GtkTearoffMenuItem *tomi = (GtkTearoffMenuItem *)widget;
      
      if (tomi->torn_off)
	{
	  GtkWidget *top = gtk_widget_get_toplevel(widget);

	  /* This should be a window */
	  if (!GTK_IS_WINDOW (top))
	    {
	      g_message(_("tearoff menu not in top level window"));
	    }
	  else
	    {
	      dialog_register (top);
	      gtk_signal_connect_object (GTK_OBJECT (top),  
					 "delete_event",
					 GTK_SIGNAL_FUNC (tearoff_delete_cb),
					 GTK_OBJECT (top));
	      
	      gtk_object_set_data (GTK_OBJECT (widget), "tearoff_menu_top",
				   top);
	    }
	}
      else
	{
	  GtkWidget *top;

	  top = (GtkWidget *) gtk_object_get_data (GTK_OBJECT (widget),
						   "tearoff_menu_top");

	  if (!top)
	    {
	      g_message (_("can't unregister tearoff menu top level window"));
	    }
	  else
	    {
	      dialog_unregister (top);
	    }
	}
    }
}

static void   
help_debug_cmd_callback (GtkWidget *widget,
			 gpointer   callback_data,
			 guint      callback_action)
{
  GtkItemFactory *factories[] = { toolbox_factory,
				  image_factory,
				  layers_factory,
				  channels_factory,
				  paths_factory,
				  load_factory,
				  save_factory };
  gint n_factories = (sizeof (factories) /
		      sizeof (factories[0]));

  GimpItemFactoryEntry *entries[] = { toolbox_entries,
				      image_entries,
				      layers_entries,
				      channels_entries,
				      paths_entries,
				      load_entries,
				      save_entries };

  gint num_entries[] = { n_toolbox_entries,
			 n_image_entries,
			 n_layers_entries,
			 n_channels_entries,
			 n_paths_entries,
			 n_load_entries,
			 n_save_entries };

  gchar *help_path;
  gint i, j;

  for (i = 0;  i < n_factories; i++)
    {
      help_path = gtk_object_get_data (GTK_OBJECT (factories[i]), "help_path");

      for (j = 0; j < num_entries[i]; j++)
	{
	  if (entries[i][j].help_page)
	    g_print ("%s%s\t\t(%s/%s)\n",
		     factories[i]->path,
		     entries[i][j].entry.path,
		     help_path,
		     entries[i][j].help_page);
	  else
	    g_print ("%s%s\n",
		     factories[i]->path,
		     entries[i][j].entry.path);
	}

      g_print ("\n");
    }
}
