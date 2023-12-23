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

#include "display-types.h"

#include "config/gimpdisplayoptions.h"

#include "core/gimpimage.h"

#include "widgets/gimpdockcolumns.h"
#include "widgets/gimpmenumodel.h"
#include "widgets/gimprender.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpcanvas.h"
#include "gimpcanvasitem.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-actions.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-expose.h"
#include "gimpdisplayshell-selection.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-scrollbars.h"
#include "gimpimagewindow.h"
#include "gimpstatusbar.h"


/*  local function prototypes  */

static GimpDisplayOptions * appearance_get_options (GimpDisplayShell *shell);


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
      gboolean fullscreen = gimp_image_window_get_fullscreen (window);

      gimp_display_shell_set_action_active (shell, "view-fullscreen",
                                            fullscreen);
    }

  gimp_display_shell_set_show_menubar        (shell,
                                              options->show_menubar);
  gimp_display_shell_set_show_statusbar      (shell,
                                              options->show_statusbar);

  gimp_display_shell_set_show_rulers         (shell,
                                              options->show_rulers);
  gimp_display_shell_set_show_scrollbars     (shell,
                                              options->show_scrollbars);
  gimp_display_shell_set_show_selection      (shell,
                                              options->show_selection);
  gimp_display_shell_set_show_layer          (shell,
                                              options->show_layer_boundary);
  gimp_display_shell_set_show_canvas         (shell,
                                              options->show_canvas_boundary);
  gimp_display_shell_set_show_guides         (shell,
                                              options->show_guides);
  gimp_display_shell_set_show_grid           (shell,
                                              options->show_grid);
  gimp_display_shell_set_show_sample_points  (shell,
                                              options->show_sample_points);
  gimp_display_shell_set_padding             (shell,
                                              options->padding_mode,
                                              options->padding_color);
  gimp_display_shell_set_padding_in_show_all (shell,
                                              options->padding_in_show_all);
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

  gimp_display_shell_set_action_active (shell, "view-show-menubar", show);
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

  gimp_display_shell_set_action_active (shell, "view-show-statusbar", show);
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
  gtk_widget_set_visible (shell->quick_mask_button, show);
  gtk_widget_set_visible (shell->zoom_button, show);

  gimp_display_shell_set_action_active (shell, "view-show-rulers", show);
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

  gimp_display_shell_set_action_active (shell, "view-show-scrollbars", show);
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

  gimp_display_shell_set_action_active (shell, "view-show-selection", show);
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

  gimp_display_shell_set_action_active (shell, "view-show-layer-boundary", show);
}

gboolean
gimp_display_shell_get_show_layer (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_layer_boundary;
}

void
gimp_display_shell_set_show_canvas (GimpDisplayShell *shell,
                                    gboolean          show)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-canvas-boundary", show, NULL);

  gimp_canvas_item_set_visible (shell->canvas_boundary,
                                show && shell->show_all);

  gimp_display_shell_set_action_active (shell, "view-show-canvas-boundary", show);
}

gboolean
gimp_display_shell_get_show_canvas (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_canvas_boundary;
}

void
gimp_display_shell_update_show_canvas (GimpDisplayShell *shell)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  gimp_canvas_item_set_visible (shell->canvas_boundary,
                                options->show_canvas_boundary &&
                                shell->show_all);
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

  gimp_display_shell_set_action_active (shell, "view-show-guides", show);
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

  gimp_display_shell_set_action_active (shell, "view-show-grid", show);
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

  gimp_display_shell_set_action_active (shell, "view-show-sample-points", show);
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
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "snap-to-grid", snap, NULL);
}

gboolean
gimp_display_shell_get_snap_to_grid (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->snap_to_grid;
}

void
gimp_display_shell_set_snap_to_guides (GimpDisplayShell *shell,
                                       gboolean          snap)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "snap-to-guides", snap, NULL);
}

