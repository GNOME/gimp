/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmapaletteeditor.h"

#include "data-editor-commands.h"
#include "palette-editor-actions.h"
#include "palette-editor-commands.h"

#include "ligma-intl.h"


static const LigmaActionEntry palette_editor_actions[] =
{
  { "palette-editor-popup", LIGMA_ICON_PALETTE,
    NC_("palette-editor-action", "Palette Editor Menu"), NULL, NULL, NULL,
    LIGMA_HELP_PALETTE_EDITOR_DIALOG },

  { "palette-editor-edit-color", LIGMA_ICON_EDIT,
    NC_("palette-editor-action", "_Edit Color..."), NULL,
    NC_("palette-editor-action", "Edit this entry"),
    palette_editor_edit_color_cmd_callback,
    LIGMA_HELP_PALETTE_EDITOR_EDIT },

  { "palette-editor-delete-color", LIGMA_ICON_EDIT_DELETE,
    NC_("palette-editor-action", "_Delete Color"), NULL,
    NC_("palette-editor-action", "Delete this entry"),
    palette_editor_delete_color_cmd_callback,
    LIGMA_HELP_PALETTE_EDITOR_DELETE }
};

static const LigmaToggleActionEntry palette_editor_toggle_actions[] =
{
  { "palette-editor-edit-active", LIGMA_ICON_LINKED,
    NC_("palette-editor-action", "Edit Active Palette"), NULL, NULL,
    data_editor_edit_active_cmd_callback,
    FALSE,
    LIGMA_HELP_PALETTE_EDITOR_EDIT_ACTIVE }
};

static const LigmaEnumActionEntry palette_editor_new_actions[] =
{
  { "palette-editor-new-color-fg", LIGMA_ICON_DOCUMENT_NEW,
    NC_("palette-editor-action", "New Color from _FG"), NULL,
    NC_("palette-editor-action",
        "Create a new entry from the foreground color"),
    FALSE, FALSE,
    LIGMA_HELP_PALETTE_EDITOR_NEW },

  { "palette-editor-new-color-bg", LIGMA_ICON_DOCUMENT_NEW,
    NC_("palette-editor-action", "New Color from _BG"), NULL,
    NC_("palette-editor-action",
        "Create a new entry from the background color"),
    TRUE, FALSE,
    LIGMA_HELP_PALETTE_EDITOR_NEW }
};

static const LigmaEnumActionEntry palette_editor_zoom_actions[] =
{
  { "palette-editor-zoom-in", LIGMA_ICON_ZOOM_IN,
    N_("Zoom _In"), NULL,
    N_("Zoom in"),
    LIGMA_ZOOM_IN, FALSE,
    LIGMA_HELP_PALETTE_EDITOR_ZOOM_IN },

  { "palette-editor-zoom-out", LIGMA_ICON_ZOOM_OUT,
    N_("Zoom _Out"), NULL,
    N_("Zoom out"),
    LIGMA_ZOOM_OUT, FALSE,
    LIGMA_HELP_PALETTE_EDITOR_ZOOM_OUT },

  { "palette-editor-zoom-all", LIGMA_ICON_ZOOM_FIT_BEST,
    N_("Zoom _All"), NULL,
    N_("Zoom all"),
    LIGMA_ZOOM_OUT_MAX, FALSE,
    LIGMA_HELP_PALETTE_EDITOR_ZOOM_ALL }
};


void
palette_editor_actions_setup (LigmaActionGroup *group)
{
  ligma_action_group_add_actions (group, "palette-editor-action",
                                 palette_editor_actions,
                                 G_N_ELEMENTS (palette_editor_actions));

  ligma_action_group_add_toggle_actions (group, "palette-editor-action",
                                        palette_editor_toggle_actions,
                                        G_N_ELEMENTS (palette_editor_toggle_actions));

  ligma_action_group_add_enum_actions (group, "palette-editor-action",
                                      palette_editor_new_actions,
                                      G_N_ELEMENTS (palette_editor_new_actions),
                                      palette_editor_new_color_cmd_callback);

  ligma_action_group_add_enum_actions (group, NULL,
                                      palette_editor_zoom_actions,
                                      G_N_ELEMENTS (palette_editor_zoom_actions),
                                      palette_editor_zoom_cmd_callback);
}

void
palette_editor_actions_update (LigmaActionGroup *group,
                               gpointer         user_data)
{
  LigmaPaletteEditor *editor      = LIGMA_PALETTE_EDITOR (user_data);
  LigmaDataEditor    *data_editor = LIGMA_DATA_EDITOR (user_data);
  LigmaData          *data;
  gboolean           editable    = FALSE;
  LigmaRGB            fg;
  LigmaRGB            bg;
  gboolean           edit_active = FALSE;

  data = data_editor->data;

  if (data)
    {
      if (data_editor->data_editable)
        editable = TRUE;
    }

  if (data_editor->context)
    {
      ligma_context_get_foreground (data_editor->context, &fg);
      ligma_context_get_background (data_editor->context, &bg);
    }

  edit_active = ligma_data_editor_get_edit_active (data_editor);

#define SET_SENSITIVE(action,condition) \
        ligma_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)
#define SET_ACTIVE(action,condition) \
        ligma_action_group_set_action_active (group, action, (condition) != 0)
#define SET_COLOR(action,color) \
        ligma_action_group_set_action_color (group, action, color, FALSE);

  SET_SENSITIVE ("palette-editor-edit-color",   editable && editor->color);
  SET_SENSITIVE ("palette-editor-delete-color", editable && editor->color);

  SET_SENSITIVE ("palette-editor-new-color-fg", editable);
  SET_SENSITIVE ("palette-editor-new-color-bg", editable);

  SET_COLOR ("palette-editor-new-color-fg", data_editor->context ? &fg : NULL);
  SET_COLOR ("palette-editor-new-color-bg", data_editor->context ? &bg : NULL);

  SET_SENSITIVE ("palette-editor-zoom-out", data);
  SET_SENSITIVE ("palette-editor-zoom-in",  data);
  SET_SENSITIVE ("palette-editor-zoom-all", data);

  SET_ACTIVE ("palette-editor-edit-active", edit_active);

#undef SET_SENSITIVE
#undef SET_ACTIVE
#undef SET_COLOR
}
