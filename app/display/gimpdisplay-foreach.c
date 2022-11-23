/* LIGMA - The GNU Image Manipulation Program
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

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmalist.h"

#include "ligmadisplay.h"
#include "ligmadisplay-foreach.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-cursor.h"


gboolean
ligma_displays_dirty (Ligma *ligma)
{
  GList *list;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);

  for (list = ligma_get_display_iter (ligma);
       list;
       list = g_list_next (list))
    {
      LigmaDisplay *display = list->data;
      LigmaImage   *image   = ligma_display_get_image (display);

      if (image && ligma_image_is_dirty (image))
        return TRUE;
    }

  return FALSE;
}

static void
ligma_displays_image_dirty_callback (LigmaImage     *image,
                                    LigmaDirtyMask  dirty_mask,
                                    LigmaContainer *container)
{
  if (ligma_image_is_dirty (image)              &&
      ligma_image_get_display_count (image) > 0 &&
      ! ligma_container_have (container, LIGMA_OBJECT (image)))
    ligma_container_add (container, LIGMA_OBJECT (image));
}

static void
ligma_displays_dirty_images_disconnect (LigmaContainer *dirty_container,
                                       LigmaContainer *global_container)
{
  GQuark handler;

  handler = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dirty_container),
                                                "clean-handler"));
  ligma_container_remove_handler (global_container, handler);

  handler = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dirty_container),
                                                "dirty-handler"));
  ligma_container_remove_handler (global_container, handler);
}

static void
ligma_displays_image_clean_callback (LigmaImage     *image,
                                    LigmaDirtyMask  dirty_mask,
                                    LigmaContainer *container)
{
  if (! ligma_image_is_dirty (image))
    ligma_container_remove (container, LIGMA_OBJECT (image));
}

LigmaContainer *
ligma_displays_get_dirty_images (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  if (ligma_displays_dirty (ligma))
    {
      LigmaContainer *container = ligma_list_new_weak (LIGMA_TYPE_IMAGE, FALSE);
      GList         *list;
      GQuark         handler;

      handler =
        ligma_container_add_handler (ligma->images, "clean",
                                    G_CALLBACK (ligma_displays_image_dirty_callback),
                                    container);
      g_object_set_data (G_OBJECT (container), "clean-handler",
                         GINT_TO_POINTER (handler));

      handler =
        ligma_container_add_handler (ligma->images, "dirty",
                                    G_CALLBACK (ligma_displays_image_dirty_callback),
                                    container);
      g_object_set_data (G_OBJECT (container), "dirty-handler",
                         GINT_TO_POINTER (handler));

      g_signal_connect_object (container, "disconnect",
                               G_CALLBACK (ligma_displays_dirty_images_disconnect),
                               G_OBJECT (ligma->images), 0);

      ligma_container_add_handler (container, "clean",
                                  G_CALLBACK (ligma_displays_image_clean_callback),
                                  container);
      ligma_container_add_handler (container, "dirty",
                                  G_CALLBACK (ligma_displays_image_clean_callback),
                                  container);

      for (list = ligma_get_image_iter (ligma);
           list;
           list = g_list_next (list))
        {
          LigmaImage *image = list->data;

          if (ligma_image_is_dirty (image) &&
              ligma_image_get_display_count (image) > 0)
            ligma_container_add (container, LIGMA_OBJECT (image));
        }

      return container;
    }

  return NULL;
}

/**
 * ligma_displays_delete:
 * @ligma:
 *
 * Calls ligma_display_delete() an all displays in the display list.
 * This closes all displays, including the first one which is usually
 * kept open.
 */
void
ligma_displays_delete (Ligma *ligma)
{
  /*  this removes the LigmaDisplay from the list, so do a while loop
   *  "around" the first element to get them all
   */
  while (! ligma_container_is_empty (ligma->displays))
    {
      LigmaDisplay *display = ligma_get_display_iter (ligma)->data;

      ligma_display_delete (display);
    }
}

/**
 * ligma_displays_close:
 * @ligma:
 *
 * Calls ligma_display_close() an all displays in the display list. The
 * first display will remain open without an image.
 */
void
ligma_displays_close (Ligma *ligma)
{
  GList *list;
  GList *iter;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  list = g_list_copy (ligma_get_display_iter (ligma));

  for (iter = list; iter; iter = g_list_next (iter))
    {
      LigmaDisplay *display = iter->data;

      ligma_display_close (display);
    }

  g_list_free (list);
}

void
ligma_displays_reconnect (Ligma      *ligma,
                         LigmaImage *old,
                         LigmaImage *new)
{
  GList *contexts = NULL;
  GList *list;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_IMAGE (old));
  g_return_if_fail (LIGMA_IS_IMAGE (new));

  /*  check which contexts refer to old_image  */
  for (list = ligma->context_list; list; list = g_list_next (list))
    {
      LigmaContext *context = list->data;

      if (ligma_context_get_image (context) == old)
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
  g_list_foreach (contexts, (GFunc) ligma_context_set_image, new);
  g_list_free (contexts);

  for (list = ligma_get_display_iter (ligma);
       list;
       list = g_list_next (list))
    {
      LigmaDisplay *display = list->data;

      if (ligma_display_get_image (display) == old)
        ligma_display_set_image (display, new);
    }
}

gint
ligma_displays_get_num_visible (Ligma *ligma)
{
  GList *list;
  gint   visible = 0;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), 0);

  for (list = ligma_get_display_iter (ligma);
       list;
       list = g_list_next (list))
    {
      LigmaDisplay      *display = list->data;
      LigmaDisplayShell *shell   = ligma_display_get_shell (display);

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
ligma_displays_set_busy (Ligma *ligma)
{
  GList *list;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  for (list = ligma_get_display_iter (ligma);
       list;
       list = g_list_next (list))
    {
      LigmaDisplayShell *shell =
        ligma_display_get_shell (LIGMA_DISPLAY (list->data));

      ligma_display_shell_set_override_cursor (shell, (LigmaCursorType) GDK_WATCH);
    }
}

void
ligma_displays_unset_busy (Ligma *ligma)
{
  GList *list;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  for (list = ligma_get_display_iter (ligma);
       list;
       list = g_list_next (list))
    {
      LigmaDisplayShell *shell =
        ligma_display_get_shell (LIGMA_DISPLAY (list->data));

      ligma_display_shell_unset_override_cursor (shell);
    }
}
