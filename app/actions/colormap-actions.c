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

#include "core/ligmacontext.h"
#include "core/ligmadrawable.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-colormap.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmahelp-ids.h"

#include "actions.h"
#include "colormap-actions.h"
#include "colormap-commands.h"

#include "ligma-intl.h"


static const LigmaActionEntry colormap_actions[] =
{
  { "colormap-popup", LIGMA_ICON_COLORMAP,
    NC_("colormap-action", "Colormap Menu"), NULL, NULL, NULL,
    LIGMA_HELP_INDEXED_PALETTE_DIALOG },

  { "colormap-edit-color", LIGMA_ICON_EDIT,
    NC_("colormap-action", "_Edit Color..."), NULL,
    NC_("colormap-action", "Edit this color"),
    colormap_edit_color_cmd_callback,
    LIGMA_HELP_INDEXED_PALETTE_EDIT }
};

static const LigmaEnumActionEntry colormap_add_color_actions[] =
{
  { "colormap-add-color-from-fg", LIGMA_ICON_LIST_ADD,
    NC_("colormap-action", "_Add Color from FG"), "",
    NC_("colormap-action", "Add current foreground color"),
    FALSE, FALSE,
    LIGMA_HELP_INDEXED_PALETTE_ADD },

  { "colormap-add-color-from-bg", LIGMA_ICON_LIST_ADD,
    NC_("colormap-action", "_Add Color from BG"), "",
    NC_("colormap-action", "Add current background color"),
    TRUE, FALSE,
    LIGMA_HELP_INDEXED_PALETTE_ADD }
};

static const LigmaEnumActionEntry colormap_to_selection_actions[] =
{
  { "colormap-selection-replace", LIGMA_ICON_SELECTION_REPLACE,
    NC_("colormap-action", "_Select this Color"), NULL,
    NC_("colormap-action", "Select all pixels with this color"),
    LIGMA_CHANNEL_OP_REPLACE, FALSE,
    LIGMA_HELP_INDEXED_PALETTE_SELECTION_REPLACE },

  { "colormap-selection-add", LIGMA_ICON_SELECTION_ADD,
    NC_("colormap-action", "_Add to Selection"), NULL,
    NC_("colormap-action", "Add all pixels with this color to the current selection"),
    LIGMA_CHANNEL_OP_ADD, FALSE,
    LIGMA_HELP_INDEXED_PALETTE_SELECTION_ADD },

  { "colormap-selection-subtract", LIGMA_ICON_SELECTION_SUBTRACT,
    NC_("colormap-action", "_Subtract from Selection"), NULL,
    NC_("colormap-action", "Subtract all pixels with this color from the current selection"),
    LIGMA_CHANNEL_OP_SUBTRACT, FALSE,
    LIGMA_HELP_INDEXED_PALETTE_SELECTION_SUBTRACT },

  { "colormap-selection-intersect", LIGMA_ICON_SELECTION_INTERSECT,
    NC_("colormap-action", "_Intersect with Selection"), NULL,
    NC_("colormap-action", "Intersect all pixels with this color with the current selection"),
    LIGMA_CHANNEL_OP_INTERSECT, FALSE,
    LIGMA_HELP_INDEXED_PALETTE_SELECTION_INTERSECT }
};

void
colormap_actions_setup (LigmaActionGroup *group)
{
  ligma_action_group_add_actions (group, "colormap-action",
                                 colormap_actions,
                                 G_N_ELEMENTS (colormap_actions));

  ligma_action_group_add_enum_actions (group, "colormap-action",
                                      colormap_add_color_actions,
                                      G_N_ELEMENTS (colormap_add_color_actions),
                                      colormap_add_color_cmd_callback);

  ligma_action_group_add_enum_actions (group, "colormap-action",
                                      colormap_to_selection_actions,
                                      G_N_ELEMENTS (colormap_to_selection_actions),
                                      colormap_to_selection_cmd_callback);
}

void
colormap_actions_update (LigmaActionGroup *group,
                         gpointer         data)
{
  LigmaImage   *image            = action_data_get_image (data);
  LigmaContext *context          = action_data_get_context (data);
  gboolean     indexed          = FALSE;
  gboolean     drawable_indexed = FALSE;
  gint         num_colors       = 0;
  LigmaRGB      fg;
  LigmaRGB      bg;

  if (image)
    {
      indexed = (ligma_image_get_base_type (image) == LIGMA_INDEXED);

      if (indexed)
        {
          GList *drawables = ligma_image_get_selected_drawables (image);

          num_colors = ligma_image_get_colormap_size (image);

          if (g_list_length (drawables) == 1)
            drawable_indexed = ligma_drawable_is_indexed (drawables->data);

          g_list_free (drawables);
        }
    }

  if (context)
    {
      ligma_context_get_foreground (context, &fg);
      ligma_context_get_background (context, &bg);
    }

#define SET_SENSITIVE(action,condition) \
        ligma_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)
#define SET_COLOR(action,color) \
        ligma_action_group_set_action_color (group, action, color, FALSE);

  SET_SENSITIVE ("colormap-edit-color",
                 indexed && num_colors > 0);

  SET_SENSITIVE ("colormap-add-color-from-fg",
                 indexed && num_colors < 256);
  SET_SENSITIVE ("colormap-add-color-from-bg",
                 indexed && num_colors < 256);

  SET_COLOR ("colormap-add-color-from-fg", context ? &fg : NULL);
  SET_COLOR ("colormap-add-color-from-bg", context ? &bg : NULL);

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
