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
#include "file-save-actions.h"

#include "gimp-intl.h"


static GimpActionEntry file_save_actions[] =
{
  { "file-save-popup", NULL, N_("File Save Menu"), NULL, NULL, NULL,
    GIMP_HELP_FILE_SAVE }
};

static GimpPlugInActionEntry file_save_file_type_actions[] =
{
  { "file-save-by-extension", NULL,
    N_("By Extension"), NULL, NULL,
    NULL,
    GIMP_HELP_FILE_SAVE_BY_EXTENSION }
};


void
file_save_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 file_save_actions,
                                 G_N_ELEMENTS (file_save_actions));

  gimp_action_group_add_plug_in_actions (group,
                                         file_save_file_type_actions,
                                         G_N_ELEMENTS (file_save_file_type_actions),
                                         G_CALLBACK (file_dialog_type_cmd_callback));

  file_dialog_actions_setup (group, group->gimp->save_procs, "gimp_xcf_save");
}

void
file_save_actions_update (GimpActionGroup *group,
                          gpointer         data)
{
}
