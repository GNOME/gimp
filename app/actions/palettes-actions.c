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
#include "palettes-actions.h"
#include "palettes-commands.h"

#include "gimp-intl.h"


static GimpActionEntry palettes_actions[] =
{
  { "palettes-popup", GIMP_STOCK_PALETTE, N_("Palettes Menu"), NULL, NULL, NULL,
    GIMP_HELP_PALETTE_DIALOG },

  { "palettes-edit", GIMP_STOCK_EDIT,
    N_("_Edit Palette..."), NULL, NULL,
    G_CALLBACK (data_edit_data_cmd_callback),
    GIMP_HELP_PALETTE_EDIT },

  { "palettes-new", GTK_STOCK_NEW,
    N_("_New Palette"), "", NULL,
    G_CALLBACK (data_new_data_cmd_callback),
    GIMP_HELP_PALETTE_NEW },

  { "palettes-import", GTK_STOCK_CONVERT,
    N_("_Import Palette..."), "", NULL,
    G_CALLBACK (palettes_import_palette_cmd_callback),
    GIMP_HELP_PALETTE_IMPORT },

  { "palettes-duplicate", GIMP_STOCK_DUPLICATE,
    N_("D_uplicate Palette"), NULL, NULL,
    G_CALLBACK (data_duplicate_data_cmd_callback),
    GIMP_HELP_PALETTE_DUPLICATE },

  { "palettes-merge", NULL,
    N_("_Merge Palettes..."), NULL, NULL,
    G_CALLBACK (palettes_merge_palettes_cmd_callback),
    GIMP_HELP_PALETTE_MERGE },

  { "palettes-delete", GTK_STOCK_DELETE,
    N_("_Delete Palette"), "", NULL,
    G_CALLBACK (data_delete_data_cmd_callback),
    GIMP_HELP_PALETTE_DELETE },

  { "palettes-refresh", GTK_STOCK_REFRESH,
    N_("_Refresh Palettes"), "", NULL,
    G_CALLBACK (data_refresh_data_cmd_callback),
    GIMP_HELP_PALETTE_REFRESH }
};


void
palettes_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 palettes_actions,
                                 G_N_ELEMENTS (palettes_actions));
}

void
palettes_actions_update (GimpActionGroup *group,
                         gpointer         user_data)
{
  GimpContainerEditor *editor;
  GimpPalette         *palette;
  GimpData            *data = NULL;

  editor = GIMP_CONTAINER_EDITOR (user_data);

  palette = gimp_context_get_palette (editor->view->context);

  if (palette)
    data = GIMP_DATA (palette);

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("palettes-edit",
		 palette && GIMP_DATA_FACTORY_VIEW (editor)->data_edit_func);
  SET_SENSITIVE ("palettes-duplicate",
		 palette && GIMP_DATA_GET_CLASS (data)->duplicate);
  SET_SENSITIVE ("palettes-merge",
		 FALSE); /* FIXME palette && GIMP_IS_CONTAINER_LIST_VIEW (editor->view)); */
  SET_SENSITIVE ("palettes-delete",
		 palette && data->deletable);

#undef SET_SENSITIVE
}
