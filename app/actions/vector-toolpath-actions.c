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

#include "core/gimpimage.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimptoolpath.h"

#include "vector-toolpath-actions.h"
#include "vector-toolpath-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry vector_toolpath_actions[] =
{
  { "vector-tool-popup", NULL,
    NC_("vector-toolpath-action", "Vector Toolpath Menu"), NULL, NULL, NULL,
    NULL },

  { "vector-toolpath-reverse-stroke", GIMP_ICON_PATH,
    NC_("vector-toolpath-action", "_Reverse Stroke"), NULL, NULL,
    vector_toolpath_reverse_stroke_cmd_callback,
    NULL }
};


#define SET_HIDE_EMPTY(action,condition) \
        gimp_action_group_set_action_hide_empty (group, action, (condition) != 0)

void
vector_toolpath_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "vector-toolpath-action",
                                 vector_toolpath_actions,
                                 G_N_ELEMENTS (vector_toolpath_actions));
}

/* The following code is written on the assumption that this is for a
 * context menu, activated by right-clicking in a text layer.
 * Therefore, the tool must have a display.  If for any reason the
 * code is adapted to a different situation, some existence testing
 * will need to be added.
 */
void
vector_toolpath_actions_update (GimpActionGroup *group,
                                gpointer         data)
{
  GimpToolPath *toolpath = GIMP_TOOL_PATH (data);

#define SET_VISIBLE(action,condition) \
        gimp_action_group_set_action_visible (group, action, (condition) != 0)
#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)
#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

  SET_SENSITIVE ("vector-toolpath-reverse-stroke", TRUE);
}
