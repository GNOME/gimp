/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmamath/ligmamath.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "config/ligmadisplayoptions.h"
#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmagrouplayer.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmacolordialog.h"
#include "widgets/ligmadock.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmauimanager.h"
#include "widgets/ligmawidgets-utils.h"
#include "widgets/ligmawindowstrategy.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplay-foreach.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-appearance.h"
#include "display/ligmadisplayshell-filter-dialog.h"
#include "display/ligmadisplayshell-rotate.h"
#include "display/ligmadisplayshell-rotate-dialog.h"
#include "display/ligmadisplayshell-scale.h"
#include "display/ligmadisplayshell-scale-dialog.h"
#include "display/ligmadisplayshell-scroll.h"
#include "display/ligmadisplayshell-close.h"
#include "display/ligmaimagewindow.h"

#include "dialogs/color-profile-dialog.h"
#include "dialogs/dialogs.h"

#include "actions.h"
#include "view-commands.h"

#include "ligma-intl.h"


#define SET_ACTIVE(manager,action_name,active) \
  { LigmaActionGroup *group = \
      ligma_ui_manager_get_action_group (manager, "view"); \
    ligma_action_group_set_action_active (group, action_name, active); }

#define IS_ACTIVE_DISPLAY(display) \
  ((display) == \
   ligma_context_get_display (ligma_get_user_context ((display)->ligma)))


/*  local function prototypes  */

static void   view_padding_color_dialog_update (LigmaColorDialog          *dialog,
                                                const LigmaRGB            *color,
                                                LigmaColorDialogState      state,
                                                LigmaDisplayShell         *shell);


/*  public functions  */

void
view_new_cmd_callback (LigmaAction *action,
                       GVariant   *value,
                       gpointer    data)
{
  LigmaDisplay      *display;
  LigmaDisplayShell *shell;
  return_if_no_display (display, data);

  shell = ligma_display_get_shell (display);

  ligma_create_display (display->ligma,
                       ligma_display_get_image (display),
                       shell->unit, ligma_zoom_model_get_factor (shell->zoom),
                       G_OBJECT (ligma_widget_get_monitor (GTK_WIDGET (shell))));
}

void
view_close_cmd_callback (LigmaAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  LigmaDisplay      *display;
  LigmaDisplayShell *shell;
  LigmaImage        *image;
  return_if_no_display (display, data);

  shell = ligma_display_get_shell (display);
  image = ligma_display_get_image (display);

  /* Check for the image so we don't close the last display. */
  if (image)
    ligma_display_shell_close (shell, FALSE);
}

void
view_scroll_center_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  LigmaDisplay *display;
  return_if_no_display (display, data);

  ligma_display_shell_scroll_center_image (ligma_display_get_shell (display),
                                          TRUE, TRUE);
}

void
view_zoom_fit_in_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaDisplay *display;
  return_if_no_display (display, data);

  ligma_display_shell_scale_fit_in (ligma_display_get_shell (display));
}

void
view_zoom_fill_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  LigmaDisplay *display;
  return_if_no_display (display, data);

  ligma_display_shell_scale_fill (ligma_display_get_shell (display));
}

void
view_zoom_selection_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaDisplay *display;
  LigmaImage   *image;
  gint         x, y, width, height;
  return_if_no_display (display, data);
  return_if_no_image (image, data);

  ligma_item_bounds (LIGMA_ITEM (ligma_image_get_mask (image)),
                    &x, &y, &width, &height);

  ligma_display_shell_scale_to_rectangle (ligma_display_get_shell (display),
                                         LIGMA_ZOOM_IN,
                                         x, y, width, height,
                                         FALSE);
}

void
view_zoom_revert_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaDisplay *display;
  return_if_no_display (display, data);

  ligma_display_shell_scale_revert (ligma_display_get_shell (display));
}

