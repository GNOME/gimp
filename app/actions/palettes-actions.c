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

#include "core/gimpcontext.h"
#include "core/gimpdata.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "data-commands.h"
#include "palettes-actions.h"
#include "palettes-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry palettes_actions[] =
{
  { "palettes-popup", GIMP_STOCK_PALETTE,
    NC_("palettes-action", "Palettes Menu"), NULL, NULL, NULL,
    GIMP_HELP_PALETTE_DIALOG },

  { "palettes-new", "document-new",
    NC_("palettes-action", "_New Palette"), NULL,
    NC_("palettes-action", "Create a new palette"),
    G_CALLBACK (data_new_cmd_callback),
    GIMP_HELP_PALETTE_NEW },

  { "palettes-import", "gtk-convert",
    NC_("palettes-action", "_Import Palette..."), NULL,
    NC_("palettes-action", "Import palette"),
    G_CALLBACK (palettes_import_cmd_callback),
    GIMP_HELP_PALETTE_IMPORT },

  { "palettes-duplicate", GIMP_STOCK_DUPLICATE,
    NC_("palettes-action", "D_uplicate Palette"), NULL,
    NC_("palettes-action", "Duplicate this palette"),
    G_CALLBACK (data_duplicate_cmd_callback),
    GIMP_HELP_PALETTE_DUPLICATE },

  { "palettes-merge", NULL,
    NC_("palettes-action", "_Merge Palettes..."), NULL,
    NC_("palettes-action", "Merge palettes"),
    G_CALLBACK (palettes_merge_cmd_callback),
    GIMP_HELP_PALETTE_MERGE },

  { "palettes-copy-location", "edit-copy",
    NC_("palettes-action", "Copy Palette _Location"), NULL,
    NC_("palettes-action", "Copy palette file location to clipboard"),
    G_CALLBACK (data_copy_location_cmd_callback),
    GIMP_HELP_PALETTE_COPY_LOCATION },

  { "palettes-delete", "edit-delete",
    NC_("palettes-action", "_Delete Palette"), NULL,
    NC_("palettes-action", "Delete this palette"),
    G_CALLBACK (data_delete_cmd_callback),
    GIMP_HELP_PALETTE_DELETE },

  { "palettes-refresh", "view-refresh",
    NC_("palettes-action", "_Refresh Palettes"), NULL,
    NC_("palettes-action", "Refresh palettes"),
    G_CALLBACK (data_refresh_cmd_callback),
    GIMP_HELP_PALETTE_REFRESH }
};

static const GimpStringActionEntry palettes_edit_actions[] =
{
  { "palettes-edit", "gtk-edit",
    NC_("palettes-action", "_Edit Palette..."), NULL,
    NC_("palettes-action", "Edit palette"),
    "gimp-palette-editor",
    GIMP_HELP_PALETTE_EDIT }
};


void
palettes_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "palettes-action",
                                 palettes_actions,
                                 G_N_ELEMENTS (palettes_actions));

  gimp_action_group_add_string_actions (group, "palettes-action",
                                        palettes_edit_actions,
                                        G_N_ELEMENTS (palettes_edit_actions),
                                        G_CALLBACK (data_edit_cmd_callback));
}

void
palettes_actions_update (GimpActionGroup *group,
                         gpointer         user_data)
{
  GimpContext *context = action_data_get_context (user_data);
  GimpPalette *palette = NULL;
  GimpData    *data    = NULL;
  GFile       *file    = NULL;

  if (context)
    {
      palette = gimp_context_get_palette (context);

      if (action_data_sel_count (user_data) > 1)
        {
          palette = NULL;
        }

      if (palette)
        {
          data = GIMP_DATA (palette);

          file = gimp_data_get_file (data);
        }
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("palettes-edit",          palette);
  SET_SENSITIVE ("palettes-duplicate",     palette && GIMP_DATA_GET_CLASS (data)->duplicate);
  SET_SENSITIVE ("palettes-merge",         FALSE); /* FIXME palette && GIMP_IS_CONTAINER_LIST_VIEW (editor->view)); */
  SET_SENSITIVE ("palettes-copy-location", palette && file);
  SET_SENSITIVE ("palettes-delete",        palette && gimp_data_is_deletable (data));

#undef SET_SENSITIVE
}
