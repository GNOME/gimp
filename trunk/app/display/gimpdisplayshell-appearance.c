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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimprender.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpcanvas.h"
#include "gimpdisplay.h"
#include "gimpdisplayoptions.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-callbacks.h"
#include "gimpdisplayshell-selection.h"
#include "gimpstatusbar.h"


#define GET_OPTIONS(shell) \
  (gimp_display_shell_get_fullscreen (shell) ? \
   shell->fullscreen_options : shell->options)

#define SET_ACTIVE(manager,action_name,active) \
  { GimpActionGroup *group = \
      gimp_ui_manager_get_action_group (manager, "view"); \
    gimp_action_group_set_action_active (group, action_name, active); }

#define SET_COLOR(manager,action_name,color) \
  { GimpActionGroup *group = \
      gimp_ui_manager_get_action_group (manager, "view"); \
    gimp_action_group_set_action_color (group, action_name, color, FALSE); }

#define IS_ACTIVE_DISPLAY(shell) \
  ((shell)->display == \
   gimp_context_get_display (gimp_get_user_context \
                             ((shell)->display->image->gimp)))


void
gimp_display_shell_set_fullscreen (GimpDisplayShell *shell,
                                   gboolean          fullscreen)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (fullscreen != gimp_display_shell_get_fullscreen (shell))
    {
      if (fullscreen)
        gtk_window_fullscreen (GTK_WINDOW (shell));
      else
        gtk_window_unfullscreen (GTK_WINDOW (shell));
    }
}

gboolean
gimp_display_shell_get_fullscreen (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return (shell->window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0;
}

void
gimp_display_shell_set_show_menubar (GimpDisplayShell *shell,
                                     gboolean          show)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = GET_OPTIONS (shell);

  g_object_set (options, "show-menubar", show, NULL);

  if (shell->menubar)
    {
      if (show)
        gtk_widget_show (shell->menubar);
      else
        gtk_widget_hide (shell->menubar);
    }

  SET_ACTIVE (shell->menubar_manager, "view-show-menubar", show);

  if (IS_ACTIVE_DISPLAY (shell))
    SET_ACTIVE (shell->popup_manager, "view-show-menubar", show);
}

gboolean
gimp_display_shell_get_show_menubar (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_OPTIONS (shell)->show_menubar;
}

void
gimp_display_shell_set_show_rulers (GimpDisplayShell *shell,
                                    gboolean          show)
{
  GimpDisplayOptions *options;
  GtkTable           *table;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = GET_OPTIONS (shell);

  g_object_set (options, "show-rulers", show, NULL);

  table = GTK_TABLE (GTK_WIDGET (shell->canvas)->parent);

  if (show)
    {
      gtk_widget_show (shell->origin);
      gtk_widget_show (shell->hrule);
      gtk_widget_show (shell->vrule);

      gtk_table_set_col_spacing (table, 0, 1);
      gtk_table_set_row_spacing (table, 0, 1);
    }
  else
    {
      gtk_widget_hide (shell->origin);
      gtk_widget_hide (shell->hrule);
      gtk_widget_hide (shell->vrule);

      gtk_table_set_col_spacing (table, 0, 0);
      gtk_table_set_row_spacing (table, 0, 0);
    }

  SET_ACTIVE (shell->menubar_manager, "view-show-rulers", show);

  if (IS_ACTIVE_DISPLAY (shell))
    SET_ACTIVE (shell->popup_manager, "view-show-rulers", show);
}

gboolean
gimp_display_shell_get_show_rulers (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_OPTIONS (shell)->show_rulers;
}

