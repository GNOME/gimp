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

#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpdisplayoptions.h"
#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimprender.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmenumodel.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpdisplayshell-scale.h"
#include "display/gimpimagewindow.h"

#include "actions.h"
#include "view-actions.h"
#include "view-commands.h"
#include "window-actions.h"
#include "window-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   view_actions_set_zoom          (GimpActionGroup   *group,
                                              GimpDisplayShell  *shell);
static void   view_actions_set_rotate        (GimpActionGroup   *group,
                                              GimpDisplayShell  *shell);
static void   view_actions_check_type_notify (GimpDisplayConfig *config,
                                              GParamSpec        *pspec,
                                              GimpActionGroup   *group);


static const GimpActionEntry view_actions[] =
{
  { "view-new", GIMP_ICON_WINDOW_NEW,
    NC_("view-action", "_New View"), NULL, { NULL },
    NC_("view-action", "Create another view on this image"),
    view_new_cmd_callback,
    GIMP_HELP_VIEW_NEW },

  { "view-close", GIMP_ICON_WINDOW_CLOSE,
    NC_("view-action",  "_Close View"), NULL, { "<primary>W", NULL },
    NC_("view-action", "Close the active image view"),
    view_close_cmd_callback,
    GIMP_HELP_FILE_CLOSE },

  { "view-scroll-center", GIMP_ICON_CENTER,
    NC_("view-action", "C_enter Image in Window"), NULL, { "<shift>J", NULL },
    NC_("view-action", "Scroll the image so that it is centered in the window"),
    view_scroll_center_cmd_callback,
    GIMP_HELP_VIEW_SCROLL_CENTER },

  { "view-zoom-fit-in", GIMP_ICON_ZOOM_FIT_BEST,
    NC_("view-action", "_Fit Image in Window"), NULL, { "<primary><shift>J", NULL },
    NC_("view-action", "Adjust the zoom ratio so that the image becomes fully visible"),
    view_zoom_fit_in_cmd_callback,
    GIMP_HELP_VIEW_ZOOM_FIT_IN },

  { "view-zoom-fill", GIMP_ICON_VIEW_ZOOM_FILL,
    NC_("view-action", "Fi_ll Window"), NULL, { NULL },
    NC_("view-action", "Adjust the zoom ratio so that the entire window is used"),
    view_zoom_fill_cmd_callback,
    GIMP_HELP_VIEW_ZOOM_FILL },

  { "view-zoom-selection", GIMP_ICON_SELECTION,
    NC_("view-action", "Zoom to _Selection"), NULL, { NULL },
    NC_("view-action", "Adjust the zoom ratio so that the selection fills the window"),
    view_zoom_selection_cmd_callback,
    GIMP_HELP_VIEW_ZOOM_SELECTION },

  { "view-zoom-revert", NULL,
    NC_("view-action", "Re_vert Zoom"), NULL, { "grave", NULL },
    NC_("view-action", "Restore the previous zoom level"),
    view_zoom_revert_cmd_callback,
    GIMP_HELP_VIEW_ZOOM_REVERT },

  { "view-rotate-other", NULL,
    NC_("view-action", "Othe_r rotation angle..."), NULL, { NULL },
    NC_("view-action", "Set a custom rotation angle"),
    view_rotate_other_cmd_callback,
    GIMP_HELP_VIEW_ROTATE_OTHER },

  { "view-flip-reset", GIMP_ICON_RESET,
    NC_("view-action", "_Reset Flipping"), NULL, { NULL },
    NC_("view-action",
        "Reset flipping to unflipped"),
    view_flip_reset_cmd_callback,
    GIMP_HELP_VIEW_FLIP_RESET },

  { "view-reset", GIMP_ICON_RESET,
    NC_("view-action", "_Reset Flip & Rotate"), NULL, { "exclam", NULL },
    NC_("view-action",
        "Reset flipping to unflipped and the angle of rotation to 0°"),
    view_reset_cmd_callback,
    GIMP_HELP_VIEW_RESET },

  { "view-navigation-window", GIMP_ICON_DIALOG_NAVIGATION,
    NC_("view-action", "Na_vigation Window"), NULL, { NULL },
    NC_("view-action", "Show an overview window for this image"),
    view_navigation_window_cmd_callback,
    GIMP_HELP_NAVIGATION_DIALOG },

  { "view-display-filters", GIMP_ICON_DISPLAY_FILTER,
    NC_("view-action", "Display _Filters..."), NULL, { NULL },
    NC_("view-action", "Configure filters applied to this view"),
    view_display_filters_cmd_callback,
    GIMP_HELP_DISPLAY_FILTER_DIALOG },

  { "view-color-management-reset", GIMP_ICON_RESET,
    NC_("view-action", "As in _Preferences"), NULL, { NULL },
    NC_("view-action",
        "Reset color management to what's configured in preferences"),
    view_color_management_reset_cmd_callback,
    GIMP_HELP_VIEW_COLOR_MANAGEMENT_RESET },

  { "view-shrink-wrap", GIMP_ICON_VIEW_SHRINK_WRAP,
    NC_("view-action", "Shrink _Wrap"), NULL, { "<primary>J", NULL },
    NC_("view-action", "Reduce the image window to the size of the image display"),
    view_shrink_wrap_cmd_callback,
    GIMP_HELP_VIEW_SHRINK_WRAP },

  { "view-open-display", NULL,
    NC_("view-action", "_Open Display..."), NULL, { NULL },
    NC_("view-action", "Connect to another display"),
    window_open_display_cmd_callback,
    NULL }
};