void
view_zoom_cmd_callback (LigmaAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  LigmaDisplayShell     *shell;
  LigmaActionSelectType  select_type;
  return_if_no_shell (shell, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  switch (select_type)
    {
    case LIGMA_ACTION_SELECT_FIRST:
      ligma_display_shell_scale (shell,
                                LIGMA_ZOOM_OUT_MAX,
                                0.0,
                                LIGMA_ZOOM_FOCUS_BEST_GUESS);
      break;

    case LIGMA_ACTION_SELECT_LAST:
      ligma_display_shell_scale (shell,
                                LIGMA_ZOOM_IN_MAX,
                                0.0,
                                LIGMA_ZOOM_FOCUS_BEST_GUESS);
      break;

    case LIGMA_ACTION_SELECT_PREVIOUS:
      ligma_display_shell_scale (shell,
                                LIGMA_ZOOM_OUT,
                                0.0,
                                LIGMA_ZOOM_FOCUS_BEST_GUESS);
      break;

    case LIGMA_ACTION_SELECT_NEXT:
      ligma_display_shell_scale (shell,
                                LIGMA_ZOOM_IN,
                                0.0,
                                LIGMA_ZOOM_FOCUS_BEST_GUESS);
      break;

    case LIGMA_ACTION_SELECT_SKIP_PREVIOUS:
      ligma_display_shell_scale (shell,
                                LIGMA_ZOOM_OUT_MORE,
                                0.0,
                                LIGMA_ZOOM_FOCUS_BEST_GUESS);
      break;

    case LIGMA_ACTION_SELECT_SKIP_NEXT:
      ligma_display_shell_scale (shell,
                                LIGMA_ZOOM_IN_MORE,
                                0.0,
                                LIGMA_ZOOM_FOCUS_BEST_GUESS);
      break;

    default:
      {
        gdouble scale = ligma_zoom_model_get_factor (shell->zoom);

        scale = action_select_value (select_type,
                                     scale,
                                     0.0, 512.0, 1.0,
                                     1.0 / 8.0, 1.0, 16.0, 0.0,
                                     FALSE);

        /* min = 1.0 / 256,  max = 256.0                */
        /* scale = min *  (max / min)**(i/n), i = 0..n  */
        scale = pow (65536.0, scale / 512.0) / 256.0;

        ligma_display_shell_scale (shell,
                                  LIGMA_ZOOM_TO,
                                  scale,
                                  LIGMA_ZOOM_FOCUS_BEST_GUESS);
        break;
      }
    }
}

void
view_zoom_explicit_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  LigmaDisplayShell *shell;
  gint              factor;
  return_if_no_shell (shell, data);

  factor = g_variant_get_int32 (value);

  if (factor != 0 /* not Other... */)
    {
      if (fabs (factor - ligma_zoom_model_get_factor (shell->zoom)) > 0.0001)
        ligma_display_shell_scale (shell,
                                  LIGMA_ZOOM_TO,
                                  (gdouble) factor / 10000,
                                  LIGMA_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS);
    }
}

/* not a LigmaActionCallback */
void
view_zoom_other_cmd_callback (LigmaAction *action,
                              gpointer    data)
{
  LigmaDisplayShell *shell;
  return_if_no_shell (shell, data);

  /* check if we are activated by the user or from
   * view_actions_set_zoom(), also this is really a GtkToggleAction
   * NOT a LigmaToggleAction
   */
  if (gtk_toggle_action_get_active ((GtkToggleAction *) action) &&
      shell->other_scale != ligma_zoom_model_get_factor (shell->zoom))
    {
      ligma_display_shell_scale_dialog (shell);
    }
}

void
view_show_all_cmd_callback (LigmaAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  LigmaDisplay      *display;
  LigmaDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = ligma_display_get_shell (display);

  active = g_variant_get_boolean (value);

  if (active != shell->show_all)
    {
      LigmaImageWindow *window = ligma_display_shell_get_window (shell);

      ligma_display_shell_set_show_all (shell, active);

      if (window)
        SET_ACTIVE (ligma_image_window_get_ui_manager (window),
                    "view-show-all", shell->show_all);

      if (IS_ACTIVE_DISPLAY (display))
        SET_ACTIVE (shell->popup_manager, "view-show-all",
                    shell->show_all);
    }
}

void
view_dot_for_dot_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaDisplay      *display;
  LigmaDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = ligma_display_get_shell (display);

  active = g_variant_get_boolean (value);

  if (active != shell->dot_for_dot)
    {
      LigmaImageWindow *window = ligma_display_shell_get_window (shell);

      ligma_display_shell_scale_set_dot_for_dot (shell, active);

      if (window)
        SET_ACTIVE (ligma_image_window_get_ui_manager (window),
                    "view-dot-for-dot", shell->dot_for_dot);

      if (IS_ACTIVE_DISPLAY (display))
        SET_ACTIVE (shell->popup_manager, "view-dot-for-dot",
                    shell->dot_for_dot);
    }
}

