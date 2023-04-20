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
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpbrusheditor.h"

#include "brush-editor-actions.h"
#include "data-editor-commands.h"

#include "gimp-intl.h"


static const GimpToggleActionEntry brush_editor_toggle_actions[] =
{
  { "brush-editor-edit-active", GIMP_ICON_LINKED,
    NC_("brush-editor-action", "Edit Active Brush"), NULL, { NULL }, NULL,
    data_editor_edit_active_cmd_callback,
    FALSE,
    GIMP_HELP_BRUSH_EDITOR_EDIT_ACTIVE }
};


void
brush_editor_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_toggle_actions (group, "brush-editor-action",
                                        brush_editor_toggle_actions,
                                        G_N_ELEMENTS (brush_editor_toggle_actions));
}

void
brush_editor_actions_update (GimpActionGroup *group,
                             gpointer         user_data)
{
  GimpDataEditor  *data_editor = GIMP_DATA_EDITOR (user_data);
  gboolean         edit_active = FALSE;

  edit_active = gimp_data_editor_get_edit_active (data_editor);

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)
#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

  SET_ACTIVE ("brush-editor-edit-active", edit_active);

#undef SET_SENSITIVE
#undef SET_ACTIVE
}
