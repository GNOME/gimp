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
#include "tool-preset-actions.h"

#include "gimp-intl.h"


static const GimpActionEntry tool_preset_actions[] =
{
  { "tool-preset-popup", GIMP_STOCK_TOOL_PRESET,
    NC_("tool-preset-action", "Tool Preset Menu"), NULL, NULL, NULL,
    GIMP_HELP_TOOL_PRESET_DIALOG },

  { "tool-preset-new", GTK_STOCK_NEW,
    NC_("tool-preset-action", "_New Tool Preset"), "",
    NC_("tool-preset-action", "Create a new tool preset"),
    G_CALLBACK (data_new_cmd_callback),
    GIMP_HELP_TOOL_PRESET_NEW },

  { "tool-preset-duplicate", GIMP_STOCK_DUPLICATE,
    NC_("tool-preset-action", "D_uplicate Tool Preset"), NULL,
    NC_("tool-preset-action", "Duplicate this tool preset"),
    G_CALLBACK (data_duplicate_cmd_callback),
    GIMP_HELP_TOOL_PRESET_DUPLICATE },

  { "tool-preset-copy-location", GTK_STOCK_COPY,
    NC_("tool-preset-action", "Copy Tool Preset _Location"), "",
    NC_("tool-preset-action", "Copy tool preset file location to clipboard"),
    G_CALLBACK (data_copy_location_cmd_callback),
    GIMP_HELP_TOOL_PRESET_COPY_LOCATION },

  { "tool-preset-delete", GTK_STOCK_DELETE,
    NC_("tool-preset-action", "_Delete ToolPreset"), "",
    NC_("tool-preset-action", "Delete this tool_preset"),
    G_CALLBACK (data_delete_cmd_callback),
    GIMP_HELP_TOOL_PRESET_DELETE },

  { "tool-preset-refresh", GTK_STOCK_REFRESH,
    NC_("tool-preset-action", "_Refresh Tool Presets"), "",
    NC_("tool-preset-action", "Refresh tool presets"),
    G_CALLBACK (data_refresh_cmd_callback),
    GIMP_HELP_TOOL_PRESET_REFRESH }
};

static const GimpStringActionEntry tool_preset_edit_actions[] =
{
  { "tool-preset-edit", GTK_STOCK_EDIT,
    NC_("tool-preset-action", "_Edit ToolPreset..."), NULL,
    NC_("tool-preset-action", "Edit tool preset"),
    "gimp-tool-preset-editor",
    GIMP_HELP_TOOL_PRESET_EDIT }
};


void
tool_preset_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "tool-preset-action",
                                 tool_preset_actions,
                                 G_N_ELEMENTS (tool_preset_actions));

  gimp_action_group_add_string_actions (group, "tool-preset-action",
                                        tool_preset_edit_actions,
                                        G_N_ELEMENTS (tool_preset_edit_actions),
                                        G_CALLBACK (data_edit_cmd_callback));
}

void
tool_preset_actions_update (GimpActionGroup *group,
                            gpointer         user_data)
{
  GimpContext    *context     = action_data_get_context (user_data);
  GimpToolPreset *tool_preset = NULL;
  GimpData       *data        = NULL;
  const gchar    *filename    = NULL;

  if (context)
    {
      tool_preset = gimp_context_get_tool_preset (context);

      if (tool_preset)
        {
          data = GIMP_DATA (tool_preset);

          filename = gimp_data_get_filename (data);
        }
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("tool-preset-edit",          tool_preset);
  SET_SENSITIVE ("tool-preset-duplicate",     tool_preset && GIMP_DATA_GET_CLASS (data)->duplicate);
  SET_SENSITIVE ("tool-preset-copy-location", tool_preset && filename);
  SET_SENSITIVE ("tool-preset-delete",        tool_preset && gimp_data_is_deletable (data));

#undef SET_SENSITIVE
}
