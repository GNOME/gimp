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

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-scale.h"
#include "display/gimpdisplayshell-selection.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpitemfactory.h"

#include "dialogs.h"
#include "info-dialog.h"
#include "info-window.h"
#include "view-commands.h"


#define return_if_no_display(gdisp, data) \
  if (GIMP_IS_DISPLAY (data)) \
    gdisp = data; \
  else if (GIMP_IS_GIMP (data)) \
    gdisp = gimp_context_get_display (gimp_get_user_context (GIMP (data))); \
  else \
    gdisp = NULL; \
  if (! gdisp) \
    return


void
view_zoom_in_cmd_callback (GtkWidget *widget,
                           gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  gimp_display_shell_scale (GIMP_DISPLAY_SHELL (gdisp->shell), GIMP_ZOOM_IN);
}

void
view_zoom_out_cmd_callback (GtkWidget *widget,
                            gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  gimp_display_shell_scale (GIMP_DISPLAY_SHELL (gdisp->shell), GIMP_ZOOM_OUT);
}

void
view_zoom_fit_cmd_callback (GtkWidget *widget,
                            gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  gimp_display_shell_scale_fit (GIMP_DISPLAY_SHELL (gdisp->shell));
}

void
view_zoom_cmd_callback (GtkWidget *widget,
			gpointer   data,
			guint      action)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  gimp_display_shell_scale (GIMP_DISPLAY_SHELL (gdisp->shell), action);
}

void
view_dot_for_dot_cmd_callback (GtkWidget *widget,
			       gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  if (shell->dot_for_dot != GTK_CHECK_MENU_ITEM (widget)->active)
    {
      gimp_display_shell_scale_set_dot_for_dot (shell,
                                                GTK_CHECK_MENU_ITEM (widget)->active);

      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                    "/View/Dot for Dot",
                                    shell->dot_for_dot);
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                    "/View/Dot for Dot",
                                    shell->dot_for_dot);
    }
}

void
view_fullscreen_cmd_callback (GtkWidget *widget,
			      gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  gboolean          fullscreen;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_display_shell_toggle_fullscreen (shell);

  fullscreen = shell->window_state & GDK_WINDOW_STATE_FULLSCREEN;

  gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
				"/View/Fullscreen",
				fullscreen);
  gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
				"/View/Fullscreen",
				fullscreen);
}

void
view_info_window_cmd_callback (GtkWidget *widget,
			       gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  if (GIMP_GUI_CONFIG (gdisp->gimage->gimp->config)->info_window_per_display)
    {
      if (! shell->info_dialog)
	shell->info_dialog = info_window_create (gdisp);

      info_dialog_popup (shell->info_dialog);
    }
  else
    {
      info_window_follow_auto (gdisp->gimage->gimp);
    }
}

void
view_navigation_window_cmd_callback (GtkWidget *widget,
                                     gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_dialog_factory_dialog_raise (global_dock_factory,
                                    "gimp-navigation-view", -1);
}

void
view_display_filters_cmd_callback (GtkWidget *widget,
                                   gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_dialog_factory_dialog_new (global_dialog_factory,
                                  "gimp-display-filters-dialog", -1);
}

void
view_toggle_selection_cmd_callback (GtkWidget *widget,
				    gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  /*  hidden == TRUE corresponds to the menu toggle being FALSE  */
  if (GTK_CHECK_MENU_ITEM (widget)->active == shell->select->hidden)
    {
      gimp_display_shell_selection_toggle (shell->select);

      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                    "/View/Show Selection",
                                    ! shell->select->hidden);
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                    "/View/Show Selection",
                                    ! shell->select->hidden);
    }
}

void
view_toggle_layer_boundary_cmd_callback (GtkWidget *widget,
                                         gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  /*  hidden == TRUE corresponds to the menu toggle being FALSE  */
  if (GTK_CHECK_MENU_ITEM (widget)->active == shell->select->layer_hidden)
    {
      gimp_display_shell_selection_toggle_layer (shell->select);

      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                    "/View/Show Layer Boundary",
                                    ! shell->select->layer_hidden);
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                    "/View/Show Layer Boundary",
                                    ! shell->select->layer_hidden);
    }
}

