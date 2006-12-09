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

#include "core/gimpcontext.h"
#include "core/gimpdata.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "data-commands.h"
#include "patterns-actions.h"

#include "gimp-intl.h"


static const GimpActionEntry patterns_actions[] =
{
  { "patterns-popup", GIMP_STOCK_PATTERN,
    N_("Patterns Menu"), NULL, NULL, NULL,
    GIMP_HELP_PATTERN_DIALOG },

  { "patterns-open-as-image", GTK_STOCK_OPEN,
    N_("_Open Pattern as Image"), "",
    N_("Open pattern as image"),
    G_CALLBACK (data_open_as_image_cmd_callback),
    GIMP_HELP_PATTERN_OPEN_AS_IMAGE },

  { "patterns-new", GTK_STOCK_NEW,
    N_("_New Pattern"), "",
    N_("New pattern"),
    G_CALLBACK (data_new_cmd_callback),
    GIMP_HELP_PATTERN_NEW },

  { "patterns-duplicate", GIMP_STOCK_DUPLICATE,
    N_("D_uplicate Pattern"), NULL,
    N_("Duplicate pattern"),
    G_CALLBACK (data_duplicate_cmd_callback),
    GIMP_HELP_PATTERN_DUPLICATE },

  { "patterns-copy-location", GTK_STOCK_COPY,
    N_("Copy Pattern _Location"), "",
    N_("Copy pattern file location to clipboard"),
    G_CALLBACK (data_copy_location_cmd_callback),
    GIMP_HELP_PATTERN_COPY_LOCATION },

  { "patterns-delete", GTK_STOCK_DELETE,
    N_("_Delete Pattern"), "",
    N_("Delete pattern"),
    G_CALLBACK (data_delete_cmd_callback),
    GIMP_HELP_PATTERN_DELETE },

  { "patterns-refresh", GTK_STOCK_REFRESH,
    N_("_Refresh Patterns"), "",
    N_("Refresh patterns"),
    G_CALLBACK (data_refresh_cmd_callback),
    GIMP_HELP_PATTERN_REFRESH }
};

static const GimpStringActionEntry patterns_edit_actions[] =
{
  { "patterns-edit", GTK_STOCK_EDIT,
    N_("_Edit Pattern..."), NULL,
    N_("Edit pattern"),
    "gimp-pattern-editor",
    GIMP_HELP_PATTERN_EDIT }
};


void
patterns_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 patterns_actions,
                                 G_N_ELEMENTS (patterns_actions));

  gimp_action_group_add_string_actions (group,
                                        patterns_edit_actions,
                                        G_N_ELEMENTS (patterns_edit_actions),
                                        G_CALLBACK (data_edit_cmd_callback));
}

void
patterns_actions_update (GimpActionGroup *group,
                         gpointer         user_data)
{
  GimpContext *context = action_data_get_context (user_data);
  GimpPattern *pattern = NULL;
  GimpData    *data    = NULL;

  if (context)
    {
      pattern = gimp_context_get_pattern (context);

      if (pattern)
        data = GIMP_DATA (pattern);
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("patterns-edit",          pattern && FALSE);
  SET_SENSITIVE ("patterns-open-as-image", pattern && data->filename);
  SET_SENSITIVE ("patterns-duplicate",     pattern && GIMP_DATA_GET_CLASS (data)->duplicate);
  SET_SENSITIVE ("patterns-copy-location", pattern && data->filename);
  SET_SENSITIVE ("patterns-delete",        pattern && data->deletable);

#undef SET_SENSITIVE
}
