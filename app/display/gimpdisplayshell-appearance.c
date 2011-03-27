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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "config/gimpdisplayoptions.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpdockcolumns.h"
#include "widgets/gimprender.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpcanvas.h"
#include "gimpcanvasitem.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-selection.h"
#include "gimpimagewindow.h"
#include "gimpstatusbar.h"


/*  local function prototypes  */

static GimpDisplayOptions *
              appearance_get_options       (GimpDisplayShell       *shell);
static void   appearance_set_action_active (GimpDisplayShell       *shell,
                                            const gchar            *action,
                                            gboolean                active);
static void   appearance_set_action_color  (GimpDisplayShell       *shell,
                                            const gchar            *action,
                                            const GimpRGB          *color);


/*  public functions  */

void
gimp_display_shell_appearance_update (GimpDisplayShell *shell)
{
  GimpDisplayOptions *options;
  GimpImageWindow    *window;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);
  window  = gimp_display_shell_get_window (shell);

  if (window)
    {
      GimpDockColumns *left_docks;
      GimpDockColumns *right_docks;
      gboolean         fullscreen;
      gboolean         has_grip;

      fullscreen = gimp_image_window_get_fullscreen (window);

      appearance_set_action_active (shell, "view-fullscreen", fullscreen);

      left_docks  = gimp_image_window_get_left_docks (window);
      right_docks = gimp_image_window_get_right_docks (window);

      has_grip = (! fullscreen &&
                  ! (left_docks  && gimp_dock_columns_get_docks (left_docks)) &&
                  ! (right_docks && gimp_dock_columns_get_docks (right_docks)));

      gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (shell->statusbar),
                                         has_grip);
    }

  gimp_display_shell_set_show_menubar       (shell,
                                             options->show_menubar);
  gimp_display_shell_set_show_statusbar     (shell,
                                             options->show_statusbar);

  gimp_display_shell_set_show_rulers        (shell,
                                             options->show_rulers);
  gimp_display_shell_set_show_scrollbars    (shell,
                                             options->show_scrollbars);
  gimp_display_shell_set_show_selection     (shell,
                                             options->show_selection);
  gimp_display_shell_set_show_layer         (shell,
                                             options->show_layer_boundary);
  gimp_display_shell_set_show_guides        (shell,
                                             options->show_guides);
  gimp_display_shell_set_show_grid          (shell,
                                             options->show_grid);
  gimp_display_shell_set_show_sample_points (shell,
                                             options->show_sample_points);
  gimp_display_shell_set_padding            (shell,
                                             options->padding_mode,
                                             &options->padding_color);
}

void
gimp_display_shell_set_show_menubar (GimpDisplayShell *shell,
                                     gboolean          show)
{
  GimpDisplayOptions *options;
  GimpImageWindow    *window;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);
  window  = gimp_display_shell_get_window (shell);

  g_object_set (options, "show-menubar", show, NULL);

  if (window && gimp_image_window_get_active_shell (window) == shell)
    {
      gimp_image_window_keep_canvas_pos (gimp_display_shell_get_window (shell));
      gimp_image_window_set_show_menubar (window, show);
    }

  appearance_set_action_active (shell, "view-show-menubar", show);
}

gboolean
gimp_display_shell_get_show_menubar (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_menubar;
}

void
gimp_display_shell_set_show_statusbar (GimpDisplayShell *shell,
                                       gboolean          show)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-statusbar", show, NULL);

  gimp_image_window_keep_canvas_pos (gimp_display_shell_get_window (shell));
  gimp_statusbar_set_visible (GIMP_STATUSBAR (shell->statusbar), show);

  appearance_set_action_active (shell, "view-show-statusbar", show);
}

gboolean
gimp_display_shell_get_show_statusbar (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_statusbar;
}

void
gimp_display_shell_set_show_rulers (GimpDisplayShell *shell,
                                    gboolean          show)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-rulers", show, NULL);

  gimp_image_window_keep_canvas_pos (gimp_display_shell_get_window (shell));
  gtk_widget_set_visible (shell->origin, show);
  gtk_widget_set_visible (shell->hrule, show);
  gtk_widget_set_visible (shell->vrule, show);

  appearance_set_action_active (shell, "view-show-rulers", show);
}

