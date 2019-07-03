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

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimpcontext.h"
#include "core/gimpdata.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "data-commands.h"
#include "patterns-actions.h"

#include "gimp-intl.h"


static const GimpActionEntry patterns_actions[] =
{
  { "patterns-popup", GIMP_ICON_PATTERN,
    NC_("patterns-action", "Patterns Menu"), NULL, NULL, NULL,
    GIMP_HELP_PATTERN_DIALOG },

  { "patterns-open-as-image", GIMP_ICON_DOCUMENT_OPEN,
    NC_("patterns-action", "_Open Pattern as Image"), NULL,
    NC_("patterns-action", "Open this pattern as an image"),
    data_open_as_image_cmd_callback,
    GIMP_HELP_PATTERN_OPEN_AS_IMAGE },

  { "patterns-new", GIMP_ICON_DOCUMENT_NEW,
    NC_("patterns-action", "_New Pattern"), NULL,
    NC_("patterns-action", "Create a new pattern"),
    data_new_cmd_callback,
    GIMP_HELP_PATTERN_NEW },

  { "patterns-duplicate", GIMP_ICON_OBJECT_DUPLICATE,
    NC_("patterns-action", "D_uplicate Pattern"), NULL,
    NC_("patterns-action", "Duplicate this pattern"),
    data_duplicate_cmd_callback,
    GIMP_HELP_PATTERN_DUPLICATE },

  { "patterns-copy-location", GIMP_ICON_EDIT_COPY,
    NC_("patterns-action", "Copy Pattern _Location"), NULL,
    NC_("patterns-action", "Copy pattern file location to clipboard"),
    data_copy_location_cmd_callback,
    GIMP_HELP_PATTERN_COPY_LOCATION },

  { "patterns-show-in-file-manager", GIMP_ICON_FILE_MANAGER,
    NC_("patterns-action", "Show in _File Manager"), NULL,
    NC_("patterns-action", "Show pattern file location in the file manager"),
    data_show_in_file_manager_cmd_callback,
    GIMP_HELP_PATTERN_SHOW_IN_FILE_MANAGER },

  { "patterns-delete", GIMP_ICON_EDIT_DELETE,
    NC_("patterns-action", "_Delete Pattern"), NULL,
    NC_("patterns-action", "Delete this pattern"),
    data_delete_cmd_callback,
    GIMP_HELP_PATTERN_DELETE },

  { "patterns-refresh", GIMP_ICON_VIEW_REFRESH,
    NC_("patterns-action", "_Refresh Patterns"), NULL,
    NC_("patterns-action", "Refresh patterns"),
    data_refresh_cmd_callback,
    GIMP_HELP_PATTERN_REFRESH }
};

static const GimpStringActionEntry patterns_edit_actions[] =
{
  { "patterns-edit", GIMP_ICON_EDIT,
    NC_("patterns-action", "_Edit Pattern..."), NULL,
    NC_("patterns-action", "Edit pattern"),
    "gimp-pattern-editor",
    GIMP_HELP_PATTERN_EDIT }
};


void
patterns_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "patterns-action",
                                 patterns_actions,
                                 G_N_ELEMENTS (patterns_actions));

  gimp_action_group_add_string_actions (group, "patterns-action",
                                        patterns_edit_actions,
                                        G_N_ELEMENTS (patterns_edit_actions),
                                        data_edit_cmd_callback);
}

void
patterns_actions_update (GimpActionGroup *group,
                         gpointer         user_data)
{
  GimpContext *context = action_data_get_context (user_data);
  GimpPattern *pattern = NULL;
  GimpData    *data    = NULL;
  GFile       *file    = NULL;

  if (context)
    {
      pattern = gimp_context_get_pattern (context);

      if (action_data_sel_count (user_data) > 1)
        {
          pattern = NULL;
        }

      if (pattern)
        {
          data = GIMP_DATA (pattern);

          file = gimp_data_get_file (data);
        }
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("patterns-edit",                 pattern && FALSE);
  SET_SENSITIVE ("patterns-open-as-image",        file);
  SET_SENSITIVE ("patterns-duplicate",            pattern && gimp_data_is_duplicatable (data));
  SET_SENSITIVE ("patterns-copy-location",        file);
  SET_SENSITIVE ("patterns-show-in-file-manager", file);
  SET_SENSITIVE ("patterns-delete",               pattern && gimp_data_is_deletable (data));

#undef SET_SENSITIVE
}
