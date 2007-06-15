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

#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimprender.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayoptions.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpdisplayshell-scale.h"
#include "display/gimpdisplayshell-selection.h"

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
  { "view-menu",                NULL, N_("_View")          },
  { "view-zoom-menu",           NULL, N_("_Zoom")          },
  { "view-padding-color-menu",  NULL, N_("_Padding Color") },
  { "view-move-to-screen-menu", GIMP_STOCK_MOVE_TO_SCREEN,
    N_("Move to Screen"), NULL, NULL, NULL,
    GIMP_HELP_VIEW_CHANGE_SCREEN },

  { "view-new", GTK_STOCK_NEW,
    N_("_New View"), "",
    N_("Create another view on this image"),
    G_CALLBACK (view_new_cmd_callback),
    GIMP_HELP_VIEW_NEW },

  { "view-close", GTK_STOCK_CLOSE,
    N_( "_Close"), "<control>W",
    N_("Close this image window"),
    G_CALLBACK (window_close_cmd_callback),
    GIMP_HELP_FILE_CLOSE },

  { "view-zoom-fit-in", GTK_STOCK_ZOOM_FIT,
    N_("_Fit Image in Window"), "<control><shift>E",
    N_("Adjust the zoom ratio so that the image becomes fully visible"),
    G_CALLBACK (view_zoom_fit_in_cmd_callback),
    GIMP_HELP_VIEW_ZOOM_FIT_IN },

  { "view-zoom-fit-to", GTK_STOCK_ZOOM_FIT,
    N_("Fit Image _to Window"), NULL,
    N_("Adjust the zoom ratio so that the window is used optimally"),
    G_CALLBACK (view_zoom_fit_to_cmd_callback),
    GIMP_HELP_VIEW_ZOOM_FIT_TO },

  { "view-zoom-revert", NULL,
    N_("Re_vert Zoom"), "grave",
    N_("Restore the previous zoom level"),
    G_CALLBACK (view_zoom_revert_cmd_callback),
    GIMP_HELP_VIEW_ZOOM_REVERT },

  { "view-navigation-window", GIMP_STOCK_NAVIGATION,
    N_("Na_vigation Window"), NULL,
    N_("Show an overview window for this image"),
    G_CALLBACK (view_navigation_window_cmd_callback),
    GIMP_HELP_NAVIGATION_DIALOG },

  { "view-display-filters", GIMP_STOCK_DISPLAY_FILTER,
    N_("Display _Filters..."), NULL,
    N_("Configure filters applied to this view"),
    G_CALLBACK (view_display_filters_cmd_callback),
    GIMP_HELP_DISPLAY_FILTER_DIALOG },

  { "view-shrink-wrap", GTK_STOCK_ZOOM_FIT,
    N_("Shrink _Wrap"), "<control>E",
    N_("Reduce the image window to the size of the image display"),
    G_CALLBACK (view_shrink_wrap_cmd_callback),
    GIMP_HELP_VIEW_SHRINK_WRAP },

  { "view-open-display", NULL,
    N_("_Open Display..."), NULL,
    N_("Connect to another display"),
    G_CALLBACK (window_open_display_cmd_callback),
    NULL }
};