gboolean
gimp_display_shell_get_show_rulers (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_rulers;
}

void
gimp_display_shell_set_show_scrollbars (GimpDisplayShell *shell,
                                        gboolean          show)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-scrollbars", show, NULL);

  gimp_image_window_keep_canvas_pos (gimp_display_shell_get_window (shell));
  gtk_widget_set_visible (shell->nav_ebox, show);
  gtk_widget_set_visible (shell->hsb, show);
  gtk_widget_set_visible (shell->vsb, show);
  gtk_widget_set_visible (shell->quick_mask_button, show);
  gtk_widget_set_visible (shell->zoom_button, show);

  appearance_set_action_active (shell, "view-show-scrollbars", show);
}

gboolean
gimp_display_shell_get_show_scrollbars (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_scrollbars;
}

void
gimp_display_shell_set_show_selection (GimpDisplayShell *shell,
                                       gboolean          show)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-selection", show, NULL);

  gimp_display_shell_selection_set_show (shell, show);

  appearance_set_action_active (shell, "view-show-selection", show);
}

gboolean
gimp_display_shell_get_show_selection (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_selection;
}

void
gimp_display_shell_set_show_layer (GimpDisplayShell *shell,
                                   gboolean          show)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-layer-boundary", show, NULL);

  gimp_canvas_item_set_visible (shell->layer_boundary, show);

  appearance_set_action_active (shell, "view-show-layer-boundary", show);
}

gboolean
gimp_display_shell_get_show_layer (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_layer_boundary;
}

void
gimp_display_shell_set_show_guides (GimpDisplayShell *shell,
                                    gboolean          show)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-guides", show, NULL);

  gimp_canvas_item_set_visible (shell->guides, show);

  appearance_set_action_active (shell, "view-show-guides", show);
}

gboolean
gimp_display_shell_get_show_guides (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_guides;
}

void
gimp_display_shell_set_show_grid (GimpDisplayShell *shell,
                                  gboolean          show)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-grid", show, NULL);

  gimp_canvas_item_set_visible (shell->grid, show);

  appearance_set_action_active (shell, "view-show-grid", show);
}

gboolean
gimp_display_shell_get_show_grid (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_grid;
}

void
gimp_display_shell_set_show_sample_points (GimpDisplayShell *shell,
                                           gboolean          show)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-sample-points", show, NULL);

  gimp_canvas_item_set_visible (shell->sample_points, show);

  appearance_set_action_active (shell, "view-show-sample-points", show);
}

gboolean
gimp_display_shell_get_show_sample_points (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_sample_points;
}

void
gimp_display_shell_set_snap_to_grid (GimpDisplayShell *shell,
                                     gboolean          snap)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (snap != shell->snap_to_grid)
    {
      shell->snap_to_grid = snap ? TRUE : FALSE;

      appearance_set_action_active (shell, "view-snap-to-grid", snap);
    }
}

gboolean
gimp_display_shell_get_snap_to_grid (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return shell->snap_to_grid;
}

void
gimp_display_shell_set_snap_to_guides (GimpDisplayShell *shell,
                                       gboolean          snap)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (snap != shell->snap_to_guides)
    {
      shell->snap_to_guides = snap ? TRUE : FALSE;

      appearance_set_action_active (shell, "view-snap-to-guides", snap);
    }
}

gboolean
gimp_display_shell_get_snap_to_guides (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return shell->snap_to_guides;
}

void
gimp_display_shell_set_snap_to_canvas (GimpDisplayShell *shell,
                                       gboolean          snap)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (snap != shell->snap_to_canvas)
    {
      shell->snap_to_canvas = snap ? TRUE : FALSE;

      appearance_set_action_active (shell, "view-snap-to-canvas", snap);
    }
}

gboolean
gimp_display_shell_get_snap_to_canvas (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return shell->snap_to_canvas;
}

void
gimp_display_shell_set_snap_to_vectors (GimpDisplayShell *shell,
                                        gboolean          snap)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (snap != shell->snap_to_vectors)
    {
      shell->snap_to_vectors = snap ? TRUE : FALSE;

      appearance_set_action_active (shell, "view-snap-to-vectors", snap);
    }
}

