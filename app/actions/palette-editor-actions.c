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

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimppaletteeditor.h"

#include "data-editor-commands.h"
#include "palette-editor-actions.h"
#include "palette-editor-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry palette_editor_actions[] =
{
  { "palette-editor-edit-color", GIMP_ICON_EDIT,
    NC_("palette-editor-action", "_Edit Color..."), NULL, { NULL },
    NC_("palette-editor-action", "Edit this entry"),
    palette_editor_edit_color_cmd_callback,
    GIMP_HELP_PALETTE_EDITOR_EDIT },

  { "palette-editor-delete-color", GIMP_ICON_EDIT_DELETE,
    NC_("palette-editor-action", "_Delete Color"), NULL, { NULL },
    NC_("palette-editor-action", "Delete this entry"),
    palette_editor_delete_color_cmd_callback,
    GIMP_HELP_PALETTE_EDITOR_DELETE }
};

static const GimpToggleActionEntry palette_editor_toggle_actions[] =
{
  { "palette-editor-edit-active", GIMP_ICON_LINKED,
    NC_("palette-editor-action", "Edit Active Palette"), NULL, { NULL }, NULL,
    data_editor_edit_active_cmd_callback,
    FALSE,
    GIMP_HELP_PALETTE_EDITOR_EDIT_ACTIVE }
};

static const GimpEnumActionEntry palette_editor_new_actions[] =
{
  { "palette-editor-new-color-fg", GIMP_ICON_DOCUMENT_NEW,
    NC_("palette-editor-action", "New Color from _FG"), NULL, { NULL },
    NC_("palette-editor-action",
        "Create a new entry from the foreground color"),
    FALSE, FALSE,
    GIMP_HELP_PALETTE_EDITOR_NEW },

  { "palette-editor-new-color-bg", GIMP_ICON_DOCUMENT_NEW,
    NC_("palette-editor-action", "New Color from _BG"), NULL, { NULL },
    NC_("palette-editor-action",
        "Create a new entry from the background color"),
    TRUE, FALSE,
    GIMP_HELP_PALETTE_EDITOR_NEW }
};

static const GimpEnumActionEntry palette_editor_zoom_actions[] =
{
  { "palette-editor-zoom-in", GIMP_ICON_ZOOM_IN,
    N_("Zoom _In"), NULL, { NULL },
    N_("Zoom in"),
    GIMP_ZOOM_IN, FALSE,
    GIMP_HELP_PALETTE_EDITOR_ZOOM_IN },

  { "palette-editor-zoom-out", GIMP_ICON_ZOOM_OUT,
    N_("Zoom _Out"), NULL, { NULL },
    N_("Zoom out"),
    GIMP_ZOOM_OUT, FALSE,
    GIMP_HELP_PALETTE_EDITOR_ZOOM_OUT },

  { "palette-editor-zoom-all", GIMP_ICON_ZOOM_FIT_BEST,
    N_("Zoom _All"), NULL, { NULL },
    N_("Zoom all"),
    GIMP_ZOOM_OUT_MAX, FALSE,
    GIMP_HELP_PALETTE_EDITOR_ZOOM_ALL }
};


void
palette_editor_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "palette-editor-action",
                                 palette_editor_actions,
                                 G_N_ELEMENTS (palette_editor_actions));

  gimp_action_group_add_toggle_actions (group, "palette-editor-action",
                                        palette_editor_toggle_actions,
                                        G_N_ELEMENTS (palette_editor_toggle_actions));

  gimp_action_group_add_enum_actions (group, "palette-editor-action",
                                      palette_editor_new_actions,
                                      G_N_ELEMENTS (palette_editor_new_actions),
                                      palette_editor_new_color_cmd_callback);

  gimp_action_group_add_enum_actions (group, NULL,
                                      palette_editor_zoom_actions,
                                      G_N_ELEMENTS (palette_editor_zoom_actions),
                                      palette_editor_zoom_cmd_callback);
}

void
palette_editor_actions_update (GimpActionGroup *group,
                               gpointer         user_data)
{
  GimpPaletteEditor *editor      = GIMP_PALETTE_EDITOR (user_data);
  GimpDataEditor    *data_editor = GIMP_DATA_EDITOR (user_data);
  GimpData          *data;
  gboolean           editable    = FALSE;
  gboolean           edit_active = FALSE;

  data = data_editor->data;

  if (data)
    {
      if (data_editor->data_editable)
        editable = TRUE;
    }

  edit_active = gimp_data_editor_get_edit_active (data_editor);

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)
#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)
#define SET_COLOR(action,color) \
        gimp_action_group_set_action_color (group, action, color, FALSE);

  SET_SENSITIVE ("palette-editor-edit-color",   editable && editor->color);
  SET_SENSITIVE ("palette-editor-delete-color", editable && editor->color);

  SET_SENSITIVE ("palette-editor-new-color-fg", editable);
  SET_SENSITIVE ("palette-editor-new-color-bg", editable);

  SET_COLOR ("palette-editor-new-color-fg", data_editor->context ? gimp_context_get_foreground (data_editor->context) : NULL);
  SET_COLOR ("palette-editor-new-color-bg", data_editor->context ? gimp_context_get_background (data_editor->context) : NULL);

  SET_SENSITIVE ("palette-editor-zoom-out", data);
  SET_SENSITIVE ("palette-editor-zoom-in",  data);
  SET_SENSITIVE ("palette-editor-zoom-all", data);

  SET_ACTIVE ("palette-editor-edit-active", edit_active);

#undef SET_SENSITIVE
#undef SET_ACTIVE
#undef SET_COLOR
}
