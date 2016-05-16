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
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpdisplayoptions.h"
#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpgrouplayer.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpcolordialog.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"
#include "widgets/gimpwindowstrategy.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpdisplayshell-filter-dialog.h"
#include "display/gimpdisplayshell-rotate.h"
#include "display/gimpdisplayshell-rotate-dialog.h"
#include "display/gimpdisplayshell-scale.h"
#include "display/gimpdisplayshell-scale-dialog.h"
#include "display/gimpdisplayshell-scroll.h"
#include "display/gimpdisplayshell-close.h"
#include "display/gimpimagewindow.h"

#include "actions.h"
#include "view-commands.h"

#include "gimp-intl.h"


#define SET_ACTIVE(manager,action_name,active) \
  { GimpActionGroup *group = \
      gimp_ui_manager_get_action_group (manager, "view"); \
    gimp_action_group_set_action_active (group, action_name, active); }

#define IS_ACTIVE_DISPLAY(display) \
  ((display) == \
   gimp_context_get_display (gimp_get_user_context ((display)->gimp)))


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

  shell = gimp_display_get_shell (display);

  gimp_create_display (display->gimp,
                       gimp_display_get_image (display),
                       shell->unit, gimp_zoom_model_get_factor (shell->zoom),
                       G_OBJECT (gtk_widget_get_screen (GTK_WIDGET (shell))),
                       gimp_widget_get_monitor (GTK_WIDGET (shell)));
}

void
view_close_cmd_callback (GtkAction *action,
                         gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  GimpImage        *image;
  return_if_no_display (display, data);

  shell = gimp_display_get_shell (display);
  image = gimp_display_get_image (display);

  /* Check for the image so we don't close the last display. */
  if (image)
    gimp_display_shell_close (shell, FALSE);
}

void
view_zoom_fit_in_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpDisplay *display;
  return_if_no_display (display, data);

  gimp_display_shell_scale_fit_in (gimp_display_get_shell (display));
}

void
view_zoom_fill_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpDisplay *display;
  return_if_no_display (display, data);

  gimp_display_shell_scale_fill (gimp_display_get_shell (display));
}

void
view_zoom_selection_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpDisplay *display;
  GimpImage   *image;
  gint         x, y, width, height;
  return_if_no_display (display, data);
  return_if_no_image (image, data);

  gimp_item_bounds (GIMP_ITEM (gimp_image_get_mask (image)),
                    &x, &y, &width, &height);

  gimp_display_shell_scale_to_rectangle (gimp_display_get_shell (display),
                                         GIMP_ZOOM_IN,
                                         x, y, width, height,
                                         FALSE);
}

void
view_zoom_revert_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpDisplay *display;
  return_if_no_display (display, data);

  gimp_display_shell_scale_revert (gimp_display_get_shell (display));
}

void
view_zoom_cmd_callback (GtkAction *action,
                        gint       value,
                        gpointer   data)
{
  GimpDisplayShell *shell;
  return_if_no_shell (shell, data);

  switch ((GimpActionSelectType) value)
    {
    case GIMP_ACTION_SELECT_FIRST:
      gimp_display_shell_scale (shell,
                                GIMP_ZOOM_OUT_MAX,
                                0.0,
                                GIMP_ZOOM_FOCUS_BEST_GUESS);
      break;

    case GIMP_ACTION_SELECT_LAST:
      gimp_display_shell_scale (shell,
                                GIMP_ZOOM_IN_MAX,
                                0.0,
                                GIMP_ZOOM_FOCUS_BEST_GUESS);
      break;

    case GIMP_ACTION_SELECT_PREVIOUS:
      gimp_display_shell_scale (shell,
                                GIMP_ZOOM_OUT,
                                0.0,
                                GIMP_ZOOM_FOCUS_BEST_GUESS);
      break;

    case GIMP_ACTION_SELECT_NEXT:
      gimp_display_shell_scale (shell,
                                GIMP_ZOOM_IN,
                                0.0,
                                GIMP_ZOOM_FOCUS_BEST_GUESS);
      break;

    case GIMP_ACTION_SELECT_SKIP_PREVIOUS:
      gimp_display_shell_scale (shell,
                                GIMP_ZOOM_OUT_MORE,
                                0.0,
                                GIMP_ZOOM_FOCUS_BEST_GUESS);
      break;

    case GIMP_ACTION_SELECT_SKIP_NEXT:
      gimp_display_shell_scale (shell,
                                GIMP_ZOOM_IN_MORE,
                                0.0,
                                GIMP_ZOOM_FOCUS_BEST_GUESS);
      break;

    default:
      {
        gdouble scale = gimp_zoom_model_get_factor (shell->zoom);

        scale = action_select_value ((GimpActionSelectType) value,
                                     scale,
                                     0.0, 512.0, 1.0,
                                     1.0 / 8.0, 1.0, 16.0, 0.0,
                                     FALSE);

        /* min = 1.0 / 256,  max = 256.0                */
        /* scale = min *  (max / min)**(i/n), i = 0..n  */
        scale = pow (65536.0, scale / 512.0) / 256.0;

        gimp_display_shell_scale (shell,
                                  GIMP_ZOOM_TO,
                                  scale,
                                  GIMP_ZOOM_FOCUS_BEST_GUESS);
        break;
      }
    }
}

