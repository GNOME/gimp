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

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"

#include "tool-options-commands.h"
#include "tool-options-menu.h"

#include "gimp-intl.h"


GimpItemFactoryEntry tool_options_menu_entries[] =
{
  { { N_("/Save Options to/New Entry..."), "",
      tool_options_save_new_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    NULL, NULL },
  { { N_("/Save Options to/new-separator"), "",
      NULL, 0,
      "<Separator>", NULL },
    NULL,
    NULL, NULL },

  { { N_("/Restore Options from/(None)"), "",
      NULL, 0,
      "<Item>", NULL },
    NULL,
    NULL, NULL },

  { { N_("/Delete Saved Options/(None)"), "",
      NULL, 0,
      "<Item>", NULL },
    NULL,
    NULL, NULL },

  { { "/reset-separator", NULL, NULL, 0, "<Separator>", NULL },
    NULL, NULL, NULL },

  { { N_("/Reset Tool Options"), "",
      tool_options_reset_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_RESET },
    NULL,
    GIMP_HELP_TOOL_OPTIONS_RESET, NULL },
  { { N_("/Reset all Tool Options..."), "",
      tool_options_reset_all_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_RESET },
    NULL,
    GIMP_HELP_TOOL_OPTIONS_RESET, NULL }
};

gint n_tool_options_menu_entries = G_N_ELEMENTS (tool_options_menu_entries);


void
tool_options_menu_update (GtkItemFactory *factory,
                          gpointer        data)
{
  GimpContext  *context;
  GimpToolInfo *tool_info;
  GtkWidget    *save_menu;
  GtkWidget    *restore_menu;
  GtkWidget    *delete_menu;
  GList        *list;

  context   = gimp_get_user_context (GIMP_ITEM_FACTORY (factory)->gimp);
  tool_info = gimp_context_get_tool (context);

#define SET_VISIBLE(menu,condition) \
        gimp_item_factory_set_visible (factory, menu, (condition) != 0)
#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_VISIBLE ("/Save Options to",      tool_info->options_presets);
  SET_VISIBLE ("/Restore Options from", tool_info->options_presets);
  SET_VISIBLE ("/Delete Saved Options", tool_info->options_presets);
  SET_VISIBLE ("/reset-separator",      tool_info->options_presets);

  SET_SENSITIVE ("/Restore Options from/(None)", FALSE);
  SET_SENSITIVE ("/Delete Saved Options/(None)", FALSE);

  if (! tool_info->options_presets)
    return;

  save_menu    = gtk_item_factory_get_widget (factory, "/Save Options to");
  restore_menu = gtk_item_factory_get_widget (factory, "/Restore Options from");
  delete_menu  = gtk_item_factory_get_widget (factory, "/Delete Saved Options");

  list = g_list_nth (GTK_MENU_SHELL (save_menu)->children, 1);

  while (g_list_next (list))
    gtk_widget_destroy (list->next->data);

  list = g_list_nth (GTK_MENU_SHELL (restore_menu)->children, 0);

  while (g_list_next (list))
    gtk_widget_destroy (list->next->data);

  list = g_list_nth (GTK_MENU_SHELL (delete_menu)->children, 0);

  while (g_list_next (list))
    gtk_widget_destroy (list->next->data);

  if (gimp_container_num_children (tool_info->options_presets))
    {
      GimpItemFactoryEntry  entry;
      GimpToolOptions      *options;

      SET_VISIBLE ("/Save Options to/new-separator", TRUE);
      SET_VISIBLE ("/Restore Options from/(None)",   FALSE);
      SET_VISIBLE ("/Delete Saved Options/(None)",   FALSE);

      entry.entry.path            = NULL;
      entry.entry.accelerator     = "";
      entry.entry.callback        = tool_options_save_to_cmd_callback;
      entry.entry.callback_action = 0;
      entry.entry.item_type       = "<StockItem>";
      entry.entry.extra_data      = GTK_STOCK_SAVE;
      entry.quark_string          = NULL;
      entry.help_id               = NULL;
      entry.description           = NULL;

      for (list = GIMP_LIST (tool_info->options_presets)->list;
           list;
           list = g_list_next (list))
        {
          options = list->data;

          entry.entry.path = g_strdup_printf ("/Save Options to/%s",
                                              GIMP_OBJECT (options)->name);
          gimp_item_factory_create_item (GIMP_ITEM_FACTORY (factory),
                                         &entry, NULL,
                                         options, 2, FALSE, FALSE);
          g_free (entry.entry.path);
        }

      entry.entry.callback   = tool_options_restore_from_cmd_callback;
      entry.entry.extra_data = GTK_STOCK_REVERT_TO_SAVED;

      for (list = GIMP_LIST (tool_info->options_presets)->list;
           list;
           list = g_list_next (list))
        {
          options = list->data;

          entry.entry.path = g_strdup_printf ("/Restore Options from/%s",
                                              GIMP_OBJECT (options)->name);
          gimp_item_factory_create_item (GIMP_ITEM_FACTORY (factory),
                                         &entry, NULL,
                                         options, 2, FALSE, FALSE);
          g_free (entry.entry.path);
        }

      entry.entry.callback   = tool_options_delete_saved_cmd_callback;
      entry.entry.extra_data = GTK_STOCK_DELETE;

      for (list = GIMP_LIST (tool_info->options_presets)->list;
           list;
           list = g_list_next (list))
        {
          options = list->data;

          entry.entry.path = g_strdup_printf ("/Delete Saved Options/%s",
                                              GIMP_OBJECT (options)->name);
          gimp_item_factory_create_item (GIMP_ITEM_FACTORY (factory),
                                         &entry, NULL,
                                         options, 2, FALSE, FALSE);
          g_free (entry.entry.path);
        }
    }
  else
    {
      SET_VISIBLE ("/Save Options to/new-separator", FALSE);
      SET_VISIBLE ("/Restore Options from/(None)",   TRUE);
      SET_VISIBLE ("/Delete Saved Options/(None)",   TRUE);
    }

#undef SET_VISIBLE
#undef SET_SENSITIVE
}
