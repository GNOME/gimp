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

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "tool-options-actions.h"
#include "tool-options-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void tool_options_actions_update_presets (GimpActionGroup *group,
                                                 const gchar     *menu,
                                                 gint             keep_n,
                                                 GCallback        callback,
                                                 const gchar     *stock_id,
                                                 const gchar     *help_id,
                                                 GimpContainer   *presets);


/*  global variables  */

static GimpActionEntry tool_options_actions[] =
{
  { "tool-options-save-menu", NULL,
    N_("_Save Options to") },

  { "tool-options-save-new", GTK_STOCK_NEW,
    N_("_New Entry..."), "", NULL,
    G_CALLBACK (tool_options_save_new_cmd_callback),
    GIMP_HELP_TOOL_OPTIONS_SAVE },

  { "tool-options-restore-menu", NULL,
    N_("_Restore Options from") },

  { "tool-options-rename-menu", NULL,
    N_("Re_name Saved Options") },

  { "tool-options-delete-menu", NULL,
    N_("_Delete Saved Options") },

  { "tool-options-reset", GIMP_STOCK_RESET,
    N_("R_eset Tool Options"), "", NULL,
    G_CALLBACK (tool_options_reset_cmd_callback),
    GIMP_HELP_TOOL_OPTIONS_RESET },

  { "tool-options-reset-all", GIMP_STOCK_RESET,
    N_("Reset _all Tool Options..."), "", NULL,
    G_CALLBACK (tool_options_reset_all_cmd_callback),
    GIMP_HELP_TOOL_OPTIONS_RESET }
};


/*  public functions  */

#define SET_VISIBLE(action,condition) \
        gimp_action_group_set_action_visible (group, action, (condition) != 0)
#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)
#define SET_IMPORTANT(action,condition) \
        gimp_action_group_set_action_important (group, action, (condition) != 0)

void
tool_options_actions_setup (GimpActionGroup *group,
                            gpointer         data)
{
  gimp_action_group_add_actions (group,
                                 tool_options_actions,
                                 G_N_ELEMENTS (tool_options_actions),
                                 data);

  SET_IMPORTANT ("tool-options-restore-menu", FALSE);
  SET_IMPORTANT ("tool-options-rename-menu",  FALSE);
  SET_IMPORTANT ("tool-options-delete-menu",  FALSE);
}

void
tool_options_actions_update (GimpActionGroup *group,
                             gpointer         data)
{
  GimpContext  *context;
  GimpToolInfo *tool_info;

  context   = gimp_get_user_context (group->gimp);
  tool_info = gimp_context_get_tool (context);

  SET_VISIBLE ("tool-options-save-menu",    tool_info->options_presets);
  SET_VISIBLE ("tool-options-restore-menu", tool_info->options_presets);
  SET_VISIBLE ("tool-options-rename-menu",  tool_info->options_presets);
  SET_VISIBLE ("tool-options-delete-menu",  tool_info->options_presets);

  if (! tool_info->options_presets)
    return;

  tool_options_actions_update_presets (group, "tool-options-save-menu", 2,
                                       G_CALLBACK (tool_options_save_to_cmd_callback),
                                       GTK_STOCK_SAVE,
                                       GIMP_HELP_TOOL_OPTIONS_SAVE,
                                       tool_info->options_presets);

  tool_options_actions_update_presets (group, "tool-options-restore-menu", 1,
                                       G_CALLBACK (tool_options_restore_from_cmd_callback),
                                       GTK_STOCK_REVERT_TO_SAVED,
                                       GIMP_HELP_TOOL_OPTIONS_RESTORE,
                                       tool_info->options_presets);

  tool_options_actions_update_presets (group, "tool-options-rename-menu", 1,
                                       G_CALLBACK (tool_options_rename_saved_cmd_callback),
                                       GIMP_STOCK_EDIT,
                                       GIMP_HELP_TOOL_OPTIONS_RENAME,
                                       tool_info->options_presets);

  tool_options_actions_update_presets (group, "tool-options-delete-menu", 1,
                                       G_CALLBACK (tool_options_delete_saved_cmd_callback),
                                       GTK_STOCK_DELETE,
                                       GIMP_HELP_TOOL_OPTIONS_DELETE,
                                       tool_info->options_presets);
}


/*  privat function  */

static void
tool_options_actions_update_presets (GimpActionGroup *group,
                                     const gchar     *menu,
                                     gint             keep_n,
                                     GCallback        callback,
                                     const gchar     *stock_id,
                                     const gchar     *help_id,
                                     GimpContainer   *presets)
{
#if 0
  GtkWidget *menu;

  menu = gtk_item_factory_get_widget (factory, menu);

  if (menu)
    {
      GList *list;
      gint   num_children;

      list = g_list_nth (GTK_MENU_SHELL (menu)->children, keep_n - 1);
      while (g_list_next (list))
        gtk_widget_destroy (g_list_next (list)->data);

      num_children = gimp_container_num_children (presets);

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
#endif
}

#undef SET_SENSITIVE
#undef SET_VISIBLE
