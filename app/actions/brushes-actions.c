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
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "brushes-actions.h"
#include "data-commands.h"

#include "gimp-intl.h"


static GimpActionEntry brushes_actions[] =
{
  { "brushes-popup", GIMP_STOCK_BRUSH,
    N_("Brushes Menu"), NULL, NULL, NULL,
    GIMP_HELP_BRUSH_DIALOG },

  { "brushes-new", GTK_STOCK_NEW,
    N_("_New Brush"), "",
    N_("New brush"),
    G_CALLBACK (data_new_data_cmd_callback),
    GIMP_HELP_BRUSH_NEW },

  { "brushes-duplicate", GIMP_STOCK_DUPLICATE,
    N_("D_uplicate Brush"), NULL,
    N_("Duplicate brush"),
    G_CALLBACK (data_duplicate_data_cmd_callback),
    GIMP_HELP_BRUSH_DUPLICATE },

  { "brushes-delete", GTK_STOCK_DELETE,
    N_("_Delete Brush"), "",
    N_("Delete brush"),
    G_CALLBACK (data_delete_data_cmd_callback),
    GIMP_HELP_BRUSH_DELETE },

  { "brushes-refresh", GTK_STOCK_REFRESH,
    N_("_Refresh Brushes"), "",
    N_("Refresh brushes"),
    G_CALLBACK (data_refresh_data_cmd_callback),
    GIMP_HELP_BRUSH_REFRESH }
};

static GimpStringActionEntry brushes_edit_actions[] =
{
  { "brushes-edit", GIMP_STOCK_EDIT,
    N_("_Edit Brush..."), NULL,
    N_("Edit brush"),
    "gimp-brush-editor",
    GIMP_HELP_BRUSH_EDIT }
};


void
brushes_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 brushes_actions,
                                 G_N_ELEMENTS (brushes_actions));

  gimp_action_group_add_string_actions (group,
                                        brushes_edit_actions,
                                        G_N_ELEMENTS (brushes_edit_actions),
                                        G_CALLBACK (data_edit_data_cmd_callback));
}

void
brushes_actions_update (GimpActionGroup *group,
                        gpointer         user_data)
{
  GimpContext *context = action_data_get_context (user_data);
  GimpBrush   *brush   = NULL;
  GimpData    *data    = NULL;

  if (context)
    {
      brush = gimp_context_get_brush (context);

      if (brush)
        data = GIMP_DATA (brush);
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("brushes-edit",      brush);
  SET_SENSITIVE ("brushes-duplicate", brush && GIMP_DATA_GET_CLASS (data)->duplicate);
  SET_SENSITIVE ("brushes-delete",    brush && data->deletable);

#undef SET_SENSITIVE
}
