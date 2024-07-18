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

#include "core/gimpimage.h"
#include "core/gimpitem.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpcomponenteditor.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "channels-actions.h"
#include "channels-commands.h"
#include "items-actions.h"

#include "gimp-intl.h"


static const GimpActionEntry channels_actions[] =
{
  { "channels-edit-attributes", GIMP_ICON_EDIT,
    NC_("channels-action", "_Edit Channel Attributes..."), NULL, { NULL },
    NC_("channels-action", "Edit the channel's name, color and opacity"),
    channels_edit_attributes_cmd_callback,
    GIMP_HELP_CHANNEL_EDIT },

  { "channels-new", GIMP_ICON_DOCUMENT_NEW,
    NC_("channels-action", "_New Channel..."), NULL, { NULL },
    NC_("channels-action", "Create a new channel"),
    channels_new_cmd_callback,
    GIMP_HELP_CHANNEL_NEW },

  { "channels-new-last-values", GIMP_ICON_DOCUMENT_NEW,
    NC_("channels-action", "_New Channel"), NULL, { NULL },
    NC_("channels-action", "Create a new channel with last used values"),
    channels_new_last_vals_cmd_callback,
    GIMP_HELP_CHANNEL_NEW },

  { "channels-duplicate", GIMP_ICON_OBJECT_DUPLICATE,
    NC_("channels-action", "D_uplicate Channels"), NULL, { NULL },
    NC_("channels-action",
        "Create duplicates of selected channels and add them to the image"),
    channels_duplicate_cmd_callback,
    GIMP_HELP_CHANNEL_DUPLICATE },

  { "channels-delete", GIMP_ICON_EDIT_DELETE,
    NC_("channels-action", "_Delete Channels"), NULL, { NULL },
    NC_("channels-action", "Delete selected channels"),
    channels_delete_cmd_callback,
    GIMP_HELP_CHANNEL_DELETE },

  { "channels-raise", GIMP_ICON_GO_UP,
    NC_("channels-action", "_Raise Channels"), NULL, { NULL },
    NC_("channels-action", "Raise these channels one step in the channel stack"),
    channels_raise_cmd_callback,
    GIMP_HELP_CHANNEL_RAISE },

  { "channels-raise-to-top", GIMP_ICON_GO_TOP,
    NC_("channels-action", "Raise Channels to _Top"), NULL, { NULL },
    NC_("channels-action",
        "Raise these channels to the top of the channel stack"),
    channels_raise_to_top_cmd_callback,
    GIMP_HELP_CHANNEL_RAISE_TO_TOP },

  { "channels-lower", GIMP_ICON_GO_DOWN,
    NC_("channels-action", "_Lower Channels"), NULL, { NULL },
    NC_("channels-action", "Lower these channels one step in the channel stack"),
    channels_lower_cmd_callback,
    GIMP_HELP_CHANNEL_LOWER },

  { "channels-lower-to-bottom", GIMP_ICON_GO_BOTTOM,
    NC_("channels-action", "Lower Channels to _Bottom"), NULL, { NULL },
    NC_("channels-action",
        "Lower these channels to the bottom of the channel stack"),
    channels_lower_to_bottom_cmd_callback,
    GIMP_HELP_CHANNEL_LOWER_TO_BOTTOM }
};

static const GimpToggleActionEntry channels_toggle_actions[] =
{
  { "channels-visible", GIMP_ICON_VISIBLE,
    NC_("channels-action", "Toggle Channel _Visibility"), NULL, { NULL }, NULL,
    channels_visible_cmd_callback,
    FALSE,
    GIMP_HELP_CHANNEL_VISIBLE },

  { "channels-lock-content", GIMP_ICON_LOCK_CONTENT,
    NC_("channels-action", "L_ock Pixels of Channel"), NULL, { NULL }, NULL,
    channels_lock_content_cmd_callback,
    FALSE,
    GIMP_HELP_CHANNEL_LOCK_PIXELS },

  { "channels-lock-position", GIMP_ICON_LOCK_POSITION,
    NC_("channels-action", "L_ock Position of Channel"), NULL, { NULL }, NULL,
    channels_lock_position_cmd_callback,
    FALSE,
    GIMP_HELP_CHANNEL_LOCK_POSITION }
};

