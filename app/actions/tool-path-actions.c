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

#include "tool-path-actions.h"
#include "tool-path-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry tool_path_actions[] =
{
  { "tool-path-delete-anchor", GIMP_ICON_PATH,
    NC_("tool-path-action", "_Delete Anchor"), NULL, { NULL }, NULL,
    tool_path_delete_anchor_cmd_callback,
    GIMP_HELP_PATH_TOOL_DELETE_ANCHOR },
  { "tool-path-shift-start", GIMP_ICON_PATH,
    NC_("tool-path-action", "Shift S_tart"), NULL, { NULL }, NULL,
    tool_path_shift_start_cmd_callback,
    GIMP_HELP_PATH_TOOL_SHIFT_START },

  { "tool-path-insert-anchor", GIMP_ICON_PATH,
    NC_("tool-path-action", "_Insert Anchor"), NULL, { NULL }, NULL,
    tool_path_insert_anchor_cmd_callback,
    GIMP_HELP_PATH_TOOL_INSERT_ANCHOR },
  { "tool-path-delete-segment", GIMP_ICON_PATH,
    NC_("tool-path-action", "Delete _Segment"), NULL, { NULL }, NULL,
    tool_path_delete_segment_cmd_callback,
    GIMP_HELP_PATH_TOOL_DELETE_SEGMENT },
  { "tool-path-reverse-stroke", GIMP_ICON_PATH,
    NC_("tool-path-action", "_Reverse Stroke"), NULL, { NULL }, NULL,
    tool_path_reverse_stroke_cmd_callback,
    GIMP_HELP_PATH_TOOL_REVERSE_STROKE }
};


#define SET_HIDE_EMPTY(action,condition) \
        gimp_action_group_set_action_hide_empty (group, action, (condition) != 0)

void
tool_path_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "tool-path-action",
                                 tool_path_actions,
                                 G_N_ELEMENTS (tool_path_actions));
}

/* The following code is written on the assumption that this is for a
 * context menu, activated by right-clicking on a tool-path widget.
 * Therefore, the tool must have a display.  If for any reason the
 * code is adapted to a different situation, some existence testing
 * will need to be added.
 */
void
tool_path_actions_update (GimpActionGroup *group,
                          gpointer         data)
{
  GimpToolPath *tool_path = GIMP_TOOL_PATH (data);
  gboolean      on_handle;
  gboolean      on_curve;

  gimp_tool_path_get_popup_state (tool_path, &on_handle, &on_curve);

#define SET_VISIBLE(action,condition) \
        gimp_action_group_set_action_visible (group, action, (condition) != 0)
#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)
#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

  SET_VISIBLE ("tool-path-shift-start",    on_handle);
  SET_VISIBLE ("tool-path-delete-anchor",  on_handle);

  SET_VISIBLE ("tool-path-insert-anchor",  !on_handle && on_curve);
  SET_VISIBLE ("tool-path-delete-segment", !on_handle && on_curve);
  SET_VISIBLE ("tool-path-reverse-stroke", on_curve);

}
