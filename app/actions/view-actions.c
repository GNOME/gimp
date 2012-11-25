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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpdisplayoptions.h"
#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpprojection.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimprender.h"
#include "widgets/gimphelp-ids.h"
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
static void   view_actions_check_type_notify (GimpDisplayConfig *config,
                                              GParamSpec        *pspec,
                                              GimpActionGroup   *group);


static const GimpActionEntry view_actions[] =
{
  { "view-menu",                NULL, NC_("view-action", "_View")          },
  { "view-zoom-menu",           NULL, NC_("view-action", "_Zoom")          },
  { "view-padding-color-menu",  NULL, NC_("view-action", "_Padding Color") },
  { "view-move-to-screen-menu", GIMP_STOCK_MOVE_TO_SCREEN,
    NC_("view-action", "Move to Screen"), NULL, NULL, NULL,
    GIMP_HELP_VIEW_CHANGE_SCREEN },

  { "view-new", GTK_STOCK_NEW,
    NC_("view-action", "_New View"), "",
    NC_("view-action", "Create another view on this image"),
    G_CALLBACK (view_new_cmd_callback),
    GIMP_HELP_VIEW_NEW },

  { "view-close", GTK_STOCK_CLOSE,
    NC_("view-action",  "_Close View"), "<primary>W",
    NC_("view-action", "Close the active image view"),
    G_CALLBACK (view_close_cmd_callback),
    GIMP_HELP_FILE_CLOSE },

  { "view-zoom-fit-in", GTK_STOCK_ZOOM_FIT,
    NC_("view-action", "_Fit Image in Window"), "<primary><shift>J",
    NC_("view-action", "Adjust the zoom ratio so that the image becomes fully visible"),
    G_CALLBACK (view_zoom_fit_in_cmd_callback),
    GIMP_HELP_VIEW_ZOOM_FIT_IN },

  { "view-zoom-fill", GTK_STOCK_ZOOM_FIT,
    NC_("view-action", "Fi_ll Window"), NULL,
    NC_("view-action", "Adjust the zoom ratio so that the entire window is used"),
    G_CALLBACK (view_zoom_fill_cmd_callback),
    GIMP_HELP_VIEW_ZOOM_FILL },

  { "view-zoom-revert", NULL,
    NC_("view-action", "Re_vert Zoom"), "grave",
    NC_("view-action", "Restore the previous zoom level"),
    G_CALLBACK (view_zoom_revert_cmd_callback),
    GIMP_HELP_VIEW_ZOOM_REVERT },

  { "view-navigation-window", GIMP_STOCK_NAVIGATION,
    NC_("view-action", "Na_vigation Window"), NULL,
    NC_("view-action", "Show an overview window for this image"),
    G_CALLBACK (view_navigation_window_cmd_callback),
    GIMP_HELP_NAVIGATION_DIALOG },

  { "view-display-filters", GIMP_STOCK_DISPLAY_FILTER,
    NC_("view-action", "Display _Filters..."), NULL,
    NC_("view-action", "Configure filters applied to this view"),
    G_CALLBACK (view_display_filters_cmd_callback),
    GIMP_HELP_DISPLAY_FILTER_DIALOG },

  { "view-shrink-wrap", GTK_STOCK_ZOOM_FIT,
    NC_("view-action", "Shrink _Wrap"), "<primary>J",
    NC_("view-action", "Reduce the image window to the size of the image display"),
    G_CALLBACK (view_shrink_wrap_cmd_callback),
    GIMP_HELP_VIEW_SHRINK_WRAP },

  { "view-open-display", NULL,
    NC_("view-action", "_Open Display..."), NULL,
    NC_("view-action", "Connect to another display"),
    G_CALLBACK (window_open_display_cmd_callback),
    NULL }
};