void
view_flip_horizontally_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaDisplay      *display;
  LigmaDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = ligma_display_get_shell (display);

  active = g_variant_get_boolean (value);

  if (active != shell->flip_horizontally)
    {
      ligma_display_shell_flip (shell, active, shell->flip_vertically);
    }
}

void
view_flip_vertically_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaDisplay      *display;
  LigmaDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = ligma_display_get_shell (display);

  active = g_variant_get_boolean (value);

  if (active != shell->flip_vertically)
    {
      ligma_display_shell_flip (shell, shell->flip_horizontally, active);
    }
}

void
view_flip_reset_cmd_callback (LigmaAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  LigmaDisplay      *display;
  LigmaDisplayShell *shell;

  return_if_no_display (display, data);

  shell = ligma_display_get_shell (display);
  ligma_display_shell_flip (shell, FALSE, FALSE);
}

void
view_rotate_absolute_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaDisplay          *display;
  LigmaDisplayShell     *shell;
  LigmaActionSelectType  select_type;
  gdouble               angle = 0.0;
  return_if_no_display (display, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  shell = ligma_display_get_shell (display);

  angle = action_select_value (select_type,
                               0.0,
                               -180.0, 180.0, 0.0,
                               1.0, 15.0, 90.0, 0.0,
                               TRUE);

  ligma_display_shell_rotate_to (shell, angle);
}

void
view_rotate_relative_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaDisplay          *display;
  LigmaDisplayShell     *shell;
  LigmaActionSelectType  select_type;
  gdouble               delta = 0.0;
  return_if_no_display (display, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  shell = ligma_display_get_shell (display);

  delta = action_select_value (select_type,
                               0.0,
                               -180.0, 180.0, 0.0,
                               1.0, 15.0, 90.0, 0.0,
                               TRUE);

  ligma_display_shell_rotate (shell, delta);
}

void
view_rotate_other_cmd_callback (LigmaAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  LigmaDisplay      *display;
  LigmaDisplayShell *shell;
  return_if_no_display (display, data);

  shell = ligma_display_get_shell (display);

  ligma_display_shell_rotate_dialog (shell);
}

void
view_reset_cmd_callback (LigmaAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  LigmaDisplay      *display;
  LigmaDisplayShell *shell;

  return_if_no_display (display, data);

  shell = ligma_display_get_shell (display);
  ligma_display_shell_rotate_to (shell, 0.0);
  ligma_display_shell_flip (shell, FALSE, FALSE);
}