void
view_zoom_explicit_cmd_callback (GtkAction *action,
                                 GtkAction *current,
                                 gpointer   data)
{
  GimpDisplayShell *shell;
  gint              value;
  return_if_no_shell (shell, data);

  value = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  if (value != 0 /* not Other... */)
    {
      if (fabs (value - gimp_zoom_model_get_factor (shell->zoom)) > 0.0001)
        gimp_display_shell_scale (shell,
                                  GIMP_ZOOM_TO,
                                  (gdouble) value / 10000,
                                  GIMP_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS);
    }
}

void
view_zoom_other_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpDisplayShell *shell;
  return_if_no_shell (shell, data);

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

  shell = gimp_display_get_shell (display);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (active != shell->dot_for_dot)
    {
      GimpImageWindow *window = gimp_display_shell_get_window (shell);

      gimp_display_shell_scale_set_dot_for_dot (shell, active);

      if (window)
        SET_ACTIVE (gimp_image_window_get_ui_manager (window),
                    "view-dot-for-dot", shell->dot_for_dot);

      if (IS_ACTIVE_DISPLAY (display))
        SET_ACTIVE (shell->popup_manager, "view-dot-for-dot",
                    shell->dot_for_dot);
    }
}

void
view_flip_horizontally_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = gimp_display_get_shell (display);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (active != shell->flip_horizontally)
    {
      gimp_display_shell_flip (shell, active, shell->flip_vertically);
    }
}

void
view_flip_vertically_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = gimp_display_get_shell (display);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (active != shell->flip_vertically)
    {
      gimp_display_shell_flip (shell, shell->flip_horizontally, active);
    }
}

void
view_rotate_absolute_cmd_callback (GtkAction *action,
                                   gint       value,
                                   gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gdouble           angle = 0.0;
  return_if_no_display (display, data);

  shell = gimp_display_get_shell (display);

  angle = action_select_value ((GimpActionSelectType) value,
                               0.0,
                               -180.0, 180.0, 0.0,
                               1.0, 15.0, 90.0, 0.0,
                               TRUE);

  gimp_display_shell_rotate_to (shell, angle);

  if (value == GIMP_ACTION_SELECT_SET_TO_DEFAULT)
    gimp_display_shell_flip (shell, FALSE, FALSE);
}

void
view_rotate_relative_cmd_callback (GtkAction *action,
                                   gint       value,
                                   gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gdouble           delta = 0.0;
  return_if_no_display (display, data);

  shell = gimp_display_get_shell (display);

  delta = action_select_value ((GimpActionSelectType) value,
                               0.0,
                               -180.0, 180.0, 0.0,
                               1.0, 15.0, 90.0, 0.0,
                               TRUE);

  gimp_display_shell_rotate (shell, delta);
}

void
view_rotate_other_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  return_if_no_display (display, data);

  shell = gimp_display_get_shell (display);

  gimp_display_shell_rotate_dialog (shell);
}

