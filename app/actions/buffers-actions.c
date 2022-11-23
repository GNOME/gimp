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

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmahelp-ids.h"

#include "actions.h"
#include "buffers-actions.h"
#include "buffers-commands.h"

#include "ligma-intl.h"


static const LigmaActionEntry buffers_actions[] =
{
  { "buffers-popup", LIGMA_ICON_BUFFER,
    NC_("buffers-action", "Buffers Menu"), NULL, NULL, NULL,
    LIGMA_HELP_BUFFER_DIALOG },

  { "buffers-paste-as-new-image", LIGMA_ICON_EDIT_PASTE_AS_NEW,
    NC_("buffers-action", "Paste Buffer as _New Image"), NULL,
    NC_("buffers-action", "Paste the selected buffer as a new image"),
    buffers_paste_as_new_image_cmd_callback,
    LIGMA_HELP_BUFFER_PASTE_AS_NEW_IMAGE },

  { "buffers-delete", LIGMA_ICON_EDIT_DELETE,
    NC_("buffers-action", "_Delete Buffer"), NULL,
    NC_("buffers-action", "Delete the selected buffer"),
    buffers_delete_cmd_callback,
    LIGMA_HELP_BUFFER_DELETE }
};

static const LigmaEnumActionEntry buffers_paste_actions[] =
{
  { "buffers-paste", LIGMA_ICON_EDIT_PASTE,
    NC_("buffers-action", "_Paste Buffer"), NULL,
    NC_("buffers-action", "Paste the selected buffer"),
    LIGMA_PASTE_TYPE_FLOATING, FALSE,
    LIGMA_HELP_BUFFER_PASTE },

  { "buffers-paste-in-place", LIGMA_ICON_EDIT_PASTE,
    NC_("buffers-action", "Paste Buffer In Pl_ace"), NULL,
    NC_("buffers-action", "Paste the selected buffer at its original position"),
    LIGMA_PASTE_TYPE_FLOATING_IN_PLACE, FALSE,
    LIGMA_HELP_BUFFER_PASTE_IN_PLACE },

  { "buffers-paste-into", LIGMA_ICON_EDIT_PASTE_INTO,
    NC_("buffers-action", "Paste Buffer _Into The Selection"), NULL,
    NC_("buffers-action", "Paste the selected buffer into the selection"),
    LIGMA_PASTE_TYPE_FLOATING_INTO, FALSE,
    LIGMA_HELP_BUFFER_PASTE_INTO },

  { "buffers-paste-into-in-place", LIGMA_ICON_EDIT_PASTE_INTO,
    NC_("buffers-action", "Paste Buffer Into The Selection In Place"), NULL,
    NC_("buffers-action",
        "Paste the selected buffer into the selection at its original position"),
    LIGMA_PASTE_TYPE_FLOATING_INTO_IN_PLACE, FALSE,
    LIGMA_HELP_BUFFER_PASTE_INTO_IN_PLACE },

  { "buffers-paste-as-new-layer", LIGMA_ICON_EDIT_PASTE_AS_NEW,
    NC_("buffers-action", "Paste Buffer as New _Layer"), NULL,
    NC_("buffers-action", "Paste the selected buffer as a new layer"),
    LIGMA_PASTE_TYPE_NEW_LAYER, FALSE,
    LIGMA_HELP_BUFFER_PASTE_AS_NEW_LAYER },

  { "buffers-paste-as-new-layer-in-place", LIGMA_ICON_EDIT_PASTE_AS_NEW,
    NC_("buffers-action", "Paste Buffer as New Layer in Place"), NULL,
    NC_("buffers-action",
        "Paste the selected buffer as a new layer at its original position"),
    LIGMA_PASTE_TYPE_NEW_LAYER_IN_PLACE, FALSE,
    LIGMA_HELP_BUFFER_PASTE_AS_NEW_LAYER_IN_PLACE },
};


void
buffers_actions_setup (LigmaActionGroup *group)
{
  ligma_action_group_add_actions (group, "buffers-action",
                                 buffers_actions,
                                 G_N_ELEMENTS (buffers_actions));

  ligma_action_group_add_enum_actions (group, "buffers-action",
                                      buffers_paste_actions,
                                      G_N_ELEMENTS (buffers_paste_actions),
                                      buffers_paste_cmd_callback);
}

void
buffers_actions_update (LigmaActionGroup *group,
                        gpointer         data)
{
  LigmaContext *context = action_data_get_context (data);
  LigmaBuffer  *buffer  = NULL;

  if (context)
    buffer = ligma_context_get_buffer (context);

#define SET_SENSITIVE(action,condition,reason) \
        ligma_action_group_set_action_sensitive (group, action, (condition) != 0, reason)

  SET_SENSITIVE ("buffers-paste",                       buffer, _("No selected buffer"));
  SET_SENSITIVE ("buffers-paste-in-place",              buffer, _("No selected buffer"));
  SET_SENSITIVE ("buffers-paste-into",                  buffer, _("No selected buffer"));
  SET_SENSITIVE ("buffers-paste-into-in-place",         buffer, _("No selected buffer"));
  SET_SENSITIVE ("buffers-paste-as-new-layer",          buffer, _("No selected buffer"));
  SET_SENSITIVE ("buffers-paste-as-new-layer-in-place", buffer, _("No selected buffer"));
  SET_SENSITIVE ("buffers-paste-as-new-image",          buffer, _("No selected buffer"));
  SET_SENSITIVE ("buffers-delete",                      buffer, _("No selected buffer"));

#undef SET_SENSITIVE
}