void
view_scroll_horizontal_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaDisplayShell     *shell;
  GtkAdjustment        *adj;
  LigmaActionSelectType  select_type;
  gdouble               offset;
  return_if_no_shell (shell, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

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
view_scroll_vertical_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaDisplayShell     *shell;
  GtkAdjustment        *adj;
  LigmaActionSelectType  select_type;
  gdouble               offset;
  return_if_no_shell (shell, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

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
view_navigation_window_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  Ligma             *ligma;
  LigmaDisplayShell *shell;
  return_if_no_ligma (ligma, data);
  return_if_no_shell (shell, data);

  ligma_window_strategy_show_dockable_dialog (LIGMA_WINDOW_STRATEGY (ligma_get_window_strategy (ligma)),
                                             ligma,
                                             ligma_dialog_factory_get_singleton (),
                                             ligma_widget_get_monitor (GTK_WIDGET (shell)),
                                             "ligma-navigation-view");
}

void
view_display_filters_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaDisplayShell *shell;
  GtkWidget        *dialog;
  return_if_no_shell (shell, data);

#define FILTERS_DIALOG_KEY "ligma-display-filters-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (shell), FILTERS_DIALOG_KEY);

  if (! dialog)
    {
      dialog = ligma_display_shell_filter_dialog_new (shell);

      dialogs_attach_dialog (G_OBJECT (shell), FILTERS_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
view_color_management_reset_cmd_callback (LigmaAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  LigmaDisplayShell *shell;
  LigmaColorConfig  *global_config;
  LigmaColorConfig  *shell_config;
  return_if_no_shell (shell, data);

  global_config = LIGMA_CORE_CONFIG (shell->display->config)->color_management;
  shell_config  = ligma_display_shell_get_color_config (shell);

  ligma_config_copy (LIGMA_CONFIG (global_config),
                    LIGMA_CONFIG (shell_config),
                    0);
  shell->color_config_set = FALSE;
}

void
view_color_management_enable_cmd_callback (LigmaAction *action,
                                           GVariant   *value,
                                           gpointer    data)
{
  LigmaDisplayShell        *shell;
  LigmaColorConfig         *color_config;
  LigmaColorManagementMode  mode;
  gboolean                 active;
  return_if_no_shell (shell, data);

  color_config = ligma_display_shell_get_color_config (shell);

  active = g_variant_get_boolean (value);

  mode = ligma_color_config_get_mode (color_config);

  if (active)
    {
      if (mode != LIGMA_COLOR_MANAGEMENT_SOFTPROOF)
        mode = LIGMA_COLOR_MANAGEMENT_DISPLAY;
    }
  else
    {
      mode = LIGMA_COLOR_MANAGEMENT_OFF;
    }

  if (mode != ligma_color_config_get_mode (color_config))
    {
      g_object_set (color_config,
                    "mode", mode,
                    NULL);
      shell->color_config_set = TRUE;
    }
}

void
view_color_management_softproof_cmd_callback (LigmaAction *action,
                                              GVariant   *value,
                                              gpointer    data)
{
  LigmaDisplayShell        *shell;
  LigmaColorConfig         *color_config;
  LigmaColorManagementMode  mode;
  gboolean                 active;
  return_if_no_shell (shell, data);

  color_config = ligma_display_shell_get_color_config (shell);

  active = g_variant_get_boolean (value);

  mode = ligma_color_config_get_mode (color_config);

  if (active)
    {
      mode = LIGMA_COLOR_MANAGEMENT_SOFTPROOF;
    }
  else
    {
      if (mode != LIGMA_COLOR_MANAGEMENT_OFF)
        mode = LIGMA_COLOR_MANAGEMENT_DISPLAY;
    }

  if (mode != ligma_color_config_get_mode (color_config))
    {
      g_object_set (color_config,
                    "mode", mode,
                    NULL);
      shell->color_config_set = TRUE;
    }
}

void
view_display_intent_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaDisplayShell          *shell;
  LigmaColorConfig           *color_config;
  LigmaColorRenderingIntent   intent;
  return_if_no_shell (shell, data);

  intent = (LigmaColorRenderingIntent) g_variant_get_int32 (value);

  color_config = ligma_display_shell_get_color_config (shell);

  if (intent != ligma_color_config_get_display_intent (color_config))
    {
      g_object_set (color_config,
                    "display-rendering-intent", intent,
                    NULL);
      shell->color_config_set = TRUE;
    }
}

void
view_display_bpc_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaDisplayShell *shell;
  LigmaColorConfig  *color_config;
  gboolean          active;
  return_if_no_shell (shell, data);

  color_config = ligma_display_shell_get_color_config (shell);

  active = g_variant_get_boolean (value);

  if (active != ligma_color_config_get_display_bpc (color_config))
    {
      g_object_set (color_config,
                    "display-use-black-point-compensation", active,
                    NULL);
      shell->color_config_set = TRUE;
    }
}

void
view_softproof_gamut_check_cmd_callback (LigmaAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  LigmaDisplayShell *shell;
  LigmaColorConfig  *color_config;
  gboolean          active;
  return_if_no_shell (shell, data);

  color_config = ligma_display_shell_get_color_config (shell);

  active = g_variant_get_boolean (value);

  if (active != ligma_color_config_get_simulation_gamut_check (color_config))
    {
      g_object_set (color_config,
                    "simulation-gamut-check", active,
                    NULL);
      shell->color_config_set = TRUE;
    }
}

void
view_toggle_selection_cmd_callback (LigmaAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  LigmaDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != ligma_display_shell_get_show_selection (shell))
    {
      ligma_display_shell_set_show_selection (shell, active);
    }
}

void
view_toggle_layer_boundary_cmd_callback (LigmaAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  LigmaDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != ligma_display_shell_get_show_layer (shell))
    {
      ligma_display_shell_set_show_layer (shell, active);
    }
}

