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

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "buffers-actions.h"
#include "buffers-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry buffers_actions[] =
{
  { "buffers-popup", GIMP_STOCK_BUFFER,
    NC_("buffers-action", "Buffers Menu"), NULL, NULL, NULL,
    GIMP_HELP_BUFFER_DIALOG },

  { "buffers-paste", "edit-paste",
    NC_("buffers-action", "_Paste Buffer"), NULL,
    NC_("buffers-action", "Paste the selected buffer"),
    G_CALLBACK (buffers_paste_cmd_callback),
    GIMP_HELP_BUFFER_PASTE },

  { "buffers-paste-into", GIMP_STOCK_PASTE_INTO,
    NC_("buffers-action", "Paste Buffer _Into"), NULL,
    NC_("buffers-action", "Paste the selected buffer into the selection"),
    G_CALLBACK (buffers_paste_into_cmd_callback),
    GIMP_HELP_BUFFER_PASTE_INTO },

  { "buffers-paste-as-new", GIMP_STOCK_PASTE_AS_NEW,
    NC_("buffers-action", "Paste Buffer as _New"), NULL,
    NC_("buffers-action", "Paste the selected buffer as a new image"),
    G_CALLBACK (buffers_paste_as_new_cmd_callback),
    GIMP_HELP_BUFFER_PASTE_AS_NEW },

  { "buffers-delete", "edit-delete",
    NC_("buffers-action", "_Delete Buffer"), NULL,
    NC_("buffers-action", "Delete the selected buffer"),
    G_CALLBACK (buffers_delete_cmd_callback),
    GIMP_HELP_BUFFER_DELETE }
};


void
buffers_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "buffers-action",
                                 buffers_actions,
                                 G_N_ELEMENTS (buffers_actions));
}

void
buffers_actions_update (GimpActionGroup *group,
                        gpointer         data)
{
  GimpContext *context = action_data_get_context (data);
  GimpBuffer  *buffer  = NULL;

  if (context)
    buffer = gimp_context_get_buffer (context);

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("buffers-paste",        buffer);
  SET_SENSITIVE ("buffers-paste-into",   buffer);
  SET_SENSITIVE ("buffers-paste-as-new", buffer);
  SET_SENSITIVE ("buffers-delete",       buffer);

#undef SET_SENSITIVE
}
