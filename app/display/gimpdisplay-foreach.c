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
#include "gimpdisplayshell.h"

#include "nav_window.h"


void
gdisplays_foreach (GFunc    func, 
		   gpointer user_data)
{
  g_slist_foreach (display_list, func, user_data);
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
	gimp_display_shell_expose_guide (GIMP_DISPLAY_SHELL (gdisp->shell),
                                         guide);
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
	gimp_display_shell_scale_shrink_wrap (GIMP_DISPLAY_SHELL (gdisp->shell));
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

      gimp_display_shell_expose_full (GIMP_DISPLAY_SHELL (gdisp->shell));
    }
}

void
gdisplays_nav_preview_resized (void)
{
  GimpDisplayShell *shell;
  GSList           *list;

  for (list = display_list; list; list = g_slist_next (list))
    {
      shell = GIMP_DISPLAY_SHELL (GIMP_DISPLAY (list->data)->shell);

      if (shell->nav_dialog)
	nav_dialog_preview_resized (shell->nav_dialog);
      
      if (shell->nav_popup)
	{
	  nav_dialog_free (NULL, shell->nav_popup);
	  shell->nav_popup = NULL;
	}
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

void
gdisplays_flush (void)
{
  static gboolean flushing = FALSE;

  GSList      *list;
  GimpDisplay *gdisp;

  /*  this prevents multiple recursive calls to this procedure  */
  if (flushing == TRUE)
    {
      g_warning ("gdisplays_flush() called recursively.");
      return;
    }

  flushing = TRUE;

  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = list->data;

      gdisplay_flush (gdisp);
    }

  flushing = FALSE;
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

void
gdisplays_set_busy (void)
{
  GSList           *list;
  GimpDisplayShell *shell;

  for (list = display_list; list; list = g_slist_next (list))
    {
      shell = GIMP_DISPLAY_SHELL (GIMP_DISPLAY (list->data)->shell);

      gimp_display_shell_install_override_cursor (shell, GDK_WATCH);
    }
}

void
gdisplays_unset_busy (void)
{
  GSList           *list;
  GimpDisplayShell *shell;

  for (list = display_list; list; list = g_slist_next (list))
    {
      shell = GIMP_DISPLAY_SHELL (GIMP_DISPLAY (list->data)->shell);
      
      gimp_display_shell_remove_override_cursor (shell);
    }
}