static const GimpEnumActionEntry channels_color_tag_actions[] =
{
  { "channels-color-tag-none", GIMP_ICON_EDIT_CLEAR,
    NC_("channels-action", "None"), NULL, { NULL },
    NC_("channels-action", "Channel Color Tag: Clear"),
    GIMP_COLOR_TAG_NONE, FALSE,
    GIMP_HELP_CHANNEL_COLOR_TAG },

  { "channels-color-tag-blue", NULL,
    NC_("channels-action", "Blue"), NULL, { NULL },
    NC_("channels-action", "Channel Color Tag: Set to Blue"),
    GIMP_COLOR_TAG_BLUE, FALSE,
    GIMP_HELP_CHANNEL_COLOR_TAG },

  { "channels-color-tag-green", NULL,
    NC_("channels-action", "Green"), NULL, { NULL },
    NC_("channels-action", "Channel Color Tag: Set to Green"),
    GIMP_COLOR_TAG_GREEN, FALSE,
    GIMP_HELP_CHANNEL_COLOR_TAG },

  { "channels-color-tag-yellow", NULL,
    NC_("channels-action", "Yellow"), NULL, { NULL },
    NC_("channels-action", "Channel Color Tag: Set to Yellow"),
    GIMP_COLOR_TAG_YELLOW, FALSE,
    GIMP_HELP_CHANNEL_COLOR_TAG },

  { "channels-color-tag-orange", NULL,
    NC_("channels-action", "Orange"), NULL, { NULL },
    NC_("channels-action", "Channel Color Tag: Set to Orange"),
    GIMP_COLOR_TAG_ORANGE, FALSE,
    GIMP_HELP_CHANNEL_COLOR_TAG },

  { "channels-color-tag-brown", NULL,
    NC_("channels-action", "Brown"), NULL, { NULL },
    NC_("channels-action", "Channel Color Tag: Set to Brown"),
    GIMP_COLOR_TAG_BROWN, FALSE,
    GIMP_HELP_CHANNEL_COLOR_TAG },

  { "channels-color-tag-red", NULL,
    NC_("channels-action", "Red"), NULL, { NULL },
    NC_("channels-action", "Channel Color Tag: Set to Red"),
    GIMP_COLOR_TAG_RED, FALSE,
    GIMP_HELP_CHANNEL_COLOR_TAG },

  { "channels-color-tag-violet", NULL,
    NC_("channels-action", "Violet"), NULL, { NULL },
    NC_("channels-action", "Channel Color Tag: Set to Violet"),
    GIMP_COLOR_TAG_VIOLET, FALSE,
    GIMP_HELP_CHANNEL_COLOR_TAG },

  { "channels-color-tag-gray", NULL,
    NC_("channels-action", "Gray"), NULL, { NULL },
    NC_("channels-action", "Channel Color Tag: Set to Gray"),
    GIMP_COLOR_TAG_GRAY, FALSE,
    GIMP_HELP_CHANNEL_COLOR_TAG }
};

static const GimpEnumActionEntry channels_to_selection_actions[] =
{
  { "channels-selection-replace", GIMP_ICON_SELECTION_REPLACE,
    NC_("channels-action", "Channels to Sele_ction"), NULL, { NULL },
    NC_("channels-action", "Replace the selection with selected channels"),
    GIMP_CHANNEL_OP_REPLACE, FALSE,
    GIMP_HELP_CHANNEL_SELECTION_REPLACE },

  { "channels-selection-add", GIMP_ICON_SELECTION_ADD,
    NC_("channels-action", "_Add Channels to Selection"), NULL, { NULL },
    NC_("channels-action", "Add selected channels to the current selection"),
    GIMP_CHANNEL_OP_ADD, FALSE,
    GIMP_HELP_CHANNEL_SELECTION_ADD },

  { "channels-selection-subtract", GIMP_ICON_SELECTION_SUBTRACT,
    NC_("channels-action", "_Subtract Channels from Selection"), NULL, { NULL },
    NC_("channels-action", "Subtract selected channels from the current selection"),
    GIMP_CHANNEL_OP_SUBTRACT, FALSE,
    GIMP_HELP_CHANNEL_SELECTION_SUBTRACT },

  { "channels-selection-intersect", GIMP_ICON_SELECTION_INTERSECT,
    NC_("channels-action", "_Intersect Channels with Selection"), NULL, { NULL },
    NC_("channels-action", "Intersect selected channels with the current selection and each other"),
    GIMP_CHANNEL_OP_INTERSECT, FALSE,
    GIMP_HELP_CHANNEL_SELECTION_INTERSECT }
};

static const GimpEnumActionEntry channels_select_actions[] =
{
  { "channels-select-top", NULL,
    NC_("channels-action", "Select _Top Channel"), NULL, { NULL },
    NC_("channels-action", "Select the topmost channel"),
    GIMP_ACTION_SELECT_FIRST, FALSE,
    GIMP_HELP_CHANNEL_TOP },

  { "channels-select-bottom", NULL,
    NC_("channels-action", "Select _Bottom Channel"), NULL, { NULL },
    NC_("channels-action", "Select the bottommost channel"),
    GIMP_ACTION_SELECT_LAST, FALSE,
    GIMP_HELP_CHANNEL_BOTTOM },

  { "channels-select-previous", NULL,
    NC_("channels-action", "Select _Previous Channels"), NULL, { NULL },
    NC_("channels-action", "Select the channels above the selected channels"),
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    GIMP_HELP_CHANNEL_PREVIOUS },

  { "channels-select-next", NULL,
    NC_("channels-action", "Select _Next Channels"), NULL, { NULL },
    NC_("channels-action", "Select the channels below the selected channels"),
    GIMP_ACTION_SELECT_NEXT, FALSE,
    GIMP_HELP_CHANNEL_NEXT }
};