static const GimpToggleActionEntry view_toggle_actions[] =
{
  { "view-dot-for-dot", NULL,
    NC_("view-action", "_Dot for Dot"), NULL,
    NC_("view-action", "A pixel on the screen represents an image pixel"),
    G_CALLBACK (view_dot_for_dot_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_DOT_FOR_DOT },

  { "view-show-selection", NULL,
    NC_("view-action", "Show _Selection"), "<primary>T",
    NC_("view-action", "Display the selection outline"),
    G_CALLBACK (view_toggle_selection_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_SELECTION },

  { "view-show-layer-boundary", NULL,
    NC_("view-action", "Show _Layer Boundary"), NULL,
    NC_("view-action", "Draw a border around the active layer"),
    G_CALLBACK (view_toggle_layer_boundary_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_LAYER_BOUNDARY },

  { "view-show-guides", NULL,
    NC_("view-action", "Show _Guides"), "<primary><shift>T",
    NC_("view-action", "Display the image's guides"),
    G_CALLBACK (view_toggle_guides_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_GUIDES },

  { "view-show-grid", NULL,
    NC_("view-action", "S_how Grid"), NULL,
    NC_("view-action", "Display the image's grid"),
    G_CALLBACK (view_toggle_grid_cmd_callback),
    FALSE,
    GIMP_HELP_VIEW_SHOW_GRID },

  { "view-show-sample-points", NULL,
    NC_("view-action", "Show Sample Points"), NULL,
    NC_("view-action", "Display the image's color sample points"),
    G_CALLBACK (view_toggle_sample_points_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_SAMPLE_POINTS },

  { "view-snap-to-guides", NULL,
    NC_("view-action", "Sn_ap to Guides"), NULL,
    NC_("view-action", "Tool operations snap to guides"),
    G_CALLBACK (view_snap_to_guides_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SNAP_TO_GUIDES },

  { "view-snap-to-grid", NULL,
    NC_("view-action", "Sna_p to Grid"), NULL,
    NC_("view-action", "Tool operations snap to the grid"),
    G_CALLBACK (view_snap_to_grid_cmd_callback),
    FALSE,
    GIMP_HELP_VIEW_SNAP_TO_GRID },

  { "view-snap-to-canvas", NULL,
    NC_("view-action", "Snap to _Canvas Edges"), NULL,
    NC_("view-action", "Tool operations snap to the canvas edges"),
    G_CALLBACK (view_snap_to_canvas_cmd_callback),
    FALSE,
    GIMP_HELP_VIEW_SNAP_TO_CANVAS },

  { "view-snap-to-vectors", NULL,
    NC_("view-action", "Snap t_o Active Path"), NULL,
    NC_("view-action", "Tool operations snap to the active path"),
    G_CALLBACK (view_snap_to_vectors_cmd_callback),
    FALSE,
    GIMP_HELP_VIEW_SNAP_TO_VECTORS },

  { "view-show-menubar", NULL,
    NC_("view-action", "Show _Menubar"), NULL,
    NC_("view-action", "Show this window's menubar"),
    G_CALLBACK (view_toggle_menubar_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_MENUBAR },

  { "view-show-rulers", NULL,
    NC_("view-action", "Show R_ulers"), "<primary><shift>R",
    NC_("view-action", "Show this window's rulers"),
    G_CALLBACK (view_toggle_rulers_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_RULERS },

  { "view-show-scrollbars", NULL,
    NC_("view-action", "Show Scroll_bars"), NULL,
    NC_("view-action", "Show this window's scrollbars"),
    G_CALLBACK (view_toggle_scrollbars_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_SCROLLBARS },

  { "view-show-statusbar", NULL,
    NC_("view-action", "Show S_tatusbar"), NULL,
    NC_("view-action", "Show this window's statusbar"),
    G_CALLBACK (view_toggle_statusbar_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_STATUSBAR },

  { "view-fullscreen", GTK_STOCK_FULLSCREEN,
    NC_("view-action", "Fullscr_een"), "F11",
    NC_("view-action", "Toggle fullscreen view"),
    G_CALLBACK (view_fullscreen_cmd_callback),
    FALSE,
    GIMP_HELP_VIEW_FULLSCREEN },

  { "view-use-gegl", GIMP_STOCK_GEGL,
    NC_("view-action", "Use GEGL"), NULL,
    NC_("view-action", "Use GEGL to create this window's projection"),
    G_CALLBACK (view_use_gegl_cmd_callback),
    FALSE,
    NULL }
};

static const GimpEnumActionEntry view_zoom_actions[] =
{
  { "view-zoom", NULL,
    "Set zoom factor", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },

  { "view-zoom-minimum", GTK_STOCK_ZOOM_OUT,
    "Zoom out as far as possible", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-maximum", GTK_STOCK_ZOOM_IN,
    "Zoom in as far as possible", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-out", GTK_STOCK_ZOOM_OUT,
    NC_("view-zoom-action", "Zoom _Out"), "minus",
    NC_("view-zoom-action", "Zoom out"),
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-in", GTK_STOCK_ZOOM_IN,
    NC_("view-zoom-action", "Zoom _In"), "plus",
    NC_("view-zoom-action", "Zoom in"),
    GIMP_ACTION_SELECT_NEXT, FALSE,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-out-accel", GIMP_STOCK_CHAR_PICKER,
    NC_("view-zoom-action", "Zoom Out"), "KP_Subtract",
    NC_("view-zoom-action", "Zoom out"),
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-in-accel", GIMP_STOCK_CHAR_PICKER,
    NC_("view-zoom-action", "Zoom In"), "KP_Add",
    NC_("view-zoom-action", "Zoom in"),
    GIMP_ACTION_SELECT_NEXT, FALSE,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-out-skip", GTK_STOCK_ZOOM_OUT,
    "Zoom out a lot", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-in-skip", GTK_STOCK_ZOOM_IN,
    "Zoom in a lot", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    GIMP_HELP_VIEW_ZOOM_IN }
};

static const GimpRadioActionEntry view_zoom_explicit_actions[] =
{
  { "view-zoom-16-1", NULL,
    NC_("view-zoom-action", "1_6:1  (1600%)"), "5",
    NC_("view-zoom-action", "Zoom 16:1"),
    160000,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-16-1-accel", NULL,
    NC_("view-zoom-action", "1_6:1  (1600%)"), "KP_5",
    NC_("view-zoom-action", "Zoom 16:1"),
    160000,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-8-1", NULL,
    NC_("view-zoom-action", "_8:1  (800%)"), "4",
    NC_("view-zoom-action", "Zoom 8:1"),
    80000,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-8-1-accel", NULL,
    NC_("view-zoom-action", "_8:1  (800%)"), "KP_4",
    NC_("view-zoom-action", "Zoom 8:1"),
    80000,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-4-1", NULL,
    NC_("view-zoom-action", "_4:1  (400%)"), "3",
    NC_("view-zoom-action", "Zoom 4:1"),
    40000,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-4-1-accel", NULL,
    NC_("view-zoom-action", "_4:1  (400%)"), "KP_3",
    NC_("view-zoom-action", "Zoom 4:1"),
    40000,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-2-1", NULL,
    NC_("view-zoom-action", "_2:1  (200%)"), "2",
    NC_("view-zoom-action", "Zoom 2:1"),
    20000,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-2-1-accel", NULL,
    NC_("view-zoom-action", "_2:1  (200%)"), "KP_2",
    NC_("view-zoom-action", "Zoom 2:1"),
    20000,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-1-1", GTK_STOCK_ZOOM_100,
    NC_("view-zoom-action", "_1:1  (100%)"), "1",
    NC_("view-zoom-action", "Zoom 1:1"),
    10000,
    GIMP_HELP_VIEW_ZOOM_100 },

  { "view-zoom-1-1-accel", GTK_STOCK_ZOOM_100,
    NC_("view-zoom-action", "_1:1  (100%)"), "KP_1",
    NC_("view-zoom-action", "Zoom 1:1"),
    10000,
    GIMP_HELP_VIEW_ZOOM_100 },

  { "view-zoom-1-2", NULL,
    NC_("view-zoom-action", "1:_2  (50%)"), "<shift>2",
    NC_("view-zoom-action", "Zoom 1:2"),
    5000,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-1-4", NULL,
    NC_("view-zoom-action", "1:_4  (25%)"), "<shift>3",
    NC_("view-zoom-action", "Zoom 1:4"),
    2500,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-1-8", NULL,
    NC_("view-zoom-action", "1:_8  (12.5%)"), "<shift>4",
    NC_("view-zoom-action", "Zoom 1:8"),
    1250,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-1-16", NULL,
    NC_("view-zoom-action", "1:1_6  (6.25%)"), "<shift>5",
    NC_("view-zoom-action", "Zoom 1:16"),
    625,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-other", NULL,
    NC_("view-zoom-action", "Othe_r..."), NULL,
    NC_("view-zoom-action", "Set a custom zoom factor"),
    0,
    GIMP_HELP_VIEW_ZOOM_OTHER }
};

static const GimpEnumActionEntry view_padding_color_actions[] =
{
  { "view-padding-color-theme", NULL,
    NC_("view-padding-color", "From _Theme"), NULL,
    NC_("view-padding-color", "Use the current theme's background color"),
    GIMP_CANVAS_PADDING_MODE_DEFAULT, FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR },

  { "view-padding-color-light-check", NULL,
    NC_("view-padding-color", "_Light Check Color"), NULL,
    NC_("view-padding-color", "Use the light check color"),
    GIMP_CANVAS_PADDING_MODE_LIGHT_CHECK, FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR },

  { "view-padding-color-dark-check", NULL,
    NC_("view-padding-color", "_Dark Check Color"), NULL,
    NC_("view-padding-color", "Use the dark check color"),
    GIMP_CANVAS_PADDING_MODE_DARK_CHECK, FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR },

  { "view-padding-color-custom", GTK_STOCK_SELECT_COLOR,
    NC_("view-padding-color", "Select _Custom Color..."), NULL,
    NC_("view-padding-color", "Use an arbitrary color"),
    GIMP_CANVAS_PADDING_MODE_CUSTOM, FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR },

  { "view-padding-color-prefs", GIMP_STOCK_RESET,
    NC_("view-padding-color", "As in _Preferences"), NULL,
    NC_("view-padding-color",
        "Reset padding color to what's configured in preferences"),
    GIMP_CANVAS_PADDING_MODE_RESET, FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR }
};

static const GimpEnumActionEntry view_scroll_horizontal_actions[] =
{
  { "view-scroll-horizontal", NULL,
    "Set horizontal scroll offset", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },

  { "view-scroll-left-border", NULL,
    "Scroll to left border", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },

  { "view-scroll-right-border", NULL,
    "Scroll to right border", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },

  { "view-scroll-left", NULL,
    "Scroll left", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },

  { "view-scroll-right", NULL,
    "Scroll right", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },

  { "view-scroll-page-left", NULL,
    "Scroll page left", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },

  { "view-scroll-page-right", NULL,
    "Scroll page right", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry view_scroll_vertical_actions[] =
{
  { "view-scroll-vertical", NULL,
    "Set vertical scroll offset", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },

  { "view-scroll-top-border", NULL,
    "Scroll to top border", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },

  { "view-scroll-bottom-border", NULL,
    "Scroll to bottom border", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },

  { "view-scroll-up", NULL,
    "Scroll up", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },

  { "view-scroll-down", NULL,
    "Scroll down", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },

  { "view-scroll-page-up", NULL,
    "Scroll page up", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },

  { "view-scroll-page-down", NULL,
    "Scroll page down", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};


void
view_actions_setup (GimpActionGroup *group)
{
  GtkAction *action;

  gimp_action_group_add_actions (group, "view-action",
                                 view_actions,
                                 G_N_ELEMENTS (view_actions));

  gimp_action_group_add_toggle_actions (group, "view-action",
                                        view_toggle_actions,
                                        G_N_ELEMENTS (view_toggle_actions));

  gimp_action_group_add_enum_actions (group, "view-zoom-action",
                                      view_zoom_actions,
                                      G_N_ELEMENTS (view_zoom_actions),
                                      G_CALLBACK (view_zoom_cmd_callback));

  gimp_action_group_add_radio_actions (group, "view-zoom-action",
                                       view_zoom_explicit_actions,
                                       G_N_ELEMENTS (view_zoom_explicit_actions),
                                       NULL,
                                       10000,
                                       G_CALLBACK (view_zoom_explicit_cmd_callback));

  gimp_action_group_add_enum_actions (group, "view-padding-color",
                                      view_padding_color_actions,
                                      G_N_ELEMENTS (view_padding_color_actions),
                                      G_CALLBACK (view_padding_color_cmd_callback));

  gimp_action_group_add_enum_actions (group, NULL,
                                      view_scroll_horizontal_actions,
                                      G_N_ELEMENTS (view_scroll_horizontal_actions),
                                      G_CALLBACK (view_scroll_horizontal_cmd_callback));

  gimp_action_group_add_enum_actions (group, NULL,
                                      view_scroll_vertical_actions,
                                      G_N_ELEMENTS (view_scroll_vertical_actions),
                                      G_CALLBACK (view_scroll_vertical_cmd_callback));

  /*  connect "activate" of view-zoom-other manually so it can be
   *  selected even if it's the active item of the radio group
   */
  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                        "view-zoom-other");
  g_signal_connect (action, "activate",
                    G_CALLBACK (view_zoom_other_cmd_callback),
                    group->user_data);

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
  GimpDisplay        *display        = action_data_get_display (data);
  GimpImage          *image          = NULL;
  GimpDisplayShell   *shell          = NULL;
  GimpDisplayOptions *options        = NULL;
  gchar              *label          = NULL;
  gboolean            fullscreen     = FALSE;
  gboolean            revert_enabled = FALSE;   /* able to revert zoom? */
  gboolean            use_gegl       = FALSE;

  if (display)
    {
      GimpImageWindow *window;

      image  = gimp_display_get_image (display);
      shell  = gimp_display_get_shell (display);
      window = gimp_display_shell_get_window (shell);

      if (window)
        fullscreen = gimp_image_window_get_fullscreen (window);

      options = (image ?
                 (fullscreen ? shell->fullscreen_options : shell->options) :
                 shell->no_image_options);

      revert_enabled = gimp_display_shell_scale_can_revert (shell);

      if (image)
        use_gegl = gimp_image_get_projection (image)->use_gegl;
    }

#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)
#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)
#define SET_COLOR(action,color) \
        gimp_action_group_set_action_color (group, action, color, FALSE)

  SET_SENSITIVE ("view-new",   image);
  SET_SENSITIVE ("view-close", image);

  SET_SENSITIVE ("view-dot-for-dot", image);
  SET_ACTIVE    ("view-dot-for-dot", display && shell->dot_for_dot);

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

  SET_SENSITIVE ("view-zoom-out",    image);
  SET_SENSITIVE ("view-zoom-in",     image);
  SET_SENSITIVE ("view-zoom-fit-in", image);
  SET_SENSITIVE ("view-zoom-fill",   image);

  SET_SENSITIVE ("view-zoom-16-1",  image);
  SET_SENSITIVE ("view-zoom-8-1",   image);
  SET_SENSITIVE ("view-zoom-4-1",   image);
  SET_SENSITIVE ("view-zoom-2-1",   image);
  SET_SENSITIVE ("view-zoom-1-1",   image);
  SET_SENSITIVE ("view-zoom-1-2",   image);
  SET_SENSITIVE ("view-zoom-1-4",   image);
  SET_SENSITIVE ("view-zoom-1-8",   image);
  SET_SENSITIVE ("view-zoom-1-16",  image);
  SET_SENSITIVE ("view-zoom-other", image);

  if (image)
    view_actions_set_zoom (group, shell);

  SET_SENSITIVE ("view-navigation-window", image);
  SET_SENSITIVE ("view-display-filters",   image);

  SET_SENSITIVE ("view-show-selection",      image);
  SET_ACTIVE    ("view-show-selection",      display && options->show_selection);
  SET_SENSITIVE ("view-show-layer-boundary", image);
  SET_ACTIVE    ("view-show-layer-boundary", display && options->show_layer_boundary);
  SET_SENSITIVE ("view-show-guides",         image);
  SET_ACTIVE    ("view-show-guides",         display && options->show_guides);
  SET_SENSITIVE ("view-show-grid",           image);
  SET_ACTIVE    ("view-show-grid",           display && options->show_grid);
  SET_SENSITIVE ("view-show-sample-points",  image);
  SET_ACTIVE    ("view-show-sample-points",  display && options->show_sample_points);

  SET_SENSITIVE ("view-snap-to-guides",      image);
  SET_ACTIVE    ("view-snap-to-guides",      display && shell->snap_to_guides);
  SET_SENSITIVE ("view-snap-to-grid",        image);
  SET_ACTIVE    ("view-snap-to-grid",        display && shell->snap_to_grid);
  SET_SENSITIVE ("view-snap-to-canvas",      image);
  SET_ACTIVE    ("view-snap-to-canvas",      display && shell->snap_to_canvas);
  SET_SENSITIVE ("view-snap-to-vectors",     image);
  SET_ACTIVE    ("view-snap-to-vectors",     display && shell->snap_to_vectors);

  SET_SENSITIVE ("view-padding-color-theme",       image);
  SET_SENSITIVE ("view-padding-color-light-check", image);
  SET_SENSITIVE ("view-padding-color-dark-check",  image);
  SET_SENSITIVE ("view-padding-color-custom",      image);
  SET_SENSITIVE ("view-padding-color-prefs",       image);

  if (display)
    {
      SET_COLOR ("view-padding-color-menu", &options->padding_color);

      if (shell->canvas)
        {
          GtkStyle *style = gtk_widget_get_style (shell->canvas);
          GimpRGB   color;

          gtk_widget_ensure_style (shell->canvas);
          gimp_rgb_set_gdk_color (&color, style->bg + GTK_STATE_NORMAL);
          gimp_rgb_set_alpha (&color, GIMP_OPACITY_OPAQUE);

          SET_COLOR ("view-padding-color-theme",  &color);
        }
    }

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
  SET_ACTIVE    ("view-use-gegl",    use_gegl);

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
#undef SET_COLOR
}


/*  private functions  */

static void
view_actions_set_zoom (GimpActionGroup  *group,
                       GimpDisplayShell *shell)
{
  const gchar *action = NULL;
  gchar       *str;
  gchar       *label;
  guint        scale;

  g_object_get (shell->zoom,
                "percentage", &str,
                NULL);

  scale = ROUND (gimp_zoom_model_get_factor (shell->zoom) * 1000);

  switch (scale)
    {
    case 16000:  action = "view-zoom-16-1";  break;
    case  8000:  action = "view-zoom-8-1";   break;
    case  4000:  action = "view-zoom-4-1";   break;
    case  2000:  action = "view-zoom-2-1";   break;
    case  1000:  action = "view-zoom-1-1";   break;
    case   500:  action = "view-zoom-1-2";   break;
    case   250:  action = "view-zoom-1-4";   break;
    case   125:  action = "view-zoom-1-8";   break;
    case    63:
    case    62:  action = "view-zoom-1-16";  break;
    }

  if (! action)
    {
      action = "view-zoom-other";

      label = g_strdup_printf (_("Othe_r (%s)..."), str);
      gimp_action_group_set_action_label (group, action, label);
      g_free (label);

      shell->other_scale = gimp_zoom_model_get_factor (shell->zoom);
    }

  gimp_action_group_set_action_active (group, action, TRUE);

  label = g_strdup_printf (_("_Zoom (%s)"), str);
  gimp_action_group_set_action_label (group, "view-zoom-menu", label);
  g_free (label);

  /* flag as dirty */
  shell->other_scale = - fabs (shell->other_scale);

  g_free (str);
}

static void
view_actions_check_type_notify (GimpDisplayConfig *config,
                                GParamSpec        *pspec,
                                GimpActionGroup   *group)
{
  gimp_action_group_set_action_color (group, "view-padding-color-light-check",
                                      gimp_render_light_check_color (),
                                      FALSE);
  gimp_action_group_set_action_color (group, "view-padding-color-dark-check",
                                      gimp_render_dark_check_color (),
                                      FALSE);
}