void
view_toggle_menubar_cmd_callback (GtkWidget *widget,
                                  gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  GtkWidget        *menubar;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  menubar = GTK_ITEM_FACTORY (shell->menubar_factory)->widget;

  if (GTK_CHECK_MENU_ITEM (widget)->active !=
      GTK_WIDGET_VISIBLE (menubar))
    {
      if (GTK_WIDGET_VISIBLE (menubar))
	gtk_widget_hide (menubar);
      else
	gtk_widget_show (menubar);

      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                    "/View/Show Menubar",
                                    GTK_WIDGET_VISIBLE (menubar));
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                    "/View/Show Menubar",
                                    GTK_WIDGET_VISIBLE (menubar));
    }
}

void
view_toggle_rulers_cmd_callback (GtkWidget *widget,
				 gpointer   data)
{
  GimpDisplay       *gdisp;
  GimpDisplayShell  *shell;
  GimpDisplayConfig *config;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  config = GIMP_DISPLAY_CONFIG (gdisp->gimage->gimp->config);

  if (GTK_CHECK_MENU_ITEM (widget)->active !=
      GTK_WIDGET_VISIBLE (shell->origin))
    {
      if (GTK_WIDGET_VISIBLE (shell->origin))
	{
          gtk_widget_hide (shell->origin);
	  gtk_widget_hide (shell->hrule);
	  gtk_widget_hide (shell->vrule);
	}
      else
	{
          gtk_widget_show (shell->origin);
	  gtk_widget_show (shell->hrule);
	  gtk_widget_show (shell->vrule);
	}

      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                    "/View/Show Rulers",
                                    GTK_WIDGET_VISIBLE (shell->origin));
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                    "/View/Show Rulers",
                                    GTK_WIDGET_VISIBLE (shell->origin));
    }
}

void
view_toggle_statusbar_cmd_callback (GtkWidget *widget,
				    gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  if (GTK_CHECK_MENU_ITEM (widget)->active !=
      GTK_WIDGET_VISIBLE (shell->statusbar))
    {
      if (GTK_WIDGET_VISIBLE (shell->statusbar))
	gtk_widget_hide (shell->statusbar);
      else
	gtk_widget_show (shell->statusbar);

      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                    "/View/Show Statusbar",
                                    GTK_WIDGET_VISIBLE (shell->statusbar));
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                    "/View/Show Statusbar",
                                    GTK_WIDGET_VISIBLE (shell->statusbar));
    }
}

void
view_toggle_guides_cmd_callback (GtkWidget *widget,
				 gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  if (GTK_CHECK_MENU_ITEM (widget)->active != gdisp->draw_guides)
    {
      GimpDisplayShell *shell;

      shell = GIMP_DISPLAY_SHELL (gdisp->shell);

      gdisp->draw_guides = GTK_CHECK_MENU_ITEM (widget)->active;

      if (gdisp->gimage->guides)
        {
          gimp_display_shell_expose_full (shell);
          gimp_display_flush (gdisp);
        }
      else
        {
          gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                        "/View/Show Guides",
                                        gdisp->draw_guides);
          gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                        "/View/Show Guides",
                                        gdisp->draw_guides);
        }
    }
}

void
view_snap_to_guides_cmd_callback (GtkWidget *widget,
				  gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  if (gdisp->snap_to_guides != GTK_CHECK_MENU_ITEM (widget)->active)
    {
      gdisp->snap_to_guides = GTK_CHECK_MENU_ITEM (widget)->active;

      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                    "/View/Snap to Guides",
                                    gdisp->snap_to_guides);
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                    "/View/Snap to Guides",
                                    gdisp->snap_to_guides);
    }
}

void
view_new_view_cmd_callback (GtkWidget *widget,
			    gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  gimp_create_display (gdisp->gimage->gimp, gdisp->gimage,
                       GIMP_DISPLAY_SHELL (gdisp->shell)->scale);
}

void
view_shrink_wrap_cmd_callback (GtkWidget *widget,
			       gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  gimp_display_shell_scale_shrink_wrap (GIMP_DISPLAY_SHELL (gdisp->shell));
}
