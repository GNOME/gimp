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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayoptions.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpdisplayshell-render.h"
#include "display/gimpdisplayshell-selection.h"

#include "actions.h"
#include "view-actions.h"
#include "view-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   view_actions_set_zoom          (GimpActionGroup   *group,
                                              GimpDisplayShell  *shell);
static void   view_actions_check_type_notify (GimpDisplayConfig *config,
                                              GParamSpec        *pspec,
                                              GimpActionGroup   *group);


static GimpActionEntry view_actions[] =
{
  { "view-menu",               NULL, N_("_View")          },
  { "view-zoom-menu",          NULL, N_("_Zoom")          },
  { "view-padding-color-menu", NULL, N_("_Padding Color") },

  { "view-new", GTK_STOCK_NEW,
    N_("_New View"), "", NULL,
    G_CALLBACK (view_new_view_cmd_callback),
    GIMP_HELP_VIEW_NEW },

  { "view-close", GTK_STOCK_CLOSE,
    N_( "_Close"), "<control>W", NULL,
    G_CALLBACK (view_close_view_cmd_callback),
    GIMP_HELP_FILE_CLOSE },

  { "view-zoom-fit-in", GTK_STOCK_ZOOM_FIT,
    N_("_Fit Image in Window"), "<control><shift>E",
    N_("Fit image in window"),
    G_CALLBACK (view_zoom_fit_in_cmd_callback),
    GIMP_HELP_VIEW_ZOOM_FIT_IN },

  { "view-zoom-fit-to", GTK_STOCK_ZOOM_FIT,
    N_("Fit Image to Window"), NULL,
    N_("Fit image to window"),
    G_CALLBACK (view_zoom_fit_to_cmd_callback),
    GIMP_HELP_VIEW_ZOOM_FIT_TO },

  { "view-info-window", GIMP_STOCK_INFO,
    N_("_Info Window"), "<control><shift>I", NULL,
    G_CALLBACK (view_info_window_cmd_callback),
    GIMP_HELP_INFO_DIALOG },

  { "view-navigation-window", GIMP_STOCK_NAVIGATION,
    N_("Na_vigation Window"), "<control><shift>N", NULL,
    G_CALLBACK (view_navigation_window_cmd_callback),
    GIMP_HELP_NAVIGATION_DIALOG },

  { "view-display-filters", GIMP_STOCK_DISPLAY_FILTER,
    N_("Display _Filters..."), NULL, NULL,
    G_CALLBACK (view_display_filters_cmd_callback),
    GIMP_HELP_DISPLAY_FILTER_DIALOG },

  { "view-shrink-wrap", GTK_STOCK_ZOOM_FIT,
    N_("Shrink _Wrap"), "<control>E",
    N_("Shrink wrap"),
    G_CALLBACK (view_shrink_wrap_cmd_callback),
    GIMP_HELP_VIEW_SHRINK_WRAP },

  { "view-move-to-screen", GIMP_STOCK_MOVE_TO_SCREEN,
    N_("Move to Screen..."), NULL, NULL,
    G_CALLBACK (view_change_screen_cmd_callback),
    GIMP_HELP_VIEW_CHANGE_SCREEN }
};

