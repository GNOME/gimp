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

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "data-commands.h"
#include "dynamics-actions.h"

#include "gimp-intl.h"


static const GimpActionEntry dynamics_actions[] =
{
  { "dynamics-popup", GIMP_STOCK_DYNAMICS,
    NC_("dynamics-action", "Paint Dynamics Menu"), NULL, NULL, NULL,
    GIMP_HELP_DYNAMICS_DIALOG },

  { "dynamics-new", GTK_STOCK_NEW,
    NC_("dynamics-action", "_New Dynamics"), "",
    NC_("dynamics-action", "Create a new dynamics"),
    G_CALLBACK (data_new_cmd_callback),
    GIMP_HELP_DYNAMICS_NEW },

  { "dynamics-duplicate", GIMP_STOCK_DUPLICATE,
    NC_("dynamics-action", "D_uplicate Dynamics"), NULL,
    NC_("dynamics-action", "Duplicate this dynamics"),
    G_CALLBACK (data_duplicate_cmd_callback),
    GIMP_HELP_DYNAMICS_DUPLICATE },

  { "dynamics-copy-location", GTK_STOCK_COPY,
    NC_("dynamics-action", "Copy Dynamics _Location"), "",
    NC_("dynamics-action", "Copy dynamics file location to clipboard"),
    G_CALLBACK (data_copy_location_cmd_callback),
    GIMP_HELP_DYNAMICS_COPY_LOCATION },

  { "dynamics-delete", GTK_STOCK_DELETE,
    NC_("dynamics-action", "_Delete Dynamics"), "",
    NC_("dynamics-action", "Delete this dynamics"),
    G_CALLBACK (data_delete_cmd_callback),
    GIMP_HELP_DYNAMICS_DELETE },

  { "dynamics-refresh", GTK_STOCK_REFRESH,
    NC_("dynamics-action", "_Refresh Dynamics"), "",
    NC_("dynamics-action", "Refresh dynamics"),
    G_CALLBACK (data_refresh_cmd_callback),
    GIMP_HELP_DYNAMICS_REFRESH }
};

static const GimpStringActionEntry dynamics_edit_actions[] =
{
  { "dynamics-edit", GTK_STOCK_EDIT,
    NC_("dynamics-action", "_Edit Dynamics..."), NULL,
    NC_("dynamics-action", "Edit dynamics"),
    "gimp-dynamics-editor",
    GIMP_HELP_DYNAMICS_EDIT }
};


void
dynamics_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "dynamics-action",
                                 dynamics_actions,
                                 G_N_ELEMENTS (dynamics_actions));

  gimp_action_group_add_string_actions (group, "dynamics-action",
                                        dynamics_edit_actions,
                                        G_N_ELEMENTS (dynamics_edit_actions),
                                        G_CALLBACK (data_edit_cmd_callback));
}

void
dynamics_actions_update (GimpActionGroup *group,
                         gpointer         user_data)
{
  GimpContext  *context  = action_data_get_context (user_data);
  GimpDynamics *dynamics = NULL;
  GimpData     *data     = NULL;
  const gchar  *filename = NULL;

  if (context)
    {
      dynamics = gimp_context_get_dynamics (context);

      if (dynamics)
        {
          data = GIMP_DATA (dynamics);

          filename = gimp_data_get_filename (data);
        }
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("dynamics-edit",          dynamics);
  SET_SENSITIVE ("dynamics-duplicate",     dynamics && GIMP_DATA_GET_CLASS (data)->duplicate);
  SET_SENSITIVE ("dynamics-copy-location", dynamics && filename);
  SET_SENSITIVE ("dynamics-delete",        dynamics && gimp_data_is_deletable (data));

#undef SET_SENSITIVE
}