void
channels_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "channels-action",
                                 channels_actions,
                                 G_N_ELEMENTS (channels_actions));

  gimp_action_group_add_toggle_actions (group, "channels-action",
                                        channels_toggle_actions,
                                        G_N_ELEMENTS (channels_toggle_actions));

  gimp_action_group_add_enum_actions (group, "channels-action",
                                      channels_color_tag_actions,
                                      G_N_ELEMENTS (channels_color_tag_actions),
                                      channels_color_tag_cmd_callback);

  gimp_action_group_add_enum_actions (group, "channels-action",
                                      channels_to_selection_actions,
                                      G_N_ELEMENTS (channels_to_selection_actions),
                                      channels_to_selection_cmd_callback);

  gimp_action_group_add_enum_actions (group, "channels-action",
                                      channels_select_actions,
                                      G_N_ELEMENTS (channels_select_actions),
                                      channels_select_cmd_callback);

  items_actions_setup (group, "channels");
}

void
channels_actions_update (GimpActionGroup *group,
                         gpointer         data)
{
  GimpImage   *image               = action_data_get_image (data);
  gboolean     fs                  = FALSE;
  gboolean     component           = FALSE;
  GList       *selected_channels   = NULL;
  gint         n_selected_channels = 0;
  gint         n_channels          = 0;
  gboolean     have_prev           = FALSE; /* At least 1 selected channel has a previous sibling. */
  gboolean     have_next           = FALSE; /* At least 1 selected channel has a next sibling.     */
  gboolean     first_selected      = FALSE; /* First channel is selected */
  gboolean     last_selected       = FALSE; /* Last channel is selected */

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
          GList *iter;

          selected_channels   = gimp_image_get_selected_channels (image);
          n_selected_channels = g_list_length (selected_channels);
          n_channels          = gimp_image_get_n_channels (image);

          for (iter = selected_channels; iter; iter = iter->next)
            {
              GList *channel_list;
              GList *list;

              channel_list = gimp_item_get_container_iter (GIMP_ITEM (iter->data));

              list = g_list_find (channel_list, iter->data);

              if (list)
                {
                  if (gimp_item_get_index (list->data) == 0)
                    first_selected = TRUE;
                  if (gimp_item_get_index (list->data) == n_channels - 1)
                    last_selected = TRUE;

                  if (g_list_previous (list))
                    have_prev = TRUE;
                  if (g_list_next (list))
                    have_next = TRUE;
                }
            }
        }
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_SENSITIVE ("channels-edit-attributes", !fs && n_selected_channels == 1);

  SET_SENSITIVE ("channels-new",             !fs && image);
  SET_SENSITIVE ("channels-new-last-values", !fs && image);
  SET_SENSITIVE ("channels-duplicate",       !fs && (n_selected_channels > 0 || component));
  SET_SENSITIVE ("channels-delete",          !fs && n_selected_channels > 0);

  SET_SENSITIVE ("channels-raise",           !fs && n_selected_channels > 0 && have_prev && !first_selected);
  SET_SENSITIVE ("channels-raise-to-top",    !fs && n_selected_channels > 0 && have_prev);
  SET_SENSITIVE ("channels-lower",           !fs && n_selected_channels > 0 && have_next && !last_selected);
  SET_SENSITIVE ("channels-lower-to-bottom", !fs && n_selected_channels > 0 && have_next);

  SET_SENSITIVE ("channels-selection-replace",   !fs && (n_selected_channels > 0 || component));
  SET_SENSITIVE ("channels-selection-add",       !fs && (n_selected_channels > 0 || component));
  SET_SENSITIVE ("channels-selection-subtract",  !fs && (n_selected_channels > 0 || component));
  SET_SENSITIVE ("channels-selection-intersect", !fs && (n_selected_channels > 0 || component));

  SET_SENSITIVE ("channels-select-top",      !fs && n_channels > 0 && (n_selected_channels == 0 || have_prev));
  SET_SENSITIVE ("channels-select-bottom",   !fs && n_channels > 0 && (n_selected_channels == 0 || have_next));
  SET_SENSITIVE ("channels-select-previous", !fs && n_selected_channels > 0 && have_prev);
  SET_SENSITIVE ("channels-select-next",     !fs && n_selected_channels > 0 && have_next);

#undef SET_SENSITIVE

  items_actions_update (group, "channels", selected_channels);
}