gboolean
gimp_display_shell_get_snap_to_vectors (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return shell->snap_to_vectors;
}

void
gimp_display_shell_set_padding (GimpDisplayShell      *shell,
                                GimpCanvasPaddingMode  padding_mode,
                                const GimpRGB         *padding_color)
{
  GimpDisplayOptions *options;
  GimpRGB             color;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (padding_color != NULL);

  options = appearance_get_options (shell);
  color   = *padding_color;

  switch (padding_mode)
    {
    case GIMP_CANVAS_PADDING_MODE_DEFAULT:
      if (shell->canvas)
        {
          GtkStyle *style;

          gtk_widget_ensure_style (shell->canvas);

          style = gtk_widget_get_style (shell->canvas);

          gimp_rgb_set_gdk_color (&color, style->bg + GTK_STATE_NORMAL);
        }
      break;

    case GIMP_CANVAS_PADDING_MODE_LIGHT_CHECK:
      color = *gimp_render_light_check_color ();
      break;

    case GIMP_CANVAS_PADDING_MODE_DARK_CHECK:
      color = *gimp_render_dark_check_color ();
      break;

    case GIMP_CANVAS_PADDING_MODE_CUSTOM:
    case GIMP_CANVAS_PADDING_MODE_RESET:
      break;
    }

  g_object_set (options,
                "padding-mode",  padding_mode,
                "padding-color", &color,
                NULL);

  gimp_canvas_set_bg_color (GIMP_CANVAS (shell->canvas), &color);

  appearance_set_action_color (shell, "view-padding-color-menu",
                               &options->padding_color);
}

void
gimp_display_shell_get_padding (GimpDisplayShell       *shell,
                                GimpCanvasPaddingMode  *padding_mode,
                                GimpRGB                *padding_color)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  if (padding_mode)
    *padding_mode = options->padding_mode;

  if (padding_color)
    *padding_color = options->padding_color;
}


/*  private functions  */

static GimpDisplayOptions *
appearance_get_options (GimpDisplayShell *shell)
{
  if (gimp_display_get_image (shell->display))
    {
      GimpImageWindow *window = gimp_display_shell_get_window (shell);

      if (window && gimp_image_window_get_fullscreen (window))
        return shell->fullscreen_options;
      else
        return shell->options;
    }

  return shell->no_image_options;
}

static void
appearance_set_action_active (GimpDisplayShell *shell,
                              const gchar      *action,
                              gboolean          active)
{
  GimpImageWindow *window = gimp_display_shell_get_window (shell);
  GimpContext     *context;

  if (window && gimp_image_window_get_active_shell (window) == shell)
    {
      GimpUIManager   *manager = gimp_image_window_get_ui_manager (window);
      GimpActionGroup *action_group;

      action_group = gimp_ui_manager_get_action_group (manager, "view");

      if (action_group)
        gimp_action_group_set_action_active (action_group, action, active);
    }

  context = gimp_get_user_context (shell->display->gimp);

  if (shell->display == gimp_context_get_display (context))
    {
      GimpActionGroup *action_group;

      action_group = gimp_ui_manager_get_action_group (shell->popup_manager,
                                                       "view");

      if (action_group)
        gimp_action_group_set_action_active (action_group, action, active);
    }
}

static void
appearance_set_action_color (GimpDisplayShell *shell,
                             const gchar      *action,
                             const GimpRGB    *color)
{
  GimpImageWindow *window = gimp_display_shell_get_window (shell);
  GimpContext     *context;

  if (window && gimp_image_window_get_active_shell (window) == shell)
    {
      GimpUIManager   *manager = gimp_image_window_get_ui_manager (window);
      GimpActionGroup *action_group;

      action_group = gimp_ui_manager_get_action_group (manager, "view");

      if (action_group)
        gimp_action_group_set_action_color (action_group, action, color, FALSE);
    }

  context = gimp_get_user_context (shell->display->gimp);

  if (shell->display == gimp_context_get_display (context))
    {
      GimpActionGroup *action_group;

      action_group = gimp_ui_manager_get_action_group (shell->popup_manager,
                                                       "view");

      if (action_group)
        gimp_action_group_set_action_color (action_group, action, color, FALSE);
    }
}
