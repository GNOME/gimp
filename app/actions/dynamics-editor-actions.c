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

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpdataeditor.h"
#include "widgets/gimphelp-ids.h"

#include "data-editor-commands.h"
#include "dynamics-editor-actions.h"

#include "gimp-intl.h"


static const GimpActionEntry dynamics_editor_actions[] =
{
  { "dynamics-editor-popup", GIMP_ICON_DYNAMICS,
    NC_("dynamics-editor-action", "Paint Dynamics Editor Menu"), NULL, NULL, NULL,
    GIMP_HELP_BRUSH_EDITOR_DIALOG }
};


static const GimpToggleActionEntry dynamics_editor_toggle_actions[] =
{
  { "dynamics-editor-edit-active", GIMP_ICON_LINKED,
    NC_("dynamics-editor-action", "Edit Active Dynamics"), NULL, NULL,
    data_editor_edit_active_cmd_callback,
    FALSE,
    GIMP_HELP_BRUSH_EDITOR_EDIT_ACTIVE }
};


void
dynamics_editor_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "dynamics-editor-action",
                                 dynamics_editor_actions,
                                 G_N_ELEMENTS (dynamics_editor_actions));

  gimp_action_group_add_toggle_actions (group, "dynamics-editor-action",
                                        dynamics_editor_toggle_actions,
                                        G_N_ELEMENTS (dynamics_editor_toggle_actions));

}

void
dynamics_editor_actions_update (GimpActionGroup *group,
                                gpointer         user_data)
{
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (user_data);
  gboolean        edit_active = FALSE;

  edit_active = gimp_data_editor_get_edit_active (data_editor);

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)
#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

  SET_ACTIVE ("dynamics-editor-edit-active", edit_active);

#undef SET_SENSITIVE
#undef SET_ACTIVE
}
