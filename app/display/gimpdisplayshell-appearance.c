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

#include "display-types.h"

#include "config/ligmadisplayoptions.h"

#include "core/ligmaimage.h"

#include "widgets/ligmadockcolumns.h"
#include "widgets/ligmarender.h"
#include "widgets/ligmawidgets-utils.h"

#include "ligmacanvas.h"
#include "ligmacanvasitem.h"
#include "ligmadisplay.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-actions.h"
#include "ligmadisplayshell-appearance.h"
#include "ligmadisplayshell-expose.h"
#include "ligmadisplayshell-selection.h"
#include "ligmadisplayshell-scroll.h"
#include "ligmadisplayshell-scrollbars.h"
#include "ligmaimagewindow.h"
#include "ligmastatusbar.h"


/*  local function prototypes  */

static LigmaDisplayOptions * appearance_get_options (LigmaDisplayShell *shell);


/*  public functions  */

void
ligma_display_shell_appearance_update (LigmaDisplayShell *shell)
{
  LigmaDisplayOptions *options;
  LigmaImageWindow    *window;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);
  window  = ligma_display_shell_get_window (shell);

  if (window)
    {
      gboolean fullscreen = ligma_image_window_get_fullscreen (window);

      ligma_display_shell_set_action_active (shell, "view-fullscreen",
                                            fullscreen);
    }

  ligma_display_shell_set_show_menubar        (shell,
                                              options->show_menubar);
  ligma_display_shell_set_show_statusbar      (shell,
                                              options->show_statusbar);

  ligma_display_shell_set_show_rulers         (shell,
                                              options->show_rulers);
  ligma_display_shell_set_show_scrollbars     (shell,
                                              options->show_scrollbars);
  ligma_display_shell_set_show_selection      (shell,
                                              options->show_selection);
  ligma_display_shell_set_show_layer          (shell,
                                              options->show_layer_boundary);
  ligma_display_shell_set_show_canvas         (shell,
                                              options->show_canvas_boundary);
  ligma_display_shell_set_show_guides         (shell,
                                              options->show_guides);
  ligma_display_shell_set_show_grid           (shell,
                                              options->show_grid);
  ligma_display_shell_set_show_sample_points  (shell,
                                              options->show_sample_points);
  ligma_display_shell_set_padding             (shell,
                                              options->padding_mode,
                                              &options->padding_color);
  ligma_display_shell_set_padding_in_show_all (shell,
                                              options->padding_in_show_all);
}

void
ligma_display_shell_set_show_menubar (LigmaDisplayShell *shell,
                                     gboolean          show)
{
  LigmaDisplayOptions *options;
  LigmaImageWindow    *window;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);
  window  = ligma_display_shell_get_window (shell);

  g_object_set (options, "show-menubar", show, NULL);

  if (window && ligma_image_window_get_active_shell (window) == shell)
    {
      ligma_image_window_keep_canvas_pos (ligma_display_shell_get_window (shell));
      ligma_image_window_set_show_menubar (window, show);
    }

  ligma_display_shell_set_action_active (shell, "view-show-menubar", show);
}

gboolean
ligma_display_shell_get_show_menubar (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_menubar;
}

void
ligma_display_shell_set_show_statusbar (LigmaDisplayShell *shell,
                                       gboolean          show)
{
  LigmaDisplayOptions *options;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-statusbar", show, NULL);

  ligma_image_window_keep_canvas_pos (ligma_display_shell_get_window (shell));
  ligma_statusbar_set_visible (LIGMA_STATUSBAR (shell->statusbar), show);

  ligma_display_shell_set_action_active (shell, "view-show-statusbar", show);
}

gboolean
ligma_display_shell_get_show_statusbar (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_statusbar;
}

void
ligma_display_shell_set_show_rulers (LigmaDisplayShell *shell,
                                    gboolean          show)
{
  LigmaDisplayOptions *options;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-rulers", show, NULL);

  ligma_image_window_keep_canvas_pos (ligma_display_shell_get_window (shell));
  gtk_widget_set_visible (shell->origin, show);
  gtk_widget_set_visible (shell->hrule, show);
  gtk_widget_set_visible (shell->vrule, show);
  gtk_widget_set_visible (shell->quick_mask_button, show);
  gtk_widget_set_visible (shell->zoom_button, show);

  ligma_display_shell_set_action_active (shell, "view-show-rulers", show);
}

