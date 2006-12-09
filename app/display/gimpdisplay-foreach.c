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

#include "display-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
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
      GimpDisplay *display = list->data;

      if (display->image->dirty)
        return TRUE;
    }

  return FALSE;
}

static void
gimp_displays_image_dirty_callback (GimpImage     *image,
                                    GimpDirtyMask  dirty_mask,
                                    GimpContainer *container)
{
  if (image->dirty && image->disp_count > 0 &&
      ! gimp_container_have (container, GIMP_OBJECT (image)))
    gimp_container_add (container, GIMP_OBJECT (image));
}

static void
gimp_displays_dirty_images_disconnect (GimpContainer *dirty_container,
                                       GimpContainer *global_container)
{
  GQuark handler;

  handler = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dirty_container),
                                                "clean-handler"));
  gimp_container_remove_handler (global_container, handler);

  handler = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dirty_container),
                                                "dirty-handler"));
  gimp_container_remove_handler (global_container, handler);
}

static void
gimp_displays_image_clean_callback (GimpImage     *image,
                                    GimpDirtyMask  dirty_mask,
                                    GimpContainer *container)
{
  if (! image->dirty)
    gimp_container_remove (container, GIMP_OBJECT (image));
}

GimpContainer *
gimp_displays_get_dirty_images (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (gimp_displays_dirty (gimp))
    {
      GimpContainer *container = gimp_list_new_weak (GIMP_TYPE_IMAGE, FALSE);
      GList         *list;
      GQuark         handler;

      handler =
        gimp_container_add_handler (gimp->images, "clean",
                                    G_CALLBACK (gimp_displays_image_dirty_callback),
                                    container);
      g_object_set_data (G_OBJECT (container), "clean-handler",
                         GINT_TO_POINTER (handler));

      handler =
        gimp_container_add_handler (gimp->images, "dirty",
                                    G_CALLBACK (gimp_displays_image_dirty_callback),
                                    container);
      g_object_set_data (G_OBJECT (container), "dirty-handler",
                         GINT_TO_POINTER (handler));

      g_signal_connect_object (container, "disconnect",
                               G_CALLBACK (gimp_displays_dirty_images_disconnect),
                               G_OBJECT (gimp->images), 0);

      gimp_container_add_handler (container, "clean",
                                  G_CALLBACK (gimp_displays_image_clean_callback),
                                  container);
      gimp_container_add_handler (container, "dirty",
                                  G_CALLBACK (gimp_displays_image_clean_callback),
                                  container);

      for (list = GIMP_LIST (gimp->images)->list;
           list;
           list = g_list_next (list))
        {
          GimpImage *image = list->data;

          if (image->dirty && image->disp_count > 0)
            gimp_container_add (container, GIMP_OBJECT (image));
        }

      return container;
    }

  return NULL;
}

void
gimp_displays_delete (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  /*  this removes the GimpDisplay from the list, so do a while loop
   *  "around" the first element to get them all
   */
  while (GIMP_LIST (gimp->displays)->list)
    {
      GimpDisplay *display = GIMP_LIST (gimp->displays)->list->data;

      gimp_display_delete (display);
    }
}

void
gimp_displays_reconnect (Gimp      *gimp,
                         GimpImage *old,
                         GimpImage *new)
{
  GList *contexts = NULL;
  GList *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_IMAGE (old));
  g_return_if_fail (GIMP_IS_IMAGE (new));

  /*  remember which contexts refer to old_image  */
  for (list = gimp->context_list; list; list = g_list_next (list))
    {
      GimpContext *context = list->data;

      if (gimp_context_get_image (context) == old)
        contexts = g_list_prepend (contexts, list->data);
    }

  for (list = GIMP_LIST (gimp->displays)->list;
       list;
       list = g_list_next (list))
    {
      GimpDisplay *display = list->data;

      if (display->image == old)
        gimp_display_reconnect (display, new);
    }

  /*  set the new_image on the remembered contexts (in reverse
   *  order, since older contexts are usually the parents of
   *  newer ones)
   */
  g_list_foreach (contexts, (GFunc) gimp_context_set_image, new);
  g_list_free (contexts);
}

void
gimp_displays_set_busy (Gimp *gimp)
{
  GList *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  for (list = GIMP_LIST (gimp->displays)->list;
       list;
       list = g_list_next (list))
    {
      GimpDisplayShell *shell =
        GIMP_DISPLAY_SHELL (GIMP_DISPLAY (list->data)->shell);

      gimp_display_shell_set_override_cursor (shell, GDK_WATCH);
    }
}

void
gimp_displays_unset_busy (Gimp *gimp)
{
  GList *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  for (list = GIMP_LIST (gimp->displays)->list;
       list;
       list = g_list_next (list))
    {
      GimpDisplayShell *shell =
        GIMP_DISPLAY_SHELL (GIMP_DISPLAY (list->data)->shell);

      gimp_display_shell_unset_override_cursor (shell);
    }
}
