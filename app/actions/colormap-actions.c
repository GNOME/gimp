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
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-colormap.h"
#include "core/gimppalette.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpcolormapeditor.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "colormap-actions.h"
#include "colormap-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry colormap_actions[] =
{
  { "colormap-edit-color", GIMP_ICON_EDIT,
    NC_("colormap-action", "_Edit Color..."), NULL, { NULL },
    NC_("colormap-action", "Edit this color"),
    colormap_edit_color_cmd_callback,
    GIMP_HELP_INDEXED_PALETTE_EDIT },

  { "colormap-delete-color", GIMP_ICON_EDIT_DELETE,
    NC_("colormap-action", "_Delete Color..."), NULL, { NULL },
    NC_("colormap-action", "Delete this color"),
    colormap_delete_color_cmd_callback,
    GIMP_HELP_INDEXED_PALETTE_EDIT }
};

static const GimpEnumActionEntry colormap_add_color_actions[] =
{
  { "colormap-add-color-from-fg", GIMP_ICON_LIST_ADD,
    NC_("colormap-action", "_Add Color from FG"), NULL, { NULL },
    NC_("colormap-action", "Add current foreground color"),
    FALSE, FALSE,
    GIMP_HELP_INDEXED_PALETTE_ADD },

  { "colormap-add-color-from-bg", GIMP_ICON_LIST_ADD,
    NC_("colormap-action", "_Add Color from BG"), NULL, { NULL },
    NC_("colormap-action", "Add current background color"),
    TRUE, FALSE,
    GIMP_HELP_INDEXED_PALETTE_ADD }
};

static const GimpEnumActionEntry colormap_to_selection_actions[] =
{
  { "colormap-selection-replace", GIMP_ICON_SELECTION_REPLACE,
    NC_("colormap-action", "_Select this Color"), NULL, { NULL },
    NC_("colormap-action", "Select all pixels with this color"),
    GIMP_CHANNEL_OP_REPLACE, FALSE,
    GIMP_HELP_INDEXED_PALETTE_SELECTION_REPLACE },

  { "colormap-selection-add", GIMP_ICON_SELECTION_ADD,
    NC_("colormap-action", "_Add to Selection"), NULL, { NULL },
    NC_("colormap-action", "Add all pixels with this color to the current selection"),
    GIMP_CHANNEL_OP_ADD, FALSE,
    GIMP_HELP_INDEXED_PALETTE_SELECTION_ADD },

  { "colormap-selection-subtract", GIMP_ICON_SELECTION_SUBTRACT,
    NC_("colormap-action", "_Subtract from Selection"), NULL, { NULL },
    NC_("colormap-action", "Subtract all pixels with this color from the current selection"),
    GIMP_CHANNEL_OP_SUBTRACT, FALSE,
    GIMP_HELP_INDEXED_PALETTE_SELECTION_SUBTRACT },

  { "colormap-selection-intersect", GIMP_ICON_SELECTION_INTERSECT,
    NC_("colormap-action", "_Intersect with Selection"), NULL, { NULL },
    NC_("colormap-action", "Intersect all pixels with this color with the current selection"),
    GIMP_CHANNEL_OP_INTERSECT, FALSE,
    GIMP_HELP_INDEXED_PALETTE_SELECTION_INTERSECT }
};

void
colormap_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "colormap-action",
                                 colormap_actions,
                                 G_N_ELEMENTS (colormap_actions));

  gimp_action_group_add_enum_actions (group, "colormap-action",
                                      colormap_add_color_actions,
                                      G_N_ELEMENTS (colormap_add_color_actions),
                                      colormap_add_color_cmd_callback);

  gimp_action_group_add_enum_actions (group, "colormap-action",
                                      colormap_to_selection_actions,
                                      G_N_ELEMENTS (colormap_to_selection_actions),
                                      colormap_to_selection_cmd_callback);
}

void
colormap_actions_update (GimpActionGroup *group,
                         gpointer         data)
{
  GimpColormapEditor *editor           = GIMP_COLORMAP_EDITOR (data);
  GimpImage          *image            = action_data_get_image (data);
  GimpContext        *context          = action_data_get_context (data);
  gboolean            indexed          = FALSE;
  gboolean            drawable_indexed = FALSE;
  gint                num_colors       = 0;

  if (image)
    {
      indexed = (gimp_image_get_base_type (image) == GIMP_INDEXED);

      if (indexed)
        {
          GList *drawables = gimp_image_get_selected_drawables (image);

          num_colors = gimp_palette_get_n_colors (gimp_image_get_colormap_palette ((image)));

          if (g_list_length (drawables) == 1)
            drawable_indexed = gimp_drawable_is_indexed (drawables->data);

          g_list_free (drawables);
        }
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)
#define SET_COLOR(action,color) \
        gimp_action_group_set_action_color (group, action, color, FALSE);

  SET_SENSITIVE ("colormap-edit-color",
                 indexed && num_colors > 0);
  SET_SENSITIVE ("colormap-delete-color",
                 indexed && num_colors > 0 &&
                 gimp_colormap_editor_is_color_deletable (editor));

  SET_SENSITIVE ("colormap-add-color-from-fg",
                 indexed && num_colors < 256);
  SET_SENSITIVE ("colormap-add-color-from-bg",
                 indexed && num_colors < 256);

  SET_COLOR ("colormap-add-color-from-fg", context ? gimp_context_get_foreground (context) : NULL);
  SET_COLOR ("colormap-add-color-from-bg", context ? gimp_context_get_background (context) : NULL);

  SET_SENSITIVE ("colormap-selection-replace",
                 drawable_indexed && num_colors > 0);
  SET_SENSITIVE ("colormap-selection-add",
                 drawable_indexed && num_colors > 0);
  SET_SENSITIVE ("colormap-selection-subtract",
                 drawable_indexed && num_colors > 0);
  SET_SENSITIVE ("colormap-selection-intersect",
                 drawable_indexed && num_colors > 0);

#undef SET_SENSITIVE
#undef SET_COLOR
}