static GimpToggleActionEntry view_toggle_actions[] =
{
  { "view-dot-for-dot", NULL,
    N_("_Dot for Dot"), NULL, NULL,
    G_CALLBACK (view_dot_for_dot_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_DOT_FOR_DOT },

  { "view-show-selection", NULL,
    N_("Show _Selection"), "<control>T", NULL,
    G_CALLBACK (view_toggle_selection_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_SELECTION },

  { "view-show-layer-boundary", NULL,
    N_("Show _Layer Boundary"), NULL, NULL,
    G_CALLBACK (view_toggle_layer_boundary_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_LAYER_BOUNDARY },

  { "view-show-guides", NULL,
    N_("Show _Guides"), "<control><shift>T", NULL,
    G_CALLBACK (view_toggle_guides_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_GUIDES },

  { "view-snap-to-guides", NULL,
    N_("Sn_ap to Guides"), NULL, NULL,
    G_CALLBACK (view_snap_to_guides_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SNAP_TO_GUIDES },

  { "view-show-grid", NULL,
    N_("S_how Grid"), NULL, NULL,
    G_CALLBACK (view_toggle_grid_cmd_callback),
    FALSE,
    GIMP_HELP_VIEW_SHOW_GRID },

  { "view-snap-to-grid", NULL,
    N_("Sna_p to Grid"), NULL, NULL,
    G_CALLBACK (view_snap_to_grid_cmd_callback),
    FALSE,
    GIMP_HELP_VIEW_SNAP_TO_GRID },

  { "view-show-menubar", NULL,
    N_("Show _Menubar"), NULL, NULL,
    G_CALLBACK (view_toggle_menubar_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_MENUBAR },

  { "view-show-rulers", NULL,
    N_("Show R_ulers"), "<control><shift>R", NULL,
    G_CALLBACK (view_toggle_rulers_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_RULERS },

  { "view-show-scrollbars", NULL,
    N_("Show Scroll_bars"), NULL, NULL,
    G_CALLBACK (view_toggle_scrollbars_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_SCROLLBARS },

  { "view-show-statusbar", NULL,
    N_("Show S_tatusbar"), NULL, NULL,
    G_CALLBACK (view_toggle_statusbar_cmd_callback),
    TRUE,
    GIMP_HELP_VIEW_SHOW_STATUSBAR },

  { "view-fullscreen", NULL,
    N_("Fullscr_een"), "F11", NULL,
    G_CALLBACK (view_fullscreen_cmd_callback),
    FALSE,
    GIMP_HELP_VIEW_FULLSCREEN }
};

static GimpEnumActionEntry view_zoom_actions[] =
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

  { "view-zoom-out-skip", GTK_STOCK_ZOOM_OUT,
    "Zoom out a lot", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-in-skip", GTK_STOCK_ZOOM_IN,
    "Zoom in a lot", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    GIMP_HELP_VIEW_ZOOM_IN }
};

static GimpRadioActionEntry view_zoom_explicit_actions[] =
{
  { "view-zoom-16-1", NULL,
    N_("16:1  (1600%)"), NULL, NULL,
    160000,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-8-1", NULL,
    N_("8:1  (800%)"), NULL, NULL,
    80000,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-4-1", NULL,
    N_("4:1  (400%)"), NULL, NULL,
    40000,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-2-1", NULL,
    N_("2:1  (200%)"), NULL, NULL,
    20000,
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-1-1", GTK_STOCK_ZOOM_100,
    N_("1:1  (100%)"), "1",
    N_("Zoom 1:1"),
    10000,
    GIMP_HELP_VIEW_ZOOM_100 },

  { "view-zoom-1-2", NULL,
    N_("1:2  (50%)"), NULL, NULL,
    5000,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-1-4", NULL,
    N_("1:4  (25%)"), NULL, NULL,
    2500,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-1-8", NULL,
    N_("1:8  (12.5%)"), NULL, NULL,
    1250,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-1-16", NULL,
    N_("1:16  (6.25%)"), NULL, NULL,
    625,
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-other", NULL,
    N_("O_ther..."), NULL, NULL,
    0,
    GIMP_HELP_VIEW_ZOOM_OTHER }
};

static GimpEnumActionEntry view_padding_color_actions[] =
{
  { "view-padding-color-theme", NULL,
    N_("From _Theme"), NULL, NULL,
    GIMP_CANVAS_PADDING_MODE_DEFAULT, FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR },

  { "view-padding-color-light-check", NULL,
    N_("_Light Check Color"), NULL, NULL,
    GIMP_CANVAS_PADDING_MODE_LIGHT_CHECK, FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR },

  { "view-padding-color-dark-check", NULL,
    N_("_Dark Check Color"), NULL, NULL,
    GIMP_CANVAS_PADDING_MODE_DARK_CHECK, FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR },

  { "view-padding-color-custom", GTK_STOCK_SELECT_COLOR,
    N_("Select _Custom Color..."), NULL, NULL,
    GIMP_CANVAS_PADDING_MODE_CUSTOM, FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR },

  { "view-padding-color-prefs", GIMP_STOCK_RESET,
    N_("As in _Preferences"), NULL, NULL,
    GIMP_CANVAS_PADDING_MODE_RESET, FALSE,
    GIMP_HELP_VIEW_PADDING_COLOR }
};

static GimpEnumActionEntry view_scroll_horizontal_actions[] =
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

static GimpEnumActionEntry view_scroll_vertical_actions[] =
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
}

void
view_actions_update (GimpActionGroup *group,
                     gpointer         data)
{
  GimpDisplay        *gdisp      = action_data_get_display (data);
  GimpDisplayShell   *shell      = NULL;
  GimpDisplayOptions *options    = NULL;
  GimpImage          *gimage     = NULL;
  gboolean            fullscreen = FALSE;
  gint                n_screens  = 1;

  if (gdisp)
    {
      shell  = GIMP_DISPLAY_SHELL (gdisp->shell);
      gimage = gdisp->gimage;

      fullscreen = gimp_display_shell_get_fullscreen (shell);

      options = fullscreen ? shell->fullscreen_options : shell->options;

      n_screens =
        gdk_display_get_n_screens (gtk_widget_get_display (GTK_WIDGET (shell)));
    }

#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)
#define SET_VISIBLE(action,condition) \
        gimp_action_group_set_action_visible (group, action, (condition) != 0)
#define SET_LABEL(action,label) \
        gimp_action_group_set_action_label (group, action, (label))
#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)
#define SET_COLOR(action,color) \
        gimp_action_group_set_action_color (group, action, color, FALSE)

  SET_SENSITIVE ("view-new",   gdisp);
  SET_SENSITIVE ("view-close", gdisp);

  SET_SENSITIVE ("view-dot-for-dot", gdisp);
  SET_ACTIVE    ("view-dot-for-dot", gdisp && shell->dot_for_dot);

  SET_SENSITIVE ("view-zoom-out",    gdisp);
  SET_SENSITIVE ("view-zoom-in",     gdisp);
  SET_SENSITIVE ("view-zoom-fit-in", gdisp);
  SET_SENSITIVE ("view-zoom-fit-to", gdisp);

  SET_SENSITIVE ("view-zoom-16-1",  gdisp);
  SET_SENSITIVE ("view-zoom-8-1",   gdisp);
  SET_SENSITIVE ("view-zoom-4-1",   gdisp);
  SET_SENSITIVE ("view-zoom-2-1",   gdisp);
  SET_SENSITIVE ("view-zoom-1-1",   gdisp);
  SET_SENSITIVE ("view-zoom-1-2",   gdisp);
  SET_SENSITIVE ("view-zoom-1-4",   gdisp);
  SET_SENSITIVE ("view-zoom-1-8",   gdisp);
  SET_SENSITIVE ("view-zoom-1-16",  gdisp);
  SET_SENSITIVE ("view-zoom-other", gdisp);

  if (gdisp)
    view_actions_set_zoom (group, shell);

  SET_SENSITIVE ("view-info-window",       gdisp);
  SET_SENSITIVE ("view-navigation-window", gdisp);
  SET_SENSITIVE ("view-display-filters",   gdisp);

  SET_SENSITIVE ("view-show-selection",      gdisp);
  SET_ACTIVE    ("view-show-selection",      gdisp && options->show_selection);
  SET_SENSITIVE ("view-show-layer-boundary", gdisp);
  SET_ACTIVE    ("view-show-layer-boundary", gdisp && options->show_layer_boundary);
  SET_ACTIVE    ("view-show-guides",         gdisp && options->show_guides);
  SET_ACTIVE    ("view-snap-to-guides",      gdisp && shell->snap_to_guides);
  SET_ACTIVE    ("view-show-grid",           gdisp && options->show_grid);
  SET_ACTIVE    ("view-snap-to-grid",        gdisp && shell->snap_to_grid);

  if (gdisp)
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

  SET_SENSITIVE ("view-show-menubar",    gdisp);
  SET_ACTIVE    ("view-show-menubar",    gdisp && options->show_menubar);
  SET_SENSITIVE ("view-show-rulers",     gdisp);
  SET_ACTIVE    ("view-show-rulers",     gdisp && options->show_rulers);
  SET_SENSITIVE ("view-show-scrollbars", gdisp);
  SET_ACTIVE    ("view-show-scrollbars", gdisp && options->show_scrollbars);
  SET_SENSITIVE ("view-show-statusbar",  gdisp);
  SET_ACTIVE    ("view-show-statusbar",  gdisp && options->show_statusbar);

  SET_SENSITIVE ("view-shrink-wrap",    gdisp);
  SET_SENSITIVE ("view-fullscreen",     gdisp);
  SET_ACTIVE    ("view-fullscreen",     gdisp && fullscreen);
  SET_VISIBLE   ("view-move-to-screen", gdisp && n_screens > 1);

#undef SET_ACTIVE
#undef SET_VISIBLE
#undef SET_LABEL
#undef SET_SENSITIVE
#undef SET_COLOR
}


/*  private functions  */

static void
view_actions_set_zoom (GimpActionGroup  *group,
                       GimpDisplayShell *shell)
{
  const gchar *action = NULL;
  guint        scale;
  gchar        buf[16];
  gchar       *label;

  scale = ROUND (shell->scale * 1000);

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

  g_snprintf (buf, sizeof (buf),
              shell->scale >= 0.15 ? "%.0f%%" : "%.2f%%",
              shell->scale * 100.0);

  if (!action)
    {
      action = "view-zoom-other";

      label = g_strdup_printf (_("Other (%s) ..."), buf);
      gimp_action_group_set_action_label (group, action, label);
      g_free (label);

      shell->other_scale = shell->scale;
    }

  gimp_action_group_set_action_active (group, action, TRUE);

  label = g_strdup_printf (_("_Zoom (%s)"), buf);
  gimp_action_group_set_action_label (group, "view-zoom-menu", label);
  g_free (label);

  /* flag as dirty */
  shell->other_scale = - fabs (shell->other_scale);
}

static void
view_actions_check_type_notify (GimpDisplayConfig *config,
                                GParamSpec        *pspec,
                                GimpActionGroup   *group)
{
  GimpRGB color;

  gimp_rgba_set_uchar (&color,
                       render_blend_light_check[0],
                       render_blend_light_check[1],
                       render_blend_light_check[2],
                       255);
  gimp_action_group_set_action_color (group, "view-padding-color-light-check",
                                      &color, FALSE);

  gimp_rgba_set_uchar (&color,
                       render_blend_dark_check[0],
                       render_blend_dark_check[1],
                       render_blend_dark_check[2],
                       255);
  gimp_action_group_set_action_color (group, "view-padding-color-dark-check",
                                      &color, FALSE);
}
