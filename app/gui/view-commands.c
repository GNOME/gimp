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
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpdisplayshell-scale.h"

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
view_zoom_out_cmd_callback (GtkWidget *widget,
                            gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  gimp_display_shell_scale (GIMP_DISPLAY_SHELL (gdisp->shell), GIMP_ZOOM_OUT);
}

void
view_zoom_in_cmd_callback (GtkWidget *widget,
                           gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  gimp_display_shell_scale (GIMP_DISPLAY_SHELL (gdisp->shell), GIMP_ZOOM_IN);
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
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  guchar            scalesrc;
  guchar            scaledest;
  return_if_no_display (gdisp, data);

  if (! GTK_CHECK_MENU_ITEM (widget)->active)
    return;

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  scalesrc  = CLAMP (action % 100, 1, 0xFF);
  scaledest = CLAMP (action / 100, 1, 0xFF);

  if (scalesrc != SCALESRC (shell) || scaledest != SCALEDEST (shell))
    gimp_display_shell_scale (shell, action);
}

void
view_zoom_other_cmd_callback (GtkWidget *widget,
                              gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  if (! GTK_CHECK_MENU_ITEM (widget)->active)
    return;

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  /*  check if we are activated by the user or from image_menu_set_zoom()  */
  if (shell->scale != shell->other_scale)
    gimp_display_shell_scale_dialog (shell);
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

  gimp_display_shell_set_fullscreen (shell,
                                     GTK_CHECK_MENU_ITEM (widget)->active);

  fullscreen = (shell->window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0;

  if (fullscreen != GTK_CHECK_MENU_ITEM (widget)->active)
    {
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                    "/View/Fullscreen",
                                    fullscreen);
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                    "/View/Fullscreen",
                                    fullscreen);
    }
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
                                    gtk_widget_get_screen (widget),
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
                                  gtk_widget_get_screen (widget),
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

  gimp_display_shell_set_show_selection (shell,
                                         GTK_CHECK_MENU_ITEM (widget)->active);
}

void
view_toggle_layer_boundary_cmd_callback (GtkWidget *widget,
                                         gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_display_shell_set_show_layer (shell,
                                     GTK_CHECK_MENU_ITEM (widget)->active);
}

void
view_toggle_menubar_cmd_callback (GtkWidget *widget,
                                  gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_display_shell_set_show_menubar (shell,
                                       GTK_CHECK_MENU_ITEM (widget)->active);
}

void
view_toggle_rulers_cmd_callback (GtkWidget *widget,
				 gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_display_shell_set_show_rulers (shell,
                                      GTK_CHECK_MENU_ITEM (widget)->active);
}

void
view_toggle_scrollbars_cmd_callback (GtkWidget *widget,
                                     gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_display_shell_set_show_scrollbars (shell,
                                          GTK_CHECK_MENU_ITEM (widget)->active);
}

void
view_toggle_statusbar_cmd_callback (GtkWidget *widget,
				    gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_display_shell_set_show_statusbar (shell,
                                         GTK_CHECK_MENU_ITEM (widget)->active);
}

void
view_toggle_guides_cmd_callback (GtkWidget *widget,
				 gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_display_shell_set_show_guides (shell,
                                      GTK_CHECK_MENU_ITEM (widget)->active);
}

void
view_snap_to_guides_cmd_callback (GtkWidget *widget,
				  gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  if (shell->snap_to_guides != GTK_CHECK_MENU_ITEM (widget)->active)
    {
      shell->snap_to_guides = GTK_CHECK_MENU_ITEM (widget)->active;

      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                    "/View/Snap to Guides",
                                    shell->snap_to_guides);
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                    "/View/Snap to Guides",
                                    shell->snap_to_guides);
    }
}


void
view_toggle_grid_cmd_callback (GtkWidget *widget,
                               gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gimp_display_shell_set_show_grid (shell,
                                    GTK_CHECK_MENU_ITEM (widget)->active);

}

void
view_snap_to_grid_cmd_callback (GtkWidget *widget,
                                gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  if (shell->snap_to_grid != GTK_CHECK_MENU_ITEM (widget)->active)
    {
      shell->snap_to_grid = GTK_CHECK_MENU_ITEM (widget)->active;

      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
                                    "/View/Snap to Grid",
                                    shell->snap_to_grid);
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
                                    "/View/Snap to Grid",
                                    shell->snap_to_grid);
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
