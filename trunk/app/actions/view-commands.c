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
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayoptions.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpdisplayshell-close.h"
#include "display/gimpdisplayshell-filter-dialog.h"
#include "display/gimpdisplayshell-scale.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpcolordialog.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpuimanager.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "view-commands.h"

#include "gimp-intl.h"


#define SET_ACTIVE(manager,action_name,active) \
  { GimpActionGroup *group = \
      gimp_ui_manager_get_action_group (manager, "view"); \
    gimp_action_group_set_action_active (group, action_name, active); }

#define IS_ACTIVE_DISPLAY(display) \
  ((display) == \
   gimp_context_get_display (gimp_get_user_context ((display)->image->gimp)))


/*  local function prototypes  */

static void   view_padding_color_dialog_update (GimpColorDialog      *dialog,
                                                const GimpRGB        *color,
                                                GimpColorDialogState  state,
                                                GimpDisplayShell     *shell);


/*  public functions  */

void
view_new_cmd_callback (GtkAction *action,
                       gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  gimp_create_display (display->image->gimp,
                       display->image,
                       shell->unit, gimp_zoom_model_get_factor (shell->zoom));
}

void
view_zoom_fit_in_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpDisplay *display;
  return_if_no_display (display, data);

  gimp_display_shell_scale_fit_in (GIMP_DISPLAY_SHELL (display->shell));
}

void
view_zoom_fit_to_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpDisplay *display;
  return_if_no_display (display, data);

  gimp_display_shell_scale_fit_to (GIMP_DISPLAY_SHELL (display->shell));
}

void
view_zoom_revert_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpDisplay *display;
  return_if_no_display (display, data);

  gimp_display_shell_scale_revert (GIMP_DISPLAY_SHELL (display->shell));
}

void
view_zoom_cmd_callback (GtkAction *action,
                        gint       value,
                        gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  switch ((GimpActionSelectType) value)
    {
    case GIMP_ACTION_SELECT_FIRST:
      gimp_display_shell_scale (shell, GIMP_ZOOM_OUT_MAX, 0.0);
      break;

    case GIMP_ACTION_SELECT_LAST:
      gimp_display_shell_scale (shell, GIMP_ZOOM_IN_MAX, 0.0);
      break;

    case GIMP_ACTION_SELECT_PREVIOUS:
      gimp_display_shell_scale (shell, GIMP_ZOOM_OUT, 0.0);
      break;

    case GIMP_ACTION_SELECT_NEXT:
      gimp_display_shell_scale (shell, GIMP_ZOOM_IN, 0.0);
      break;

    case GIMP_ACTION_SELECT_SKIP_PREVIOUS:
      gimp_display_shell_scale (shell, GIMP_ZOOM_OUT_MORE, 0.0);
      break;

    case GIMP_ACTION_SELECT_SKIP_NEXT:
      gimp_display_shell_scale (shell, GIMP_ZOOM_IN_MORE, 0.0);
      break;

    default:
      {
        gdouble scale = gimp_zoom_model_get_factor (shell->zoom);

        scale = action_select_value ((GimpActionSelectType) value,
                                     scale,
                                     0.0, 512.0,
                                     1.0 / 8.0, 1.0, 16.0, 0.0,
                                     FALSE);

        /* min = 1.0 / 256,  max = 256.0                */
        /* scale = min *  (max / min)**(i/n), i = 0..n  */
        scale = pow (65536.0, scale / 512.0) / 256.0;

        gimp_display_shell_scale (shell, GIMP_ZOOM_TO, scale);
        break;
      }
    }
}

void
view_zoom_explicit_cmd_callback (GtkAction *action,
                                 GtkAction *current,
                                 gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gint              value;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  value = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  if (value != 0 /* not Other... */)
    {
      if (fabs (value - gimp_zoom_model_get_factor (shell->zoom)) > 0.0001)
        gimp_display_shell_scale (shell, GIMP_ZOOM_TO, (gdouble) value / 10000);
    }
}

void
view_zoom_other_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  /* check if we are activated by the user or from
   * view_actions_set_zoom()
   */
  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)) &&
      shell->other_scale != gimp_zoom_model_get_factor (shell->zoom))
    {
      gimp_display_shell_scale_dialog (shell);
    }
}

void
view_dot_for_dot_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (active != shell->dot_for_dot)
    {
      gimp_display_shell_scale_set_dot_for_dot (shell, active);

      SET_ACTIVE (shell->menubar_manager, "view-dot-for-dot",
                  shell->dot_for_dot);

      if (IS_ACTIVE_DISPLAY (display))
        SET_ACTIVE (shell->popup_manager, "view-dot-for-dot",
                    shell->dot_for_dot);
    }
}

void
view_scroll_horizontal_cmd_callback (GtkAction *action,
                                     gint       value,
                                     gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gdouble           offset;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  offset = action_select_value ((GimpActionSelectType) value,
                                shell->hsbdata->value,
                                shell->hsbdata->lower,
                                shell->hsbdata->upper -
                                shell->hsbdata->page_size,
                                1,
                                shell->hsbdata->step_increment,
                                shell->hsbdata->page_increment,
                                0,
                                FALSE);
  gtk_adjustment_set_value (shell->hsbdata, offset);
}

void
view_scroll_vertical_cmd_callback (GtkAction *action,
                                   gint       value,
                                   gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gdouble           offset;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  offset = action_select_value ((GimpActionSelectType) value,
                                shell->vsbdata->value,
                                shell->vsbdata->lower,
                                shell->vsbdata->upper -
                                shell->vsbdata->page_size,
                                1,
                                shell->vsbdata->step_increment,
                                shell->vsbdata->page_increment,
                                0,
                                FALSE);
  gtk_adjustment_set_value (shell->vsbdata, offset);
}