void
view_toggle_canvas_boundary_cmd_callback (LigmaAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  LigmaDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != ligma_display_shell_get_show_canvas (shell))
    {
      ligma_display_shell_set_show_canvas (shell, active);
    }
}

void
view_toggle_menubar_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != ligma_display_shell_get_show_menubar (shell))
    {
      ligma_display_shell_set_show_menubar (shell, active);
    }
}

void
view_toggle_rulers_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  LigmaDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != ligma_display_shell_get_show_rulers (shell))
    {
      ligma_display_shell_set_show_rulers (shell, active);
    }
}

void
view_toggle_scrollbars_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != ligma_display_shell_get_show_scrollbars (shell))
    {
      ligma_display_shell_set_show_scrollbars (shell, active);
    }
}

void
view_toggle_statusbar_cmd_callback (LigmaAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  LigmaDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != ligma_display_shell_get_show_statusbar (shell))
    {
      ligma_display_shell_set_show_statusbar (shell, active);
    }
}

void
view_toggle_guides_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  LigmaDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != ligma_display_shell_get_show_guides (shell))
    {
      ligma_display_shell_set_show_guides (shell, active);
    }
}

void
view_toggle_grid_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != ligma_display_shell_get_show_grid (shell))
    {
      ligma_display_shell_set_show_grid (shell, active);
    }
}

void
view_toggle_sample_points_cmd_callback (LigmaAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  LigmaDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != ligma_display_shell_get_show_sample_points (shell))
    {
      ligma_display_shell_set_show_sample_points (shell, active);
    }
}

void
view_snap_to_guides_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != ligma_display_shell_get_snap_to_guides (shell))
    {
      ligma_display_shell_set_snap_to_guides (shell, active);
    }
}

void
view_snap_to_grid_cmd_callback (LigmaAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  LigmaDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != ligma_display_shell_get_snap_to_grid (shell))
    {
      ligma_display_shell_set_snap_to_grid (shell, active);
    }
}

void
view_snap_to_canvas_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != ligma_display_shell_get_snap_to_canvas (shell))
    {
      ligma_display_shell_set_snap_to_canvas (shell, active);
    }
}

void
view_snap_to_vectors_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != ligma_display_shell_get_snap_to_vectors (shell))
    {
      ligma_display_shell_set_snap_to_vectors (shell, active);
    }
}

void
view_padding_color_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  LigmaDisplay           *display;
  LigmaImageWindow       *window;
  LigmaDisplayShell      *shell;
  LigmaDisplayOptions    *options;
  LigmaCanvasPaddingMode  padding_mode;
  gboolean               fullscreen;
  return_if_no_display (display, data);

  padding_mode = (LigmaCanvasPaddingMode) g_variant_get_int32 (value);

  shell  = ligma_display_get_shell (display);
  window = ligma_display_shell_get_window (shell);

  if (window)
    fullscreen = ligma_image_window_get_fullscreen (window);
  else
    fullscreen = FALSE;

  if (fullscreen)
    options = shell->fullscreen_options;
  else
    options = shell->options;