gboolean
gimp_display_shell_get_snap_to_guides (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->snap_to_guides;
}

void
gimp_display_shell_set_snap_to_canvas (GimpDisplayShell *shell,
                                       gboolean          snap)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "snap-to-canvas", snap, NULL);
}

gboolean
gimp_display_shell_get_snap_to_canvas (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->snap_to_canvas;
}

void
gimp_display_shell_set_snap_to_vectors (GimpDisplayShell *shell,
                                        gboolean          snap)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "snap-to-path", snap, NULL);
}

gboolean
gimp_display_shell_get_snap_to_vectors (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->snap_to_path;
}

void
gimp_display_shell_set_snap_to_bbox (GimpDisplayShell *shell,
                                     gboolean          snap)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "snap-to-bbox", snap, NULL);
}

gboolean
gimp_display_shell_get_snap_to_bbox (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->snap_to_bbox;
}

void
gimp_display_shell_set_snap_to_equidistance (GimpDisplayShell *shell,
                                             gboolean          snap)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "snap-to-equidistance", snap, NULL);
}

gboolean
gimp_display_shell_get_snap_to_equidistance (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->snap_to_equidistance;
}

void
gimp_display_shell_set_padding (GimpDisplayShell      *shell,
                                GimpCanvasPaddingMode  padding_mode,
                                GeglColor             *padding_color)
{
  GimpImageWindow    *window;
  GimpMenuModel      *model;
  GimpDisplayOptions *options;
  GeglColor          *color;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GEGL_IS_COLOR (padding_color));

  options = appearance_get_options (shell);
  color   = padding_color;

  switch (padding_mode)
    {
    case GIMP_CANVAS_PADDING_MODE_DEFAULT:
      break;

    case GIMP_CANVAS_PADDING_MODE_LIGHT_CHECK:
      color = GEGL_COLOR (gimp_render_check_color1 ());
      break;

    case GIMP_CANVAS_PADDING_MODE_DARK_CHECK:
      color = GEGL_COLOR (gimp_render_check_color2 ());
      break;

    case GIMP_CANVAS_PADDING_MODE_CUSTOM:
    case GIMP_CANVAS_PADDING_MODE_RESET:
      break;
    }

  color = gegl_color_duplicate (color);
  g_object_set (options,
                "padding-mode",  padding_mode,
                "padding-color", color,
                NULL);

  gimp_canvas_set_padding (GIMP_CANVAS (shell->canvas),
                           padding_mode, color);

  window = gimp_display_shell_get_window (shell);
  model  = gimp_image_window_get_menubar_model (window);

  if (padding_mode != GIMP_CANVAS_PADDING_MODE_DEFAULT)
    gimp_menu_model_set_color (model, "/View/Padding color", options->padding_color);
  else
    gimp_menu_model_set_color (model, "/View/Padding color", NULL);

  g_object_unref (color);
}

void
gimp_display_shell_get_padding (GimpDisplayShell       *shell,
                                GimpCanvasPaddingMode  *padding_mode,
                                GeglColor             **padding_color)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  if (padding_mode)
    *padding_mode = options->padding_mode;

  if (padding_color)
    *padding_color = gegl_color_duplicate (options->padding_color);
}

void
gimp_display_shell_set_padding_in_show_all (GimpDisplayShell *shell,
                                            gboolean          keep)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  if (options->padding_in_show_all != keep)
    {
      g_object_set (options, "padding-in-show-all", keep, NULL);

      if (shell->display)
        {
          gimp_display_shell_scroll_clamp_and_update (shell);
          gimp_display_shell_scrollbars_update (shell);

          gimp_display_shell_expose_full (shell);
        }

      gimp_display_shell_set_action_active (shell,
                                            "view-padding-color-in-show-all",
                                            keep);

      g_object_notify (G_OBJECT (shell), "infinite-canvas");
    }
}

gboolean
gimp_display_shell_get_padding_in_show_all (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->padding_in_show_all;
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
