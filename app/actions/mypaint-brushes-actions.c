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
#include "core/gimpmybrush.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "data-commands.h"
#include "mypaint-brushes-actions.h"

#include "gimp-intl.h"


static const GimpActionEntry mypaint_brushes_actions[] =
{
  { "mypaint-brushes-popup", GIMP_ICON_MYPAINT_BRUSH,
    NC_("mypaint-brushes-action", "MyPaint Brushes Menu"), NULL, NULL, NULL,
    GIMP_HELP_MYPAINT_BRUSH_DIALOG },

  { "mypaint-brushes-new", GIMP_ICON_DOCUMENT_NEW,
    NC_("mypaint-brushes-action", "_New MyPaint Brush"), NULL,
    NC_("mypaint-brushes-action", "Create a new MyPaint brush"),
    data_new_cmd_callback,
    GIMP_HELP_MYPAINT_BRUSH_NEW },

  { "mypaint-brushes-duplicate", GIMP_ICON_OBJECT_DUPLICATE,
    NC_("mypaint-brushes-action", "D_uplicate MyPaint Brush"), NULL,
    NC_("mypaint-brushes-action", "Duplicate this MyPaint brush"),
    data_duplicate_cmd_callback,
    GIMP_HELP_MYPAINT_BRUSH_DUPLICATE },

  { "mypaint-brushes-copy-location", GIMP_ICON_EDIT_COPY,
    NC_("mypaint-brushes-action", "Copy MyPaint Brush _Location"), NULL,
    NC_("mypaint-brushes-action", "Copy MyPaint brush file location to clipboard"),
    data_copy_location_cmd_callback,
    GIMP_HELP_MYPAINT_BRUSH_COPY_LOCATION },

  { "mypaint-brushes-show-in-file-manager", GIMP_ICON_FILE_MANAGER,
    NC_("mypaint-brushes-action", "Show in _File Manager"), NULL,
    NC_("mypaint-brushes-action", "Show MyPaint brush file location in the file manager"),
    data_show_in_file_manager_cmd_callback,
    GIMP_HELP_MYPAINT_BRUSH_SHOW_IN_FILE_MANAGER },

  { "mypaint-brushes-delete", GIMP_ICON_EDIT_DELETE,
    NC_("mypaint-brushes-action", "_Delete MyPaint Brush"), NULL,
    NC_("mypaint-brushes-action", "Delete this MyPaint brush"),
    data_delete_cmd_callback,
    GIMP_HELP_MYPAINT_BRUSH_DELETE },

  { "mypaint-brushes-refresh", GIMP_ICON_VIEW_REFRESH,
    NC_("mypaint-brushes-action", "_Refresh MyPaint Brushes"), NULL,
    NC_("mypaint-brushes-action", "Refresh MyPaint brushes"),
    data_refresh_cmd_callback,
    GIMP_HELP_MYPAINT_BRUSH_REFRESH }
};

static const GimpStringActionEntry mypaint_brushes_edit_actions[] =
{
  { "mypaint-brushes-edit", GIMP_ICON_EDIT,
    NC_("mypaint-brushes-action", "_Edit MyPaint Brush..."), NULL,
    NC_("mypaint-brushes-action", "Edit MyPaint brush"),
    "gimp-mybrush-editor",
    GIMP_HELP_MYPAINT_BRUSH_EDIT }
};


void
mypaint_brushes_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "mypaint-brushes-action",
                                 mypaint_brushes_actions,
                                 G_N_ELEMENTS (mypaint_brushes_actions));

  gimp_action_group_add_string_actions (group, "mypaint-brushes-action",
                                        mypaint_brushes_edit_actions,
                                        G_N_ELEMENTS (mypaint_brushes_edit_actions),
                                        data_edit_cmd_callback);
}

void
mypaint_brushes_actions_update (GimpActionGroup *group,
                                gpointer         user_data)
{
  GimpContext *context = action_data_get_context (user_data);
  GimpMybrush *brush   = NULL;
  GimpData    *data    = NULL;
  GFile       *file    = NULL;

  if (context)
    {
      brush = gimp_context_get_mybrush (context);

      if (action_data_sel_count (user_data) > 1)
        {
          brush = NULL;
        }

      if (brush)
        {
          data = GIMP_DATA (brush);

          file = gimp_data_get_file (data);
        }
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("mypaint-brushes-edit",                 brush && FALSE);
  SET_SENSITIVE ("mypaint-brushes-duplicate",            brush && gimp_data_is_duplicatable (data));
  SET_SENSITIVE ("mypaint-brushes-copy-location",        file);
  SET_SENSITIVE ("mypaint-brushes-show-in-file-manager", file);
  SET_SENSITIVE ("mypaint-brushes-delete",               brush && gimp_data_is_deletable (data));

#undef SET_SENSITIVE
}
