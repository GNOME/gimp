/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpsamplepointeditor.h"

#include "sample-point-editor-actions.h"
#include "sample-point-editor-commands.h"

#include "gimp-intl.h"


static GimpActionEntry sample_point_editor_actions[] =
{
  { "sample-point-editor-popup", GIMP_STOCK_SAMPLE_POINT,
    N_("Sample Point Menu"), NULL, NULL, NULL,
    GIMP_HELP_SAMPLE_POINT_DIALOG }
};

static GimpToggleActionEntry sample_point_editor_toggle_actions[] =
{
  { "sample-point-editor-sample-merged", NULL,
    N_("_Sample Merged"), "",
    N_("Sample Merged"),
    G_CALLBACK (sample_point_editor_sample_merged_cmd_callback),
    TRUE,
    GIMP_HELP_SAMPLE_POINT_SAMPLE_MERGED }
};


void
sample_point_editor_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 sample_point_editor_actions,
                                 G_N_ELEMENTS (sample_point_editor_actions));

  gimp_action_group_add_toggle_actions (group,
                                        sample_point_editor_toggle_actions,
                                        G_N_ELEMENTS (sample_point_editor_toggle_actions));
}

void
sample_point_editor_actions_update (GimpActionGroup *group,
                                    gpointer         data)
{
  GimpSamplePointEditor *editor = GIMP_SAMPLE_POINT_EDITOR (data);

#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

  SET_ACTIVE ("sample-point-editor-sample-merged",
              gimp_sample_point_editor_get_sample_merged (editor));

#undef SET_ACTIVE
}
