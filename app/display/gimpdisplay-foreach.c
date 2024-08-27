/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "display-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"

#include "gimpdisplay.h"
#include "gimpdisplay-foreach.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-cursor.h"
#include "gimpimagewindow.h"


gboolean
gimp_displays_dirty (Gimp *gimp)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  for (list = gimp_get_display_iter (gimp);
       list;
       list = g_list_next (list))
    {
      GimpDisplay *display = list->data;
      GimpImage   *image   = gimp_display_get_image (display);

      if (image && gimp_image_is_dirty (image))
        return TRUE;
    }

  return FALSE;
}

static void
gimp_displays_image_dirty_callback (GimpImage     *image,
                                    GimpDirtyMask  dirty_mask,
                                    GimpContainer *container)
{
  if (gimp_image_is_dirty (image)              &&
      gimp_image_get_display_count (image) > 0 &&
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
  if (! gimp_image_is_dirty (image))
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

      for (list = gimp_get_image_iter (gimp);
           list;
           list = g_list_next (list))
        {
          GimpImage *image = list->data;

          if (gimp_image_is_dirty (image) &&
              gimp_image_get_display_count (image) > 0)
            gimp_container_add (container, GIMP_OBJECT (image));
        }

      return container;
    }

  return NULL;
}

/**
 * gimp_displays_delete:
 * @gimp:
 *
 * Calls gimp_display_delete() an all displays in the display list.
 * This closes all displays, including the first one which is usually
 * kept open.
 */
void
gimp_displays_delete (Gimp *gimp)
{
  /*  this removes the GimpDisplay from the list, so do a while loop
   *  "around" the first element to get them all
   */
  while (! gimp_container_is_empty (gimp->displays))
    {
      GimpDisplay *display = gimp_get_display_iter (gimp)->data;

      gimp_display_delete (display);
    }
}

/**
 * gimp_displays_close:
 * @gimp:
 *
 * Calls gimp_display_close() an all displays in the display list. The
 * first display will remain open without an image.
 */
void
gimp_displays_close (Gimp *gimp)
{
  GList *list;
  GList *iter;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  list = g_list_copy (gimp_get_display_iter (gimp));

  for (iter = list; iter; iter = g_list_next (iter))
    {
      GimpDisplay *display = iter->data;

      gimp_display_close (display);
    }

  g_list_free (list);
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

  /*  check which contexts refer to old_image  */
  for (list = gimp->context_list; list; list = g_list_next (list))
    {
      GimpContext *context = list->data;

      if (gimp_context_get_image (context) == old)
        contexts = g_list_prepend (contexts, list->data);
    }

  /*  set the new_image on the remembered contexts (in reverse order,
   *  since older contexts are usually the parents of newer
   *  ones). Also, update the contexts before the displays, or we
   *  might run into menu update functions that would see an
   *  inconsistent state (display = new, context = old), and thus
   *  inadvertently call actions as if the user had selected a menu
   *  item.
   */
  g_list_foreach (contexts, (GFunc) gimp_context_set_image, new);
  g_list_free (contexts);

  for (list = gimp_get_display_iter (gimp);
       list;
       list = g_list_next (list))
    {
      GimpDisplay *display = list->data;

      if (gimp_display_get_image (display) == old)
        gimp_display_set_image (display, new);
    }
}

gint
gimp_displays_get_num_visible (Gimp *gimp)
{
  GList *list;
  gint   visible = 0;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), 0);

  for (list = gimp_get_display_iter (gimp);
       list;
       list = g_list_next (list))
    {
      GimpDisplay      *display = list->data;
      GimpDisplayShell *shell   = gimp_display_get_shell (display);

      if (gtk_widget_is_drawable (GTK_WIDGET (shell)))
        {
          GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (shell));

          if (GTK_IS_WINDOW (toplevel))
            {
              GdkWindow      *window = gtk_widget_get_window (toplevel);
              GdkWindowState  state  = gdk_window_get_state (window);

              if ((state & (GDK_WINDOW_STATE_WITHDRAWN |
                            GDK_WINDOW_STATE_ICONIFIED)) == 0)
                {
                  visible++;
                }
            }
        }
    }

  return visible;
}

void
gimp_displays_set_busy (Gimp *gimp)
{
  GList *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  for (list = gimp_get_display_iter (gimp);
       list;
       list = g_list_next (list))
    {
      GimpDisplayShell *shell =
        gimp_display_get_shell (GIMP_DISPLAY (list->data));

      gimp_display_shell_set_override_cursor (shell, (GimpCursorType) GDK_WATCH);
    }
}

void
gimp_displays_unset_busy (Gimp *gimp)
{
  GList *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  for (list = gimp_get_display_iter (gimp);
       list;
       list = g_list_next (list))
    {
      GimpDisplayShell *shell =
        gimp_display_get_shell (GIMP_DISPLAY (list->data));

      gimp_display_shell_unset_override_cursor (shell);
    }
}

/**
 * gimp_displays_accept_focus_events:
 * @gimp:
 *
 * When this returns %FALSE, we should ignore focus events. In
 * particular, I had weird cases in multi-window mode, before a new
 * image window was actually displayed on screen, there seemed to be
 * infinite loops of focus events switching the active window between
 * the current active one and the one being created, with flickering
 * window title and the new display never actually displayed until we
 * broke the loop by manually getting out of focus (see #11957).
 * This is why when any of the display is not fully drawn yet (which
 * may mean it is either being created or moved to a new window), we
 * temporarily ignore focus events on all displays.
 */
gboolean
gimp_displays_accept_focus_events (Gimp *gimp)
{
  GList    *windows;
  gboolean  accept = TRUE;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  windows = gimp_get_image_windows (gimp);
  for (GList *iter = windows; iter; iter = iter->next)
    {
      GimpDisplayShell *shell = gimp_image_window_get_active_shell (iter->data);

      if (! gimp_display_shell_is_drawn (shell))
        {
          accept = FALSE;
          break;
        }
    }
  g_list_free (windows);

  return accept;
}
