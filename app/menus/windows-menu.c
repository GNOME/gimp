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

#include "libgimpthumb/gimpthumb.h"

#include "menus-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimplist.h"
#include "core/gimpviewable.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpuimanager.h"

#include "display/gimpdisplay.h"

#include "windows-menu.h"


static void   windows_menu_display_add    (GimpContainer    *container,
                                           GimpDisplay      *display,
                                           GimpUIManager    *manager);
static void   windows_menu_display_remove (GimpContainer    *container,
                                           GimpDisplay      *display,
                                           GimpUIManager    *manager);
static void   windows_menu_image_notify   (GimpDisplay      *display,
                                           const GParamSpec *unused,
                                           GimpUIManager    *manager);


void
windows_menu_setup (GimpUIManager *manager,
                    const gchar   *ui_path)
{
  GList *list;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);

  g_signal_connect_object (manager->gimp->displays, "add",
                           G_CALLBACK (windows_menu_display_add),
                           manager, 0);
  g_signal_connect_object (manager->gimp->displays, "remove",
                           G_CALLBACK (windows_menu_display_remove),
                           manager, 0);

  g_object_set_data (G_OBJECT (manager), "image-menu-ui-path",
                     (gpointer) ui_path);

  for (list = GIMP_LIST (manager->gimp->displays)->list;
       list;
       list = g_list_next (list))
    {
      GimpDisplay *display = list->data;

      windows_menu_display_add (manager->gimp->displays, display, manager);
    }
}


/*  private functions  */

static void
windows_menu_display_add (GimpContainer *container,
                          GimpDisplay   *display,
                          GimpUIManager *manager)
{
  g_signal_connect_object (display, "notify::image",
                           G_CALLBACK (windows_menu_image_notify),
                           manager, 0);

  if (display->image)
    windows_menu_image_notify (display, NULL, manager);
}

static void
windows_menu_display_remove (GimpContainer *container,
                             GimpDisplay   *display,
                             GimpUIManager *manager)
{
  gchar *merge_key = g_strdup_printf ("windows-display-%04d-merge-id",
                                      gimp_display_get_ID (display));
  guint  merge_id;

  merge_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (manager),
                                                  merge_key));

  if (merge_id)
    gtk_ui_manager_remove_ui (GTK_UI_MANAGER (manager), merge_id);

  g_object_set_data (G_OBJECT (manager), merge_key, NULL);

  g_free (merge_key);
}

static void
windows_menu_image_notify (GimpDisplay      *display,
                           const GParamSpec *unused,
                           GimpUIManager    *manager)
{
  if (display->image)
    {
      gchar *merge_key = g_strdup_printf ("windows-display-%04d-merge-id",
                                          gimp_display_get_ID (display));
      guint  merge_id;

      merge_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (manager),
                                                      merge_key));

      if (! merge_id)
        {
          const gchar *ui_path;
          gchar       *action_name;
          gchar       *action_path;

          ui_path = g_object_get_data (G_OBJECT (manager), "image-menu-ui-path");

          action_name = g_strdup_printf ("windows-display-%04d",
                                         gimp_display_get_ID (display));
          action_path = g_strdup_printf ("%s/Windows/Images", ui_path);

          merge_id = gtk_ui_manager_new_merge_id (GTK_UI_MANAGER (manager));

          g_object_set_data (G_OBJECT (manager), merge_key,
                             GUINT_TO_POINTER (merge_id));

          gtk_ui_manager_add_ui (GTK_UI_MANAGER (manager), merge_id,
                                 action_path, action_name, action_name,
                                 GTK_UI_MANAGER_MENUITEM,
                                 FALSE);

          g_free (action_path);
          g_free (action_name);
        }

      g_free (merge_key);
    }
  else
    {
      windows_menu_display_remove (manager->gimp->displays, display, manager);
    }
}
