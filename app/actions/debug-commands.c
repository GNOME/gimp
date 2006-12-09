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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "widgets/gimpmenufactory.h"
#include "widgets/gimpuimanager.h"

#include "menus/menus.h"

#include "actions.h"
#include "debug-commands.h"


#ifdef ENABLE_DEBUG_MENU

/*  local function prototypes  */

static void   debug_dump_menus_recurse_menu (GtkWidget  *menu,
                                             gint        depth,
                                             gchar      *path);
static void   debug_print_qdata             (GimpObject *object);
static void   debug_print_qdata_foreach     (GQuark      key_id,
                                             gpointer    data,
                                             gpointer    user_data);


/*  public functions  */

void
debug_mem_profile_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  extern gboolean  gimp_debug_memsize;
  Gimp            *gimp;
  return_if_no_gimp (gimp, data);

  gimp_debug_memsize = TRUE;

  gimp_object_get_memsize (GIMP_OBJECT (gimp), NULL);

  gimp_debug_memsize = FALSE;
}

void
debug_dump_menus_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GList *list;

  for (list = global_menu_factory->registered_menus;
       list;
       list = g_list_next (list))
    {
      GimpMenuFactoryEntry *entry = list->data;
      GList                *managers;

      managers = gimp_ui_managers_from_name (entry->identifier);

      if (managers)
        {
          GimpUIManager *manager = managers->data;
          GList         *list;

          for (list = manager->registered_uis; list; list = g_list_next (list))
            {
              GimpUIManagerUIEntry *ui_entry = list->data;

              if (GTK_IS_MENU_SHELL (ui_entry->widget))
                {
                  g_print ("\n\n========================================\n"
                           "Menu: %s%s\n"
                           "========================================\n\n",
                           entry->identifier, ui_entry->ui_path);

                  debug_dump_menus_recurse_menu (ui_entry->widget, 1,
                                                 entry->identifier);
                  g_print ("\n");
                }
            }
        }
    }
}

void
debug_dump_managers_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GList *list;

  for (list = global_menu_factory->registered_menus;
       list;
       list = g_list_next (list))
    {
      GimpMenuFactoryEntry *entry = list->data;
      GList                *managers;

      managers = gimp_ui_managers_from_name (entry->identifier);

      if (managers)
        {
          g_print ("\n\n========================================\n"
                   "UI Manager: %s\n"
                   "========================================\n\n",
                   entry->identifier);

          g_print (gtk_ui_manager_get_ui (managers->data));
        }
    }
}

void
debug_dump_attached_data_cmd_callback (GtkAction *action,
                                       gpointer   data)
{
  Gimp        *gimp         = action_data_get_gimp (data);
  GimpContext *user_context = gimp_get_user_context (gimp);

  debug_print_qdata (GIMP_OBJECT (gimp));
  debug_print_qdata (GIMP_OBJECT (user_context));
}


/*  private functions  */

static void
debug_dump_menus_recurse_menu (GtkWidget *menu,
                               gint       depth,
                               gchar     *path)
{
  GtkWidget   *menu_item;
  GList       *list;
  const gchar *label;
  gchar       *help_page;
  gchar       *full_path;
  gchar       *format_str;

  for (list = GTK_MENU_SHELL (menu)->children; list; list = g_list_next (list))
    {
      menu_item = GTK_WIDGET (list->data);

      if (GTK_IS_LABEL (GTK_BIN (menu_item)->child))
        {
          label = gtk_label_get_text (GTK_LABEL (GTK_BIN (menu_item)->child));
          full_path = g_strconcat (path, "/", label, NULL);

          help_page = g_object_get_data (G_OBJECT (menu_item), "gimp-help-id");
          help_page = g_strdup (help_page);

          format_str = g_strdup_printf ("%%%ds%%%ds %%-20s %%s\n",
                                        depth * 2, depth * 2 - 40);
          g_print (format_str,
                   "", label, "", help_page ? help_page : "");
          g_free (format_str);
          g_free (help_page);

          if (GTK_MENU_ITEM (menu_item)->submenu)
            debug_dump_menus_recurse_menu (GTK_MENU_ITEM (menu_item)->submenu,
                                           depth + 1, full_path);

          g_free (full_path);
        }
    }
}

static void
debug_print_qdata (GimpObject *object)
{
  g_print ("\nData attached to '%s':\n\n", gimp_object_get_name (object));
  g_datalist_foreach (&G_OBJECT (object)->qdata,
                      debug_print_qdata_foreach,
                      NULL);
  g_print ("\n");
}

static void
debug_print_qdata_foreach (GQuark   key_id,
                           gpointer data,
                           gpointer user_data)
{
  g_print ("%s: %p\n", g_quark_to_string (key_id), data);
}

#endif /* ENABLE_DEBUG_MENU */