void
gimp_display_shell_set_show_scrollbars (GimpDisplayShell *shell,
                                        gboolean          show)
{
  GimpDisplayOptions *options;
  GtkBox             *hbox;
  GtkBox             *vbox;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = GET_OPTIONS (shell);

  g_object_set (options, "show-scrollbars", show, NULL);

  hbox = GTK_BOX (shell->vsb->parent->parent);
  vbox = GTK_BOX (shell->hsb->parent->parent);

  if (show)
    {
      gtk_widget_show (shell->nav_ebox);
      gtk_widget_show (shell->hsb);
      gtk_widget_show (shell->vsb);
      gtk_widget_show (shell->quick_mask_button);
      gtk_widget_show (shell->zoom_button);

      gtk_box_set_spacing (hbox, 1);
      gtk_box_set_spacing (vbox, 1);
    }
  else
    {
      gtk_widget_hide (shell->nav_ebox);
      gtk_widget_hide (shell->hsb);
      gtk_widget_hide (shell->vsb);
      gtk_widget_hide (shell->quick_mask_button);
      gtk_widget_hide (shell->zoom_button);

      gtk_box_set_spacing (hbox, 0);
      gtk_box_set_spacing (vbox, 0);
    }

  SET_ACTIVE (shell->menubar_manager, "view-show-scrollbars", show);

  if (IS_ACTIVE_DISPLAY (shell))
    SET_ACTIVE (shell->popup_manager, "view-show-scrollbars", show);
}

gboolean
gimp_display_shell_get_show_scrollbars (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_OPTIONS (shell)->show_scrollbars;
}

void
gimp_display_shell_set_show_statusbar (GimpDisplayShell *shell,
                                       gboolean          show)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = GET_OPTIONS (shell);

  g_object_set (options, "show-statusbar", show, NULL);

  gimp_statusbar_set_visible (GIMP_STATUSBAR (shell->statusbar), show);

  SET_ACTIVE (shell->menubar_manager, "view-show-statusbar", show);

  if (IS_ACTIVE_DISPLAY (shell))
    SET_ACTIVE (shell->popup_manager, "view-show-statusbar", show);
}

gboolean
gimp_display_shell_get_show_statusbar (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_OPTIONS (shell)->show_statusbar;
}

void
gimp_display_shell_set_show_selection (GimpDisplayShell *shell,
                                       gboolean          show)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = GET_OPTIONS (shell);

  g_object_set (options, "show-selection", show, NULL);

  gimp_display_shell_selection_set_hidden (shell, ! show);

  SET_ACTIVE (shell->menubar_manager, "view-show-selection", show);

  if (IS_ACTIVE_DISPLAY (shell))
    SET_ACTIVE (shell->popup_manager, "view-show-selection", show);
}

gboolean
gimp_display_shell_get_show_selection (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_OPTIONS (shell)->show_selection;
}

void
gimp_display_shell_set_show_layer (GimpDisplayShell *shell,
                                   gboolean          show)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = GET_OPTIONS (shell);

  g_object_set (options, "show-layer-boundary", show, NULL);

  gimp_display_shell_selection_layer_set_hidden (shell, ! show);

  SET_ACTIVE (shell->menubar_manager, "view-show-layer-boundary", show);

  if (IS_ACTIVE_DISPLAY (shell))
    SET_ACTIVE (shell->popup_manager, "view-show-layer-boundary", show);
}

gboolean
gimp_display_shell_get_show_layer (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_OPTIONS (shell)->show_layer_boundary;
}

void
gimp_display_shell_set_show_transform (GimpDisplayShell *shell,
                                       gboolean          show)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  shell->show_transform_preview = show;
}

gboolean
gimp_display_shell_get_show_transform (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return shell->show_transform_preview;
}

void
gimp_display_shell_set_show_guides (GimpDisplayShell *shell,
                                    gboolean          show)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = GET_OPTIONS (shell);

  g_object_set (options, "show-guides", show, NULL);

  if (shell->display->image->guides)
    gimp_display_shell_expose_full (shell);

  SET_ACTIVE (shell->menubar_manager, "view-show-guides", show);

  if (IS_ACTIVE_DISPLAY (shell))
    SET_ACTIVE (shell->popup_manager, "view-show-guides", show);
}

gboolean
gimp_display_shell_get_show_guides (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_OPTIONS (shell)->show_guides;
}

