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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimperrorconsole.h"
#include "widgets/gimphelp-ids.h"

#include "error-console-actions.h"
#include "error-console-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry error_console_actions[] =
{
  { "error-console-popup", GIMP_STOCK_WARNING,
    NC_("error-console-action", "Error Console Menu"), NULL, NULL, NULL,
    GIMP_HELP_ERRORS_DIALOG },

  { "error-console-clear", "edit-clear",
    NC_("error-console-action", "_Clear"), NULL,
    NC_("error-console-action", "Clear error console"),
    G_CALLBACK (error_console_clear_cmd_callback),
    GIMP_HELP_ERRORS_CLEAR },

  { "error-console-select-all", NULL,
    NC_("error-console-action", "Select _All"), "",
    NC_("error-console-action", "Select all error messages"),
    G_CALLBACK (error_console_select_all_cmd_callback),
    GIMP_HELP_ERRORS_SELECT_ALL }
};

static const GimpEnumActionEntry error_console_save_actions[] =
{
  { "error-console-save-all", "document-save-as",
    NC_("error-console-action", "_Save Error Log to File..."), NULL,
    NC_("error-console-action", "Write all error messages to a file"),
    FALSE, FALSE,
    GIMP_HELP_ERRORS_SAVE },

  { "error-console-save-selection", "document-save-as",
    NC_("error-console-action", "Save S_election to File..."), NULL,
    NC_("error-console-action", "Write the selected error messages to a file"),
    TRUE, FALSE,
    GIMP_HELP_ERRORS_SAVE }
};


void
error_console_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "error-console-action",
                                 error_console_actions,
                                 G_N_ELEMENTS (error_console_actions));

  gimp_action_group_add_enum_actions (group, "error-console-action",
                                      error_console_save_actions,
                                      G_N_ELEMENTS (error_console_save_actions),
                                      G_CALLBACK (error_console_save_cmd_callback));
}

void
error_console_actions_update (GimpActionGroup *group,
                              gpointer         data)
{
  GimpErrorConsole *console = GIMP_ERROR_CONSOLE (data);
  gboolean          selection;

  selection = gtk_text_buffer_get_selection_bounds (console->text_buffer,
                                                    NULL, NULL);

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("error-console-clear",          TRUE);
  SET_SENSITIVE ("error-console-select-all",     TRUE);
  SET_SENSITIVE ("error-console-save-all",       TRUE);
  SET_SENSITIVE ("error-console-save-selection", selection);

#undef SET_SENSITIVE
}
