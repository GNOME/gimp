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

#include "display-types.h"

#include "core/gimpimage.h"

#include "gimpdisplay.h"
#include "gimpdisplay-foreach.h"
#include "gimpdisplay-scale.h"

#include "nav_window.h"


void
gdisplays_foreach (GFunc    func, 
		   gpointer user_data)
{
  g_slist_foreach (display_list, func, user_data);
}

void
gdisplays_update_title (GimpImage *gimage)
{
  GimpDisplay *gdisp;
  GSList      *list;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = (GimpDisplay *) list->data;

      if (gdisp->gimage == gimage)
	gdisplay_update_title (gdisp);
    }
}

void
gdisplays_resize_cursor_label (GimpImage *gimage)
{
  GimpDisplay *gdisp;
  GSList      *list;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = (GimpDisplay *) list->data;

      if (gdisp->gimage == gimage)
	gdisplay_resize_cursor_label (gdisp);
    }
}

void
gdisplays_setup_scale (GimpImage *gimage)
{
  GimpDisplay *gdisp;
  GSList      *list;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = (GimpDisplay *) list->data;

      if (gdisp->gimage == gimage)
	gimp_display_scale_setup (gdisp);
    }
}

void
gdisplays_update_area (GimpImage *gimage,
		       gint       x,
		       gint       y,
		       gint       w,
		       gint       h)
{
  GimpDisplay *gdisp;
  GSList      *list;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = (GimpDisplay *) list->data;

      if (gdisp->gimage == gimage)
	gdisplay_add_update_area (gdisp, x, y, w, h);
    }
}

void
gdisplays_expose_guides (GimpImage *gimage)
{
  GimpDisplay *gdisp;
  GSList      *list;
  GList       *guide_list;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = (GimpDisplay *) list->data;

      if (gdisp->gimage == gimage)
	{
	  for (guide_list = gdisp->gimage->guides;
	       guide_list;
	       guide_list = g_list_next (guide_list))
	    {
	      gdisplay_expose_guide (gdisp, guide_list->data);
	    }
	}
    }
}

void
gdisplays_expose_guide (GimpImage *gimage,
			GimpGuide *guide)
{
  GimpDisplay *gdisp;
  GSList      *list;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (guide != NULL);

  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = (GimpDisplay *) list->data;

      if (gdisp->gimage == gimage)
	gdisplay_expose_guide (gdisp, guide);
    }
}

void
gdisplays_update_full (GimpImage *gimage)
{
  GimpDisplay *gdisp;
  GSList      *list;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = (GimpDisplay *) list->data;

      if (gdisp->gimage == gimage)
	gdisplay_add_update_area (gdisp, 0, 0,
				  gdisp->gimage->width,
				  gdisp->gimage->height);
    }
}

void
gdisplays_shrink_wrap (GimpImage *gimage)
{
  GimpDisplay *gdisp;
  GSList      *list;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = (GimpDisplay *) list->data;

      if (gdisp->gimage == gimage)
	gimp_display_scale_shrink_wrap (gdisp);
    }
}

void
gdisplays_expose_full (void)
{
  GimpDisplay *gdisp;
  GSList      *list;

  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = (GimpDisplay *) list->data;

      gdisplay_expose_full (gdisp);
    }
}

void
gdisplays_nav_preview_resized (void)
{
  GimpDisplay *gdisp;
  GSList      *list;

  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = (GimpDisplay *) list->data;

      if (gdisp->window_nav_dialog)
	nav_dialog_preview_resized (gdisp->window_nav_dialog);
      
      if (gdisp->nav_popup)
	{
	  nav_dialog_free (NULL, gdisp->nav_popup);
	  gdisp->nav_popup = NULL;
	}
    }
}

void
gdisplays_selection_visibility (GimpImage            *gimage,
				GimpSelectionControl  control)
{
  GimpDisplay *gdisp;
  GSList      *list;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = (GimpDisplay *) list->data;

      if (gdisp->gimage == gimage)
	gdisplay_selection_visibility (gdisp, control);
    }
}

gboolean
gdisplays_dirty (void)
{
  gboolean  dirty = FALSE;
  GSList   *list;

  for (list = display_list; list; list = g_slist_next (list))
    {
      if (((GimpDisplay *) list->data)->gimage->dirty != 0)
	dirty = TRUE;
    }

  return dirty;
}

void
gdisplays_delete (void)
{
  GimpDisplay *gdisp;

  /*  destroying the shell removes the GimpDisplay from the list, so
   *  do a while loop "around" the first element to get them all
   */
  while (display_list)
    {
      gdisp = (GimpDisplay *) display_list->data;

      gtk_widget_destroy (gdisp->shell);
    }
}

GimpDisplay *
gdisplays_check_valid (GimpDisplay *gtest, 
		       GimpImage   *gimage)
{
  /* Give a gdisp check that it is still valid and points to the required
   * GimpImage. If not return the first gDisplay that does point to the 
   * gimage. If none found return NULL;
   */

  GimpDisplay *gdisp;
  GimpDisplay *gdisp_found = NULL;
  GSList      *list;

  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = (GimpDisplay *) list->data;

      if (gdisp == gtest)
	return gtest;

      if (!gdisp_found && gdisp->gimage == gimage)
	gdisp_found = gdisp;
    }

  return gdisp_found;
}

static void
gdisplays_flush_whenever (gboolean now)
{
  static gboolean flushing = FALSE;

  GSList *list;

  /*  this prevents multiple recursive calls to this procedure  */
  if (flushing == TRUE)
    {
      g_warning ("gdisplays_flush() called recursively.");
      return;
    }

  flushing = TRUE;

  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisplay_flush_whenever ((GimpDisplay *) list->data, now);
    }

  flushing = FALSE;
}

void
gdisplays_flush (void)
{
  gdisplays_flush_whenever (FALSE);
}

void
gdisplays_flush_now (void)
{
  gdisplays_flush_whenever (TRUE);
}

void
gdisplays_reconnect (GimpImage *old,
		     GimpImage *new)
{
  GSList      *list;
  GimpDisplay *gdisp;

  g_return_if_fail (old != NULL);
  g_return_if_fail (GIMP_IS_IMAGE (new));
  
  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = list->data;
      
      if (gdisp->gimage == old)
	gdisplay_reconnect (gdisp, new);
    }
}
