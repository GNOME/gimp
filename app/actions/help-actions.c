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
#include "widgets/gimphelp-ids.h"

#include "help-actions.h"
#include "help-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry help_actions[] =
{
  { "help-menu", NULL, N_("_Help") },

  { "help-help", GTK_STOCK_HELP,
    N_("_Help"), "F1", NULL,
    G_CALLBACK (help_help_cmd_callback),
    GIMP_HELP_HELP },

  { "help-context-help", GTK_STOCK_HELP,
    N_("_Context Help"), "<shift>F1", NULL,
    G_CALLBACK (help_context_help_cmd_callback),
    GIMP_HELP_HELP_CONTEXT }
};


void
help_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 help_actions,
                                 G_N_ELEMENTS (help_actions));
}

void
help_actions_update (GimpActionGroup *group,
                     gpointer         data)
{
}
