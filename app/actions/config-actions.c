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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "actions-types.h"

#include "core/gimp.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "config-actions.h"
#include "config-commands.h"

#include "gimp-intl.h"


static const GimpToggleActionEntry config_toggle_actions[] =
{
  { "config-use-gegl", NULL,
    NC_("config-action", "Use _GEGL"), NULL,
    NC_("config-action", "If possible, use GEGL for image processing"),
    G_CALLBACK (config_use_gegl_cmd_callback),
    FALSE,
    GIMP_HELP_CONFIG_USE_GEGL }
};


static void
config_actions_use_gegl_notify (GObject         *config,
                                GParamSpec      *pspec,
                                GimpActionGroup *group)
{
  gboolean active;

  g_object_get (config,
                "use-gegl", &active,
                NULL);

  gimp_action_group_set_action_active (group, "config-use-gegl", active);
}

void
config_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_toggle_actions (group, "config-action",
                                        config_toggle_actions,
                                        G_N_ELEMENTS (config_toggle_actions));

  config_actions_use_gegl_notify (G_OBJECT (group->gimp->config), NULL, group);

  g_signal_connect_object (group->gimp->config,
                           "notify::use-gegl",
                           G_CALLBACK (config_actions_use_gegl_notify),
                           group, 0);
}

void
config_actions_update (GimpActionGroup *group,
                       gpointer         data)
{
}