static const GimpToggleActionEntry view_toggle_actions[] =
{

  { "view-show-all", NULL,
    NC_("view-action", "Show _All"), NULL, { NULL },
    NC_("view-action", "Show full image content"),
    view_show_all_cmd_callback,
    FALSE,
    GIMP_HELP_VIEW_SHOW_ALL },

  { "view-dot-for-dot", NULL,
    NC_("view-action", "_Dot for Dot"), NULL, { NULL },
    NC_("view-action", "A pixel on the screen represents an image pixel"),
    view_dot_for_dot_cmd_callback,
    TRUE,
    GIMP_HELP_VIEW_DOT_FOR_DOT },

  { "view-color-management-enable", NULL,
    NC_("view-action", "_Color-Manage this View"), NULL, { NULL },
    NC_("view-action", "Use color management for this view"),
    view_color_management_enable_cmd_callback,
    TRUE,
    GIMP_HELP_VIEW_COLOR_MANAGE_VIEW },

  { "view-color-management-softproof", NULL,
    NC_("view-action", "_Proof Colors"), NULL, { NULL },
    NC_("view-action", "Use this view for soft-proofing"),
    view_color_management_softproof_cmd_callback,
    FALSE,
    GIMP_HELP_VIEW_SOFT_PROOF_VIEW },

  { "view-display-black-point-compensation", NULL,
    NC_("view-action", "_Black Point Compensation"), NULL, { NULL },
    NC_("view-action", "Use black point compensation for image display"),
    view_display_bpc_cmd_callback,
    TRUE,
    GIMP_HELP_VIEW_SOFT_PROOF_BLACK_POINT },

  { "view-softproof-gamut-check", NULL,
    NC_("view-action", "_Mark Out Of Gamut Colors"), NULL, { NULL },
    NC_("view-action", "When soft-proofing, mark colors which cannot "
        "be represented in the target color space"),
    view_softproof_gamut_check_cmd_callback,
    FALSE,
    GIMP_HELP_VIEW_MARK_OUT_OF_GAMUT },

  { "view-show-selection", NULL,
    NC_("view-action", "Show _Selection"), NULL, { "<primary>T", NULL },
    NC_("view-action", "Display the selection outline"),
    view_toggle_selection_cmd_callback,
    TRUE,
    GIMP_HELP_VIEW_SHOW_SELECTION },

  { "view-show-layer-boundary", NULL,
    NC_("view-action", "Show _Layer Boundary"), NULL, { NULL },
    NC_("view-action", "Draw a border around the active layer"),
    view_toggle_layer_boundary_cmd_callback,
    TRUE,
    GIMP_HELP_VIEW_SHOW_LAYER_BOUNDARY },

  { "view-show-canvas-boundary", NULL,
    NC_("view-action", "Show Canvas Bounda_ry"), NULL, { NULL },
    NC_("view-action", "Draw a border around the canvas"),
    view_toggle_canvas_boundary_cmd_callback,
    TRUE,
    GIMP_HELP_VIEW_SHOW_CANVAS_BOUNDARY },

  { "view-show-guides", NULL,
    NC_("view-action", "Show _Guides"), NULL, { "<primary><shift>T", NULL },
    NC_("view-action", "Display the image's guides"),
    view_toggle_guides_cmd_callback,
    TRUE,
    GIMP_HELP_VIEW_SHOW_GUIDES },

  { "view-show-grid", NULL,
    NC_("view-action", "S_how Grid"), NULL, { NULL },
    NC_("view-action", "Display the image's grid"),
    view_toggle_grid_cmd_callback,
    FALSE,
    GIMP_HELP_VIEW_SHOW_GRID },

  { "view-show-sample-points", NULL,
    NC_("view-action", "Sh_ow Sample Points"), NULL, { NULL },
    NC_("view-action", "Display the image's color sample points"),
    view_toggle_sample_points_cmd_callback,
    TRUE,
    GIMP_HELP_VIEW_SHOW_SAMPLE_POINTS },

  { "view-snap-to-guides", NULL,
    NC_("view-action", "Snap to Gu_ides"), NULL, { NULL },
    NC_("view-action", "Tool operations snap to guides"),
    view_snap_to_guides_cmd_callback,
    TRUE,
    GIMP_HELP_VIEW_SNAP_TO_GUIDES },

  { "view-snap-to-grid", NULL,
    NC_("view-action", "Sna_p to Grid"), NULL, { NULL },
    NC_("view-action", "Tool operations snap to the grid"),
    view_snap_to_grid_cmd_callback,
    FALSE,
    GIMP_HELP_VIEW_SNAP_TO_GRID },

  { "view-snap-to-canvas", NULL,
    NC_("view-action", "Snap to _Canvas Edges"), NULL, { NULL },
    NC_("view-action", "Tool operations snap to the canvas edges"),
    view_snap_to_canvas_cmd_callback,
    FALSE,
    GIMP_HELP_VIEW_SNAP_TO_CANVAS },

  { "view-snap-to-vectors", NULL,
    NC_("view-action", "Snap t_o Active Path"), NULL, { NULL },
    NC_("view-action", "Tool operations snap to the active path"),
    view_snap_to_path_cmd_callback,
    FALSE,
    GIMP_HELP_VIEW_SNAP_TO_PATH },

  { "view-snap-to-bbox", NULL,
    NC_("view-action", "Snap to _Bounding Boxes"), NULL, { NULL },
    NC_("view-action", "Tool operations snap to the bounding boxes"),
    view_snap_to_bbox_cmd_callback,
    FALSE,
    GIMP_HELP_VIEW_SNAP_TO_BBOX },

  { "view-snap-to-equidistance", NULL,
    NC_("view-action", "Snap to _Equidistance"), NULL, { NULL },
    NC_("view-action", "Tool operations snap to the equidistance between three bounding boxes"),
    view_snap_to_equidistance_cmd_callback,
    FALSE,
    GIMP_HELP_VIEW_SNAP_TO_EQUIDISTANCE },

  { "view-show-menubar", NULL,
    NC_("view-action", "Show _Menubar"), NULL, { NULL },
    NC_("view-action", "Show this window's menubar"),
    view_toggle_menubar_cmd_callback,
    TRUE,
    GIMP_HELP_VIEW_SHOW_MENUBAR },

  { "view-show-rulers", NULL,
    NC_("view-action", "Show R_ulers"), NULL, { "<primary><shift>R", NULL },
    NC_("view-action", "Show this window's rulers"),
    view_toggle_rulers_cmd_callback,
    TRUE,
    GIMP_HELP_VIEW_SHOW_RULERS },

  { "view-show-scrollbars", NULL,
    NC_("view-action", "Show Scroll_bars"), NULL, { NULL },
    NC_("view-action", "Show this window's scrollbars"),
    view_toggle_scrollbars_cmd_callback,
    TRUE,
    GIMP_HELP_VIEW_SHOW_SCROLLBARS },

  { "view-show-statusbar", NULL,
    NC_("view-action", "Show S_tatusbar"), NULL, { NULL },
    NC_("view-action", "Show this window's statusbar"),
    view_toggle_statusbar_cmd_callback,
    TRUE,
    GIMP_HELP_VIEW_SHOW_STATUSBAR },

  { "view-fullscreen", GIMP_ICON_VIEW_FULLSCREEN,
    NC_("view-action", "Fullscr_een"), NULL, { "F11", NULL },
    NC_("view-action", "Toggle fullscreen view"),
    view_fullscreen_cmd_callback,
    FALSE,
    GIMP_HELP_VIEW_FULLSCREEN }
};

