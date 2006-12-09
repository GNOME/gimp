/* GIMP - The GNU Image Manipulation Program
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

#include "core/gimpimage.h"
#include "core/gimplist.h"

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
    N_("Channels Menu"), NULL, NULL, NULL,
    GIMP_HELP_CHANNEL_DIALOG },

  { "channels-edit-attributes", GTK_STOCK_EDIT,
    N_("_Edit Channel Attributes..."), NULL,
    N_("Edit the channel's name, color and opacity"),
    G_CALLBACK (channels_edit_attributes_cmd_callback),
    GIMP_HELP_CHANNEL_EDIT },

  { "channels-new", GTK_STOCK_NEW,
    N_("_New Channel..."), "",
    N_("Create a new channel"),
    G_CALLBACK (channels_new_cmd_callback),
    GIMP_HELP_CHANNEL_NEW },

  { "channels-new-last-values", GTK_STOCK_NEW,
    N_("_New Channel"), "",
    N_("Create a new channel with last used values"),
    G_CALLBACK (channels_new_last_vals_cmd_callback),
    GIMP_HELP_CHANNEL_NEW },

  { "channels-duplicate", GIMP_STOCK_DUPLICATE,
    N_("D_uplicate Channel"), NULL,
    N_("Create a duplicate of this channel and add it to the image"),
    G_CALLBACK (channels_duplicate_cmd_callback),
    GIMP_HELP_CHANNEL_DUPLICATE },

  { "channels-delete", GTK_STOCK_DELETE,
    N_("_Delete Channel"), "",
    N_("Delete this channel"),
    G_CALLBACK (channels_delete_cmd_callback),
    GIMP_HELP_CHANNEL_DELETE },

  { "channels-raise", GTK_STOCK_GO_UP,
    N_("_Raise Channel"), "",
    N_("Raise this channel one step in the channel stack"),
    G_CALLBACK (channels_raise_cmd_callback),
    GIMP_HELP_CHANNEL_RAISE },

  { "channels-raise-to-top", GTK_STOCK_GOTO_TOP,
    N_("Raise Channel to _Top"), "",
    N_("Raise this channel to the top of the channel stack"),
    G_CALLBACK (channels_raise_to_top_cmd_callback),
    GIMP_HELP_CHANNEL_RAISE_TO_TOP },

  { "channels-lower", GTK_STOCK_GO_DOWN,
    N_("_Lower Channel"), "",
    N_("Lower this channel one step in the channel stack"),
    G_CALLBACK (channels_lower_cmd_callback),
    GIMP_HELP_CHANNEL_LOWER },

  { "channels-lower-to-bottom", GTK_STOCK_GOTO_BOTTOM,
    N_("Lower Channel to _Bottom"), "",
    N_("Lower this channel to the bottom of the channel stack"),
    G_CALLBACK (channels_lower_to_bottom_cmd_callback),
    GIMP_HELP_CHANNEL_LOWER_TO_BOTTOM }
};

static const GimpEnumActionEntry channels_to_selection_actions[] =
{
  { "channels-selection-replace", GIMP_STOCK_SELECTION_REPLACE,
    N_("Channel to Sele_ction"), NULL,
    N_("Replace the selection with this channel"),
    GIMP_CHANNEL_OP_REPLACE, FALSE,
    GIMP_HELP_CHANNEL_SELECTION_REPLACE },

  { "channels-selection-add", GIMP_STOCK_SELECTION_ADD,
    N_("_Add to Selection"), NULL,
    N_("Add this channel to the current selection"),
    GIMP_CHANNEL_OP_ADD, FALSE,
    GIMP_HELP_CHANNEL_SELECTION_ADD },

  { "channels-selection-subtract", GIMP_STOCK_SELECTION_SUBTRACT,
    N_("_Subtract from Selection"), NULL,
    N_("Subtract this channel from the current selection"),
    GIMP_CHANNEL_OP_SUBTRACT, FALSE,
    GIMP_HELP_CHANNEL_SELECTION_SUBTRACT },

  { "channels-selection-intersect", GIMP_STOCK_SELECTION_INTERSECT,
    N_("_Intersect with Selection"), NULL,
    N_("Intersect this channel with the current selection"),
    GIMP_CHANNEL_OP_INTERSECT, FALSE,
    GIMP_HELP_CHANNEL_SELECTION_INTERSECT }
};


void
channels_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 channels_actions,
                                 G_N_ELEMENTS (channels_actions));

  gimp_action_group_add_enum_actions (group,
                                      channels_to_selection_actions,
                                      G_N_ELEMENTS (channels_to_selection_actions),
                                      G_CALLBACK (channels_to_selection_cmd_callback));
}

void
channels_actions_update (GimpActionGroup *group,
                         gpointer         data)
{
  GimpImage   *image    = action_data_get_image (data);
  GimpChannel *channel   = NULL;
  gboolean     fs        = FALSE;
  gboolean     component = FALSE;
  GList       *next      = NULL;
  GList       *prev      = NULL;

  if (image)
    {
      fs = (gimp_image_floating_sel (image) != NULL);

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
              GList *list;

              list = g_list_find (GIMP_LIST (image->channels)->list, channel);

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
