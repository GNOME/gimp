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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimpcontext.h"
#include "core/gimpdata.h"
#include "core/gimptoolpreset.h"
#include "core/gimptooloptions.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "data-commands.h"
#include "tool-presets-actions.h"
#include "tool-presets-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry tool_presets_actions[] =
{
  { "tool-presets-popup", GIMP_STOCK_TOOL_PRESET,
    NC_("tool-presets-action", "Tool Presets Menu"), NULL, NULL, NULL,
    GIMP_HELP_TOOL_PRESET_DIALOG },

  { "tool-presets-new", "document-new",
    NC_("tool-presets-action", "_New Tool Preset"), NULL,
    NC_("tool-presets-action", "Create a new tool preset"),
    G_CALLBACK (data_new_cmd_callback),
    GIMP_HELP_TOOL_PRESET_NEW },

  { "tool-presets-duplicate", GIMP_STOCK_DUPLICATE,
    NC_("tool-presets-action", "D_uplicate Tool Preset"), NULL,
    NC_("tool-presets-action", "Duplicate this tool preset"),
    G_CALLBACK (data_duplicate_cmd_callback),
    GIMP_HELP_TOOL_PRESET_DUPLICATE },

  { "tool-presets-copy-location", "edit-copy",
    NC_("tool-presets-action", "Copy Tool Preset _Location"), NULL,
    NC_("tool-presets-action", "Copy tool preset file location to clipboard"),
    G_CALLBACK (data_copy_location_cmd_callback),
    GIMP_HELP_TOOL_PRESET_COPY_LOCATION },

  { "tool-presets-show-in-file-manager", GIMP_STOCK_FILE_MANAGER,
    NC_("tool-presets-action", "Show in _File Manager"), NULL,
    NC_("tool-presets-action", "Show tool preset file location in the file manager"),
    G_CALLBACK (data_show_in_file_manager_cmd_callback),
    GIMP_HELP_TOOL_PRESET_SHOW_IN_FILE_MANAGER },

  { "tool-presets-restore", GIMP_STOCK_REVERT,
    NC_("tool-presets-action", "_Restore Tool Preset"), NULL,
    NC_("tool-presets-action", "Restore this tool preset"),
    G_CALLBACK (tool_presets_restore_cmd_callback),
    GIMP_HELP_TOOL_PRESET_RESTORE },

  { "tool-presets-delete", "edit-delete",
    NC_("tool-presets-action", "_Delete Tool Preset"), NULL,
    NC_("tool-presets-action", "Delete this tool preset"),
    G_CALLBACK (data_delete_cmd_callback),
    GIMP_HELP_TOOL_PRESET_DELETE },

  { "tool-presets-refresh", "view-refresh",
    NC_("tool-presets-action", "_Refresh Tool Presets"), NULL,
    NC_("tool-presets-action", "Refresh tool presets"),
    G_CALLBACK (data_refresh_cmd_callback),
    GIMP_HELP_TOOL_PRESET_REFRESH }
};

static const GimpStringActionEntry tool_presets_edit_actions[] =
{
  { "tool-presets-edit", "gtk-edit",
    NC_("tool-presets-action", "_Edit Tool Preset..."), NULL,
    NC_("tool-presets-action", "Edit this tool preset"),
    "gimp-tool-preset-editor",
    GIMP_HELP_TOOL_PRESET_EDIT }
};


void
tool_presets_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "tool-presets-action",
                                 tool_presets_actions,
                                 G_N_ELEMENTS (tool_presets_actions));

  gimp_action_group_add_string_actions (group, "tool-presets-action",
                                        tool_presets_edit_actions,
                                        G_N_ELEMENTS (tool_presets_edit_actions),
                                        G_CALLBACK (data_edit_cmd_callback));
}

void
tool_presets_actions_update (GimpActionGroup *group,
                             gpointer         user_data)
{
  GimpContext    *context     = action_data_get_context (user_data);
  GimpToolPreset *tool_preset = NULL;
  GimpData       *data        = NULL;
  GFile          *file        = NULL;

  if (context)
    {
      tool_preset = gimp_context_get_tool_preset (context);

      if (tool_preset)
        {
          data = GIMP_DATA (tool_preset);

          file = gimp_data_get_file (data);
        }
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("tool-presets-edit",                 tool_preset);
  SET_SENSITIVE ("tool-presets-duplicate",            tool_preset && GIMP_DATA_GET_CLASS (data)->duplicate);
  SET_SENSITIVE ("tool-presets-copy-location",        file);
  SET_SENSITIVE ("tool-presets-show-in-file-manager", file);
  SET_SENSITIVE ("tool-presets-restore",              tool_preset);
  SET_SENSITIVE ("tool-presets-delete",               tool_preset && gimp_data_is_deletable (data));

#undef SET_SENSITIVE
}
