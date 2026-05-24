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

#include "help-actions.h"
#include "help-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry help_actions[] =
{
  { "help-help", "gimp-prefs-help-system",
    NC_("help-action", "_Help"), NULL, { "F1", NULL },
    NC_("help-action", "Open the GIMP user manual"),
    help_help_cmd_callback,
    GIMP_HELP_HELP },

  { "help-context-help", "gimp-prefs-help-system",
    NC_("help-action", "_Context Help"), NULL, { "<shift>F1", NULL },
    NC_("help-action", "Show the help for a specific user interface item"),
    help_context_help_cmd_callback,
    GIMP_HELP_HELP_CONTEXT }
};

static const GimpStringActionEntry help_online_actions[] =
{
  { "help-online-main-web-site", GIMP_ICON_WEB,
    NC_("help-action", "_Main Web Site"), NULL, { NULL },
    NC_("help-action", "Bookmark to the GIMP web site"),
    "https://www.gimp.org/",
    GIMP_HELP_HELP },

  { "help-online-docs-web-site", GIMP_ICON_WEB,
    NC_("help-action", "_User Manual Web Site"), NULL, { NULL },
    NC_("help-action", "Bookmark to the Documentation web site"),
    "https://docs.gimp.org/",
    GIMP_HELP_HELP },

  { "help-online-bug-tracker", GIMP_ICON_WEB,
    NC_("help-action", "_Bug Reports"), NULL, { NULL },
    NC_("help-action", "Bookmark to the bug tracker of GIMP"),
    "https://gitlab.gnome.org/GNOME/gimp/issues",
    GIMP_HELP_HELP },

  { "help-online-developer-web-site", GIMP_ICON_WEB,
    NC_("help-action", "_Developer Web Site"), NULL, { NULL },
    NC_("help-action", "Bookmark to the Developer web site"),
    "https://developer.gimp.org/",
    GIMP_HELP_HELP }
};

void
help_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "help-action",
                                 help_actions,
                                 G_N_ELEMENTS (help_actions));
  gimp_action_group_add_string_actions (group, "help-action",
                                        help_online_actions,
                                        G_N_ELEMENTS (help_online_actions),
                                        help_online_cmd_callback);
}

void
help_actions_update (GimpActionGroup *group,
                     gpointer         data)
{
}
