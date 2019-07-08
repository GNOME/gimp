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

#include "core/gimpcontext.h"

#include "text/gimpfont.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "data-commands.h"
#include "fonts-actions.h"

#include "gimp-intl.h"


static const GimpActionEntry fonts_actions[] =
{
  { "fonts-popup", GIMP_ICON_FONT,
    NC_("fonts-action", "Fonts Menu"), NULL, NULL, NULL,
    GIMP_HELP_FONT_DIALOG },

  { "fonts-refresh", GIMP_ICON_VIEW_REFRESH,
    NC_("fonts-action", "_Rescan Font List"), NULL,
    NC_("fonts-action", "Rescan the installed fonts"),
    data_refresh_cmd_callback,
    GIMP_HELP_FONT_REFRESH }
};


void
fonts_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "fonts-action",
                                 fonts_actions,
                                 G_N_ELEMENTS (fonts_actions));
}

void
fonts_actions_update (GimpActionGroup *group,
                      gpointer         data)
{
#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("fonts-refresh", TRUE);

#undef SET_SENSITIVE
}
