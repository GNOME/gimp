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
#include "core/gimpgrouplayer.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpcolordialog.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimptoggleaction.h"
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

#include "dialogs/color-profile-dialog.h"
#include "dialogs/dialogs.h"

#include "menus/menus.h"

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

static void   view_padding_color_dialog_update (GimpColorDialog          *dialog,
                                                GeglColor                *color,
                                                GimpColorDialogState      state,
                                                GimpDisplayShell         *shell);


/*  public functions  */

void
view_new_cmd_callback (GimpAction *action,
                       GVariant   *value,
                       gpointer    data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  return_if_no_display (display, data);

  shell = gimp_display_get_shell (display);

  gimp_create_display (display->gimp,
                       gimp_display_get_image (display),
                       shell->unit, gimp_zoom_model_get_factor (shell->zoom),
                       G_OBJECT (gimp_widget_get_monitor (GTK_WIDGET (shell))));
}

void
view_close_cmd_callback (GimpAction *action,
                         GVariant   *value,
                         gpointer    data)
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
view_scroll_center_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpDisplay *display;
  return_if_no_display (display, data);

  gimp_display_shell_scroll_center_image (gimp_display_get_shell (display),
                                          TRUE, TRUE);
}

void
view_zoom_fit_in_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpDisplay *display;
  return_if_no_display (display, data);

  gimp_display_shell_scale_fit_in (gimp_display_get_shell (display));
}

void
view_zoom_fill_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpDisplay *display;
  return_if_no_display (display, data);

  gimp_display_shell_scale_fill (gimp_display_get_shell (display));
}

void
view_zoom_selection_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
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
view_zoom_revert_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpDisplay *display;
  return_if_no_display (display, data);

  gimp_display_shell_scale_revert (gimp_display_get_shell (display));
}

void
view_zoom_cmd_callback (GimpAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  GimpDisplayShell     *shell;
  GimpActionSelectType  select_type;
  return_if_no_shell (shell, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  switch (select_type)
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

        scale = action_select_value (select_type,
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
view_zoom_explicit_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpDisplayShell *shell;
  gint              factor;
  return_if_no_shell (shell, data);

  factor = g_variant_get_int32 (value);

  if (factor != 0 /* not Other... */)
    {
      if (fabs (factor - gimp_zoom_model_get_factor (shell->zoom)) > 0.0001)
        gimp_display_shell_scale (shell,
                                  GIMP_ZOOM_TO,
                                  (gdouble) factor / 10000,
                                  GIMP_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS);
    }
  else
    {
      gimp_display_shell_scale_dialog (shell);
    }
}

void
view_show_all_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = gimp_display_get_shell (display);

  active = g_variant_get_boolean (value);

  if (active != shell->show_all)
    {
      GimpImageWindow *window = gimp_display_shell_get_window (shell);

      gimp_display_shell_set_show_all (shell, active);

      if (window)
        SET_ACTIVE (menus_get_image_manager_singleton (display->gimp),
                    "view-show-all", shell->show_all);

      if (IS_ACTIVE_DISPLAY (display))
        SET_ACTIVE (shell->popup_manager, "view-show-all",
                    shell->show_all);
    }
}

void
view_dot_for_dot_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = gimp_display_get_shell (display);

  active = g_variant_get_boolean (value);

  if (active != shell->dot_for_dot)
    {
      GimpImageWindow *window = gimp_display_shell_get_window (shell);

      gimp_display_shell_scale_set_dot_for_dot (shell, active);

      if (window)
        SET_ACTIVE (menus_get_image_manager_singleton (display->gimp),
                    "view-dot-for-dot", shell->dot_for_dot);

      if (IS_ACTIVE_DISPLAY (display))
        SET_ACTIVE (shell->popup_manager, "view-dot-for-dot",
                    shell->dot_for_dot);
    }
}

void
view_flip_horizontally_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = gimp_display_get_shell (display);

  active = g_variant_get_boolean (value);

  if (active != shell->flip_horizontally)
    {
      gimp_display_shell_flip (shell, active, shell->flip_vertically);
    }
}

