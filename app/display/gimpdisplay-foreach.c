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

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"

#include "gimpdisplay.h"
#include "gimpdisplay-foreach.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-cursor.h"


gboolean
gimp_displays_dirty (Gimp *gimp)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  for (list = GIMP_LIST (gimp->displays)->list;
       list;
       list = g_list_next (list))
    {
      if (((GimpDisplay *) list->data)->gimage->dirty)
	return TRUE;
    }

  return FALSE;
}

GimpContainer *
gimp_displays_get_dirty_images (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (gimp_displays_dirty (gimp))
    {
      GimpContainer *container = gimp_list_new_weak (GIMP_TYPE_IMAGE, FALSE);
      GList         *list;

      for (list = GIMP_LIST (gimp->displays)->list;
           list;
           list = g_list_next (list))
        {
          GimpDisplay *display = list->data;
          GimpImage   *image   = display->gimage;

          if (image->dirty &&
              ! gimp_container_have (container, GIMP_OBJECT (image)))
            {
              gimp_container_add (container, GIMP_OBJECT (image));

              g_signal_connect_object (image, "clean",
                                       G_CALLBACK (gimp_container_remove),
                                       container, G_CONNECT_SWAPPED);
            }
        }

      return container;
    }

  return NULL;
}

void
gimp_displays_delete (Gimp *gimp)
{
  GimpDisplay *gdisp;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  /*  this removes the GimpDisplay from the list, so do a while loop
   *  "around" the first element to get them all
   */
  while (GIMP_LIST (gimp->displays)->list)
    {
      gdisp = (GimpDisplay *) GIMP_LIST (gimp->displays)->list->data;

      gimp_display_delete (gdisp);
    }
}

void
gimp_displays_reconnect (Gimp      *gimp,
                         GimpImage *old,
                         GimpImage *new)
{
  GList       *list;
  GimpDisplay *gdisp;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_IMAGE (old));
  g_return_if_fail (GIMP_IS_IMAGE (new));

  for (list = GIMP_LIST (gimp->displays)->list;
       list;
       list = g_list_next (list))
    {
      gdisp = list->data;

      if (gdisp->gimage == old)
	gimp_display_reconnect (gdisp, new);
    }
}

void
gimp_displays_set_busy (Gimp *gimp)
{
  GList            *list;
  GimpDisplayShell *shell;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  for (list = GIMP_LIST (gimp->displays)->list;
       list;
       list = g_list_next (list))
    {
      shell = GIMP_DISPLAY_SHELL (GIMP_DISPLAY (list->data)->shell);

      gimp_display_shell_set_override_cursor (shell, GDK_WATCH);
    }
}

void
gimp_displays_unset_busy (Gimp *gimp)
{
  GList            *list;
  GimpDisplayShell *shell;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  for (list = GIMP_LIST (gimp->displays)->list;
       list;
       list = g_list_next (list))
    {
      shell = GIMP_DISPLAY_SHELL (GIMP_DISPLAY (list->data)->shell);

      gimp_display_shell_unset_override_cursor (shell);
    }
}
