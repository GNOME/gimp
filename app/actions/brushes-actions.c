/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligmabrushgenerated.h"
#include "core/ligmacontext.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmahelp-ids.h"

#include "actions.h"
#include "brushes-actions.h"
#include "data-commands.h"

#include "ligma-intl.h"


static const LigmaActionEntry brushes_actions[] =
{
  { "brushes-popup", LIGMA_ICON_BRUSH,
    NC_("brushes-action", "Brushes Menu"), NULL, NULL, NULL,
    LIGMA_HELP_BRUSH_DIALOG },

  { "brushes-open-as-image", LIGMA_ICON_DOCUMENT_OPEN,
    NC_("brushes-action", "_Open Brush as Image"), NULL,
    NC_("brushes-action", "Open brush as image"),
    data_open_as_image_cmd_callback,
    LIGMA_HELP_BRUSH_OPEN_AS_IMAGE },

  { "brushes-new", LIGMA_ICON_DOCUMENT_NEW,
    NC_("brushes-action", "_New Brush"), NULL,
    NC_("brushes-action", "Create a new brush"),
    data_new_cmd_callback,
    LIGMA_HELP_BRUSH_NEW },

  { "brushes-duplicate", LIGMA_ICON_OBJECT_DUPLICATE,
    NC_("brushes-action", "D_uplicate Brush"), NULL,
    NC_("brushes-action", "Duplicate this brush"),
    data_duplicate_cmd_callback,
    LIGMA_HELP_BRUSH_DUPLICATE },

  { "brushes-copy-location", LIGMA_ICON_EDIT_COPY,
    NC_("brushes-action", "Copy Brush _Location"), NULL,
    NC_("brushes-action", "Copy brush file location to clipboard"),
    data_copy_location_cmd_callback,
    LIGMA_HELP_BRUSH_COPY_LOCATION },

  { "brushes-show-in-file-manager", LIGMA_ICON_FILE_MANAGER,
    NC_("brushes-action", "Show in _File Manager"), NULL,
    NC_("brushes-action", "Show brush file location in the file manager"),
    data_show_in_file_manager_cmd_callback,
    LIGMA_HELP_BRUSH_SHOW_IN_FILE_MANAGER },

  { "brushes-delete", LIGMA_ICON_EDIT_DELETE,
    NC_("brushes-action", "_Delete Brush"), NULL,
    NC_("brushes-action", "Delete this brush"),
    data_delete_cmd_callback,
    LIGMA_HELP_BRUSH_DELETE },

  { "brushes-refresh", LIGMA_ICON_VIEW_REFRESH,
    NC_("brushes-action", "_Refresh Brushes"), NULL,
    NC_("brushes-action", "Refresh brushes"),
    data_refresh_cmd_callback,
    LIGMA_HELP_BRUSH_REFRESH }
};

static const LigmaStringActionEntry brushes_edit_actions[] =
{
  { "brushes-edit", LIGMA_ICON_EDIT,
    NC_("brushes-action", "_Edit Brush..."), NULL,
    NC_("brushes-action", "Edit this brush"),
    "ligma-brush-editor",
    LIGMA_HELP_BRUSH_EDIT }
};


void
brushes_actions_setup (LigmaActionGroup *group)
{
  ligma_action_group_add_actions (group, "brushes-action",
                                 brushes_actions,
                                 G_N_ELEMENTS (brushes_actions));

  ligma_action_group_add_string_actions (group, "brushes-action",
                                        brushes_edit_actions,
                                        G_N_ELEMENTS (brushes_edit_actions),
                                        data_edit_cmd_callback);
}

void
brushes_actions_update (LigmaActionGroup *group,
                        gpointer         user_data)
{
  LigmaContext *context = action_data_get_context (user_data);
  LigmaBrush   *brush   = NULL;
  LigmaData    *data    = NULL;
  GFile       *file    = NULL;

  if (context)
    {
      brush = ligma_context_get_brush (context);

      if (action_data_sel_count (user_data) > 1)
        {
          brush = NULL;
        }

      if (brush)
        {
          data = LIGMA_DATA (brush);

          file = ligma_data_get_file (data);
        }
    }

#define SET_SENSITIVE(action,condition) \
        ligma_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_SENSITIVE ("brushes-edit",                 brush);
  SET_SENSITIVE ("brushes-open-as-image",        file && ! LIGMA_IS_BRUSH_GENERATED (brush));
  SET_SENSITIVE ("brushes-duplicate",            brush && ligma_data_is_duplicatable (data));
  SET_SENSITIVE ("brushes-copy-location",        file);
  SET_SENSITIVE ("brushes-show-in-file-manager", file);
  SET_SENSITIVE ("brushes-delete",               brush && ligma_data_is_deletable (data));

#undef SET_SENSITIVE
}
