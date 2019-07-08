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
#include "core/gimpchannel.h"
#include "core/gimpimage.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "select-actions.h"
#include "select-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry select_actions[] =
{
  { "selection-popup", GIMP_ICON_SELECTION,
    NC_("select-action", "Selection Editor Menu"), NULL, NULL, NULL,
    GIMP_HELP_SELECTION_DIALOG },

  { "select-menu", NULL, NC_("select-action", "_Select") },

  { "select-all", GIMP_ICON_SELECTION_ALL,
    NC_("select-action", "_All"), "<primary>A",
    NC_("select-action", "Select everything"),
    select_all_cmd_callback,
    GIMP_HELP_SELECTION_ALL },

  { "select-none", GIMP_ICON_SELECTION_NONE,
    NC_("select-action", "_None"), "<primary><shift>A",
    NC_("select-action", "Dismiss the selection"),
    select_none_cmd_callback,
    GIMP_HELP_SELECTION_NONE },

  { "select-invert", GIMP_ICON_INVERT,
    NC_("select-action", "_Invert"), "<primary>I",
    NC_("select-action", "Invert the selection"),
    select_invert_cmd_callback,
    GIMP_HELP_SELECTION_INVERT },

  { "select-float", GIMP_ICON_LAYER_FLOATING_SELECTION,
    NC_("select-action", "_Float"), "<primary><shift>L",
    NC_("select-action", "Create a floating selection"),
    select_float_cmd_callback,
    GIMP_HELP_SELECTION_FLOAT },

  { "select-feather", NULL,
    NC_("select-action", "Fea_ther..."), NULL,
    NC_("select-action",
        "Blur the selection border so that it fades out smoothly"),
    select_feather_cmd_callback,
    GIMP_HELP_SELECTION_FEATHER },

  { "select-sharpen", NULL,
    NC_("select-action", "_Sharpen"), NULL,
    NC_("select-action", "Remove fuzziness from the selection"),
    select_sharpen_cmd_callback,
    GIMP_HELP_SELECTION_SHARPEN },

  { "select-shrink", GIMP_ICON_SELECTION_SHRINK,
    NC_("select-action", "S_hrink..."), NULL,
    NC_("select-action", "Contract the selection"),
    select_shrink_cmd_callback,
    GIMP_HELP_SELECTION_SHRINK },

  { "select-grow", GIMP_ICON_SELECTION_GROW,
    NC_("select-action", "_Grow..."), NULL,
    NC_("select-action", "Enlarge the selection"),
    select_grow_cmd_callback,
    GIMP_HELP_SELECTION_GROW },

  { "select-border", GIMP_ICON_SELECTION_BORDER,
    NC_("select-action", "Bo_rder..."), NULL,
    NC_("select-action", "Replace the selection by its border"),
    select_border_cmd_callback,
    GIMP_HELP_SELECTION_BORDER },

  { "select-flood", NULL,
    NC_("select-action", "Re_move Holes"), NULL,
    NC_("select-action", "Remove holes from the selection"),
    select_flood_cmd_callback,
    GIMP_HELP_SELECTION_FLOOD },

  { "select-save", GIMP_ICON_SELECTION_TO_CHANNEL,
    NC_("select-action", "Save to _Channel"), NULL,
    NC_("select-action", "Save the selection to a channel"),
    select_save_cmd_callback,
    GIMP_HELP_SELECTION_TO_CHANNEL },

  { "select-fill", GIMP_ICON_TOOL_BUCKET_FILL,
    NC_("select-action", "_Fill Selection Outline..."), NULL,
    NC_("select-action", "Fill the selection outline"),
    select_fill_cmd_callback,
    GIMP_HELP_SELECTION_FILL },

  { "select-fill-last-values", GIMP_ICON_TOOL_BUCKET_FILL,
    NC_("select-action", "_Fill Selection Outline"), NULL,
    NC_("select-action", "Fill the selection outline with last used values"),
    select_fill_last_vals_cmd_callback,
    GIMP_HELP_SELECTION_FILL },

  { "select-stroke", GIMP_ICON_SELECTION_STROKE,
    NC_("select-action", "_Stroke Selection..."), NULL,
    NC_("select-action", "Paint along the selection outline"),
    select_stroke_cmd_callback,
    GIMP_HELP_SELECTION_STROKE },

  { "select-stroke-last-values", GIMP_ICON_SELECTION_STROKE,
    NC_("select-action", "_Stroke Selection"), NULL,
    NC_("select-action", "Stroke the selection with last used values"),
    select_stroke_last_vals_cmd_callback,
    GIMP_HELP_SELECTION_STROKE }
};


void
select_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "select-action",
                                 select_actions,
                                 G_N_ELEMENTS (select_actions));
}

void
select_actions_update (GimpActionGroup *group,
                       gpointer         data)
{
  GimpImage    *image    = action_data_get_image (data);
  GimpDrawable *drawable = NULL;
  gboolean      fs       = FALSE;
  gboolean      sel      = FALSE;
  gboolean      writable = FALSE;
  gboolean      children = FALSE;

  if (image)
    {
      drawable = gimp_image_get_active_drawable (image);

      if (drawable)
        {
          writable = ! gimp_item_is_content_locked (GIMP_ITEM (drawable));

          if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
            children = TRUE;
        }

      fs  = (gimp_image_get_floating_selection (image) != NULL);
      sel = ! gimp_channel_is_empty (gimp_image_get_mask (image));
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("select-all",    drawable);
  SET_SENSITIVE ("select-none",   drawable && sel);
  SET_SENSITIVE ("select-invert", drawable);
  SET_SENSITIVE ("select-float",  writable && !children && sel);

  SET_SENSITIVE ("select-feather", drawable && sel);
  SET_SENSITIVE ("select-sharpen", drawable && sel);
  SET_SENSITIVE ("select-shrink",  drawable && sel);
  SET_SENSITIVE ("select-grow",    drawable && sel);
  SET_SENSITIVE ("select-border",  drawable && sel);
  SET_SENSITIVE ("select-flood",   drawable && sel);

  SET_SENSITIVE ("select-save",               drawable && !fs);
  SET_SENSITIVE ("select-fill",               writable && !children && sel);
  SET_SENSITIVE ("select-fill-last-values",   writable && !children && sel);
  SET_SENSITIVE ("select-stroke",             writable && !children && sel);
  SET_SENSITIVE ("select-stroke-last-values", writable && !children && sel);

#undef SET_SENSITIVE
}