void
view_flip_vertically_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = gimp_display_get_shell (display);

  active = g_variant_get_boolean (value);

  if (active != shell->flip_vertically)
    {
      gimp_display_shell_flip (shell, shell->flip_horizontally, active);
    }
}

void
view_flip_reset_cmd_callback (GimpAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;

  return_if_no_display (display, data);

  shell = gimp_display_get_shell (display);
  gimp_display_shell_flip (shell, FALSE, FALSE);
}

void
view_rotate_absolute_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpDisplay          *display;
  GimpDisplayShell     *shell;
  GimpActionSelectType  select_type;
  gdouble               angle = 0.0;
  return_if_no_display (display, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  shell = gimp_display_get_shell (display);

  angle = action_select_value (select_type,
                               0.0,
                               -180.0, 180.0, 0.0,
                               1.0, 15.0, 90.0, 0.0,
                               TRUE);

  gimp_display_shell_rotate_to (shell, angle);
}

void
view_rotate_relative_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpDisplay          *display;
  GimpDisplayShell     *shell;
  GimpActionSelectType  select_type;
  gdouble               delta = 0.0;
  return_if_no_display (display, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  shell = gimp_display_get_shell (display);

  delta = action_select_value (select_type,
                               0.0,
                               -180.0, 180.0, 0.0,
                               1.0, 15.0, 90.0, 0.0,
                               TRUE);

  gimp_display_shell_rotate (shell, delta);
}

void
view_rotate_other_cmd_callback (GimpAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  return_if_no_display (display, data);

  shell = gimp_display_get_shell (display);

  gimp_display_shell_rotate_dialog (shell);
}

void
view_reset_cmd_callback (GimpAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;

  return_if_no_display (display, data);

  shell = gimp_display_get_shell (display);
  gimp_display_shell_rotate_to (shell, 0.0);
  gimp_display_shell_flip (shell, FALSE, FALSE);
}

void
view_scroll_horizontal_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpDisplayShell     *shell;
  GtkAdjustment        *adj;
  GimpActionSelectType  select_type;
  gdouble               offset;
  return_if_no_shell (shell, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  adj = shell->hsbdata;

  offset = action_select_value (select_type,
                                gtk_adjustment_get_value (adj),
                                gtk_adjustment_get_lower (adj),
                                gtk_adjustment_get_upper (adj) -
                                gtk_adjustment_get_page_size (adj),
                                gtk_adjustment_get_lower (adj),
                                1,
                                gtk_adjustment_get_step_increment (adj),
                                gtk_adjustment_get_page_increment (adj),
                                0,
                                FALSE);

  gtk_adjustment_set_value (shell->hsbdata, offset);
}

void
view_scroll_vertical_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpDisplayShell     *shell;
  GtkAdjustment        *adj;
  GimpActionSelectType  select_type;
  gdouble               offset;
  return_if_no_shell (shell, data);

  select_type = (GimpActionSelectType) g_variant_get_int32 (value);

  adj = shell->vsbdata;

  offset = action_select_value (select_type,
                                gtk_adjustment_get_value (adj),
                                gtk_adjustment_get_lower (adj),
                                gtk_adjustment_get_upper (adj) -
                                gtk_adjustment_get_page_size (adj),
                                gtk_adjustment_get_lower (adj),
                                1,
                                gtk_adjustment_get_step_increment (adj),
                                gtk_adjustment_get_page_increment (adj),
                                0,
                                FALSE);

  gtk_adjustment_set_value (shell->vsbdata, offset);
}

void
view_navigation_window_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  Gimp             *gimp;
  GimpDisplayShell *shell;
  return_if_no_gimp (gimp, data);
  return_if_no_shell (shell, data);

  gimp_window_strategy_show_dockable_dialog (GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (gimp)),
                                             gimp,
                                             gimp_dialog_factory_get_singleton (),
                                             gimp_widget_get_monitor (GTK_WIDGET (shell)),
                                             "gimp-navigation-view");
}

void
view_display_filters_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpDisplayShell *shell;
  GtkWidget        *dialog;
  return_if_no_shell (shell, data);

