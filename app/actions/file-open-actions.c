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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "file-dialog-actions.h"
#include "file-dialog-commands.h"
#include "file-open-actions.h"

#include "gimp-intl.h"


static GimpActionEntry file_open_actions[] =
{
  { "file-open-popup", NULL, N_("File Open Menu"), NULL, NULL, NULL,
    GIMP_HELP_FILE_OPEN }
};

static GimpPlugInActionEntry file_open_file_type_actions[] =
{
  { "file-open-automatic", NULL,
    N_("Automatic"), NULL, NULL,
    NULL,
    GIMP_HELP_FILE_OPEN_BY_EXTENSION }
};


void
file_open_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 file_open_actions,
                                 G_N_ELEMENTS (file_open_actions));

  gimp_action_group_add_plug_in_actions (group,
                                         file_open_file_type_actions,
                                         G_N_ELEMENTS (file_open_file_type_actions),
                                         G_CALLBACK (file_dialog_type_cmd_callback));

  file_dialog_actions_setup (group, group->gimp->load_procs, "gimp_xcf_load");
}

void
file_open_actions_update (GimpActionGroup *group,
                          gpointer         data)
{
}
