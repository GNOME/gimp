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

#include "core/gimpcontext.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "documents-actions.h"
#include "documents-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry documents_actions[] =
{
  { "documents-open", GIMP_ICON_DOCUMENT_OPEN,
    NC_("documents-action", "_Open Image"), NULL, { NULL },
    NC_("documents-action", "Open the selected entry"),
    documents_open_cmd_callback,
    GIMP_HELP_DOCUMENT_OPEN },

  { "documents-raise-or-open", NULL,
    NC_("documents-action", "_Raise or Open Image"), NULL, { NULL },
    NC_("documents-action", "Raise window if already open"),
    documents_raise_or_open_cmd_callback,
    GIMP_HELP_DOCUMENT_OPEN },

  { "documents-file-open-dialog", NULL,
    NC_("documents-action", "File Open _Dialog"), NULL, { NULL },
    NC_("documents-action", "Open image dialog"),
    documents_file_open_dialog_cmd_callback,
    GIMP_HELP_DOCUMENT_OPEN },

  { "documents-copy-location", GIMP_ICON_EDIT_COPY,
    NC_("documents-action", "Copy Image _Location"), NULL, { NULL },
    NC_("documents-action", "Copy image location to clipboard"),
    documents_copy_location_cmd_callback,
    GIMP_HELP_DOCUMENT_COPY_LOCATION },

  { "documents-show-in-file-manager", GIMP_ICON_FILE_MANAGER,
    NC_("documents-action", "Show in _File Manager"), NULL, { NULL },
    NC_("documents-action", "Show image location in the file manager"),
    documents_show_in_file_manager_cmd_callback,
    GIMP_HELP_DOCUMENT_SHOW_IN_FILE_MANAGER },

  { "documents-remove", GIMP_ICON_LIST_REMOVE,
    NC_("documents-action", "Remove _Entry"), NULL, { NULL },
    NC_("documents-action", "Remove the selected entry"),
    documents_remove_cmd_callback,
    GIMP_HELP_DOCUMENT_REMOVE },

  { "documents-clear", GIMP_ICON_SHRED,
    NC_("documents-action", "_Clear History"), NULL, { NULL },
    NC_("documents-action", "Clear the entire document history"),
    documents_clear_cmd_callback,
    GIMP_HELP_DOCUMENT_CLEAR },

  { "documents-recreate-preview", GIMP_ICON_VIEW_REFRESH,
    NC_("documents-action", "Recreate _Preview"), NULL, { NULL },
    NC_("documents-action", "Recreate preview"),
    documents_recreate_preview_cmd_callback,
    GIMP_HELP_DOCUMENT_REFRESH },

  { "documents-reload-previews", NULL,
    NC_("documents-action", "Reload _all Previews"), NULL, { NULL },
    NC_("documents-action", "Reload all previews"),
    documents_reload_previews_cmd_callback,
    GIMP_HELP_DOCUMENT_REFRESH },

  { "documents-remove-dangling", NULL,
    NC_("documents-action", "Remove Dangling E_ntries"), NULL, { NULL },
    NC_("documents-action",
        "Remove entries for which the corresponding file is not available"),
    documents_remove_dangling_cmd_callback,
    GIMP_HELP_DOCUMENT_REFRESH }
};


void
documents_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "documents-action",
                                 documents_actions,
                                 G_N_ELEMENTS (documents_actions));
}

void
documents_actions_update (GimpActionGroup *group,
                          gpointer         data)
{
  GimpContext   *context;
  GimpImagefile *imagefile = NULL;

  context = action_data_get_context (data);

  if (context)
    imagefile = gimp_context_get_imagefile (context);

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_SENSITIVE ("documents-open",                 imagefile);
  SET_SENSITIVE ("documents-raise-or-open",        imagefile);
  SET_SENSITIVE ("documents-file-open-dialog",     TRUE);
  SET_SENSITIVE ("documents-copy-location",        imagefile);
  SET_SENSITIVE ("documents-show-in-file-manager", imagefile);
  SET_SENSITIVE ("documents-remove",               imagefile);
  SET_SENSITIVE ("documents-clear",                TRUE);
  SET_SENSITIVE ("documents-recreate-preview",     imagefile);
  SET_SENSITIVE ("documents-reload-previews",      imagefile);
  SET_SENSITIVE ("documents-remove-dangling",      imagefile);

#undef SET_SENSITIVE
}