#define FILTERS_DIALOG_KEY "gimp-display-filters-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (shell), FILTERS_DIALOG_KEY);

  if (! dialog)
    {
      dialog = gimp_display_shell_filter_dialog_new (shell);

      dialogs_attach_dialog (G_OBJECT (shell), FILTERS_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
view_color_management_reset_cmd_callback (GimpAction *action,
                                          GVariant   *value,
                                          gpointer    data)
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
view_color_management_enable_cmd_callback (GimpAction *action,
                                           GVariant   *value,
                                           gpointer    data)
{
  GimpDisplayShell        *shell;
  GimpColorConfig         *color_config;
  GimpColorManagementMode  mode;
  gboolean                 active;
  return_if_no_shell (shell, data);

  color_config = gimp_display_shell_get_color_config (shell);

  active = g_variant_get_boolean (value);

  mode = gimp_color_config_get_mode (color_config);

  if (active)
    {
      if (mode != GIMP_COLOR_MANAGEMENT_SOFTPROOF)
        mode = GIMP_COLOR_MANAGEMENT_DISPLAY;
    }
  else
    {
      mode = GIMP_COLOR_MANAGEMENT_OFF;
    }

  if (mode != gimp_color_config_get_mode (color_config))
    {
      g_object_set (color_config,
                    "mode", mode,
                    NULL);
      shell->color_config_set = TRUE;
    }
}

void
view_color_management_softproof_cmd_callback (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data)
{
  GimpDisplayShell        *shell;
  GimpColorConfig         *color_config;
  GimpColorManagementMode  mode;
  gboolean                 active;
  return_if_no_shell (shell, data);

  color_config = gimp_display_shell_get_color_config (shell);

  active = g_variant_get_boolean (value);

  mode = gimp_color_config_get_mode (color_config);

  if (active)
    {
      mode = GIMP_COLOR_MANAGEMENT_SOFTPROOF;
    }
  else
    {
      if (mode != GIMP_COLOR_MANAGEMENT_OFF)
        mode = GIMP_COLOR_MANAGEMENT_DISPLAY;
    }

  if (mode != gimp_color_config_get_mode (color_config))
    {
      g_object_set (color_config,
                    "mode", mode,
                    NULL);
      shell->color_config_set = TRUE;
    }
}

void
view_display_intent_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpDisplayShell          *shell;
  GimpColorConfig           *color_config;
  GimpColorRenderingIntent   intent;
  return_if_no_shell (shell, data);

  intent = (GimpColorRenderingIntent) g_variant_get_int32 (value);

  color_config = gimp_display_shell_get_color_config (shell);

  if (intent != gimp_color_config_get_display_intent (color_config))
    {
      g_object_set (color_config,
                    "display-rendering-intent", intent,
                    NULL);
      shell->color_config_set = TRUE;
    }
}

void
view_display_bpc_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpDisplayShell *shell;
  GimpColorConfig  *color_config;
  gboolean          active;
  return_if_no_shell (shell, data);

  color_config = gimp_display_shell_get_color_config (shell);

  active = g_variant_get_boolean (value);

  if (active != gimp_color_config_get_display_bpc (color_config))
    {
      g_object_set (color_config,
                    "display-use-black-point-compensation", active,
                    NULL);
      shell->color_config_set = TRUE;
    }
}

void
view_softproof_gamut_check_cmd_callback (GimpAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  GimpDisplayShell *shell;
  GimpColorConfig  *color_config;
  gboolean          active;
  return_if_no_shell (shell, data);

  color_config = gimp_display_shell_get_color_config (shell);

  active = g_variant_get_boolean (value);

  if (active != gimp_color_config_get_simulation_gamut_check (color_config))
    {
      g_object_set (color_config,
                    "simulation-gamut-check", active,
                    NULL);
      shell->color_config_set = TRUE;
    }
}

void
view_toggle_selection_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != gimp_display_shell_get_show_selection (shell))
    {
      gimp_display_shell_set_show_selection (shell, active);
    }
}

