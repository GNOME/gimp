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

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmahelp-ids.h"

#include "display/ligmacursorview.h"

#include "cursor-info-actions.h"
#include "cursor-info-commands.h"

#include "ligma-intl.h"


static const LigmaActionEntry cursor_info_actions[] =
{
  { "cursor-info-popup", LIGMA_ICON_CURSOR,
    NC_("cursor-info-action", "Pointer Information Menu"), NULL, NULL, NULL,
    LIGMA_HELP_POINTER_INFO_DIALOG }
};

static const LigmaToggleActionEntry cursor_info_toggle_actions[] =
{
  { "cursor-info-sample-merged", NULL,
    NC_("cursor-info-action", "_Sample Merged"), "",
    NC_("cursor-info-action", "Use the composite color of all visible layers"),
    cursor_info_sample_merged_cmd_callback,
    TRUE,
    LIGMA_HELP_POINTER_INFO_SAMPLE_MERGED }
};


void
cursor_info_actions_setup (LigmaActionGroup *group)
{
  ligma_action_group_add_actions (group, "cursor-info-action",
                                 cursor_info_actions,
                                 G_N_ELEMENTS (cursor_info_actions));

  ligma_action_group_add_toggle_actions (group, "cursor-info-action",
                                        cursor_info_toggle_actions,
                                        G_N_ELEMENTS (cursor_info_toggle_actions));
}

void
cursor_info_actions_update (LigmaActionGroup *group,
                            gpointer         data)
{
  LigmaCursorView *view = LIGMA_CURSOR_VIEW (data);

#define SET_ACTIVE(action,condition) \
        ligma_action_group_set_action_active (group, action, (condition) != 0)

  SET_ACTIVE ("cursor-info-sample-merged",
              ligma_cursor_view_get_sample_merged (view));

#undef SET_ACTIVE
}
