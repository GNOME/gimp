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

#include "core/core-types.h"

#include "core/gimpimage.h"

#include "info-dialog.h"
#include "info-window.h"
#include "view-commands.h"

#include "gdisplay.h"
#include "gdisplay_ops.h"
#include "gimprc.h"
#include "nav_window.h"
#include "scale.h"
#include "selection.h"


#define return_if_no_display(gdisp) \
        gdisp = gdisplay_active (); \
        if (!gdisp) return


void
view_zoomin_cmd_callback (GtkWidget *widget,
			  gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  change_scale (gdisp, GIMP_ZOOM_IN);
}

void
view_zoomout_cmd_callback (GtkWidget *widget,
			   gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  change_scale (gdisp, GIMP_ZOOM_OUT);
}

void
view_zoom_cmd_callback (GtkWidget *widget,
			gpointer   data,
			guint      action)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  change_scale (gdisp, action);
}

void
view_dot_for_dot_cmd_callback (GtkWidget *widget,
			       gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  gdisplay_set_dot_for_dot (gdisp, GTK_CHECK_MENU_ITEM (widget)->active);
}

void
view_info_window_cmd_callback (GtkWidget *widget,
			       gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  if (! gimprc.info_window_follows_mouse)
    {
      if (! gdisp->window_info_dialog)
	gdisp->window_info_dialog = info_window_create (gdisp);

      info_window_update (gdisp);
      info_dialog_popup (gdisp->window_info_dialog);
    }
  else
    {
      info_window_follow_auto ();
    }
}

void
view_nav_window_cmd_callback (GtkWidget *widget,
			      gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  if (gimprc.nav_window_per_display)
    {
      if (! gdisp->window_nav_dialog)
	gdisp->window_nav_dialog = nav_dialog_create (gdisp);

      nav_dialog_popup (gdisp->window_nav_dialog);
    }
  else
    {
      nav_dialog_follow_auto ();
    }
}

void
view_toggle_selection_cmd_callback (GtkWidget *widget,
				    gpointer   data)
{
  GDisplay *gdisp;
  gint      new_val;

  return_if_no_display (gdisp);

  new_val = GTK_CHECK_MENU_ITEM (widget)->active;

  /*  hidden == TRUE corresponds to the menu toggle being FALSE  */
  if (new_val == gdisp->select->hidden)
    {
      selection_toggle (gdisp->select);

      gdisplays_flush ();
    }
}

void
view_toggle_rulers_cmd_callback (GtkWidget *widget,
				 gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  if (! GTK_CHECK_MENU_ITEM (widget)->active)
    {
      if (GTK_WIDGET_VISIBLE (gdisp->origin))
	{
	  gtk_widget_hide (gdisp->origin);
	  gtk_widget_hide (gdisp->hrule);
	  gtk_widget_hide (gdisp->vrule);

	  gtk_widget_queue_resize (GTK_WIDGET (gdisp->origin->parent));
	}
    }
  else
    {
      if (! GTK_WIDGET_VISIBLE (gdisp->origin))
	{
	  gtk_widget_show (gdisp->origin);
	  gtk_widget_show (gdisp->hrule);
	  gtk_widget_show (gdisp->vrule);

	  gtk_widget_queue_resize (GTK_WIDGET (gdisp->origin->parent));
	}
    }
}

void
view_toggle_statusbar_cmd_callback (GtkWidget *widget,
				    gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  if (! GTK_CHECK_MENU_ITEM (widget)->active)
    {
      if (GTK_WIDGET_VISIBLE (gdisp->statusarea))
	gtk_widget_hide (gdisp->statusarea);
    }
  else
    {
      if (! GTK_WIDGET_VISIBLE (gdisp->statusarea))
	gtk_widget_show (gdisp->statusarea);
    }
}

void
view_toggle_guides_cmd_callback (GtkWidget *widget,
				 gpointer   data)
{
  GDisplay *gdisp;
  gint      old_val;

  return_if_no_display (gdisp);

  old_val = gdisp->draw_guides;
  gdisp->draw_guides = GTK_CHECK_MENU_ITEM (widget)->active;

  if ((old_val != gdisp->draw_guides) && gdisp->gimage->guides)
    {
      gdisplay_expose_full (gdisp);
      gdisplays_flush ();
    }
}

void
view_snap_to_guides_cmd_callback (GtkWidget *widget,
				  gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  gdisp->snap_to_guides = GTK_CHECK_MENU_ITEM (widget)->active;
}

void
view_new_view_cmd_callback (GtkWidget *widget,
			    gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  gdisplay_new_view (gdisp);
}

void
view_shrink_wrap_cmd_callback (GtkWidget *widget,
			       gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  shrink_wrap_display (gdisp);
}