static const GimpEnumActionEntry view_zoom_actions[] =
{
  { "view-zoom", NULL,
    NC_("view-zoom-action", "Set zoom factor"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },

  { "view-zoom-minimum", GIMP_ICON_ZOOM_OUT,
    NC_("view-zoom-action", "Zoom out as far as possible"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-maximum", GIMP_ICON_ZOOM_IN,
    NC_("view-zoom-action", "Zoom in as far as possible"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-out", GIMP_ICON_ZOOM_OUT,
    NC_("view-zoom-action", "Zoom _Out"), NULL, { "minus", "KP_Subtract", "ZoomOut", NULL },
    NC_("view-zoom-action", "Zoom out"),
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-in", GIMP_ICON_ZOOM_IN,
    NC_("view-zoom-action", "Zoom _In"), NULL, { "plus", "KP_Add", "ZoomIn", NULL },
    NC_("view-zoom-action", "Zoom in"),
    GIMP_ACTION_SELECT_NEXT, FALSE,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-out-skip", GIMP_ICON_ZOOM_OUT,
    NC_("view-zoom-action", "Zoom out a lot"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-in-skip", GIMP_ICON_ZOOM_IN,
    NC_("view-zoom-action", "Zoom in a lot"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    GIMP_HELP_VIEW_ZOOM_IN }
};

static const GimpEnumActionEntry view_zoom_explicit_actions[] =
{
  { "view-zoom-16-1", NULL,
    NC_("view-zoom-action", "Zoom 16:1 (1600%)"),
    NC_("view-zoom-action", "1_6:1  (1600%)"),
    { "5", "KP_5", NULL },
    NC_("view-zoom-action", "Zoom 16:1"),
    160000, FALSE,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-8-1", NULL,
    NC_("view-zoom-action", "Zoom 8:1 (800%)"),
    NC_("view-zoom-action", "_8:1  (800%)"),
    { "4", "KP_4", NULL },
    NC_("view-zoom-action", "Zoom 8:1"),
    80000, FALSE,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-4-1", NULL,
    NC_("view-zoom-action", "Zoom 4:1 (400%)"),
    NC_("view-zoom-action", "_4:1  (400%)"),
    { "3", "KP_3", NULL },
    NC_("view-zoom-action", "Zoom 4:1"),
    40000, FALSE,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-2-1", NULL,
    NC_("view-zoom-action", "Zoom 2:1 (200%)"),
    NC_("view-zoom-action", "_2:1  (200%)"),
    { "2", "KP_2", NULL },
    NC_("view-zoom-action", "Zoom 2:1"),
    20000, FALSE,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-1-1", GIMP_ICON_ZOOM_ORIGINAL,
    NC_("view-zoom-action", "Zoom 1:1 (100%)"),
    NC_("view-zoom-action", "_1:1  (100%)"),
    { "1", "KP_1", NULL },
    NC_("view-zoom-action", "Zoom 1:1"),
    10000, FALSE,
    GIMP_HELP_VIEW_ZOOM_100 },

  { "view-zoom-1-2", NULL,
    NC_("view-zoom-action", "Zoom 1:2 (50%)"),
    NC_("view-zoom-action", "1:_2  (50%)"),
    { "<shift>2", "<shift>KP_2", NULL },
    NC_("view-zoom-action", "Zoom 1:2"),
    5000, FALSE,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-1-4", NULL,
    NC_("view-zoom-action", "Zoom 1:4 (25%)"),
    NC_("view-zoom-action", "1:_4  (25%)"),
    { "<shift>3", "<shift>KP_3", NULL },
    NC_("view-zoom-action", "Zoom 1:4"),
    2500, FALSE,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-1-8", NULL,
    NC_("view-zoom-action", "Zoom 1:8 (12.5%)"),
    NC_("view-zoom-action", "1:_8  (12.5%)"),
    { "<shift>4", "<shift>KP_4", NULL },
    NC_("view-zoom-action", "Zoom 1:8"),
    1250, FALSE,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-1-16", NULL,
    NC_("view-zoom-action", "Zoom 1:16 (6.25%)"),
    NC_("view-zoom-action", "1:1_6  (6.25%)"),
    { "<shift>5", "<shift>KP_5", NULL },
    NC_("view-zoom-action", "Zoom 1:16"),
    625, FALSE,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-other", NULL,
    NC_("view-zoom-action", "Set a custom zoom factor"),
    NC_("view-zoom-action", "Othe_r zoom factor..."),
    { NULL },
    NC_("view-zoom-action", "Set a custom zoom factor"),
    0, TRUE,
    GIMP_HELP_VIEW_ZOOM_OTHER }
};

static const GimpToggleActionEntry view_flip_actions[] =
{
  { "view-flip-horizontally", GIMP_ICON_OBJECT_FLIP_HORIZONTAL,
    NC_("view-action", "Flip _Horizontally"), NULL, { NULL },
    NC_("view-action", "Flip the view horizontally"),
    view_flip_horizontally_cmd_callback,
    FALSE,
    GIMP_HELP_VIEW_FLIP },

  { "view-flip-vertically", GIMP_ICON_OBJECT_FLIP_VERTICAL,
    NC_("view-action", "Flip _Vertically"), NULL, { NULL },
    NC_("view-action", "Flip the view vertically"),
    view_flip_vertically_cmd_callback,
    FALSE,
    GIMP_HELP_VIEW_FLIP }
};

static const GimpEnumActionEntry view_rotate_absolute_actions[] =
{
  { "view-rotate-set-absolute", NULL,
    NC_("view-action", "Display Rotation Absolute Angle Set"),
    NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },

  { "view-rotate-reset", GIMP_ICON_RESET,
    NC_("view-action", "_Reset Rotate"), NULL, { NULL },
    NC_("view-action",
        "Reset the angle of rotation to 0°"),
    GIMP_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    GIMP_HELP_VIEW_ROTATE_RESET },
};

static const GimpEnumActionEntry view_rotate_relative_actions[] =
{
  { "view-rotate-15", GIMP_ICON_OBJECT_ROTATE_90,
    NC_("view-action", "Rotate 15° _clockwise"), NULL, { NULL },
    NC_("view-action", "Rotate the view 15 degrees to the right"),
    GIMP_ACTION_SELECT_NEXT, FALSE,
    GIMP_HELP_VIEW_ROTATE_15 },

  { "view-rotate-90", GIMP_ICON_OBJECT_ROTATE_90,
    NC_("view-action", "Rotate 90° _clockwise"), NULL, { NULL },
    NC_("view-action", "Rotate the view 90 degrees to the right"),
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    GIMP_HELP_VIEW_ROTATE_90 },

  { "view-rotate-180", GIMP_ICON_OBJECT_ROTATE_180,
    NC_("view-action", "Rotate _180°"), NULL, { NULL },
    NC_("view-action", "Turn the view upside-down"),
    GIMP_ACTION_SELECT_LAST, FALSE,
    GIMP_HELP_VIEW_ROTATE_180 },

  { "view-rotate-270", GIMP_ICON_OBJECT_ROTATE_270,
    NC_("view-action", "Rotate 90° counter-clock_wise"), NULL, { NULL },
    NC_("view-action", "Rotate the view 90 degrees to the left"),
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    GIMP_HELP_VIEW_ROTATE_270 },

  { "view-rotate-345", GIMP_ICON_OBJECT_ROTATE_270,
    NC_("view-action", "Rotate 15° counter-clock_wise"), NULL, { NULL },
    NC_("view-action", "Rotate the view 15 degrees to the left"),
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    GIMP_HELP_VIEW_ROTATE_345 }
};

static const GimpRadioActionEntry view_display_intent_actions[] =
{
  { "view-display-intent-perceptual", NULL,
    NC_("view-action", "_Perceptual"), NULL, { NULL },
    NC_("view-action", "Display rendering intent is perceptual"),
    GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
    GIMP_HELP_VIEW_SOFT_PROOF_RENDERING_INTENT },

  { "view-display-intent-relative-colorimetric", NULL,
    NC_("view-action", "_Relative Colorimetric"), NULL, { NULL },
    NC_("view-action", "Display rendering intent is relative colorimetric"),
    GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
    GIMP_HELP_VIEW_SOFT_PROOF_RENDERING_INTENT },

  { "view-display-intent-saturation", NULL,
    NC_("view-action", "_Saturation"), NULL, { NULL },
    NC_("view-action", "Display rendering intent is saturation"),
    GIMP_COLOR_RENDERING_INTENT_SATURATION,
    GIMP_HELP_VIEW_SOFT_PROOF_RENDERING_INTENT },

  { "view-display-intent-absolute-colorimetric", NULL,
    NC_("view-action", "_Absolute Colorimetric"), NULL, { NULL },
    NC_("view-action", "Display rendering intent is absolute colorimetric"),
    GIMP_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC,
    GIMP_HELP_VIEW_SOFT_PROOF_RENDERING_INTENT }
};

static const GimpEnumActionEntry view_padding_color_actions[] =
{
  { "view-padding-color-theme", NULL,
    NC_("view-padding-color", "From _Theme"), NULL, { NULL },
    NC_("view-padding-color", "Use the current theme's background color"),
    GIMP_CANVAS_PADDING_MODE_DEFAULT, FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR },

  { "view-padding-color-light-check", NULL,
    NC_("view-padding-color", "_Light Check Color"), NULL, { NULL },
    NC_("view-padding-color", "Use the light check color"),
    GIMP_CANVAS_PADDING_MODE_LIGHT_CHECK, FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR },

  { "view-padding-color-dark-check", NULL,
    NC_("view-padding-color", "_Dark Check Color"), NULL, { NULL },
    NC_("view-padding-color", "Use the dark check color"),
    GIMP_CANVAS_PADDING_MODE_DARK_CHECK, FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR },

  { "view-padding-color-custom", GIMP_ICON_PALETTE,
    NC_("view-padding-color", "_Custom Color..."), NULL, { NULL },
    NC_("view-padding-color", "Use an arbitrary color"),
    GIMP_CANVAS_PADDING_MODE_CUSTOM, FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR },

  { "view-padding-color-prefs", GIMP_ICON_RESET,
    NC_("view-padding-color", "As in _Preferences"), NULL, { NULL },
    NC_("view-padding-color",
        "Reset padding color to what's configured in preferences"),
    GIMP_CANVAS_PADDING_MODE_RESET, FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR }
};

static const GimpToggleActionEntry view_padding_color_toggle_actions[] =
{
  { "view-padding-color-in-show-all", NULL,
    NC_("view-padding-color", "Keep Padding in \"Show _All\" Mode"), NULL, { NULL },
    NC_("view-padding-color",
        "Keep canvas padding when \"View -> Show All\" is enabled"),
    view_padding_color_in_show_all_cmd_callback,
    FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR }
};

static const GimpEnumActionEntry view_scroll_horizontal_actions[] =
{
  { "view-scroll-horizontal", NULL,
    NC_("view-action", "Set horizontal scroll offset"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },

  { "view-scroll-left-border", NULL,
    NC_("view-action", "Scroll to left border"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },

  { "view-scroll-right-border", NULL,
    NC_("view-action", "Scroll to right border"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },

  { "view-scroll-left", NULL,
    NC_("view-action", "Scroll left"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },

  { "view-scroll-right", NULL,
    NC_("view-action", "Scroll right"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },

  { "view-scroll-page-left", NULL,
    NC_("view-action", "Scroll page left"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },

  { "view-scroll-page-right", NULL,
    NC_("view-action", "Scroll page right"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry view_scroll_vertical_actions[] =
{
  { "view-scroll-vertical", NULL,
    NC_("view-action", "Set vertical scroll offset"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },

  { "view-scroll-top-border", NULL,
    NC_("view-action", "Scroll to top border"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },

  { "view-scroll-bottom-border", NULL,
    NC_("view-action", "Scroll to bottom border"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },

  { "view-scroll-up", NULL,
    NC_("view-action", "Scroll up"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },

  { "view-scroll-down", NULL,
    NC_("view-action", "Scroll down"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },

  { "view-scroll-page-up", NULL,
    NC_("view-action", "Scroll page up"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },

  { "view-scroll-page-down", NULL,
    NC_("view-action", "Scroll page down"), NULL, { NULL }, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};


void
view_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "view-action",
                                 view_actions,
                                 G_N_ELEMENTS (view_actions));

  gimp_action_group_add_toggle_actions (group, "view-action",
                                        view_toggle_actions,
                                        G_N_ELEMENTS (view_toggle_actions));

  gimp_action_group_add_enum_actions (group, "view-zoom-action",
                                      view_zoom_actions,
                                      G_N_ELEMENTS (view_zoom_actions),
                                      view_zoom_cmd_callback);

  gimp_action_group_add_enum_actions (group, "view-zoom-action",
                                      view_zoom_explicit_actions,
                                      G_N_ELEMENTS (view_zoom_explicit_actions),
                                      view_zoom_explicit_cmd_callback);

  gimp_action_group_add_toggle_actions (group, "view-action",
                                        view_flip_actions,
                                        G_N_ELEMENTS (view_flip_actions));

  gimp_action_group_add_enum_actions (group, "view-action",
                                      view_rotate_absolute_actions,
                                      G_N_ELEMENTS (view_rotate_absolute_actions),
                                      view_rotate_absolute_cmd_callback);

  gimp_action_group_add_enum_actions (group, "view-action",
                                      view_rotate_relative_actions,
                                      G_N_ELEMENTS (view_rotate_relative_actions),
                                      view_rotate_relative_cmd_callback);

  gimp_action_group_add_radio_actions (group, "view-action",
                                       view_display_intent_actions,
                                       G_N_ELEMENTS (view_display_intent_actions),
                                       NULL,
                                       GIMP_COLOR_MANAGEMENT_DISPLAY,
                                       view_display_intent_cmd_callback);

  gimp_action_group_add_enum_actions (group, "view-padding-color",
                                      view_padding_color_actions,
                                      G_N_ELEMENTS (view_padding_color_actions),
                                      view_padding_color_cmd_callback);

  gimp_action_group_add_toggle_actions (group, "view-padding-color",
                                        view_padding_color_toggle_actions,
                                        G_N_ELEMENTS (view_padding_color_toggle_actions));

  gimp_action_group_add_enum_actions (group, "view-action",
                                      view_scroll_horizontal_actions,
                                      G_N_ELEMENTS (view_scroll_horizontal_actions),
                                      view_scroll_horizontal_cmd_callback);

  gimp_action_group_add_enum_actions (group, "view-action",
                                      view_scroll_vertical_actions,
                                      G_N_ELEMENTS (view_scroll_vertical_actions),
                                      view_scroll_vertical_cmd_callback);

  g_signal_connect_object (group->gimp->config, "notify::check-type",
                           G_CALLBACK (view_actions_check_type_notify),
                           group, 0);
  view_actions_check_type_notify (GIMP_DISPLAY_CONFIG (group->gimp->config),
                                  NULL, group);

  if (GIMP_IS_IMAGE_WINDOW (group->user_data) ||
      GIMP_IS_GIMP (group->user_data))
    {
      /*  add window actions only if the context of the group is
       *  the display itself or the global popup (not if the context
       *  is a dock)
       *  (see dock-actions.c)
       */
      window_actions_setup (group, GIMP_HELP_VIEW_CHANGE_SCREEN);
    }
}

void
view_actions_update (GimpActionGroup *group,
                     gpointer         data)
{
  GimpDisplay        *display           = action_data_get_display (data);
  GimpImage          *image             = NULL;
  GimpDisplayShell   *shell             = NULL;
  GimpDisplayOptions *options           = NULL;
  GimpColorConfig    *color_config      = NULL;
  gchar              *label             = NULL;
  gboolean            fullscreen        = FALSE;
  gboolean            revert_enabled    = FALSE;   /* able to revert zoom? */
  gboolean            flip_horizontally = FALSE;
  gboolean            flip_vertically   = FALSE;
  gboolean            cm                = FALSE;
  gboolean            sp                = FALSE;
  gboolean            d_bpc             = FALSE;
  gboolean            gamut             = FALSE;

  if (display)
    {
      GimpImageWindow *window;
      const gchar     *action = NULL;

      image  = gimp_display_get_image (display);
      shell  = gimp_display_get_shell (display);
      window = gimp_display_shell_get_window (shell);

      if (window)
        fullscreen = gimp_image_window_get_fullscreen (window);

      options = (image ?
                 (fullscreen ? shell->fullscreen_options : shell->options) :
                 shell->no_image_options);

      revert_enabled = gimp_display_shell_scale_can_revert (shell);

      flip_horizontally = shell->flip_horizontally;
      flip_vertically   = shell->flip_vertically;

      color_config = gimp_display_shell_get_color_config (shell);

      switch (gimp_color_config_get_mode (color_config))
        {
        case GIMP_COLOR_MANAGEMENT_OFF:
          break;

        case GIMP_COLOR_MANAGEMENT_DISPLAY:
          cm = (image != NULL);
          break;

        case GIMP_COLOR_MANAGEMENT_SOFTPROOF:
          cm = (image != NULL);
          sp = (image != NULL);
          break;
        }

      switch (gimp_color_config_get_display_intent (color_config))
        {
        case GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL:
          action = "view-display-intent-perceptual";
          break;

        case GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC:
          action = "view-display-intent-relative-colorimetric";
          break;

        case GIMP_COLOR_RENDERING_INTENT_SATURATION:
          action = "view-display-intent-saturation";
          break;

        case GIMP_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC:
          action = "view-display-intent-absolute-colorimetric";
          break;
        }

      gimp_action_group_set_action_active (group, action, TRUE);

      d_bpc  = gimp_color_config_get_display_bpc (color_config);
      gamut  = gimp_color_config_get_simulation_gamut_check (color_config);
    }

#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)
#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_SENSITIVE ("view-new",   image);
  SET_SENSITIVE ("view-close", image);

  SET_SENSITIVE ("view-show-all", image);
  SET_ACTIVE    ("view-show-all", display && shell->show_all);

  SET_SENSITIVE ("view-dot-for-dot", image);
  SET_ACTIVE    ("view-dot-for-dot", display && shell->dot_for_dot);

  SET_SENSITIVE ("view-scroll-center", image);

  SET_SENSITIVE ("view-zoom-revert", revert_enabled);
  if (revert_enabled)
    {
      label = g_strdup_printf (_("Re_vert Zoom (%d%%)"),
                               ROUND (shell->last_scale * 100));
      gimp_action_group_set_action_label (group, "view-zoom-revert", label);
      g_free (label);
    }
  else
    {
      gimp_action_group_set_action_label (group, "view-zoom-revert",
                                          _("Re_vert Zoom"));
    }

  SET_SENSITIVE ("view-zoom",              image);
  SET_SENSITIVE ("view-zoom-minimum",      image);
  SET_SENSITIVE ("view-zoom-maximum",      image);
  SET_SENSITIVE ("view-zoom-in",           image);
  SET_SENSITIVE ("view-zoom-in-skip",      image);
  SET_SENSITIVE ("view-zoom-out",          image);
  SET_SENSITIVE ("view-zoom-out-skip",     image);

  SET_SENSITIVE ("view-zoom-fit-in",       image);
  SET_SENSITIVE ("view-zoom-fill",         image);
  SET_SENSITIVE ("view-zoom-selection",    image);
  SET_SENSITIVE ("view-zoom-revert",       image);

  SET_SENSITIVE ("view-zoom-16-1",         image);
  SET_SENSITIVE ("view-zoom-8-1",          image);
  SET_SENSITIVE ("view-zoom-4-1",          image);
  SET_SENSITIVE ("view-zoom-2-1",          image);
  SET_SENSITIVE ("view-zoom-1-1",          image);
  SET_SENSITIVE ("view-zoom-1-2",          image);
  SET_SENSITIVE ("view-zoom-1-4",          image);
  SET_SENSITIVE ("view-zoom-1-8",          image);
  SET_SENSITIVE ("view-zoom-1-16",         image);
  SET_SENSITIVE ("view-zoom-other",        image);

  SET_SENSITIVE ("view-flip-horizontally", image);
  SET_ACTIVE    ("view-flip-horizontally", flip_horizontally);

  SET_SENSITIVE ("view-flip-reset",        image);
  SET_SENSITIVE ("view-flip-vertically",   image);
  SET_ACTIVE    ("view-flip-vertically",   flip_vertically);

  SET_SENSITIVE ("view-rotate-reset",      image);
  SET_SENSITIVE ("view-rotate-15",         image);
  SET_SENSITIVE ("view-rotate-345",        image);
  SET_SENSITIVE ("view-rotate-90",         image);
  SET_SENSITIVE ("view-rotate-180",        image);
  SET_SENSITIVE ("view-rotate-270",        image);
  SET_SENSITIVE ("view-rotate-other",      image);

  SET_SENSITIVE ("view-reset",             image);

  if (image)
    {
      view_actions_set_zoom (group, shell);
      view_actions_set_rotate (group, shell);
    }

  SET_SENSITIVE ("view-navigation-window", image);
  SET_SENSITIVE ("view-display-filters",   image);

  SET_SENSITIVE ("view-color-management-enable",                image);
  SET_ACTIVE    ("view-color-management-enable",                cm);
  SET_SENSITIVE ("view-color-management-softproof",             image);
  SET_ACTIVE    ("view-color-management-softproof",             sp);
  SET_SENSITIVE ("view-display-intent-perceptual",              cm);
  SET_SENSITIVE ("view-display-intent-relative-colorimetric",   cm);
  SET_SENSITIVE ("view-display-intent-saturation",              cm);
  SET_SENSITIVE ("view-display-intent-absolute-colorimetric",   cm);
  SET_SENSITIVE ("view-display-black-point-compensation",       cm);
  SET_ACTIVE    ("view-display-black-point-compensation",       d_bpc);
  SET_ACTIVE    ("view-softproof-gamut-check",                  gamut);
  SET_SENSITIVE ("view-color-management-reset",                 image);

  SET_SENSITIVE ("view-show-selection",       image);
  SET_ACTIVE    ("view-show-selection",       display && options->show_selection);
  SET_SENSITIVE ("view-show-layer-boundary",  image);
  SET_ACTIVE    ("view-show-layer-boundary",  display && options->show_layer_boundary);
  SET_SENSITIVE ("view-show-canvas-boundary", image && shell->show_all);
  SET_ACTIVE    ("view-show-canvas-boundary", display && options->show_canvas_boundary);
  SET_SENSITIVE ("view-show-guides",          image);
  SET_ACTIVE    ("view-show-guides",          display && options->show_guides);
  SET_SENSITIVE ("view-show-grid",            image);
  SET_ACTIVE    ("view-show-grid",            display && options->show_grid);
  SET_SENSITIVE ("view-show-sample-points",   image);
  SET_ACTIVE    ("view-show-sample-points",   display && options->show_sample_points);

  SET_SENSITIVE ("view-snap-to-guides",       image);
  SET_ACTIVE    ("view-snap-to-guides",       display && options->snap_to_guides);
  SET_SENSITIVE ("view-snap-to-grid",         image);
  SET_ACTIVE    ("view-snap-to-grid",         display && options->snap_to_grid);
  SET_SENSITIVE ("view-snap-to-canvas",       image);
  SET_ACTIVE    ("view-snap-to-canvas",       display && options->snap_to_canvas);
  SET_SENSITIVE ("view-snap-to-vectors",      image);
  SET_ACTIVE    ("view-snap-to-vectors",      display && options->snap_to_path);
  SET_SENSITIVE ("view-snap-to-bbox",         image);
  SET_ACTIVE    ("view-snap-to-bbox",         display && options->snap_to_bbox);
  SET_SENSITIVE ("view-snap-to-equidistance", image);
  SET_ACTIVE    ("view-snap-to-equidistance", display && options->snap_to_equidistance);

  SET_SENSITIVE ("view-padding-color-theme",       image);
  SET_SENSITIVE ("view-padding-color-light-check", image);
  SET_SENSITIVE ("view-padding-color-dark-check",  image);
  SET_SENSITIVE ("view-padding-color-custom",      image);
  SET_SENSITIVE ("view-padding-color-prefs",       image);

  SET_SENSITIVE ("view-padding-color-in-show-all", image);
  SET_ACTIVE    ("view-padding-color-in-show-all", display && options->padding_in_show_all);

  SET_SENSITIVE ("view-show-menubar",    image);
  SET_ACTIVE    ("view-show-menubar",    display && options->show_menubar);
  SET_SENSITIVE ("view-show-rulers",     image);
  SET_ACTIVE    ("view-show-rulers",     display && options->show_rulers);
  SET_SENSITIVE ("view-show-scrollbars", image);
  SET_ACTIVE    ("view-show-scrollbars", display && options->show_scrollbars);
  SET_SENSITIVE ("view-show-statusbar",  image);
  SET_ACTIVE    ("view-show-statusbar",  display && options->show_statusbar);

  SET_SENSITIVE ("view-shrink-wrap", image);
  SET_ACTIVE    ("view-fullscreen",  display && fullscreen);

  if (GIMP_IS_IMAGE_WINDOW (group->user_data) ||
      GIMP_IS_GIMP (group->user_data))
    {
      GtkWidget *window = NULL;

      if (shell)
        window = gtk_widget_get_toplevel (GTK_WIDGET (shell));

      /*  see view_actions_setup()  */
      if (GTK_IS_WINDOW (window))
        window_actions_update (group, window);
    }

#undef SET_ACTIVE
#undef SET_SENSITIVE
}


/*  private functions  */

static void
view_actions_set_zoom (GimpActionGroup  *group,
                       GimpDisplayShell *shell)
{
  GimpImageWindow *window;
  GimpMenuModel   *model;
  gchar           *str;
  gchar           *label;

  g_object_get (shell->zoom,
                "percentage", &str,
                NULL);

  window = gimp_display_shell_get_window (shell);
  model  = gimp_image_window_get_menubar_model (window);
  label  = g_strdup_printf (_("_Zoom (%s)"), str);
  gimp_menu_model_set_title (model, "/View/Zoom", label);
  g_free (label);
  g_free (str);
}

static void
view_actions_set_rotate (GimpActionGroup  *group,
                         GimpDisplayShell *shell)
{
  const gchar     *flip;
  GimpImageWindow *window;
  GimpMenuModel   *model;
  gchar           *label;

  if (shell->flip_horizontally &&
      shell->flip_vertically)
    {
      /* please preserve the trailing space */
      /* H: Horizontal, V: Vertical */
      flip = _("(H+V) ");
    }
  else if (shell->flip_horizontally)
    {
      /* please preserve the trailing space */
      /* H: Horizontal */
      flip = _("(H) ");
    }
  else if (shell->flip_vertically)
    {
      /* please preserve the trailing space */
      /* V: Vertical */
      flip = _("(V) ");
    }
  else
    {
      flip = "";
    }

  window = gimp_display_shell_get_window (shell);
  model  = gimp_image_window_get_menubar_model (window);
  label = g_strdup_printf (_("_Flip %s& Rotate (%d°)"),
                           flip, (gint) shell->rotate_angle);
  gimp_menu_model_set_title (model, "/View/Flip & Rotate", label);
  g_free (label);
}

static void
view_actions_check_type_notify (GimpDisplayConfig *config,
                                GParamSpec        *pspec,
                                GimpActionGroup   *group)
{
  gimp_action_group_set_action_color (group, "view-padding-color-light-check",
                                      (GeglColor *) gimp_render_check_color1 (), FALSE);
  gimp_action_group_set_action_color (group, "view-padding-color-dark-check",
                                      (GeglColor *) gimp_render_check_color2 (), FALSE);
}
