/* The GIMP -- an image manipulation program
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
#include "widgets/gimpitemtreeview.h"

#include "channels-actions.h"
#include "channels-commands.h"

#include "gimp-intl.h"


static GimpActionEntry channels_actions[] =
{
  { "channels-edit-attributes", GIMP_STOCK_EDIT,
    N_("_Edit Channel Attributes..."), NULL, NULL,
    G_CALLBACK (channels_edit_attributes_cmd_callback),
    GIMP_HELP_CHANNEL_EDIT },

  { "channels-new", GTK_STOCK_NEW,
    N_("/_New Channel..."), "", NULL,
    G_CALLBACK (channels_new_cmd_callback),
    GIMP_HELP_CHANNEL_NEW },

  { "channels-duplicate", GIMP_STOCK_DUPLICATE,
    N_("D_uplicate Channel"), NULL, NULL,
    G_CALLBACK (channels_duplicate_cmd_callback),
    GIMP_HELP_CHANNEL_DUPLICATE },

  { "channels-delete", GTK_STOCK_DELETE,
    N_("_Delete Channel"), "", NULL,
    G_CALLBACK (channels_delete_cmd_callback),
    GIMP_HELP_CHANNEL_DELETE },

  { "channels-raise", GTK_STOCK_GO_UP,
    N_("_Raise Channel"), "", NULL,
    G_CALLBACK (channels_raise_cmd_callback),
    GIMP_HELP_CHANNEL_RAISE },

  { "channels-raise-to-top", GTK_STOCK_GOTO_TOP,
    N_("Raise Channel to _Top"), "", NULL,
    G_CALLBACK (channels_raise_to_top_cmd_callback),
    GIMP_HELP_CHANNEL_RAISE_TO_TOP },

  { "channels-lower", GTK_STOCK_GO_DOWN,
    N_("_Lower Channel"), "", NULL,
    G_CALLBACK (channels_lower_cmd_callback),
    GIMP_HELP_CHANNEL_LOWER },

  { "channels-lower-to-bottom", GTK_STOCK_GOTO_BOTTOM,
    N_("Lower Channel to _Bottom"), "", NULL,
    G_CALLBACK (channels_lower_to_bottom_cmd_callback),
    GIMP_HELP_CHANNEL_LOWER_TO_BOTTOM }
};

static GimpEnumActionEntry channels_to_selection_actions[] =
{
  { "channels-selection-replace", GIMP_STOCK_SELECTION_REPLACE,
    N_("Channel to Sele_ction"), NULL, NULL,
    GIMP_CHANNEL_OP_REPLACE,
    GIMP_HELP_CHANNEL_SELECTION_REPLACE },

  { "channels-selection-add", GIMP_STOCK_SELECTION_ADD,
    N_("_Add to Selection"), NULL, NULL,
    GIMP_CHANNEL_OP_ADD,
    GIMP_HELP_CHANNEL_SELECTION_ADD },

  { "channels-selection-subtract", GIMP_STOCK_SELECTION_SUBTRACT,
    N_("_Subtract from Selection"), NULL, NULL,
    GIMP_CHANNEL_OP_SUBTRACT,
    GIMP_HELP_CHANNEL_SELECTION_SUBTRACT },

  { "channels-selection-intersect", GIMP_STOCK_SELECTION_INTERSECT,
    N_("_Intersect with Selection"), NULL, NULL,
    GIMP_CHANNEL_OP_INTERSECT,
    GIMP_HELP_CHANNEL_SELECTION_INTERSECT }
};


void
channels_actions_setup (GimpActionGroup *group,
                        gpointer         data)
{
  gimp_action_group_add_actions (group,
                                 channels_actions,
                                 G_N_ELEMENTS (channels_actions),
                                 data);

  gimp_action_group_add_enum_actions (group,
                                      channels_to_selection_actions,
                                      G_N_ELEMENTS (channels_to_selection_actions),
                                      G_CALLBACK (channels_to_selection_cmd_callback),
                                      data);
}

void
channels_actions_update (GimpActionGroup *group,
                         gpointer         data)
{
  GimpImage   *gimage    = NULL;
  GimpChannel *channel   = NULL;
  gboolean     fs        = FALSE;
  gboolean     component = FALSE;
  GList       *next      = NULL;
  GList       *prev      = NULL;

  if (GIMP_IS_COMPONENT_EDITOR (data))
    {
      gimage = GIMP_IMAGE_EDITOR (data)->gimage;

      if (gimage)
        {
          if (GIMP_COMPONENT_EDITOR (data)->clicked_component != -1)
            component = TRUE;
        }
    }
  else
    {
      gimage = GIMP_ITEM_TREE_VIEW (data)->gimage;

      if (gimage)
        {
          GList *list;

          channel = gimp_image_get_active_channel (gimage);

          for (list = GIMP_LIST (gimage->channels)->list;
               list;
               list = g_list_next (list))
            {
              if (channel == (GimpChannel *) list->data)
                {
                  prev = g_list_previous (list);
                  next = g_list_next (list);
                  break;
                }
            }
        }
    }

  if (gimage)
    fs = (gimp_image_floating_sel (gimage) != NULL);

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("channels-edit-attributes", !fs && channel);

  SET_SENSITIVE ("channels-new",             !fs && gimage);
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
