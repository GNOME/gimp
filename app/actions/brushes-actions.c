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

#include "core/gimpbrushgenerated.h"
#include "core/gimpcontext.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "brushes-actions.h"
#include "data-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry brushes_actions[] =
{
  { "brushes-open-as-image", GIMP_ICON_DOCUMENT_OPEN,
    NC_("brushes-action", "_Open Brush as Image"), NULL, { NULL },
    NC_("brushes-action", "Open brush as image"),
    data_open_as_image_cmd_callback,
    GIMP_HELP_BRUSH_OPEN_AS_IMAGE },

  { "brushes-new", GIMP_ICON_DOCUMENT_NEW,
    NC_("brushes-action", "_New Brush"), NULL, { NULL },
    NC_("brushes-action", "Create a new brush"),
    data_new_cmd_callback,
    GIMP_HELP_BRUSH_NEW },

  { "brushes-duplicate", GIMP_ICON_OBJECT_DUPLICATE,
    NC_("brushes-action", "D_uplicate Brush"), NULL, { NULL },
    NC_("brushes-action", "Duplicate this brush"),
    data_duplicate_cmd_callback,
    GIMP_HELP_BRUSH_DUPLICATE },

  { "brushes-copy-location", GIMP_ICON_EDIT_COPY,
    NC_("brushes-action", "Copy Brush _Location"), NULL, { NULL },
    NC_("brushes-action", "Copy brush file location to clipboard"),
    data_copy_location_cmd_callback,
    GIMP_HELP_BRUSH_COPY_LOCATION },

  { "brushes-show-in-file-manager", GIMP_ICON_FILE_MANAGER,
    NC_("brushes-action", "Show in _File Manager"), NULL, { NULL },
    NC_("brushes-action", "Show brush file location in the file manager"),
    data_show_in_file_manager_cmd_callback,
    GIMP_HELP_BRUSH_SHOW_IN_FILE_MANAGER },

  { "brushes-delete", GIMP_ICON_EDIT_DELETE,
    NC_("brushes-action", "_Delete Brush"), NULL, { NULL },
    NC_("brushes-action", "Delete this brush"),
    data_delete_cmd_callback,
    GIMP_HELP_BRUSH_DELETE },

  { "brushes-refresh", GIMP_ICON_VIEW_REFRESH,
    NC_("brushes-action", "_Refresh Brushes"), NULL, { NULL },
    NC_("brushes-action", "Refresh brushes"),
    data_refresh_cmd_callback,
    GIMP_HELP_BRUSH_REFRESH }
};

static const GimpStringActionEntry brushes_edit_actions[] =
{
  { "brushes-edit", GIMP_ICON_EDIT,
    NC_("brushes-action", "_Edit Brush..."), NULL, { NULL },
    NC_("brushes-action", "Edit this brush"),
    "gimp-brush-editor",
    GIMP_HELP_BRUSH_EDIT }
};


void
brushes_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "brushes-action",
                                 brushes_actions,
                                 G_N_ELEMENTS (brushes_actions));

  gimp_action_group_add_string_actions (group, "brushes-action",
                                        brushes_edit_actions,
                                        G_N_ELEMENTS (brushes_edit_actions),
                                        data_edit_cmd_callback);
}

void
brushes_actions_update (GimpActionGroup *group,
                        gpointer         user_data)
{
  GimpContext *context = action_data_get_context (user_data);
  GimpBrush   *brush   = NULL;
  GimpData    *data    = NULL;
  GFile       *file    = NULL;

  if (context)
    {
      brush = gimp_context_get_brush (context);

      if (action_data_sel_count (user_data) > 1)
        {
          brush = NULL;
        }

      if (brush)
        {
          data = GIMP_DATA (brush);

          file = gimp_data_get_file (data);
        }
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_SENSITIVE ("brushes-edit",                 brush);
  SET_SENSITIVE ("brushes-open-as-image",        file && ! GIMP_IS_BRUSH_GENERATED (brush));
  SET_SENSITIVE ("brushes-duplicate",            brush && gimp_data_is_duplicatable (data));
  SET_SENSITIVE ("brushes-copy-location",        file);
  SET_SENSITIVE ("brushes-show-in-file-manager", file);
  SET_SENSITIVE ("brushes-delete",               brush && gimp_data_is_deletable (data));

#undef SET_SENSITIVE
}