static const GimpToggleActionEntry view_toggle_actions[] =
{
  { "view-dot-for-dot", NULL,
    N_("_Dot for Dot"), NULL,
    N_("A pixel on the screen represents an image pixel"),
    G_CALLBACK (view_dot_for_dot_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_DOT_FOR_DOT },

  { "view-show-selection", NULL,
    N_("Show _Selection"), "<control>T",
    N_("Display the selection outline"),
    G_CALLBACK (view_toggle_selection_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_SELECTION },

  { "view-show-layer-boundary", NULL,
    N_("Show _Layer Boundary"), NULL,
    N_("Draw a border around the active layer"),
    G_CALLBACK (view_toggle_layer_boundary_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_LAYER_BOUNDARY },

  { "view-show-guides", NULL,
    N_("Show _Guides"), "<control><shift>T",
    N_("Display the image's guides"),
    G_CALLBACK (view_toggle_guides_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_GUIDES },

  { "view-show-grid", NULL,
    N_("S_how Grid"), NULL,
    N_("Display the image's grid"),
    G_CALLBACK (view_toggle_grid_cmd_callback),
    FALSE,
    GIMP_HELP_VIEW_SHOW_GRID },

  { "view-show-sample-points", NULL,
    N_("Show Sample Points"), NULL,
    N_("Display the image's color sample points"),
    G_CALLBACK (view_toggle_sample_points_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_SAMPLE_POINTS },

  { "view-snap-to-guides", NULL,
    N_("Sn_ap to Guides"), NULL,
    N_("Tool operations snap to guides"),
    G_CALLBACK (view_snap_to_guides_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SNAP_TO_GUIDES },

  { "view-snap-to-grid", NULL,
    N_("Sna_p to Grid"), NULL,
    N_("Tool operations snap to the grid"),
    G_CALLBACK (view_snap_to_grid_cmd_callback),
    FALSE,
    GIMP_HELP_VIEW_SNAP_TO_GRID },

  { "view-snap-to-canvas", NULL,
    N_("Snap to _Canvas Edges"), NULL,
    N_("Tool operations snap to the canvas edges"),
    G_CALLBACK (view_snap_to_canvas_cmd_callback),
    FALSE,
    GIMP_HELP_VIEW_SNAP_TO_CANVAS },

  { "view-snap-to-vectors", NULL,
    N_("Snap t_o Active Path"), NULL,
    N_("Tool operations snap to the active path"),
    G_CALLBACK (view_snap_to_vectors_cmd_callback),
    FALSE,
    GIMP_HELP_VIEW_SNAP_TO_VECTORS },

  { "view-show-menubar", NULL,
    N_("Show _Menubar"), NULL,
    N_("Show this window's menubar"),
    G_CALLBACK (view_toggle_menubar_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_MENUBAR },

  { "view-show-rulers", NULL,
    N_("Show R_ulers"), "<control><shift>R",
    N_("Show this window's rulers"),
    G_CALLBACK (view_toggle_rulers_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_RULERS },

  { "view-show-scrollbars", NULL,
    N_("Show Scroll_bars"), NULL,
    N_("Show this window's scrollbars"),
    G_CALLBACK (view_toggle_scrollbars_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_SCROLLBARS },

  { "view-show-statusbar", NULL,
    N_("Show S_tatusbar"), NULL,
    N_("Show this window's statusbar"),
    G_CALLBACK (view_toggle_statusbar_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_STATUSBAR },

  { "view-fullscreen", GTK_STOCK_FULLSCREEN,
    N_("Fullscr_een"), "F11",
    N_("Toggle fullscreen view"),
    G_CALLBACK (view_fullscreen_cmd_callback),
    FALSE,
    GIMP_HELP_VIEW_FULLSCREEN }
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
    N_("Zoom _Out"), "minus",
    N_("Zoom out"),
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-in", GTK_STOCK_ZOOM_IN,
    N_("Zoom _In"), "plus",
    N_("Zoom in"),
    GIMP_ACTION_SELECT_NEXT, FALSE,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-out-accel", GIMP_STOCK_CHAR_PICKER,
    N_("Zoom Out"), "KP_Subtract",
    N_("Zoom out"),
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-in-accel", GIMP_STOCK_CHAR_PICKER,
    N_("Zoom In"), "KP_Add",
    N_("Zoom in"),
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
    N_("1_6:1  (1600%)"), NULL,
    N_("Zoom 16:1"),
    160000,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-8-1", NULL,
    N_("_8:1  (800%)"), NULL,
    N_("Zoom 8:1"),
    80000,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-4-1", NULL,
    N_("_4:1  (400%)"), NULL,
    N_("Zoom 4:1"),
    40000,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-2-1", NULL,
    N_("_2:1  (200%)"), NULL,
    N_("Zoom 1:1"),
    20000,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-1-1", GTK_STOCK_ZOOM_100,
    N_("_1:1  (100%)"), "1",
    N_("Zoom 1:1"),
    10000,
    GIMP_HELP_VIEW_ZOOM_100 },

  { "view-zoom-1-2", NULL,
    N_("1:_2  (50%)"), NULL,
    N_("Zoom 1:2"),
    5000,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-1-4", NULL,
    N_("1:_4  (25%)"), NULL,
    N_("Zoom 1:4"),
    2500,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-1-8", NULL,
    N_("1:_8  (12.5%)"), NULL,
    N_("Zoom 1:8"),
    1250,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-1-16", NULL,
    N_("1:1_6  (6.25%)"), NULL,
    N_("Zoom 1:16"),
    625,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-other", NULL,
    N_("Othe_r..."), NULL,
    N_("Set a custom zoom factor"),
    0,
    GIMP_HELP_VIEW_ZOOM_OTHER }
};

static const GimpEnumActionEntry view_padding_color_actions[] =
{
  { "view-padding-color-theme", NULL,
    N_("From _Theme"), NULL,
    N_("Use the current theme's background color"),
    GIMP_CANVAS_PADDING_MODE_DEFAULT, FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR },

  { "view-padding-color-light-check", NULL,
    N_("_Light Check Color"), NULL,
    N_("Use the light check color"),
    GIMP_CANVAS_PADDING_MODE_LIGHT_CHECK, FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR },

  { "view-padding-color-dark-check", NULL,
    N_("_Dark Check Color"), NULL,
    N_("Use the dark check color"),
    GIMP_CANVAS_PADDING_MODE_DARK_CHECK, FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR },

  { "view-padding-color-custom", GTK_STOCK_SELECT_COLOR,
    N_("Select _Custom Color..."), NULL,
    N_("Use an arbitrary color"),
    GIMP_CANVAS_PADDING_MODE_CUSTOM, FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR },

  { "view-padding-color-prefs", GIMP_STOCK_RESET,
    N_("As in _Preferences"), NULL,
    N_("Reset padding color to what's configured in preferences"),
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

  gimp_action_group_add_actions (group,
                                 view_actions,
                                 G_N_ELEMENTS (view_actions));

  gimp_action_group_add_toggle_actions (group,
                                        view_toggle_actions,
                                        G_N_ELEMENTS (view_toggle_actions));

  gimp_action_group_add_enum_actions (group,
                                      view_zoom_actions,
                                      G_N_ELEMENTS (view_zoom_actions),
                                      G_CALLBACK (view_zoom_cmd_callback));

  gimp_action_group_add_radio_actions (group,
                                       view_zoom_explicit_actions,
                                       G_N_ELEMENTS (view_zoom_explicit_actions),
                                       NULL,
                                       10000,
                                       G_CALLBACK (view_zoom_explicit_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      view_padding_color_actions,
                                      G_N_ELEMENTS (view_padding_color_actions),
                                      G_CALLBACK (view_padding_color_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      view_scroll_horizontal_actions,
                                      G_N_ELEMENTS (view_scroll_horizontal_actions),
                                      G_CALLBACK (view_scroll_horizontal_cmd_callback));

  gimp_action_group_add_enum_actions (group,
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

  if (GIMP_IS_DISPLAY (group->user_data) ||
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
  GimpDisplayShell   *shell          = NULL;
  GimpDisplayOptions *options        = NULL;
  gchar              *label          = NULL;
  gboolean            fullscreen     = FALSE;
  gboolean            revert_enabled = FALSE;   /* able to revert zoom? */

  if (display)
    {
      shell = GIMP_DISPLAY_SHELL (display->shell);

      fullscreen = gimp_display_shell_get_fullscreen (shell);

      options = fullscreen ? shell->fullscreen_options : shell->options;

      revert_enabled = gimp_display_shell_scale_can_revert (shell);
    }

#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)
#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)
#define SET_COLOR(action,color) \
        gimp_action_group_set_action_color (group, action, color, FALSE)

  SET_SENSITIVE ("view-new",   display);
  SET_SENSITIVE ("view-close", display);

  SET_SENSITIVE ("view-dot-for-dot", display);
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
                                          N_("Re_vert Zoom"));
    }

  SET_SENSITIVE ("view-zoom-out",    display);
  SET_SENSITIVE ("view-zoom-in",     display);
  SET_SENSITIVE ("view-zoom-fit-in", display);
  SET_SENSITIVE ("view-zoom-fit-to", display);

  SET_SENSITIVE ("view-zoom-16-1",  display);
  SET_SENSITIVE ("view-zoom-8-1",   display);
  SET_SENSITIVE ("view-zoom-4-1",   display);
  SET_SENSITIVE ("view-zoom-2-1",   display);
  SET_SENSITIVE ("view-zoom-1-1",   display);
  SET_SENSITIVE ("view-zoom-1-2",   display);
  SET_SENSITIVE ("view-zoom-1-4",   display);
  SET_SENSITIVE ("view-zoom-1-8",   display);
  SET_SENSITIVE ("view-zoom-1-16",  display);
  SET_SENSITIVE ("view-zoom-other", display);

  if (display)
    view_actions_set_zoom (group, shell);

  SET_SENSITIVE ("view-navigation-window", display);
  SET_SENSITIVE ("view-display-filters",   display);

  SET_SENSITIVE ("view-show-selection",      display);
  SET_ACTIVE    ("view-show-selection",      display && options->show_selection);
  SET_SENSITIVE ("view-show-layer-boundary", display);
  SET_ACTIVE    ("view-show-layer-boundary", display && options->show_layer_boundary);
  SET_SENSITIVE ("view-show-guides",         display);
  SET_ACTIVE    ("view-show-guides",         display && options->show_guides);
  SET_SENSITIVE ("view-show-grid",           display);
  SET_ACTIVE    ("view-show-grid",           display && options->show_grid);
  SET_SENSITIVE ("view-show-sample-points",  display);
  SET_ACTIVE    ("view-show-sample-points",  display && options->show_sample_points);

  SET_SENSITIVE ("view-snap-to-guides",      display);
  SET_ACTIVE    ("view-snap-to-guides",      display && shell->snap_to_guides);
  SET_SENSITIVE ("view-snap-to-grid",        display);
  SET_ACTIVE    ("view-snap-to-grid",        display && shell->snap_to_grid);
  SET_SENSITIVE ("view-snap-to-canvas",      display);
  SET_ACTIVE    ("view-snap-to-canvas",      display && shell->snap_to_canvas);
  SET_SENSITIVE ("view-snap-to-vectors",     display);
  SET_ACTIVE    ("view-snap-to-vectors",     display && shell->snap_to_vectors);

  SET_SENSITIVE ("view-padding-color-theme",       display);
  SET_SENSITIVE ("view-padding-color-light-check", display);
  SET_SENSITIVE ("view-padding-color-dark-check",  display);
  SET_SENSITIVE ("view-padding-color-custom",      display);
  SET_SENSITIVE ("view-padding-color-prefs",       display);

  if (display)
    {
      SET_COLOR ("view-padding-color-menu", &options->padding_color);

      if (shell->canvas)
        {
          GimpRGB color;

          gtk_widget_ensure_style (shell->canvas);
          gimp_rgb_set_gdk_color (&color,
                                  shell->canvas->style->bg + GTK_STATE_NORMAL);
          gimp_rgb_set_alpha (&color, GIMP_OPACITY_OPAQUE);

          SET_COLOR ("view-padding-color-theme",  &color);
        }
    }

  SET_SENSITIVE ("view-show-menubar",    display);
  SET_ACTIVE    ("view-show-menubar",    display && options->show_menubar);
  SET_SENSITIVE ("view-show-rulers",     display);
  SET_ACTIVE    ("view-show-rulers",     display && options->show_rulers);
  SET_SENSITIVE ("view-show-scrollbars", display);
  SET_ACTIVE    ("view-show-scrollbars", display && options->show_scrollbars);
  SET_SENSITIVE ("view-show-statusbar",  display);
  SET_ACTIVE    ("view-show-statusbar",  display && options->show_statusbar);

  SET_SENSITIVE ("view-shrink-wrap", display);
  SET_SENSITIVE ("view-fullscreen",  display);
  SET_ACTIVE    ("view-fullscreen",  display && fullscreen);

  if (GIMP_IS_DISPLAY (group->user_data) ||
      GIMP_IS_GIMP (group->user_data))
    {
      /*  see view_actions_setup()  */
      window_actions_update (group, display ? display->shell : NULL);
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
  GimpRGB color;

  gimp_rgba_set_uchar (&color,
                       gimp_render_blend_light_check[0],
                       gimp_render_blend_light_check[1],
                       gimp_render_blend_light_check[2],
                       255);
  gimp_action_group_set_action_color (group, "view-padding-color-light-check",
                                      &color, FALSE);

  gimp_rgba_set_uchar (&color,
                       gimp_render_blend_dark_check[0],
                       gimp_render_blend_dark_check[1],
                       gimp_render_blend_dark_check[2],
                       255);
  gimp_action_group_set_action_color (group, "view-padding-color-dark-check",
                                      &color, FALSE);
}
