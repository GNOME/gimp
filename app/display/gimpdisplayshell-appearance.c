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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimpitemfactory.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpcanvas.h"
#include "gimpdisplay.h"
#include "gimpdisplayoptions.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-callbacks.h"
#include "gimpdisplayshell-selection.h"
#include "gimpdisplayshell-render.h"


#define GET_OPTIONS(shell) \
  (gimp_display_shell_get_fullscreen (shell) ? \
   shell->fullscreen_options : shell->options)

#define IS_ACTIVE_DISPLAY(shell) \
  ((shell)->gdisp == \
   gimp_context_get_display (gimp_get_user_context \
                             ((shell)->gdisp->gimage->gimp)))


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
  GtkWidget          *menubar;
  GtkContainer       *vbox;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = GET_OPTIONS (shell);

  g_object_set (options, "show-menubar", show, NULL);

  menubar = GTK_ITEM_FACTORY (shell->menubar_factory)->widget;
  vbox    = GTK_CONTAINER (shell->qmask->parent->parent);

  if (show)
    gtk_widget_show (menubar);
  else
    gtk_widget_hide (menubar);

  if (options->show_menubar || options->show_statusbar)
    gtk_container_set_border_width (vbox, 2);
  else
    gtk_container_set_border_width (vbox, 0);

  gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                "/View/Show Menubar", show);

  if (IS_ACTIVE_DISPLAY (shell))
    gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                  "/View/Show Menubar", show);
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

  gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                "/View/Show Rulers", show);

  if (IS_ACTIVE_DISPLAY (shell))
    gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                  "/View/Show Rulers", show);
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
      gtk_widget_show (shell->qmask);
      gtk_widget_show (shell->padding_button);

      gtk_box_set_spacing (hbox, 1);
      gtk_box_set_spacing (vbox, 1);
    }
  else
    {
      gtk_widget_hide (shell->nav_ebox);
      gtk_widget_hide (shell->hsb);
      gtk_widget_hide (shell->vsb);
      gtk_widget_hide (shell->qmask);
      gtk_widget_hide (shell->padding_button);

      gtk_box_set_spacing (hbox, 0);
      gtk_box_set_spacing (vbox, 0);
    }

  gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                "/View/Show Scrollbars", show);

  if (IS_ACTIVE_DISPLAY (shell))
    gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                  "/View/Show Scrollbars", show);
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
  GtkContainer       *vbox;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = GET_OPTIONS (shell);

  g_object_set (options, "show-statusbar", show, NULL);

  vbox = GTK_CONTAINER (shell->qmask->parent->parent);

  if (show)
    gtk_widget_show (shell->statusbar);
  else
    gtk_widget_hide (shell->statusbar);

  if (options->show_menubar || options->show_statusbar)
    gtk_container_set_border_width (vbox, 2);
  else
    gtk_container_set_border_width (vbox, 0);

  gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                "/View/Show Statusbar", show);

  if (IS_ACTIVE_DISPLAY (shell))
    gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                  "/View/Show Statusbar", show);
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

  if (shell->select)
    gimp_display_shell_selection_set_hidden (shell->select, ! show);

  gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                "/View/Show Selection", show);

  if (IS_ACTIVE_DISPLAY (shell))
    gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                  "/View/Show Selection", show);
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

  if (shell->select)
    gimp_display_shell_selection_layer_set_hidden (shell->select, ! show);

  gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                "/View/Show Layer Boundary", show);

  if (IS_ACTIVE_DISPLAY (shell))
    gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                  "/View/Show Layer Boundary", show);
}

gboolean
gimp_display_shell_get_show_layer (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_OPTIONS (shell)->show_layer_boundary;
}

void
gimp_display_shell_set_show_grid (GimpDisplayShell *shell,
                                  gboolean          show)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = GET_OPTIONS (shell);

  g_object_set (options, "show-grid", show, NULL);

  if (shell->gdisp->gimage->grid)
    gimp_display_shell_expose_full (shell);

  gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                "/View/Show Grid", show);

  if (IS_ACTIVE_DISPLAY (shell))
    gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                  "/View/Show Grid", show);
}

gboolean
gimp_display_shell_get_show_grid (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_OPTIONS (shell)->show_grid;
}

void
gimp_display_shell_set_snap_to_grid (GimpDisplayShell *shell,
                                     gboolean          snap)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (snap != shell->snap_to_grid)
    {
      shell->snap_to_grid = snap ? TRUE : FALSE;

      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                    "/View/Snap to Grid", snap);

      if (IS_ACTIVE_DISPLAY (shell))
        gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                      "/View/Snap to Grid", snap);
    }
}

gboolean
gimp_display_shell_get_snap_to_grid (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return shell->snap_to_grid;
}

void
gimp_display_shell_set_show_guides (GimpDisplayShell *shell,
                                    gboolean          show)
{
  GimpDisplayOptions *options;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  options = GET_OPTIONS (shell);

  g_object_set (options, "show-guides", show, NULL);

  if (shell->gdisp->gimage->guides)
    gimp_display_shell_expose_full (shell);

  gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                "/View/Show Guides", show);

  if (IS_ACTIVE_DISPLAY (shell))
    gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                  "/View/Show Guides", show);
}

gboolean
gimp_display_shell_get_show_guides (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_OPTIONS (shell)->show_guides;
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
                          render_blend_light_check[0],
                          render_blend_light_check[1],
                          render_blend_light_check[2]);
      break;

    case GIMP_CANVAS_PADDING_MODE_DARK_CHECK:
      gimp_rgb_set_uchar (&color,
                          render_blend_dark_check[0],
                          render_blend_dark_check[1],
                          render_blend_dark_check[2]);
      break;

    case GIMP_CANVAS_PADDING_MODE_CUSTOM:
      break;
    }

  g_object_set (options,
                "padding-mode",  padding_mode,
                "padding-color", &color,
                NULL);

  gimp_canvas_set_bg_color (GIMP_CANVAS (shell->canvas), &color);

  if (shell->padding_button)
    {
      g_signal_handlers_block_by_func (shell->padding_button,
                                       gimp_display_shell_color_button_changed,
                                       shell);

      gimp_color_button_set_color (GIMP_COLOR_BUTTON (shell->padding_button),
                                   &color);

      g_signal_handlers_unblock_by_func (shell->padding_button,
                                         gimp_display_shell_color_button_changed,
                                         shell);
    }

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
