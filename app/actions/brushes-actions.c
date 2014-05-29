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

#include "core/gimpbrushgenerated.h"
#include "core/gimpcontext.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "brushes-actions.h"
#include "data-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry brushes_actions[] =
{
  { "brushes-popup", GIMP_STOCK_BRUSH,
    NC_("brushes-action", "Brushes Menu"), NULL, NULL, NULL,
    GIMP_HELP_BRUSH_DIALOG },

  { "brushes-open-as-image", "document-open",
    NC_("brushes-action", "_Open Brush as Image"), NULL,
    NC_("brushes-action", "Open brush as image"),
    G_CALLBACK (data_open_as_image_cmd_callback),
    GIMP_HELP_BRUSH_OPEN_AS_IMAGE },

  { "brushes-new", "document-new",
    NC_("brushes-action", "_New Brush"), NULL,
    NC_("brushes-action", "Create a new brush"),
    G_CALLBACK (data_new_cmd_callback),
    GIMP_HELP_BRUSH_NEW },

  { "brushes-duplicate", GIMP_STOCK_DUPLICATE,
    NC_("brushes-action", "D_uplicate Brush"), NULL,
    NC_("brushes-action", "Duplicate this brush"),
    G_CALLBACK (data_duplicate_cmd_callback),
    GIMP_HELP_BRUSH_DUPLICATE },

  { "brushes-copy-location", "edit-copy",
    NC_("brushes-action", "Copy Brush _Location"), NULL,
    NC_("brushes-action", "Copy brush file location to clipboard"),
    G_CALLBACK (data_copy_location_cmd_callback),
    GIMP_HELP_BRUSH_COPY_LOCATION },

  { "brushes-delete", "edit-delete",
    NC_("brushes-action", "_Delete Brush"), NULL,
    NC_("brushes-action", "Delete this brush"),
    G_CALLBACK (data_delete_cmd_callback),
    GIMP_HELP_BRUSH_DELETE },

  { "brushes-refresh", "view-refresh",
    NC_("brushes-action", "_Refresh Brushes"), NULL,
    NC_("brushes-action", "Refresh brushes"),
    G_CALLBACK (data_refresh_cmd_callback),
    GIMP_HELP_BRUSH_REFRESH }
};

static const GimpStringActionEntry brushes_edit_actions[] =
{
  { "brushes-edit", "gtk-edit",
    NC_("brushes-action", "_Edit Brush..."), NULL,
    NC_("brushes-action", "Edit this brush"),
    "gimp-brush-editor",
    GIMP_HELP_BRUSH_EDIT }
};


void
brushes_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "brushes-action",
                                 brushes_actions,
                                 G_N_ELEMENTS (brushes_actions));

  gimp_action_group_add_string_actions (group, "brushes-action",
                                        brushes_edit_actions,
                                        G_N_ELEMENTS (brushes_edit_actions),
                                        G_CALLBACK (data_edit_cmd_callback));
}

void
brushes_actions_update (GimpActionGroup *group,
                        gpointer         user_data)
{
  GimpContext *context  = action_data_get_context (user_data);
  GimpBrush   *brush    = NULL;
  GimpData    *data     = NULL;
  const gchar *filename = NULL;

  if (context)
    {
      brush = gimp_context_get_brush (context);

      if (action_data_sel_count (user_data) > 1)
        {
          brush = NULL;
        }

      if (brush)
        {
          data = GIMP_DATA (brush);

          filename = gimp_data_get_filename (data);
        }
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("brushes-edit",          brush);
  SET_SENSITIVE ("brushes-open-as-image", brush && filename && ! GIMP_IS_BRUSH_GENERATED (brush));
  SET_SENSITIVE ("brushes-duplicate",     brush && GIMP_DATA_GET_CLASS (data)->duplicate);
  SET_SENSITIVE ("brushes-copy-location", brush && filename);
  SET_SENSITIVE ("brushes-delete",        brush && gimp_data_is_deletable (data));

#undef SET_SENSITIVE
}
