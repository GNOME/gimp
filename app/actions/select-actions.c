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
  { "select-all", GIMP_ICON_SELECTION_ALL,
    NC_("select-action", "_All"), NULL, { "<primary>A", NULL },
    NC_("select-action", "Select everything"),
    select_all_cmd_callback,
    GIMP_HELP_SELECTION_ALL },

  { "select-none", GIMP_ICON_SELECTION_NONE,
    NC_("select-action", "_None"), NULL, { "<primary><shift>A", NULL },
    NC_("select-action", "Dismiss the selection"),
    select_none_cmd_callback,
    GIMP_HELP_SELECTION_NONE },

  { "select-invert", GIMP_ICON_INVERT,
    NC_("select-action", "_Invert"), NULL, { "<primary>I", NULL },
    NC_("select-action", "Invert the selection"),
    select_invert_cmd_callback,
    GIMP_HELP_SELECTION_INVERT },

  { "select-cut-float", GIMP_ICON_LAYER_FLOATING_SELECTION,
    NC_("select-action", "Cu_t and Float"), NULL, { "<primary><shift>L", NULL },
    NC_("select-action", "Cut the selection directly into a floating selection"),
    select_cut_float_cmd_callback,
    GIMP_HELP_SELECTION_FLOAT },

  { "select-copy-float", GIMP_ICON_LAYER_FLOATING_SELECTION,
    NC_("select-action", "_Copy and Float"), NULL, { NULL },
    NC_("select-action", "Copy the selection directly into a floating selection"),
    select_copy_float_cmd_callback,
    GIMP_HELP_SELECTION_FLOAT },

  { "select-feather", NULL,
    NC_("select-action", "Fea_ther..."), NULL, { NULL },
    NC_("select-action",
        "Blur the selection border so that it fades out smoothly"),
    select_feather_cmd_callback,
    GIMP_HELP_SELECTION_FEATHER },

  { "select-sharpen", NULL,
    NC_("select-action", "_Sharpen"), NULL, { NULL },
    NC_("select-action", "Remove fuzziness from the selection"),
    select_sharpen_cmd_callback,
    GIMP_HELP_SELECTION_SHARPEN },

  { "select-shrink", GIMP_ICON_SELECTION_SHRINK,
    NC_("select-action", "S_hrink..."), NULL, { NULL },
    NC_("select-action", "Contract the selection"),
    select_shrink_cmd_callback,
    GIMP_HELP_SELECTION_SHRINK },

  { "select-grow", GIMP_ICON_SELECTION_GROW,
    NC_("select-action", "_Grow..."), NULL, { NULL },
    NC_("select-action", "Enlarge the selection"),
    select_grow_cmd_callback,
    GIMP_HELP_SELECTION_GROW },

  { "select-border", GIMP_ICON_SELECTION_BORDER,
    NC_("select-action", "Bo_rder..."), NULL, { NULL },
    NC_("select-action", "Replace the selection by its border"),
    select_border_cmd_callback,
    GIMP_HELP_SELECTION_BORDER },

  { "select-flood", NULL,
    NC_("select-action", "Re_move Holes"), NULL, { NULL },
    NC_("select-action", "Remove holes from the selection"),
    select_flood_cmd_callback,
    GIMP_HELP_SELECTION_FLOOD },

  { "select-save", GIMP_ICON_SELECTION_TO_CHANNEL,
    NC_("select-action", "Save to _Channel"), NULL, { NULL },
    NC_("select-action", "Save the selection to a channel"),
    select_save_cmd_callback,
    GIMP_HELP_SELECTION_TO_CHANNEL },

  { "select-fill", GIMP_ICON_TOOL_BUCKET_FILL,
    NC_("select-action", "_Fill Selection Outline..."), NULL, { NULL },
    NC_("select-action", "Fill the selection outline"),
    select_fill_cmd_callback,
    GIMP_HELP_SELECTION_FILL },

  { "select-fill-last-values", GIMP_ICON_TOOL_BUCKET_FILL,
    NC_("select-action", "_Fill Selection Outline with last values"), NULL, { NULL },
    NC_("select-action", "Fill the selection outline with last used values"),
    select_fill_last_vals_cmd_callback,
    GIMP_HELP_SELECTION_FILL },

  { "select-stroke", GIMP_ICON_SELECTION_STROKE,
    NC_("select-action", "_Stroke Selection..."), NULL, { NULL },
    NC_("select-action", "Paint along the selection outline"),
    select_stroke_cmd_callback,
    GIMP_HELP_SELECTION_STROKE },

  { "select-stroke-last-values", GIMP_ICON_SELECTION_STROKE,
    NC_("select-action", "_Stroke Selection with last values"), NULL, { NULL },
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
  gboolean      fs       = FALSE;
  gboolean      sel      = FALSE;
  gboolean      sel_all  = FALSE;

  GList        *drawables    = NULL;
  GList        *iter;
  gboolean      all_writable = TRUE;
  gboolean      no_groups    = TRUE;

  if (image)
    {
      drawables = gimp_image_get_selected_drawables (image);

      for (iter = drawables; iter; iter = iter->next)
        {
          if (gimp_item_is_content_locked (iter->data, NULL))
            all_writable = FALSE;

          if (gimp_viewable_get_children (iter->data))
            no_groups = FALSE;

          if (! all_writable && ! no_groups)
            break;
        }

      fs      = (gimp_image_get_floating_selection (image) != NULL);
      sel     = ! gimp_channel_is_empty (gimp_image_get_mask (image));
      sel_all = gimp_channel_is_full (gimp_image_get_mask (image));
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_SENSITIVE ("select-all",    image && ! sel_all);
  SET_SENSITIVE ("select-none",   image && sel);
  SET_SENSITIVE ("select-invert", image);

  SET_SENSITIVE ("select-cut-float",  g_list_length (drawables) == 1 && sel                 &&
                                      ! gimp_item_is_content_locked (drawables->data, NULL) &&
                                      ! gimp_viewable_get_children (drawables->data));
  SET_SENSITIVE ("select-copy-float", g_list_length (drawables) == 1 && sel                 &&
                                      ! gimp_item_is_content_locked (drawables->data, NULL) &&
                                      ! gimp_viewable_get_children (drawables->data));

  SET_SENSITIVE ("select-feather", image && sel);
  SET_SENSITIVE ("select-sharpen", image && sel);
  SET_SENSITIVE ("select-shrink",  image && sel);
  SET_SENSITIVE ("select-grow",    image && sel);
  SET_SENSITIVE ("select-border",  image && sel);
  SET_SENSITIVE ("select-flood",   image && sel);

  SET_SENSITIVE ("select-save",               image && !fs);
  SET_SENSITIVE ("select-fill",               drawables && all_writable && no_groups && sel);
  SET_SENSITIVE ("select-fill-last-values",   drawables && all_writable && no_groups && sel);
  SET_SENSITIVE ("select-stroke",             drawables && all_writable && no_groups && sel);
  SET_SENSITIVE ("select-stroke-last-values", drawables && all_writable && no_groups && sel);

#undef SET_SENSITIVE

  g_list_free (drawables);
}
