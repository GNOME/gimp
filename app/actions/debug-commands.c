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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "actions-types.h"

#include "core/gimpobject.h"

#include "widgets/gimpmenufactory.h"
#include "widgets/gimpuimanager.h"

#include "menus/menus.h"

#include "debug-commands.h"


#ifdef ENABLE_DEBUG_MENU

/*  local function prototypes  */

static void   debug_dump_menus_recurse_menu (GtkWidget *menu,
                                             gint       depth,
                                             gchar     *path);


/*  public functions  */

void
debug_mem_profile_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  extern gboolean gimp_debug_memsize;

  gimp_debug_memsize = TRUE;

  gimp_object_get_memsize (GIMP_OBJECT (data), NULL);

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
#if 0
      FIXME
      GimpMenuFactoryEntry *entry = list->data;
      GimpItemFactory      *item_factory;

      item_factory = gimp_item_factory_from_path (entry->identifier);

      if (item_factory)
        {
          GtkWidget *menu_item;

          g_print ("%s\n", entry->identifier);

          menu_item = gtk_item_factory_get_item (GTK_ITEM_FACTORY (item_factory),
                                                 entry->entries[0].entry.path);

          if (menu_item         &&
              menu_item->parent &&
              GTK_IS_MENU (menu_item->parent))
            debug_dump_menus_recurse_menu (menu_item->parent, 1,
                                           entry->identifier);

          g_print ("\n");
        }
#endif
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


/*  private functions  */

static void
debug_dump_menus_recurse_menu (GtkWidget *menu,
                               gint       depth,
                               gchar     *path)
{
#if 0
  GtkItemFactory *item_factory;
  GtkWidget      *menu_item;
  GList          *list;
  const gchar    *label;
  gchar          *help_page;
  gchar          *full_path;
  gchar          *format_str;

  for (list = GTK_MENU_SHELL (menu)->children; list; list = g_list_next (list))
    {
      menu_item = GTK_WIDGET (list->data);

      if (GTK_IS_LABEL (GTK_BIN (menu_item)->child))
	{
	  label = gtk_label_get_text (GTK_LABEL (GTK_BIN (menu_item)->child));
	  full_path = g_strconcat (path, "/", label, NULL);

	  item_factory = GTK_ITEM_FACTORY (gimp_item_factory_from_path (path));
          help_page    = g_object_get_data (G_OBJECT (menu_item),
                                            "gimp-help-id");

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
#endif
}

#endif /* ENABLE_DEBUG_MENU */
