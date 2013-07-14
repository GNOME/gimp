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
#include "core/gimpimage.h"
#include "core/gimpimage-colormap.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "colormap-actions.h"
#include "colormap-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry colormap_actions[] =
{
  { "colormap-popup", GIMP_STOCK_COLORMAP,
    NC_("colormap-action", "Colormap Menu"), NULL, NULL, NULL,
    GIMP_HELP_INDEXED_PALETTE_DIALOG },

  { "colormap-edit-color", GTK_STOCK_EDIT,
    NC_("colormap-action", "_Edit Color..."), NULL,
    NC_("colormap-action", "Edit this color"),
    G_CALLBACK (colormap_edit_color_cmd_callback),
    GIMP_HELP_INDEXED_PALETTE_EDIT }
};

static const GimpEnumActionEntry colormap_add_color_actions[] =
{
  { "colormap-add-color-from-fg", GTK_STOCK_ADD,
    NC_("colormap-action", "_Add Color from FG"), "",
    NC_("colormap-action", "Add current foreground color"),
    FALSE, FALSE,
    GIMP_HELP_INDEXED_PALETTE_ADD },

  { "colormap-add-color-from-bg", GTK_STOCK_ADD,
    NC_("colormap-action", "_Add Color from BG"), "",
    NC_("colormap-action", "Add current background color"),
    TRUE, FALSE,
    GIMP_HELP_INDEXED_PALETTE_ADD }
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
                                      G_CALLBACK (colormap_add_color_cmd_callback));
}

void
colormap_actions_update (GimpActionGroup *group,
                         gpointer         data)
{
  GimpImage   *image      = action_data_get_image (data);
  GimpContext *context    = action_data_get_context (data);
  gboolean     indexed    = FALSE;
  gint         num_colors = 0;
  GimpRGB      fg;
  GimpRGB      bg;

  if (image)
    {
      indexed    = gimp_image_base_type (image) == GIMP_INDEXED;
      num_colors = gimp_image_get_colormap_size (image);
    }

  if (context)
    {
      gimp_context_get_foreground (context, &fg);
      gimp_context_get_background (context, &bg);
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)
#define SET_COLOR(action,color) \
        gimp_action_group_set_action_color (group, action, color, FALSE);

  SET_SENSITIVE ("colormap-edit-color",
                 image && indexed && num_colors > 0);
  SET_SENSITIVE ("colormap-add-color-from-fg",
                 image && indexed && num_colors < 256);
  SET_SENSITIVE ("colormap-add-color-from-bg",
                 image && indexed && num_colors < 256);

  SET_COLOR ("colormap-add-color-from-fg", context ? &fg : NULL);
  SET_COLOR ("colormap-add-color-from-bg", context ? &bg : NULL);

#undef SET_SENSITIVE
#undef SET_COLOR
}
