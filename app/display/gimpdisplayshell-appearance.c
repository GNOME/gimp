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

#include "core/gimpimage.h"

#include "widgets/gimpitemfactory.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-callbacks.h"
#include "gimpdisplayshell-selection.h"
#include "gimpdisplayshell-render.h"


#define GET_APPEARANCE(shell) \
  (gimp_display_shell_get_fullscreen (shell) ? \
   &shell->fullscreen_appearance : &shell->appearance)


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
gimp_display_shell_set_show_selection (GimpDisplayShell *shell,
                                       gboolean          show)
{
  GimpDisplayShellAppearance *appearance;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  appearance = GET_APPEARANCE (shell);

  if (show != appearance->selection)
    {
      appearance->selection = show ? TRUE : FALSE;

      if (shell->select)
        gimp_display_shell_selection_set_hidden (shell->select, ! show);

      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                    "/View/Show Selection", show);
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                    "/View/Show Selection", show);
    }
}

gboolean
gimp_display_shell_get_show_selection (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_APPEARANCE (shell)->selection;
}

void
gimp_display_shell_set_show_layer (GimpDisplayShell *shell,
                                   gboolean          show)
{
  GimpDisplayShellAppearance *appearance;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  appearance = GET_APPEARANCE (shell);

  if (show != appearance->active_layer)
    {
      appearance->active_layer = show ? TRUE : FALSE;

      if (shell->select)
        gimp_display_shell_selection_layer_set_hidden (shell->select, ! show);

      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                    "/View/Show Layer Boundary", show);
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                    "/View/Show Layer Boundary", show);
    }
}

gboolean
gimp_display_shell_get_show_layer (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_APPEARANCE (shell)->active_layer;
}

void
gimp_display_shell_set_show_grid (GimpDisplayShell *shell,
                                  gboolean          show)
{
  GimpDisplayShellAppearance *appearance;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  appearance = GET_APPEARANCE (shell);

  if (show != appearance->grid)
    {
      appearance->grid = show ? TRUE : FALSE;

      if (shell->gdisp->gimage->grid)
        gimp_display_shell_expose_full (shell);

      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                    "/View/Show Grid", show);
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                    "/View/Show Grid", show);
    }
}

gboolean
gimp_display_shell_get_show_grid (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_APPEARANCE (shell)->grid;
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
  GimpDisplayShellAppearance *appearance;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  appearance = GET_APPEARANCE (shell);

  if (show != appearance->guides)
    {
      appearance->guides = show ? TRUE : FALSE;

      if (shell->gdisp->gimage->guides)
        gimp_display_shell_expose_full (shell);

      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                    "/View/Show Guides", show);
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                    "/View/Show Guides", show);
    }
}

gboolean
gimp_display_shell_get_show_guides (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_APPEARANCE (shell)->guides;
}

void
gimp_display_shell_set_show_menubar (GimpDisplayShell *shell,
                                     gboolean          show)
{
  GimpDisplayShellAppearance *appearance;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  appearance = GET_APPEARANCE (shell);

  if (show != appearance->menubar)
    {
      GtkWidget    *menubar;
      GtkContainer *vbox;

      menubar = GTK_ITEM_FACTORY (shell->menubar_factory)->widget;
      vbox    = GTK_CONTAINER (shell->qmask->parent->parent);

      if (show)
	gtk_widget_show (menubar);
      else
	gtk_widget_hide (menubar);

      appearance->menubar = show ? TRUE : FALSE;

      if (appearance->menubar || appearance->statusbar)
        gtk_container_set_border_width (vbox, 2);
      else
        gtk_container_set_border_width (vbox, 0);

      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                    "/View/Show Menubar", show);
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                    "/View/Show Menubar", show);
    }
}

gboolean
gimp_display_shell_get_show_menubar (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_APPEARANCE (shell)->menubar;
}

void
gimp_display_shell_set_show_rulers (GimpDisplayShell *shell,
                                    gboolean          show)
{
  GimpDisplayShellAppearance *appearance;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  appearance = GET_APPEARANCE (shell);

  if (show != appearance->rulers)
    {
      GtkTable *table;

      table = GTK_TABLE (shell->canvas->parent);

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

      appearance->rulers = show ? TRUE : FALSE;

      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                    "/View/Show Rulers", show);
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                    "/View/Show Rulers", show);
    }
}

gboolean
gimp_display_shell_get_show_rulers (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_APPEARANCE (shell)->rulers;
}