void
gimp_display_shell_set_show_grid (GimpDisplayShell *shell,
                                  gboolean          show)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = GET_OPTIONS (shell);

  g_object_set (options, "show-grid", show, NULL);

  if (shell->display->image->grid)
    gimp_display_shell_expose_full (shell);

  SET_ACTIVE (shell->menubar_manager, "view-show-grid", show);

  if (IS_ACTIVE_DISPLAY (shell))
    SET_ACTIVE (shell->popup_manager, "view-show-grid", show);
}

gboolean
gimp_display_shell_get_show_grid (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_OPTIONS (shell)->show_grid;
}

void
gimp_display_shell_set_show_sample_points (GimpDisplayShell *shell,
                                           gboolean          show)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = GET_OPTIONS (shell);

  g_object_set (options, "show-sample-points", show, NULL);

  if (shell->display->image->sample_points)
    gimp_display_shell_expose_full (shell);

  SET_ACTIVE (shell->menubar_manager, "view-show-sample-points", show);

  if (IS_ACTIVE_DISPLAY (shell))
    SET_ACTIVE (shell->popup_manager, "view-show-sample-points", show);
}

gboolean
gimp_display_shell_get_show_sample_points (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_OPTIONS (shell)->show_sample_points;
}

void
gimp_display_shell_set_snap_to_grid (GimpDisplayShell *shell,
                                     gboolean          snap)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (snap != shell->snap_to_grid)
    {
      shell->snap_to_grid = snap ? TRUE : FALSE;

      SET_ACTIVE (shell->menubar_manager, "view-snap-to-grid", snap);

      if (IS_ACTIVE_DISPLAY (shell))
        SET_ACTIVE (shell->popup_manager, "view-snap-to-grid", snap);
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

      SET_ACTIVE (shell->menubar_manager, "view-snap-to-guides", snap);

      if (IS_ACTIVE_DISPLAY (shell))
        SET_ACTIVE (shell->popup_manager, "view-snap-to-guides", snap);
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

      SET_ACTIVE (shell->menubar_manager, "view-snap-to-canvas", snap);

      if (IS_ACTIVE_DISPLAY (shell))
        SET_ACTIVE (shell->popup_manager, "view-snap-to-canvas", snap);
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

      SET_ACTIVE (shell->menubar_manager, "view-snap-to-vectors", snap);

      if (IS_ACTIVE_DISPLAY (shell))
        SET_ACTIVE (shell->popup_manager, "view-snap-to-vectors", snap);
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

  options = GET_OPTIONS (shell);
  color   = *padding_color;

  switch (padding_mode)
    {
    case GIMP_CANVAS_PADDING_MODE_DEFAULT:
      if (shell->canvas)
        {
          gtk_widget_ensure_style (shell->canvas);
          gimp_rgb_set_gdk_color (&color,
                                  shell->canvas->style->bg + GTK_STATE_NORMAL);
        }
      break;

    case GIMP_CANVAS_PADDING_MODE_LIGHT_CHECK:
      gimp_rgb_set_uchar (&color,
                          gimp_render_blend_light_check[0],
                          gimp_render_blend_light_check[1],
                          gimp_render_blend_light_check[2]);
      break;

    case GIMP_CANVAS_PADDING_MODE_DARK_CHECK:
      gimp_rgb_set_uchar (&color,
                          gimp_render_blend_dark_check[0],
                          gimp_render_blend_dark_check[1],
                          gimp_render_blend_dark_check[2]);
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

  SET_COLOR (shell->menubar_manager, "view-padding-color-menu",
             &options->padding_color);

  if (IS_ACTIVE_DISPLAY (shell))
    SET_COLOR (shell->popup_manager, "view-padding-color-menu",
               &options->padding_color);

  gimp_display_shell_expose_full (shell);
}

void
gimp_display_shell_get_padding (GimpDisplayShell      *shell,
                                GimpCanvasPaddingMode *padding_mode,
                                GimpRGB               *padding_color)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = GET_OPTIONS (shell);

  if (padding_mode)
    *padding_mode = options->padding_mode;

  if (padding_color)
    *padding_color = options->padding_color;
}
