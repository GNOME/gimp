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

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpsamplepointeditor.h"

#include "sample-points-actions.h"
#include "sample-points-commands.h"

#include "gimp-intl.h"


static const GimpToggleActionEntry sample_points_toggle_actions[] =
{
  { "sample-points-sample-merged", NULL,
    NC_("sample-points-action", "_Sample Merged"), NULL, { NULL },
    NC_("sample-points-action",
        "Use the composite color of all visible layers"),
    sample_points_sample_merged_cmd_callback,
    TRUE,
    GIMP_HELP_SAMPLE_POINT_SAMPLE_MERGED }
};


void
sample_points_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_toggle_actions (group, "sample-points-action",
                                        sample_points_toggle_actions,
                                        G_N_ELEMENTS (sample_points_toggle_actions));
}

void
sample_points_actions_update (GimpActionGroup *group,
                              gpointer         data)
{
  GimpSamplePointEditor *editor = GIMP_SAMPLE_POINT_EDITOR (data);

#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

  SET_ACTIVE ("sample-points-sample-merged",
              gimp_sample_point_editor_get_sample_merged (editor));

#undef SET_ACTIVE
}