gboolean
ligma_display_shell_get_show_rulers (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_rulers;
}

void
ligma_display_shell_set_show_scrollbars (LigmaDisplayShell *shell,
                                        gboolean          show)
{
  LigmaDisplayOptions *options;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-scrollbars", show, NULL);

  ligma_image_window_keep_canvas_pos (ligma_display_shell_get_window (shell));
  gtk_widget_set_visible (shell->nav_ebox, show);
  gtk_widget_set_visible (shell->hsb, show);
  gtk_widget_set_visible (shell->vsb, show);
  gtk_widget_set_visible (shell->quick_mask_button, show);
  gtk_widget_set_visible (shell->zoom_button, show);

  ligma_display_shell_set_action_active (shell, "view-show-scrollbars", show);
}

gboolean
ligma_display_shell_get_show_scrollbars (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_scrollbars;
}

void
ligma_display_shell_set_show_selection (LigmaDisplayShell *shell,
                                       gboolean          show)
{
  LigmaDisplayOptions *options;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-selection", show, NULL);

  ligma_display_shell_selection_set_show (shell, show);

  ligma_display_shell_set_action_active (shell, "view-show-selection", show);
}

gboolean
ligma_display_shell_get_show_selection (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_selection;
}

void
ligma_display_shell_set_show_layer (LigmaDisplayShell *shell,
                                   gboolean          show)
{
  LigmaDisplayOptions *options;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-layer-boundary", show, NULL);

  ligma_canvas_item_set_visible (shell->layer_boundary, show);

  ligma_display_shell_set_action_active (shell, "view-show-layer-boundary", show);
}

gboolean
ligma_display_shell_get_show_layer (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_layer_boundary;
}

void
ligma_display_shell_set_show_canvas (LigmaDisplayShell *shell,
                                    gboolean          show)
{
  LigmaDisplayOptions *options;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-canvas-boundary", show, NULL);

  ligma_canvas_item_set_visible (shell->canvas_boundary,
                                show && shell->show_all);

  ligma_display_shell_set_action_active (shell, "view-show-canvas-boundary", show);
}

gboolean
ligma_display_shell_get_show_canvas (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_canvas_boundary;
}

void
ligma_display_shell_update_show_canvas (LigmaDisplayShell *shell)
{
  LigmaDisplayOptions *options;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  ligma_canvas_item_set_visible (shell->canvas_boundary,
                                options->show_canvas_boundary &&
                                shell->show_all);
}

void
ligma_display_shell_set_show_guides (LigmaDisplayShell *shell,
                                    gboolean          show)
{
  LigmaDisplayOptions *options;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-guides", show, NULL);

  ligma_canvas_item_set_visible (shell->guides, show);

  ligma_display_shell_set_action_active (shell, "view-show-guides", show);
}

gboolean
ligma_display_shell_get_show_guides (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_guides;
}

void
ligma_display_shell_set_show_grid (LigmaDisplayShell *shell,
                                  gboolean          show)
{
  LigmaDisplayOptions *options;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-grid", show, NULL);

  ligma_canvas_item_set_visible (shell->grid, show);

  ligma_display_shell_set_action_active (shell, "view-show-grid", show);
}

gboolean
ligma_display_shell_get_show_grid (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_grid;
}

void
ligma_display_shell_set_show_sample_points (LigmaDisplayShell *shell,
                                           gboolean          show)
{
  LigmaDisplayOptions *options;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-sample-points", show, NULL);

  ligma_canvas_item_set_visible (shell->sample_points, show);

  ligma_display_shell_set_action_active (shell, "view-show-sample-points", show);
}

gboolean
ligma_display_shell_get_show_sample_points (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_sample_points;
}

void
ligma_display_shell_set_snap_to_grid (LigmaDisplayShell *shell,
                                     gboolean          snap)
{
  LigmaDisplayOptions *options;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "snap-to-grid", snap, NULL);
}