void
view_scroll_horizontal_cmd_callback (GtkAction *action,
                                     gint       value,
                                     gpointer   data)
{
  GimpDisplayShell *shell;
  gdouble           offset;
  return_if_no_shell (shell, data);

  offset = action_select_value ((GimpActionSelectType) value,
                                gtk_adjustment_get_value (shell->hsbdata),
                                gtk_adjustment_get_lower (shell->hsbdata),
                                gtk_adjustment_get_upper (shell->hsbdata) -
                                gtk_adjustment_get_page_size (shell->hsbdata),
                                gtk_adjustment_get_lower (shell->hsbdata),
                                1,
                                gtk_adjustment_get_step_increment (shell->hsbdata),
                                gtk_adjustment_get_page_increment (shell->hsbdata),
                                0,
                                FALSE);

  gtk_adjustment_set_value (shell->hsbdata, offset);
}

void
view_scroll_vertical_cmd_callback (GtkAction *action,
                                   gint       value,
                                   gpointer   data)
{
  GimpDisplayShell *shell;
  gdouble           offset;
  return_if_no_shell (shell, data);

  offset = action_select_value ((GimpActionSelectType) value,
                                gtk_adjustment_get_value (shell->vsbdata),
                                gtk_adjustment_get_lower (shell->vsbdata),
                                gtk_adjustment_get_upper (shell->vsbdata) -
                                gtk_adjustment_get_page_size (shell->vsbdata),
                                gtk_adjustment_get_lower (shell->vsbdata),
                                1,
                                gtk_adjustment_get_step_increment (shell->vsbdata),
                                gtk_adjustment_get_page_increment (shell->vsbdata),
                                0,
                                FALSE);

  gtk_adjustment_set_value (shell->vsbdata, offset);
}

void
view_navigation_window_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  Gimp             *gimp;
  GimpDisplayShell *shell;
  return_if_no_gimp (gimp, data);
  return_if_no_shell (shell, data);

  gimp_window_strategy_show_dockable_dialog (GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (gimp)),
                                             gimp,
                                             gimp_dialog_factory_get_singleton (),
                                             gtk_widget_get_screen (GTK_WIDGET (shell)),
                                             gimp_widget_get_monitor (GTK_WIDGET (shell)),
                                             "gimp-navigation-view");
}

void
view_display_filters_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpDisplayShell *shell;
  return_if_no_shell (shell, data);

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
view_color_management_reset_cmd_callback (GtkAction *action,
                                          gpointer   data)
{
  GimpDisplayShell *shell;
  GimpColorConfig  *global_config;
  GimpColorConfig  *shell_config;
  return_if_no_shell (shell, data);

  global_config = GIMP_CORE_CONFIG (shell->display->config)->color_management;
  shell_config  = gimp_display_shell_get_color_config (shell);

  gimp_config_copy (GIMP_CONFIG (global_config),
                    GIMP_CONFIG (shell_config),
                    0);
  shell->color_config_set = FALSE;
}

void
view_color_management_mode_cmd_callback (GtkAction *action,
                                         GtkAction *current,
                                         gpointer   data)
{
  GimpDisplayShell        *shell;
  GimpColorConfig         *color_config;
  GimpColorManagementMode  value;
  return_if_no_shell (shell, data);

  color_config = gimp_display_shell_get_color_config (shell);

  value = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  if (value != color_config->mode)
    {
      g_object_set (color_config,
                    "mode", value,
                    NULL);
      shell->color_config_set = TRUE;
    }
}

void
view_color_management_intent_cmd_callback (GtkAction *action,
                                           GtkAction *current,
                                           gpointer   data)
{
  GimpDisplayShell          *shell;
  GimpColorConfig           *color_config;
  GimpColorRenderingIntent   value;
  return_if_no_shell (shell, data);

  color_config = gimp_display_shell_get_color_config (shell);

  value = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  switch (color_config->mode)
    {
    case GIMP_COLOR_MANAGEMENT_DISPLAY:
      if (value != color_config->display_intent)
        {
          g_object_set (color_config,
                        "display-rendering-intent", value,
                        NULL);
          shell->color_config_set = TRUE;
        }
      break;

    case GIMP_COLOR_MANAGEMENT_SOFTPROOF:
      if (value != color_config->simulation_intent)
        {
          g_object_set (color_config,
                        "simulation-rendering-intent", value,
                        NULL);
          shell->color_config_set = TRUE;
        }
      break;

    default:
      break;
    }
}

