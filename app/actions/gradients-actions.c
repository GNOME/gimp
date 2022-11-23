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

#include "core/ligmacontext.h"
#include "core/ligmadata.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmahelp-ids.h"

#include "actions.h"
#include "data-commands.h"
#include "gradients-actions.h"
#include "gradients-commands.h"

#include "ligma-intl.h"


static const LigmaActionEntry gradients_actions[] =
{
  { "gradients-popup", LIGMA_ICON_GRADIENT,
    NC_("gradients-action", "Gradients Menu"), NULL, NULL, NULL,
    LIGMA_HELP_GRADIENT_DIALOG },

  { "gradients-new", LIGMA_ICON_DOCUMENT_NEW,
    NC_("gradients-action", "_New Gradient"), NULL,
    NC_("gradients-action", "Create a new gradient"),
    data_new_cmd_callback,
    LIGMA_HELP_GRADIENT_NEW },

  { "gradients-duplicate", LIGMA_ICON_OBJECT_DUPLICATE,
    NC_("gradients-action", "D_uplicate Gradient"), NULL,
    NC_("gradients-action", "Duplicate this gradient"),
    data_duplicate_cmd_callback,
    LIGMA_HELP_GRADIENT_DUPLICATE },

  { "gradients-copy-location", LIGMA_ICON_EDIT_COPY,
    NC_("gradients-action", "Copy Gradient _Location"), NULL,
    NC_("gradients-action", "Copy gradient file location to clipboard"),
    data_copy_location_cmd_callback,
    LIGMA_HELP_GRADIENT_COPY_LOCATION },

  { "gradients-show-in-file-manager", LIGMA_ICON_FILE_MANAGER,
    NC_("gradients-action", "Show in _File Manager"), NULL,
    NC_("gradients-action", "Show gradient file location in the file manager"),
    data_show_in_file_manager_cmd_callback,
    LIGMA_HELP_GRADIENT_SHOW_IN_FILE_MANAGER },

  { "gradients-save-as-pov", LIGMA_ICON_DOCUMENT_SAVE_AS,
    NC_("gradients-action", "Save as _POV-Ray..."), NULL,
    NC_("gradients-action", "Save gradient as POV-Ray"),
    gradients_save_as_pov_ray_cmd_callback,
    LIGMA_HELP_GRADIENT_SAVE_AS_POV },

  { "gradients-delete", LIGMA_ICON_EDIT_DELETE,
    NC_("gradients-action", "_Delete Gradient"), NULL,
    NC_("gradients-action", "Delete this gradient"),
    data_delete_cmd_callback,
    LIGMA_HELP_GRADIENT_DELETE },

  { "gradients-refresh", LIGMA_ICON_VIEW_REFRESH,
    NC_("gradients-action", "_Refresh Gradients"), NULL,
    NC_("gradients-action", "Refresh gradients"),
    data_refresh_cmd_callback,
    LIGMA_HELP_GRADIENT_REFRESH }
};

static const LigmaStringActionEntry gradients_edit_actions[] =
{
  { "gradients-edit", LIGMA_ICON_EDIT,
    NC_("gradients-action", "_Edit Gradient..."), NULL,
    NC_("gradients-action", "Edit this gradient"),
    "ligma-gradient-editor",
    LIGMA_HELP_GRADIENT_EDIT }
};


void
gradients_actions_setup (LigmaActionGroup *group)
{
  ligma_action_group_add_actions (group, "gradients-action",
                                 gradients_actions,
                                 G_N_ELEMENTS (gradients_actions));

  ligma_action_group_add_string_actions (group, "gradients-action",
                                        gradients_edit_actions,
                                        G_N_ELEMENTS (gradients_edit_actions),
                                        data_edit_cmd_callback);
}

void
gradients_actions_update (LigmaActionGroup *group,
                          gpointer         user_data)
{
  LigmaContext  *context  = action_data_get_context (user_data);
  LigmaGradient *gradient = NULL;
  LigmaData     *data     = NULL;
  GFile        *file     = NULL;

  if (context)
    {
      gradient = ligma_context_get_gradient (context);

      if (action_data_sel_count (user_data) > 1)
        {
          gradient = NULL;
        }

      if (gradient)
        {
          data = LIGMA_DATA (gradient);

          file = ligma_data_get_file (data);
        }
    }

#define SET_SENSITIVE(action,condition) \
        ligma_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_SENSITIVE ("gradients-edit",                 gradient);
  SET_SENSITIVE ("gradients-duplicate",            gradient);
  SET_SENSITIVE ("gradients-save-as-pov",          gradient);
  SET_SENSITIVE ("gradients-copy-location",        file);
  SET_SENSITIVE ("gradients-show-in-file-manager", file);
  SET_SENSITIVE ("gradients-delete",               gradient && ligma_data_is_deletable (data));

#undef SET_SENSITIVE
}