#define PADDING_COLOR_DIALOG_KEY "ligma-padding-color-dialog"

  switch (padding_mode)
    {
    case LIGMA_CANVAS_PADDING_MODE_DEFAULT:
    case LIGMA_CANVAS_PADDING_MODE_LIGHT_CHECK:
    case LIGMA_CANVAS_PADDING_MODE_DARK_CHECK:
      dialogs_destroy_dialog (G_OBJECT (shell), PADDING_COLOR_DIALOG_KEY);

      options->padding_mode_set = TRUE;

      ligma_display_shell_set_padding (shell, padding_mode,
                                      &options->padding_color);
      break;

    case LIGMA_CANVAS_PADDING_MODE_CUSTOM:
      {
        GtkWidget             *dialog;
        LigmaRGB               *old_color = g_new (LigmaRGB, 1);
        LigmaCanvasPaddingMode  old_padding_mode;

        dialog = dialogs_get_dialog (G_OBJECT (shell), PADDING_COLOR_DIALOG_KEY);

        if (! dialog)
          {
            LigmaImage        *image = ligma_display_get_image (display);
            LigmaDisplayShell *shell = ligma_display_get_shell (display);

            dialog =
              ligma_color_dialog_new (LIGMA_VIEWABLE (image),
                                     action_data_get_context (data),
                                     FALSE,
                                     _("Set Canvas Padding Color"),
                                     LIGMA_ICON_FONT,
                                     _("Set Custom Canvas Padding Color"),
                                     GTK_WIDGET (shell),
                                     NULL, NULL,
                                     &options->padding_color,
                                     TRUE, FALSE);

            g_signal_connect (dialog, "update",
                              G_CALLBACK (view_padding_color_dialog_update),
                              shell);

            dialogs_attach_dialog (G_OBJECT (shell),
                                   PADDING_COLOR_DIALOG_KEY, dialog);
          }
        *old_color       = options->padding_color;
        old_padding_mode = options->padding_mode;
        g_object_set_data_full (G_OBJECT (dialog), "old-color",
                                old_color, g_free);
        g_object_set_data (G_OBJECT (dialog), "old-padding-mode",
                           GINT_TO_POINTER (old_padding_mode));

        gtk_window_present (GTK_WINDOW (dialog));
      }
      break;

    case LIGMA_CANVAS_PADDING_MODE_RESET:
      dialogs_destroy_dialog (G_OBJECT (shell), PADDING_COLOR_DIALOG_KEY);

      {
        LigmaDisplayOptions *default_options;

        options->padding_mode_set = FALSE;

        if (fullscreen)
          default_options = display->config->default_fullscreen_view;
        else
          default_options = display->config->default_view;

        ligma_display_shell_set_padding (shell,
                                        default_options->padding_mode,
                                        &default_options->padding_color);
        ligma_display_shell_set_padding_in_show_all (shell,
                                                    default_options->padding_in_show_all);
      }
      break;
    }
}

void
view_padding_color_in_show_all_cmd_callback (LigmaAction *action,
                                             GVariant   *value,
                                             gpointer    data)
{
  LigmaDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != ligma_display_shell_get_padding_in_show_all (shell))
    {
      ligma_display_shell_set_padding_in_show_all (shell, active);
    }
}

void
view_shrink_wrap_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaDisplayShell *shell;
  return_if_no_shell (shell, data);

  ligma_display_shell_scale_shrink_wrap (shell, FALSE);
}

void
view_fullscreen_cmd_callback (LigmaAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  LigmaDisplay      *display;
  LigmaDisplayShell *shell;
  LigmaImageWindow  *window;
  return_if_no_display (display, data);

  shell  = ligma_display_get_shell (display);
  window = ligma_display_shell_get_window (shell);

  if (window)
    {
      gboolean active = g_variant_get_boolean (value);

      ligma_image_window_set_fullscreen (window, active);
    }
}


/*  private functions  */

static void
view_padding_color_dialog_update (LigmaColorDialog      *dialog,
                                  const LigmaRGB        *color,
                                  LigmaColorDialogState  state,
                                  LigmaDisplayShell     *shell)
{
  LigmaImageWindow       *window;
  LigmaDisplayOptions    *options;
  LigmaRGB               *old_color;
  LigmaCanvasPaddingMode  old_padding_mode;
  gboolean               fullscreen;

  window           = ligma_display_shell_get_window (shell);
  old_color        = g_object_get_data (G_OBJECT (dialog), "old-color");
  old_padding_mode = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dialog), "old-padding-mode"));

  g_return_if_fail (old_color);

  if (window)
    fullscreen = ligma_image_window_get_fullscreen (window);
  else
    fullscreen = FALSE;

  if (fullscreen)
    options = shell->fullscreen_options;
  else
    options = shell->options;

  switch (state)
    {
    case LIGMA_COLOR_DIALOG_OK:
      options->padding_mode_set = TRUE;

      ligma_display_shell_set_padding (shell, LIGMA_CANVAS_PADDING_MODE_CUSTOM,
                                      color);
      gtk_widget_destroy (GTK_WIDGET (dialog));
      break;

    case LIGMA_COLOR_DIALOG_CANCEL:
      ligma_display_shell_set_padding (shell,
                                      old_padding_mode,
                                      old_color);
      gtk_widget_destroy (GTK_WIDGET (dialog));
      break;

    case LIGMA_COLOR_DIALOG_UPDATE:
      ligma_display_shell_set_padding (shell, LIGMA_CANVAS_PADDING_MODE_CUSTOM,
                                      color);
      break;

    default:
      break;
    }
}