void
view_color_management_bpc_cmd_callback (GtkAction *action,
                                        gpointer   data)
{
  GimpDisplayShell *shell;
  GimpColorConfig  *color_config;
  gboolean          active;
  return_if_no_shell (shell, data);

  color_config = gimp_display_shell_get_color_config (shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  switch (color_config->mode)
    {
    case GIMP_COLOR_MANAGEMENT_DISPLAY:
      if (active != color_config->display_use_black_point_compensation)
        {
          g_object_set (color_config,
                        "display-use-black-point-compensation", active,
                        NULL);
          shell->color_config_set = TRUE;
        }
      break;

    case GIMP_COLOR_MANAGEMENT_SOFTPROOF:
      if (active != color_config->simulation_use_black_point_compensation)
        {
          g_object_set (color_config,
                        "simulation-use-black-point-compensation", active,
                        NULL);
          shell->color_config_set = TRUE;
        }
      break;

    default:
      break;
    }
}

void
view_color_management_gamut_check_cmd_callback (GtkAction *action,
                                                gpointer   data)
{
  GimpDisplayShell *shell;
  GimpColorConfig  *color_config;
  gboolean          active;
  return_if_no_shell (shell, data);

  color_config = gimp_display_shell_get_color_config (shell);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (active != color_config->simulation_gamut_check)
    {
      g_object_set (color_config,
                    "simulation-gamut-check", active,
                    NULL);
      shell->color_config_set = TRUE;
    }
}

void
view_toggle_selection_cmd_callback (GtkAction *action,
                                    gpointer   data)
{
  GimpDisplayShell *shell;
  gboolean          active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
  return_if_no_shell (shell, data);

  if (active == gimp_display_shell_get_show_selection (shell))
    return;

  gimp_display_shell_set_show_selection (shell, active);
}

void
view_toggle_layer_boundary_cmd_callback (GtkAction *action,
                                         gpointer   data)
{
  GimpDisplayShell *shell;
  gboolean          active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
  return_if_no_shell (shell, data);

  if (active == gimp_display_shell_get_show_layer (shell))
    return;

  gimp_display_shell_set_show_layer (shell, active);
}

void
view_toggle_menubar_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpDisplayShell *shell;
  gboolean          active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
  return_if_no_shell (shell, data);

  if (active == gimp_display_shell_get_show_menubar (shell))
    return;

  gimp_display_shell_set_show_menubar (shell, active);
}

void
view_toggle_rulers_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GimpDisplayShell *shell;
  gboolean          active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
  return_if_no_shell (shell, data);

  if (active == gimp_display_shell_get_show_rulers (shell))
    return;

  gimp_display_shell_set_show_rulers (shell, active);
}

void
view_toggle_scrollbars_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpDisplayShell *shell;
  gboolean          active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
  return_if_no_shell (shell, data);

  if (active == gimp_display_shell_get_show_scrollbars (shell))
    return;

  gimp_display_shell_set_show_scrollbars (shell, active);
}

void
view_toggle_statusbar_cmd_callback (GtkAction *action,
                                    gpointer   data)
{
  GimpDisplayShell *shell;
  gboolean          active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
  return_if_no_shell (shell, data);

  if (active == gimp_display_shell_get_show_statusbar (shell))
    return;

  gimp_display_shell_set_show_statusbar (shell, active);
}

void
view_toggle_guides_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GimpDisplayShell *shell;
  gboolean          active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
  return_if_no_shell (shell, data);

  if (active == gimp_display_shell_get_show_guides (shell))
    return;

  gimp_display_shell_set_show_guides (shell, active);
}

