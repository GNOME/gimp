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

#include "core/gimpcontext.h"
#include "core/gimpdata.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpcontainereditor.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimphelp-ids.h"

#include "data-commands.h"
#include "patterns-actions.h"

#include "gimp-intl.h"


static GimpActionEntry patterns_actions[] =
{
  { "patterns-popup", GIMP_STOCK_PATTERN, N_("Patterns Menu") },

  { "patterns-edit", GIMP_STOCK_EDIT,
    N_("_Edit Pattern..."), NULL, NULL,
    G_CALLBACK (data_edit_data_cmd_callback),
    GIMP_HELP_PATTERN_EDIT },

  { "patterns-new", GTK_STOCK_NEW,
    N_("_New Pattern"), "", NULL,
    G_CALLBACK (data_new_data_cmd_callback),
    GIMP_HELP_PATTERN_NEW },

  { "patterns-duplicate", GIMP_STOCK_DUPLICATE,
    N_("D_uplicate Pattern"), NULL, NULL,
    G_CALLBACK (data_duplicate_data_cmd_callback),
    GIMP_HELP_PATTERN_DUPLICATE },

  { "patterns-delete", GTK_STOCK_DELETE,
    N_("_Delete Pattern..."), "", NULL,
    G_CALLBACK (data_delete_data_cmd_callback),
    GIMP_HELP_PATTERN_DELETE },

  { "patterns-refresh", GTK_STOCK_REFRESH,
    N_("_Refresh Patterns"), "", NULL,
    G_CALLBACK (data_refresh_data_cmd_callback),
    GIMP_HELP_PATTERN_REFRESH }
};


void
patterns_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 patterns_actions,
                                 G_N_ELEMENTS (patterns_actions));
}

void
patterns_actions_update (GimpActionGroup *group,
                         gpointer         user_data)
{
  GimpContainerEditor *editor;
  GimpPattern         *pattern;
  GimpData            *data = NULL;

  editor = GIMP_CONTAINER_EDITOR (user_data);

  pattern = gimp_context_get_pattern (editor->view->context);

  if (pattern)
    data = GIMP_DATA (pattern);

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("patterns-edit",
		 pattern && GIMP_DATA_FACTORY_VIEW (editor)->data_edit_func);
  SET_SENSITIVE ("patterns-duplicate",
		 pattern && GIMP_DATA_GET_CLASS (data)->duplicate);
  SET_SENSITIVE ("patterns-delete",
		 pattern && data->deletable);

#undef SET_SENSITIVE
}