void
view_toggle_layer_boundary_cmd_callback (GimpAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != gimp_display_shell_get_show_layer (shell))
    {
      gimp_display_shell_set_show_layer (shell, active);
    }
}

void
view_toggle_canvas_boundary_cmd_callback (GimpAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != gimp_display_shell_get_show_canvas (shell))
    {
      gimp_display_shell_set_show_canvas (shell, active);
    }
}

void
view_toggle_menubar_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != gimp_display_shell_get_show_menubar (shell))
    {
      gimp_display_shell_set_show_menubar (shell, active);
    }
}

void
view_toggle_rulers_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != gimp_display_shell_get_show_rulers (shell))
    {
      gimp_display_shell_set_show_rulers (shell, active);
    }
}

void
view_toggle_scrollbars_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != gimp_display_shell_get_show_scrollbars (shell))
    {
      gimp_display_shell_set_show_scrollbars (shell, active);
    }
}

void
view_toggle_statusbar_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != gimp_display_shell_get_show_statusbar (shell))
    {
      gimp_display_shell_set_show_statusbar (shell, active);
    }
}

void
view_toggle_guides_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != gimp_display_shell_get_show_guides (shell))
    {
      gimp_display_shell_set_show_guides (shell, active);
    }
}

void
view_toggle_grid_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != gimp_display_shell_get_show_grid (shell))
    {
      gimp_display_shell_set_show_grid (shell, active);
    }
}

void
view_toggle_sample_points_cmd_callback (GimpAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != gimp_display_shell_get_show_sample_points (shell))
    {
      gimp_display_shell_set_show_sample_points (shell, active);
    }
}

void
view_snap_to_guides_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != gimp_display_shell_get_snap_to_guides (shell))
    {
      gimp_display_shell_set_snap_to_guides (shell, active);
    }
}

void
view_snap_to_grid_cmd_callback (GimpAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != gimp_display_shell_get_snap_to_grid (shell))
    {
      gimp_display_shell_set_snap_to_grid (shell, active);
    }
}

void
view_snap_to_canvas_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != gimp_display_shell_get_snap_to_canvas (shell))
    {
      gimp_display_shell_set_snap_to_canvas (shell, active);
    }
}

void
view_snap_to_path_cmd_callback (GimpAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != gimp_display_shell_get_snap_to_path (shell))
    {
      gimp_display_shell_set_snap_to_path (shell, active);
    }
}

void
view_snap_to_bbox_cmd_callback (GimpAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != gimp_display_shell_get_snap_to_bbox (shell))
    {
      gimp_display_shell_set_snap_to_bbox (shell, active);
    }
}

void
view_snap_to_equidistance_cmd_callback (GimpAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != gimp_display_shell_get_snap_to_equidistance (shell))
    gimp_display_shell_set_snap_to_equidistance (shell, active);
}

void
view_padding_color_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpDisplay           *display;
  GimpImageWindow       *window;
  GimpDisplayShell      *shell;
  GimpDisplayOptions    *options;
  GimpCanvasPaddingMode  padding_mode;
  gboolean               fullscreen;
  return_if_no_display (display, data);

  padding_mode = (GimpCanvasPaddingMode) g_variant_get_int32 (value);

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