void
view_navigation_window_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  gimp_dialog_factory_dialog_raise (global_dock_factory,
                                    gtk_widget_get_screen (display->shell),
                                    "gimp-navigation-view", -1);
}

void
view_display_filters_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

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
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_show_selection (shell, active);
}

void
view_toggle_layer_boundary_cmd_callback (GtkAction *action,
                                         gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_show_layer (shell, active);
}

void
view_toggle_menubar_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_show_menubar (shell, active);
}

void
view_toggle_rulers_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_show_rulers (shell, active);
}

void
view_toggle_scrollbars_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_show_scrollbars (shell, active);
}

void
view_toggle_statusbar_cmd_callback (GtkAction *action,
                                    gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_show_statusbar (shell, active);
}

void
view_toggle_guides_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_show_guides (shell, active);
}

void
view_toggle_grid_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_show_grid (shell, active);
}

void
view_toggle_sample_points_cmd_callback (GtkAction *action,
                                        gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_show_sample_points (shell, active);
}

void
view_snap_to_guides_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_snap_to_guides (shell, active);
}

void
view_snap_to_grid_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_snap_to_grid (shell, active);
}

void
view_snap_to_canvas_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_snap_to_canvas (shell, active);
}

void
view_snap_to_vectors_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_snap_to_vectors (shell, active);
}

void
view_padding_color_cmd_callback (GtkAction *action,
                                 gint       value,
                                 gpointer   data)
{
  GimpDisplay        *display;
  GimpDisplayShell   *shell;
  GimpDisplayOptions *options;
  gboolean            fullscreen;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  fullscreen = gimp_display_shell_get_fullscreen (shell);

  if (fullscreen)
    options = shell->fullscreen_options;
  else
    options = shell->options;

  switch ((GimpCanvasPaddingMode) value)
    {
    case GIMP_CANVAS_PADDING_MODE_DEFAULT:
    case GIMP_CANVAS_PADDING_MODE_LIGHT_CHECK:
    case GIMP_CANVAS_PADDING_MODE_DARK_CHECK:
      g_object_set_data (G_OBJECT (shell), "padding-color-dialog", NULL);

      options->padding_mode_set = TRUE;

      gimp_display_shell_set_padding (shell, (GimpCanvasPaddingMode) value,
                                      &options->padding_color);
      break;

    case GIMP_CANVAS_PADDING_MODE_CUSTOM:
      {
        GtkWidget *color_dialog;

        color_dialog = g_object_get_data (G_OBJECT (shell),
                                          "padding-color-dialog");

        if (! color_dialog)
          {
            color_dialog =
              gimp_color_dialog_new (GIMP_VIEWABLE (display->image),
                                     action_data_get_context (data),
                                     _("Set Canvas Padding Color"),
                                     GTK_STOCK_SELECT_COLOR,
                                     _("Set Custom Canvas Padding Color"),
                                     display->shell,
                                     NULL, NULL,
                                     &options->padding_color,
                                     FALSE, FALSE);

            g_signal_connect (color_dialog, "update",
                              G_CALLBACK (view_padding_color_dialog_update),
                              display->shell);

            g_object_set_data_full (G_OBJECT (shell), "padding-color-dialog",
                                    color_dialog,
                                    (GDestroyNotify) gtk_widget_destroy);
          }

        gtk_window_present (GTK_WINDOW (color_dialog));
      }
      break;

    case GIMP_CANVAS_PADDING_MODE_RESET:
      g_object_set_data (G_OBJECT (shell), "padding-color-dialog", NULL);

      {
        GimpDisplayConfig  *config;
        GimpDisplayOptions *default_options;

        config = GIMP_DISPLAY_CONFIG (display->image->gimp->config);

        options->padding_mode_set = FALSE;

        if (fullscreen)
          default_options = config->default_fullscreen_view;
        else
          default_options = config->default_view;

        gimp_display_shell_set_padding (shell,
                                        default_options->padding_mode,
                                        &default_options->padding_color);
      }
      break;
    }
}

void
view_shrink_wrap_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpDisplay *display;
  return_if_no_display (display, data);

  gimp_display_shell_scale_shrink_wrap (GIMP_DISPLAY_SHELL (display->shell));
}

void
view_fullscreen_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = GIMP_DISPLAY_SHELL (display->shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  gimp_display_shell_set_fullscreen (shell, active);
}


/*  private functions  */

static void
view_padding_color_dialog_update (GimpColorDialog      *dialog,
                                  const GimpRGB        *color,
                                  GimpColorDialogState  state,
                                  GimpDisplayShell     *shell)
{
  GimpDisplayOptions *options;
  gboolean            fullscreen;

  fullscreen = gimp_display_shell_get_fullscreen (shell);

  if (fullscreen)
    options = shell->fullscreen_options;
  else
    options = shell->options;

  switch (state)
    {
    case GIMP_COLOR_DIALOG_OK:
      options->padding_mode_set = TRUE;

      gimp_display_shell_set_padding (shell, GIMP_CANVAS_PADDING_MODE_CUSTOM,
                                      color);
      /* fallthru */

    case GIMP_COLOR_DIALOG_CANCEL:
      g_object_set_data (G_OBJECT (shell), "padding-color-dialog", NULL);
      break;

    default:
      break;
    }
}
