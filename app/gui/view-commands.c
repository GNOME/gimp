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

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-scale.h"
#include "display/gimpdisplayshell-selection.h"

#include "widgets/gimpdialogfactory.h"

#include "dialogs.h"
#include "info-dialog.h"
#include "info-window.h"
#include "view-commands.h"

#include "gimprc.h"
#include "nav_window.h"


#define return_if_no_display(gdisp, data) \
  gdisp = gimp_context_get_display (gimp_get_user_context (GIMP (data))); \
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
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  gimp_display_shell_scale_set_dot_for_dot (GIMP_DISPLAY_SHELL (gdisp->shell),
                                            GTK_CHECK_MENU_ITEM (widget)->active);
}

void
view_info_window_cmd_callback (GtkWidget *widget,
			       gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  if (! gimprc.info_window_follows_mouse)
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

  if (gimprc.nav_window_per_display)
    {
      if (! shell->nav_dialog)
	shell->nav_dialog = nav_dialog_create (shell);

      nav_dialog_show (shell->nav_dialog);
    }
  else
    {
      nav_dialog_show_auto (gdisp->gimage->gimp);
    }
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
  gint              new_val;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  new_val = GTK_CHECK_MENU_ITEM (widget)->active;

  /*  hidden == TRUE corresponds to the menu toggle being FALSE  */
  if (new_val == shell->select->hidden)
    {
      gimp_display_shell_selection_toggle (shell->select);

      gimp_display_shell_flush (shell);
    }
}

void
view_toggle_layer_boundary_cmd_callback (GtkWidget *widget,
                                         gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  gint              new_val;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  new_val = GTK_CHECK_MENU_ITEM (widget)->active;

  /*  hidden == TRUE corresponds to the menu toggle being FALSE  */
  if (new_val == shell->select->layer_hidden)
    {
      gimp_display_shell_selection_toggle_layer (shell->select);

      gimp_display_shell_flush (shell);
    }
}

void
view_toggle_rulers_cmd_callback (GtkWidget *widget,
				 gpointer   data)
{
  GimpDisplay      *gdisp;
  GimpDisplayShell *shell;
  return_if_no_display (gdisp, data);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  if (! GTK_CHECK_MENU_ITEM (widget)->active)
    {
      if (GTK_WIDGET_VISIBLE (shell->origin))
	{
	  gtk_widget_hide (shell->origin);
	  gtk_widget_hide (shell->hrule);
	  gtk_widget_hide (shell->vrule);

	  gtk_widget_queue_resize (GTK_WIDGET (shell->origin->parent));
	}
    }
  else
    {
      if (! GTK_WIDGET_VISIBLE (shell->origin))
	{
	  gtk_widget_show (shell->origin);
	  gtk_widget_show (shell->hrule);
	  gtk_widget_show (shell->vrule);

	  gtk_widget_queue_resize (GTK_WIDGET (shell->origin->parent));
	}
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

  if (! GTK_CHECK_MENU_ITEM (widget)->active)
    {
      if (GTK_WIDGET_VISIBLE (shell->statusbar))
	gtk_widget_hide (shell->statusbar);
    }
  else
    {
      if (! GTK_WIDGET_VISIBLE (shell->statusbar))
	gtk_widget_show (shell->statusbar);
    }
}

void
view_toggle_guides_cmd_callback (GtkWidget *widget,
				 gpointer   data)
{
  GimpDisplay *gdisp;
  gboolean     old_val;
  return_if_no_display (gdisp, data);

  old_val = gdisp->draw_guides;
  gdisp->draw_guides = GTK_CHECK_MENU_ITEM (widget)->active;

  if ((old_val != gdisp->draw_guides) && gdisp->gimage->guides)
    {
      gimp_display_shell_expose_full (GIMP_DISPLAY_SHELL (gdisp->shell));
      gimp_display_flush (gdisp);
    }
}

void
view_snap_to_guides_cmd_callback (GtkWidget *widget,
				  gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  gdisp->snap_to_guides = GTK_CHECK_MENU_ITEM (widget)->active;
}

void
view_new_view_cmd_callback (GtkWidget *widget,
			    gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  gimp_create_display (gdisp->gimage->gimp, gdisp->gimage, gdisp->scale);
}

void
view_shrink_wrap_cmd_callback (GtkWidget *widget,
			       gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  gimp_display_shell_scale_shrink_wrap (GIMP_DISPLAY_SHELL (gdisp->shell));
}
