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

#include "core/gimpimage.h"
#include "core/gimpitem.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpcomponenteditor.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "channels-actions.h"
#include "channels-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry channels_actions[] =
{
  { "channels-popup", GIMP_STOCK_CHANNELS,
    NC_("channels-action", "Channels Menu"), NULL, NULL, NULL,
    GIMP_HELP_CHANNEL_DIALOG },

  { "channels-edit-attributes", GTK_STOCK_EDIT,
    NC_("channels-action", "_Edit Channel Attributes..."), NULL,
    NC_("channels-action", "Edit the channel's name, color and opacity"),
    G_CALLBACK (channels_edit_attributes_cmd_callback),
    GIMP_HELP_CHANNEL_EDIT },

  { "channels-new", GTK_STOCK_NEW,
    NC_("channels-action", "_New Channel..."), "",
    NC_("channels-action", "Create a new channel"),
    G_CALLBACK (channels_new_cmd_callback),
    GIMP_HELP_CHANNEL_NEW },

  { "channels-new-last-values", GTK_STOCK_NEW,
    NC_("channels-action", "_New Channel"), "",
    NC_("channels-action", "Create a new channel with last used values"),
    G_CALLBACK (channels_new_last_vals_cmd_callback),
    GIMP_HELP_CHANNEL_NEW },

  { "channels-duplicate", GIMP_STOCK_DUPLICATE,
    NC_("channels-action", "D_uplicate Channel"), NULL,
    NC_("channels-action",
        "Create a duplicate of this channel and add it to the image"),
    G_CALLBACK (channels_duplicate_cmd_callback),
    GIMP_HELP_CHANNEL_DUPLICATE },

  { "channels-delete", GTK_STOCK_DELETE,
    NC_("channels-action", "_Delete Channel"), "",
    NC_("channels-action", "Delete this channel"),
    G_CALLBACK (channels_delete_cmd_callback),
    GIMP_HELP_CHANNEL_DELETE },

  { "channels-raise", GTK_STOCK_GO_UP,
    NC_("channels-action", "_Raise Channel"), "",
    NC_("channels-action", "Raise this channel one step in the channel stack"),
    G_CALLBACK (channels_raise_cmd_callback),
    GIMP_HELP_CHANNEL_RAISE },

  { "channels-raise-to-top", GTK_STOCK_GOTO_TOP,
    NC_("channels-action", "Raise Channel to _Top"), "",
    NC_("channels-action",
        "Raise this channel to the top of the channel stack"),
    G_CALLBACK (channels_raise_to_top_cmd_callback),
    GIMP_HELP_CHANNEL_RAISE_TO_TOP },

  { "channels-lower", GTK_STOCK_GO_DOWN,
    NC_("channels-action", "_Lower Channel"), "",
    NC_("channels-action", "Lower this channel one step in the channel stack"),
    G_CALLBACK (channels_lower_cmd_callback),
    GIMP_HELP_CHANNEL_LOWER },

  { "channels-lower-to-bottom", GTK_STOCK_GOTO_BOTTOM,
    NC_("channels-action", "Lower Channel to _Bottom"), "",
    NC_("channels-action",
        "Lower this channel to the bottom of the channel stack"),
    G_CALLBACK (channels_lower_to_bottom_cmd_callback),
    GIMP_HELP_CHANNEL_LOWER_TO_BOTTOM }
};

static const GimpEnumActionEntry channels_to_selection_actions[] =
{
  { "channels-selection-replace", GIMP_STOCK_SELECTION_REPLACE,
    NC_("channels-action", "Channel to Sele_ction"), NULL,
    NC_("channels-action", "Replace the selection with this channel"),
    GIMP_CHANNEL_OP_REPLACE, FALSE,
    GIMP_HELP_CHANNEL_SELECTION_REPLACE },

  { "channels-selection-add", GIMP_STOCK_SELECTION_ADD,
    NC_("channels-action", "_Add to Selection"), NULL,
    NC_("channels-action", "Add this channel to the current selection"),
    GIMP_CHANNEL_OP_ADD, FALSE,
    GIMP_HELP_CHANNEL_SELECTION_ADD },

  { "channels-selection-subtract", GIMP_STOCK_SELECTION_SUBTRACT,
    NC_("channels-action", "_Subtract from Selection"), NULL,
    NC_("channels-action", "Subtract this channel from the current selection"),
    GIMP_CHANNEL_OP_SUBTRACT, FALSE,
    GIMP_HELP_CHANNEL_SELECTION_SUBTRACT },

  { "channels-selection-intersect", GIMP_STOCK_SELECTION_INTERSECT,
    NC_("channels-action", "_Intersect with Selection"), NULL,
    NC_("channels-action", "Intersect this channel with the current selection"),
    GIMP_CHANNEL_OP_INTERSECT, FALSE,
    GIMP_HELP_CHANNEL_SELECTION_INTERSECT }
};


void
channels_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "channels-action",
                                 channels_actions,
                                 G_N_ELEMENTS (channels_actions));

  gimp_action_group_add_enum_actions (group, "channels-action",
                                      channels_to_selection_actions,
                                      G_N_ELEMENTS (channels_to_selection_actions),
                                      G_CALLBACK (channels_to_selection_cmd_callback));
}

void
channels_actions_update (GimpActionGroup *group,
                         gpointer         data)
{
  GimpImage   *image     = action_data_get_image (data);
  GimpChannel *channel   = NULL;
  gboolean     fs        = FALSE;
  gboolean     component = FALSE;
  GList       *next      = NULL;
  GList       *prev      = NULL;

  if (image)
    {
      fs = (gimp_image_get_floating_selection (image) != NULL);

      if (GIMP_IS_COMPONENT_EDITOR (data))
        {
          if (GIMP_COMPONENT_EDITOR (data)->clicked_component != -1)
            component = TRUE;
        }
      else
        {
          channel = gimp_image_get_active_channel (image);

          if (channel)
            {
              GList *channel_list;
              GList *list;

              channel_list = gimp_item_get_container_iter (GIMP_ITEM (channel));

              list = g_list_find (channel_list, channel);

              if (list)
                {
                  prev = g_list_previous (list);
                  next = g_list_next (list);
                }
            }
        }
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("channels-edit-attributes", !fs && channel);

  SET_SENSITIVE ("channels-new",             !fs && image);
  SET_SENSITIVE ("channels-new-last-values", !fs && image);
  SET_SENSITIVE ("channels-duplicate",       !fs && (channel || component));
  SET_SENSITIVE ("channels-delete",          !fs && channel);

  SET_SENSITIVE ("channels-raise",           !fs && channel && prev);
  SET_SENSITIVE ("channels-raise-to-top",    !fs && channel && prev);
  SET_SENSITIVE ("channels-lower",           !fs && channel && next);
  SET_SENSITIVE ("channels-lower-to-bottom", !fs && channel && next);

  SET_SENSITIVE ("channels-selection-replace",   !fs && (channel || component));
  SET_SENSITIVE ("channels-selection-add",       !fs && (channel || component));
  SET_SENSITIVE ("channels-selection-subtract",  !fs && (channel || component));
  SET_SENSITIVE ("channels-selection-intersect", !fs && (channel || component));

#undef SET_SENSITIVE
}
