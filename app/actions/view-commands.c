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

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpdisplayshell-filter-dialog.h"
#include "display/gimpdisplayshell-scale.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpuimanager.h"

#include "gui/dialogs.h"
#include "gui/info-dialog.h"
#include "gui/info-window.h"

#include "actions.h"
#include "view-commands.h"


#define return_if_no_display(gdisp, data) \
  gdisp = action_data_get_display (data); \
  if (! gdisp) \
    return

#define SET_ACTIVE(manager,group_name,action_name,active) \
  { GimpActionGroup *group = \
      gimp_ui_manager_get_action_group (manager, group_name); \
    gimp_action_group_set_action_active (group, action_name, active); }

#define IS_ACTIVE_DISPLAY(gdisp) \
  ((gdisp) == \
   gimp_context_get_display (gimp_get_user_context ((gdisp)->gimage->gimp)))


void
view_zoom_out_cmd_callback (GtkAction *action,
                            gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  gimp_display_shell_scale (GIMP_DISPLAY_SHELL (gdisp->shell),
                            GIMP_ZOOM_OUT, 0.0);
}

void
view_zoom_in_cmd_callback (GtkAction *action,
                           gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  gimp_display_shell_scale (GIMP_DISPLAY_SHELL (gdisp->shell),
                            GIMP_ZOOM_IN, 0.0);
}

void
view_zoom_fit_cmd_callback (GtkAction *action,
                            gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  gimp_display_shell_scale_fit (GIMP_DISPLAY_SHELL (gdisp->shell));
}

void
view_zoom_cmd_callback (GtkAction *action,
                        GtkAction *current,
			gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  gint              value;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  value = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  if (value == 0 /* Other... */)
    {
      /* check if we are activated by the user
       * or from view_actions_set_zoom()
       */
      if (shell->scale != shell->other_scale)
        gimp_display_shell_scale_dialog (shell);
    }
  else
    {
      if (fabs (value - shell->scale) > 0.0001)
        gimp_display_shell_scale (shell, GIMP_ZOOM_TO, (gdouble) value / 10000);
    }
}

void
view_dot_for_dot_cmd_callback (GtkAction *action,
			       gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (active != shell->dot_for_dot)
    {
      gimp_display_shell_scale_set_dot_for_dot (shell, active);

      SET_ACTIVE (shell->menubar_manager, "view", "view-dot-for-dot",
                  shell->dot_for_dot);

      if (IS_ACTIVE_DISPLAY (gdisp))
        SET_ACTIVE (shell->popup_manager, "view", "view-dot-for-dot",
                    shell->dot_for_dot);
    }
}

void
view_info_window_cmd_callback (GtkAction *action,
			       gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  if (GIMP_GUI_CONFIG (gdisp->gimage->gimp->config)->info_window_per_display)
    {
      if (! shell->info_dialog)
	shell->info_dialog = info_window_create (gdisp);

      /* To update the fields of the info window for the first time. *
       * It's no use updating it in info_window_create() because the *
       * pointer of the info window is not present in the shell yet. */
      info_window_update (gdisp);

      info_dialog_present (shell->info_dialog);
    }
  else
    {
      info_window_follow_auto (gdisp->gimage->gimp);
    }
}

void
view_navigation_window_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_dialog_factory_dialog_raise (global_dock_factory,
                                    gtk_widget_get_screen (gdisp->shell),
                                    "gimp-navigation-view", -1);
}

void
view_display_filters_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  if (! shell->filters_dialog)
    {
      shell->filters_dialog = gimp_display_shell_filter_dialog_new (shell);

      g_signal_connect (shell->filters_dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &shell->filters_dialog);
    }

  gtk_window_present (GTK_WINDOW (shell->filters_dialog));
}

void
view_toggle_selection_cmd_callback (GtkAction *action,
				    gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_show_selection (shell, active);
}

void
view_toggle_layer_boundary_cmd_callback (GtkAction *action,
                                         gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_show_layer (shell, active);
}

void
view_toggle_menubar_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_show_menubar (shell, active);
}

void
view_toggle_rulers_cmd_callback (GtkAction *action,
				 gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_show_rulers (shell, active);
}

void
view_toggle_scrollbars_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_show_scrollbars (shell, active);
}

void
view_toggle_statusbar_cmd_callback (GtkAction *action,
				    gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_show_statusbar (shell, active);
}

void
view_toggle_guides_cmd_callback (GtkAction *action,
				 gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_show_guides (shell, active);
}

void
view_snap_to_guides_cmd_callback (GtkAction *action,
				  gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_snap_to_guides (shell, active);
}


void
view_toggle_grid_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_show_grid (shell, active);
}

void
view_snap_to_grid_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_snap_to_grid (shell, active);
}

void
view_new_view_cmd_callback (GtkAction *action,
			    gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  gimp_create_display (gdisp->gimage->gimp, gdisp->gimage,
                       GIMP_DISPLAY_SHELL (gdisp->shell)->scale);
}

void
view_shrink_wrap_cmd_callback (GtkAction *action,
			       gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  gimp_display_shell_scale_shrink_wrap (GIMP_DISPLAY_SHELL (gdisp->shell));
}

void
view_fullscreen_cmd_callback (GtkAction *action,
			      gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  gboolean          active;
  gboolean          fullscreen;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_fullscreen (shell, active);

  fullscreen = (shell->window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0;

  if (active != fullscreen)
    {
      SET_ACTIVE (shell->menubar_manager, "view", "view-fullscreen",
                  fullscreen);

      if (IS_ACTIVE_DISPLAY (gdisp))
        SET_ACTIVE (shell->popup_manager, "view", "view-fullscreen",
                    fullscreen);
    }
}

static void
view_change_screen_confirm_callback (GtkWidget *query_box,
                                     gint       value,
                                     gpointer   data)
{
  GdkScreen *screen;

  screen = gdk_display_get_screen (gtk_widget_get_display (GTK_WIDGET (data)),
                                   value);

  if (screen)
    gtk_window_set_screen (GTK_WINDOW (data), screen);
}

static void
view_change_screen_destroy_callback (GtkWidget *query_box,
                                     GtkWidget *shell)
{
  g_object_set_data (G_OBJECT (shell), "gimp-change-screen-dialog", NULL);
}

void
view_change_screen_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GimpDisplay *gdisp;
  GdkScreen   *screen;
  GdkDisplay  *display;
  gint         cur_screen;
  gint         num_screens;
  GtkWidget   *qbox;
  return_if_no_display (gdisp, data);

  qbox = g_object_get_data (G_OBJECT (gdisp->shell),
                            "gimp-change-screen-dialog");

  if (qbox)
    {
      gtk_window_present (GTK_WINDOW (qbox));
      return;
    }

  screen  = gtk_widget_get_screen (gdisp->shell);
  display = gtk_widget_get_display (gdisp->shell);

  cur_screen  = gdk_screen_get_number (screen);
  num_screens = gdk_display_get_n_screens (display);

  qbox = gimp_query_int_box ("Move Display to Screen",
                             gdisp->shell,
                             NULL, 0,
                             "Enter destination screen",
                             cur_screen, 0, num_screens - 1,
                             G_OBJECT (gdisp->shell), "destroy",
                             view_change_screen_confirm_callback,
                             gdisp->shell);

  g_object_set_data (G_OBJECT (gdisp->shell), "gimp-change-screen-dialog", qbox);

  g_signal_connect (qbox, "destroy",
                    G_CALLBACK (view_change_screen_destroy_callback),
                    gdisp->shell);

  gtk_widget_show (qbox);
}
