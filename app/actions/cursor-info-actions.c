/* GIMP - The GNU Image Manipulation Program
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
#include "widgets/gimpcursorview.h"
#include "widgets/gimphelp-ids.h"

#include "cursor-info-actions.h"
#include "cursor-info-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry cursor_info_actions[] =
{
  { "cursor-info-popup", GIMP_STOCK_CURSOR,
    N_("Pointer Information Menu"), NULL, NULL, NULL,
    GIMP_HELP_POINTER_INFO_DIALOG }
};

static const GimpToggleActionEntry cursor_info_toggle_actions[] =
{
  { "cursor-info-sample-merged", NULL,
    N_("_Sample Merged"), "",
    N_("Sample Merged"),
    G_CALLBACK (cursor_info_sample_merged_cmd_callback),
    TRUE,
    GIMP_HELP_POINTER_INFO_SAMPLE_MERGED }
};


void
cursor_info_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 cursor_info_actions,
                                 G_N_ELEMENTS (cursor_info_actions));

  gimp_action_group_add_toggle_actions (group,
                                        cursor_info_toggle_actions,
                                        G_N_ELEMENTS (cursor_info_toggle_actions));
}

void
cursor_info_actions_update (GimpActionGroup *group,
                            gpointer         data)
{
  GimpCursorView *view = GIMP_CURSOR_VIEW (data);

#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

  SET_ACTIVE ("cursor-info-sample-merged",
              gimp_cursor_view_get_sample_merged (view));

#undef SET_ACTIVE
}