void
gimp_display_shell_set_show_scrollbars (GimpDisplayShell *shell,
                                        gboolean          show)
{
  GimpDisplayShellAppearance *appearance;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  appearance = GET_APPEARANCE (shell);

  if (show != appearance->scrollbars)
    {
      GtkBox *hbox;
      GtkBox *vbox;

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

      appearance->scrollbars = show ? TRUE : FALSE;

      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                    "/View/Show Scrollbars", show);
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                    "/View/Show Scrollbars", show);
    }
}

gboolean
gimp_display_shell_get_show_scrollbars (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_APPEARANCE (shell)->scrollbars;
}

void
gimp_display_shell_set_show_statusbar (GimpDisplayShell *shell,
                                       gboolean          show)
{
  GimpDisplayShellAppearance *appearance;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  appearance = GET_APPEARANCE (shell);

  if (show != appearance->statusbar)
    {
      GtkContainer *vbox;

      vbox = GTK_CONTAINER (shell->qmask->parent->parent);

      if (show)
	gtk_widget_show (shell->statusbar);
      else
	gtk_widget_hide (shell->statusbar);

      appearance->statusbar = show ? TRUE : FALSE;

      if (appearance->menubar || appearance->statusbar)
        gtk_container_set_border_width (vbox, 2);
      else
        gtk_container_set_border_width (vbox, 0);

      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                    "/View/Show Statusbar", show);
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                    "/View/Show Statusbar", show);
    }
}

gboolean
gimp_display_shell_get_show_statusbar (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return GET_APPEARANCE (shell)->statusbar;
}

void
gimp_display_shell_set_padding (GimpDisplayShell       *shell,
                                GimpDisplayPaddingMode  padding_mode,
                                const GimpRGB          *padding_color)
{
  GimpDisplayShellAppearance *appearance;
  guchar                      r, g, b;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (padding_color != NULL);

  appearance = GET_APPEARANCE (shell);

  appearance->padding_mode = padding_mode;

  switch (appearance->padding_mode)
    {
    case GIMP_DISPLAY_PADDING_MODE_DEFAULT:
      if (shell->canvas)
        {
          gtk_widget_ensure_style (shell->canvas);

          r = shell->canvas->style->bg[GTK_STATE_NORMAL].red   >> 8;
          g = shell->canvas->style->bg[GTK_STATE_NORMAL].green >> 8;
          b = shell->canvas->style->bg[GTK_STATE_NORMAL].blue  >> 8;

          gimp_rgb_set_uchar (&appearance->padding_color, r, g, b);
        }
      break;

    case GIMP_DISPLAY_PADDING_MODE_LIGHT_CHECK:
      gimp_rgb_set_uchar (&appearance->padding_color,
                          render_blend_light_check[0],
                          render_blend_light_check[1],
                          render_blend_light_check[2]);
      break;

    case GIMP_DISPLAY_PADDING_MODE_DARK_CHECK:
      gimp_rgb_set_uchar (&appearance->padding_color,
                          render_blend_dark_check[0],
                          render_blend_dark_check[1],
                          render_blend_dark_check[2]);
      break;

    case GIMP_DISPLAY_PADDING_MODE_CUSTOM:
      appearance->padding_color = *padding_color;
      break;
    }

  if (GTK_WIDGET_REALIZED (shell->canvas))
    {
      GdkColormap *colormap;
      GdkColor     color;

      gimp_rgb_get_gdk_color (&appearance->padding_color, &color);

      colormap = gdk_drawable_get_colormap (shell->canvas->window);
      g_return_if_fail (colormap != NULL);
      gdk_colormap_alloc_color (colormap, &color, FALSE, TRUE);

      gdk_window_set_background (shell->canvas->window, &color);
    }

  if (shell->padding_button)
    {
      g_signal_handlers_block_by_func (shell->padding_button,
                                       gimp_display_shell_color_button_changed,
                                       shell);

      gimp_color_button_set_color (GIMP_COLOR_BUTTON (shell->padding_button),
                                   &appearance->padding_color);

      g_signal_handlers_unblock_by_func (shell->padding_button,
                                         gimp_display_shell_color_button_changed,
                                         shell);
    }

  gimp_display_shell_expose_full (shell);
}

void
gimp_display_shell_get_padding (GimpDisplayShell       *shell,
                                GimpDisplayPaddingMode *padding_mode,
                                GimpRGB                *padding_color)
{
  GimpDisplayShellAppearance *appearance;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  appearance = GET_APPEARANCE (shell);

  if (padding_mode)
    *padding_mode = appearance->padding_mode;

  if (padding_color)
    *padding_color = appearance->padding_color;
}
