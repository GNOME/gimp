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

#include "actions-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayoptions.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpdisplayshell-selection.h"

#include "view-actions.h"
#include "gui/view-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   view_actions_set_zoom (GimpActionGroup  *group,
                                     GimpDisplayShell *shell);


static GimpActionEntry view_actions[] =
{
  { "view-menu", NULL,
    N_("/_View") },

  { "view-zoom-menu", NULL,
    N_("_Zoom") },

  { "view-new", GTK_STOCK_NEW,
    N_("_New View"), "", NULL,
    G_CALLBACK (view_new_view_cmd_callback),
    GIMP_HELP_VIEW_NEW },

  { "view-zoom-out", GTK_STOCK_ZOOM_OUT,
    N_("Zoom _Out"), "minus", NULL,
    G_CALLBACK (view_zoom_out_cmd_callback),
    GIMP_HELP_VIEW_ZOOM_OUT },

  { "view-zoom-in", GTK_STOCK_ZOOM_IN,
    N_("Zoom _In"), "plus", NULL,
    G_CALLBACK (view_zoom_in_cmd_callback),
    GIMP_HELP_VIEW_ZOOM_IN },

  { "view-zoom-fit", GTK_STOCK_ZOOM_FIT,
    N_("Zoom to _Fit Window"), "<control><shift>E", NULL,
    G_CALLBACK (view_zoom_fit_cmd_callback),
    GIMP_HELP_VIEW_ZOOM_FIT },

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

  { "view-shrink-wrap", NULL,
    N_("Shrink _Wrap"), "<control>E", NULL,
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

  { "view-snap-to-giudes", NULL,
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

static GimpRadioActionEntry view_zoom_actions[] =
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

  { "view-zoom-1-1", NULL,
    N_("1:1  (100%)"), "1", NULL,
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
    "/View/Zoom/O_ther...", NULL, NULL,
    0,
    GIMP_HELP_VIEW_ZOOM_OTHER }
};


void
view_actions_setup (GimpActionGroup *group,
                    gpointer         data)
{
  gimp_action_group_add_actions (group,
                                 view_actions,
                                 G_N_ELEMENTS (view_actions),
                                 data);

  gimp_action_group_add_toggle_actions (group,
                                        view_toggle_actions,
                                        G_N_ELEMENTS (view_toggle_actions),
                                        data);

  gimp_action_group_add_radio_actions (group,
                                       view_zoom_actions,
                                       G_N_ELEMENTS (view_zoom_actions),
                                       10000,
                                       G_CALLBACK (view_zoom_cmd_callback),
                                       data);
}

void
view_actions_update (GimpActionGroup *group,
                     gpointer         data)
{
  GimpDisplay        *gdisp      = NULL;
  GimpDisplayShell   *shell      = NULL;
  GimpDisplayOptions *options    = NULL;
  GimpImage          *gimage     = NULL;
  gboolean            fullscreen = FALSE;
  gint                n_screens  = 1;

  if (GIMP_IS_DISPLAY_SHELL (data))
    {
      shell = GIMP_DISPLAY_SHELL (data);
      gdisp = shell->gdisp;
    }
  else if (GIMP_IS_DISPLAY (data))
    {
      gdisp = GIMP_DISPLAY (data);
      shell = GIMP_DISPLAY_SHELL (gdisp->shell);
    }

  if (gdisp)
    {
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

  SET_SENSITIVE ("view-new", gdisp);

  SET_SENSITIVE ("view-dot-for-dot", gdisp);
  SET_ACTIVE    ("view-dot-for-dot", gdisp && shell->dot_for_dot);

  SET_SENSITIVE ("view-zoom-out", gdisp);
  SET_SENSITIVE ("view-zoom-in",  gdisp);
  SET_SENSITIVE ("view-zoom-fit", gdisp);

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
      action = "/View/Zoom/Other...";

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