void
view_toggle_grid_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GimpDisplayShell *shell;
  gboolean          active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
  return_if_no_shell (shell, data);

  if (active == gimp_display_shell_get_show_grid (shell))
    return;

  gimp_display_shell_set_show_grid (shell, active);
}

void
view_toggle_sample_points_cmd_callback (GtkAction *action,
                                        gpointer   data)
{
  GimpDisplayShell *shell;
  gboolean          active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
  return_if_no_shell (shell, data);

  if (active == gimp_display_shell_get_show_sample_points (shell))
    return;

  gimp_display_shell_set_show_sample_points (shell, active);
}

void
view_snap_to_guides_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpDisplayShell *shell;
  gboolean          active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
  return_if_no_shell (shell, data);

  if (active == gimp_display_shell_get_snap_to_guides (shell))
    return;

  gimp_display_shell_set_snap_to_guides (shell, active);
}

void
view_snap_to_grid_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  GimpDisplayShell *shell;
  gboolean          active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
  return_if_no_shell (shell, data);

  if (active == gimp_display_shell_get_snap_to_grid (shell))
    return;

  gimp_display_shell_set_snap_to_grid (shell, active);
}

void
view_snap_to_canvas_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpDisplayShell *shell;
  gboolean          active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
  return_if_no_shell (shell, data);

  if (active == gimp_display_shell_get_snap_to_canvas (shell))
    return;

  gimp_display_shell_set_snap_to_canvas (shell, active);
}

void
view_snap_to_vectors_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpDisplayShell *shell;
  gboolean          active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
  return_if_no_shell (shell, data);

  if (active == gimp_display_shell_get_snap_to_vectors (shell))
    return;

  gimp_display_shell_set_snap_to_vectors (shell, active);
}

void
view_padding_color_cmd_callback (GtkAction *action,
                                 gint       value,
                                 gpointer   data)
{
  GimpDisplay        *display;
  GimpImageWindow    *window;
  GimpDisplayShell   *shell;
  GimpDisplayOptions *options;
  gboolean            fullscreen;
  return_if_no_display (display, data);

  shell  = gimp_display_get_shell (display);
  window = gimp_display_shell_get_window (shell);

  if (window)
    fullscreen = gimp_image_window_get_fullscreen (window);
  else
    fullscreen = FALSE;

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
            GimpImage        *image = gimp_display_get_image (display);
            GimpDisplayShell *shell = gimp_display_get_shell (display);

            color_dialog =
              gimp_color_dialog_new (GIMP_VIEWABLE (image),
                                     action_data_get_context (data),
                                     _("Set Canvas Padding Color"),
                                     "gtk-select-color",
                                     _("Set Custom Canvas Padding Color"),
                                     GTK_WIDGET (shell),
                                     NULL, NULL,
                                     &options->padding_color,
                                     FALSE, FALSE);

            g_signal_connect (color_dialog, "update",
                              G_CALLBACK (view_padding_color_dialog_update),
                              shell);

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
        GimpDisplayOptions *default_options;

        options->padding_mode_set = FALSE;

        if (fullscreen)
          default_options = display->config->default_fullscreen_view;
        else
          default_options = display->config->default_view;

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
  GimpDisplayShell *shell;
  return_if_no_shell (shell, data);

  gimp_display_shell_scale_shrink_wrap (shell, FALSE);
}

void
view_fullscreen_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  GimpImageWindow  *window;
  return_if_no_display (display, data);

  shell  = gimp_display_get_shell (display);
  window = gimp_display_shell_get_window (shell);

  if (window)
    {
      gboolean active;

      active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

      gimp_image_window_set_fullscreen (window, active);
    }
}


/*  private functions  */

static void
view_padding_color_dialog_update (GimpColorDialog      *dialog,
                                  const GimpRGB        *color,
                                  GimpColorDialogState  state,
                                  GimpDisplayShell     *shell)
{
  GimpImageWindow    *window;
  GimpDisplayOptions *options;
  gboolean            fullscreen;

  window = gimp_display_shell_get_window (shell);

  if (window)
    fullscreen = gimp_image_window_get_fullscreen (window);
  else
    fullscreen = FALSE;

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
