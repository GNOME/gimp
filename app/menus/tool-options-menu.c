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

#include "actions/tool-options-commands.h"

#include "menus.h"
#include "tool-options-menu.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void tool_options_menu_update_presets (GtkItemFactory         *factory,
                                              const gchar            *menu_path,
                                              gint                    keep_n,
                                              gboolean                has_none,
                                              GtkItemFactoryCallback  callback,
                                              const gchar            *stock_id,
                                              const gchar            *help_id,
                                              GimpContainer          *presets);


/*  global variables  */

GimpItemFactoryEntry tool_options_menu_entries[] =
{
  MENU_BRANCH (N_("/_Save Options to")),

  { { N_("/Save Options to/_New Entry..."), "",
      tool_options_save_new_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    GIMP_HELP_TOOL_OPTIONS_SAVE, NULL },
  { { "/Save Options to/new-separator", "",
      NULL, 0,
      "<Separator>", NULL },
    NULL,
    NULL, NULL },

  MENU_BRANCH (N_("/_Restore Options from")),

  { { N_("/Restore Options from/(None)"), "",
      NULL, 0,
      "<Item>", NULL },
    NULL,
    NULL, NULL },

  MENU_BRANCH (N_("/Re_name Saved Options")),

  { { N_("/Rename Saved Options/(None)"), "",
      NULL, 0,
      "<Item>", NULL },
    NULL,
    NULL, NULL },

  MENU_BRANCH (N_("/_Delete Saved Options")),

  { { N_("/Delete Saved Options/(None)"), "",
      NULL, 0,
      "<Item>", NULL },
    NULL,
    NULL, NULL },

  { { "/reset-separator", NULL, NULL, 0, "<Separator>", NULL },
    NULL, NULL, NULL },

  { { N_("/R_eset Tool Options"), "",
      tool_options_reset_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_RESET },
    NULL,
    GIMP_HELP_TOOL_OPTIONS_RESET, NULL },
  { { N_("/Reset _all Tool Options..."), "",
      tool_options_reset_all_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_RESET },
    NULL,
    GIMP_HELP_TOOL_OPTIONS_RESET, NULL }
};

gint n_tool_options_menu_entries = G_N_ELEMENTS (tool_options_menu_entries);


/*  public functions  */

void
tool_options_menu_setup (GimpItemFactory *factory,
                         gpointer         callback_data)
{
  gimp_item_factory_set_sensitive (GTK_ITEM_FACTORY (factory),
                                   "/Restore Options from/(None)", FALSE);
  gimp_item_factory_set_sensitive (GTK_ITEM_FACTORY (factory),
                                   "/Rename Saved Options/(None)", FALSE);
  gimp_item_factory_set_sensitive (GTK_ITEM_FACTORY (factory),
                                   "/Delete Saved Options/(None)", FALSE);
}

#define SET_VISIBLE(menu,condition) \
        gimp_item_factory_set_visible (factory, menu, (condition) != 0)

void
tool_options_menu_update (GtkItemFactory *factory,
                          gpointer        data)
{
  GimpContext  *context;
  GimpToolInfo *tool_info;

  context   = gimp_get_user_context (GIMP_ITEM_FACTORY (factory)->gimp);
  tool_info = gimp_context_get_tool (context);

  SET_VISIBLE ("/Save Options to",      tool_info->options_presets);
  SET_VISIBLE ("/Restore Options from", tool_info->options_presets);
  SET_VISIBLE ("/Rename Saved Options", tool_info->options_presets);
  SET_VISIBLE ("/Delete Saved Options", tool_info->options_presets);
  SET_VISIBLE ("/reset-separator",      tool_info->options_presets);

  if (! tool_info->options_presets)
    return;

  SET_VISIBLE ("/Save Options to/new-separator",
               gimp_container_num_children (tool_info->options_presets) > 0);

  tool_options_menu_update_presets (factory, "/Save Options to", 2, FALSE,
                                    tool_options_save_to_cmd_callback,
                                    GTK_STOCK_SAVE,
                                    GIMP_HELP_TOOL_OPTIONS_SAVE,
                                    tool_info->options_presets);

  tool_options_menu_update_presets (factory, "/Restore Options from", 1, TRUE,
                                    tool_options_restore_from_cmd_callback,
                                    GTK_STOCK_REVERT_TO_SAVED,
                                    GIMP_HELP_TOOL_OPTIONS_RESTORE,
                                    tool_info->options_presets);

  tool_options_menu_update_presets (factory, "/Rename Saved Options", 1, TRUE,
                                    tool_options_rename_saved_cmd_callback,
                                    GIMP_STOCK_EDIT,
                                    GIMP_HELP_TOOL_OPTIONS_RENAME,
                                    tool_info->options_presets);

  tool_options_menu_update_presets (factory, "/Delete Saved Options", 1, TRUE,
                                    tool_options_delete_saved_cmd_callback,
                                    GTK_STOCK_DELETE,
                                    GIMP_HELP_TOOL_OPTIONS_DELETE,
                                    tool_info->options_presets);
}


/*  privat function  */

static void
tool_options_menu_update_presets (GtkItemFactory         *factory,
                                  const gchar            *menu_path,
                                  gint                    keep_n,
                                  gboolean                has_none,
                                  GtkItemFactoryCallback  callback,
                                  const gchar            *stock_id,
                                  const gchar            *help_id,
                                  GimpContainer          *presets)
{
  GtkWidget *menu;

  menu = gtk_item_factory_get_widget (factory, menu_path);

  if (menu)
    {
      GList *list;
      gint   num_children;

      list = g_list_nth (GTK_MENU_SHELL (menu)->children, keep_n - 1);
      while (g_list_next (list))
        gtk_widget_destroy (g_list_next (list)->data);

      num_children = gimp_container_num_children (presets);

      if (has_none)
        {
          gchar *none;

          none = g_strdup_printf ("%s/(None)", menu_path);
          SET_VISIBLE (none, num_children == 0);
          g_free (none);
        }

      if (num_children > 0)
        {
          GimpItemFactoryEntry entry;

          entry.entry.path            = NULL;
          entry.entry.accelerator     = "";
          entry.entry.callback        = callback;
          entry.entry.callback_action = 0;
          entry.entry.item_type       = stock_id ? "<StockItem>" : "<Item>";
          entry.entry.extra_data      = stock_id;
          entry.quark_string          = NULL;
          entry.help_id               = help_id;
          entry.description           = NULL;

          for (list = GIMP_LIST (presets)->list;
               list;
               list = g_list_next (list))
            {
              GimpToolOptions *options = list->data;

              entry.entry.path = g_strdup_printf ("%s/%s",
                                                  menu_path,
                                                  GIMP_OBJECT (options)->name);
              gimp_item_factory_create_item (GIMP_ITEM_FACTORY (factory),
                                             &entry, NULL,
                                             options, 2, FALSE);
              g_free (entry.entry.path);
            }
        }
    }
}

#undef SET_VISIBLE