gboolean
ligma_display_shell_get_snap_to_grid (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->snap_to_grid;
}

void
ligma_display_shell_set_snap_to_guides (LigmaDisplayShell *shell,
                                       gboolean          snap)
{
  LigmaDisplayOptions *options;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "snap-to-guides", snap, NULL);
}

gboolean
ligma_display_shell_get_snap_to_guides (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->snap_to_guides;
}

void
ligma_display_shell_set_snap_to_canvas (LigmaDisplayShell *shell,
                                       gboolean          snap)
{
  LigmaDisplayOptions *options;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "snap-to-canvas", snap, NULL);
}

gboolean
ligma_display_shell_get_snap_to_canvas (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->snap_to_canvas;
}

void
ligma_display_shell_set_snap_to_vectors (LigmaDisplayShell *shell,
                                        gboolean          snap)
{
  LigmaDisplayOptions *options;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "snap-to-path", snap, NULL);
}

gboolean
ligma_display_shell_get_snap_to_vectors (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->snap_to_path;
}

void
ligma_display_shell_set_padding (LigmaDisplayShell      *shell,
                                LigmaCanvasPaddingMode  padding_mode,
                                const LigmaRGB         *padding_color)
{
  LigmaDisplayOptions *options;
  LigmaRGB             color;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (padding_color != NULL);

  options = appearance_get_options (shell);
  color   = *padding_color;

  switch (padding_mode)
    {
    case LIGMA_CANVAS_PADDING_MODE_DEFAULT:
      break;

    case LIGMA_CANVAS_PADDING_MODE_LIGHT_CHECK:
      color = *ligma_render_check_color1 ();
      break;

    case LIGMA_CANVAS_PADDING_MODE_DARK_CHECK:
      color = *ligma_render_check_color2 ();
      break;

    case LIGMA_CANVAS_PADDING_MODE_CUSTOM:
    case LIGMA_CANVAS_PADDING_MODE_RESET:
      break;
    }

  g_object_set (options,
                "padding-mode",  padding_mode,
                "padding-color", &color,
                NULL);

  ligma_canvas_set_padding (LIGMA_CANVAS (shell->canvas),
                           padding_mode, &color);

  if (padding_mode != LIGMA_CANVAS_PADDING_MODE_DEFAULT)
    ligma_display_shell_set_action_color (shell, "view-padding-color-menu",
                                         &options->padding_color);
  else
    ligma_display_shell_set_action_color (shell, "view-padding-color-menu",
                                         NULL);
}

void
ligma_display_shell_get_padding (LigmaDisplayShell      *shell,
                                LigmaCanvasPaddingMode *padding_mode,
                                LigmaRGB               *padding_color)
{
  LigmaDisplayOptions *options;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  if (padding_mode)
    *padding_mode = options->padding_mode;

  if (padding_color)
    *padding_color = options->padding_color;
}

void
ligma_display_shell_set_padding_in_show_all (LigmaDisplayShell *shell,
                                            gboolean          keep)
{
  LigmaDisplayOptions *options;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  if (options->padding_in_show_all != keep)
    {
      g_object_set (options, "padding-in-show-all", keep, NULL);

      if (shell->display)
        {
          ligma_display_shell_scroll_clamp_and_update (shell);
          ligma_display_shell_scrollbars_update (shell);

          ligma_display_shell_expose_full (shell);
        }

      ligma_display_shell_set_action_active (shell,
                                            "view-padding-color-in-show-all",
                                            keep);

      g_object_notify (G_OBJECT (shell), "infinite-canvas");
    }
}

gboolean
ligma_display_shell_get_padding_in_show_all (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->padding_in_show_all;
}


/*  private functions  */

static LigmaDisplayOptions *
appearance_get_options (LigmaDisplayShell *shell)
{
  if (ligma_display_get_image (shell->display))
    {
      LigmaImageWindow *window = ligma_display_shell_get_window (shell);

      if (window && ligma_image_window_get_fullscreen (window))
        return shell->fullscreen_options;
      else
        return shell->options;
    }

  return shell->no_image_options;
}