#define PADDING_COLOR_DIALOG_KEY "gimp-padding-color-dialog"

  switch (padding_mode)
    {
    case GIMP_CANVAS_PADDING_MODE_DEFAULT:
    case GIMP_CANVAS_PADDING_MODE_LIGHT_CHECK:
    case GIMP_CANVAS_PADDING_MODE_DARK_CHECK:
      dialogs_destroy_dialog (G_OBJECT (shell), PADDING_COLOR_DIALOG_KEY);

      options->padding_mode_set = TRUE;

      gimp_display_shell_set_padding (shell, padding_mode,
                                      options->padding_color);
      break;

    case GIMP_CANVAS_PADDING_MODE_CUSTOM:
      {
        GtkWidget             *dialog;
        GeglColor             *old_color;
        GimpCanvasPaddingMode  old_padding_mode;

        dialog = dialogs_get_dialog (G_OBJECT (shell), PADDING_COLOR_DIALOG_KEY);

        if (! dialog)
          {
            GimpImage        *image = gimp_display_get_image (display);
            GimpDisplayShell *shell = gimp_display_get_shell (display);

            dialog =
              gimp_color_dialog_new (GIMP_VIEWABLE (image),
                                     action_data_get_context (data),
                                     FALSE,
                                     _("Set Canvas Padding Color"),
                                     GIMP_ICON_FONT,
                                     _("Set Custom Canvas Padding Color"),
                                     GTK_WIDGET (shell),
                                     NULL, NULL,
                                     options->padding_color,
                                     TRUE, FALSE);

            g_signal_connect (dialog, "update",
                              G_CALLBACK (view_padding_color_dialog_update),
                              shell);

            dialogs_attach_dialog (G_OBJECT (shell),
                                   PADDING_COLOR_DIALOG_KEY, dialog);
          }
        old_color        = gegl_color_duplicate (options->padding_color);
        old_padding_mode = options->padding_mode;
        g_object_set_data_full (G_OBJECT (dialog), "old-color",
                                old_color, g_object_unref);
        g_object_set_data (G_OBJECT (dialog), "old-padding-mode",
                           GINT_TO_POINTER (old_padding_mode));

        gtk_window_present (GTK_WINDOW (dialog));
      }
      break;

    case GIMP_CANVAS_PADDING_MODE_RESET:
      dialogs_destroy_dialog (G_OBJECT (shell), PADDING_COLOR_DIALOG_KEY);

      {
        GimpDisplayOptions *default_options;

        options->padding_mode_set = FALSE;

        if (fullscreen)
          default_options = display->config->default_fullscreen_view;
        else
          default_options = display->config->default_view;

        gimp_display_shell_set_padding (shell,
                                        default_options->padding_mode,
                                        default_options->padding_color);
        gimp_display_shell_set_padding_in_show_all (shell,
                                                    default_options->padding_in_show_all);
      }
      break;
    }
}

void
view_padding_color_in_show_all_cmd_callback (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data)
{
  GimpDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != gimp_display_shell_get_padding_in_show_all (shell))
    {
      gimp_display_shell_set_padding_in_show_all (shell, active);
    }
}

void
view_shrink_wrap_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpDisplayShell *shell;
  return_if_no_shell (shell, data);

  gimp_display_shell_scale_shrink_wrap (shell, FALSE);
}

void
view_fullscreen_cmd_callback (GimpAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  GimpDisplay      *display;
  GimpDisplayShell *shell;
  GimpImageWindow  *window;
  return_if_no_display (display, data);

  shell  = gimp_display_get_shell (display);
  window = gimp_display_shell_get_window (shell);

  if (window)
    {
      gboolean active = g_variant_get_boolean (value);

      gimp_image_window_set_fullscreen (window, active);
    }
}


/*  private functions  */

static void
view_padding_color_dialog_update (GimpColorDialog      *dialog,
                                  GeglColor            *color,
                                  GimpColorDialogState  state,
                                  GimpDisplayShell     *shell)
{
  GimpImageWindow       *window;
  GimpDisplayOptions    *options;
  GeglColor             *old_color;
  GimpCanvasPaddingMode  old_padding_mode;
  gboolean               fullscreen;

  window           = gimp_display_shell_get_window (shell);
  old_color        = g_object_get_data (G_OBJECT (dialog), "old-color");
  old_padding_mode = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dialog), "old-padding-mode"));

  g_return_if_fail (old_color);

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
      gtk_widget_destroy (GTK_WIDGET (dialog));
      break;

    case GIMP_COLOR_DIALOG_CANCEL:
      gimp_display_shell_set_padding (shell,
                                      old_padding_mode,
                                      old_color);
      gtk_widget_destroy (GTK_WIDGET (dialog));
      break;

    case GIMP_COLOR_DIALOG_UPDATE:
      gimp_display_shell_set_padding (shell, GIMP_CANVAS_PADDING_MODE_CUSTOM,
                                      color);
      break;

    default:
      break;
    }
}
