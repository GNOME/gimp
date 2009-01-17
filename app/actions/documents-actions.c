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
  { "documents-popup", "document-open-recent",
    NC_("documents-action", "Documents Menu"), NULL, NULL, NULL,
    GIMP_HELP_DOCUMENT_DIALOG },

  { "documents-open", GTK_STOCK_OPEN,
    NC_("documents-action", "_Open Image"), "",
    NC_("documents-action", "Open the selected entry"),
    G_CALLBACK (documents_open_cmd_callback),
    GIMP_HELP_DOCUMENT_OPEN },

  { "documents-raise-or-open", NULL,
    NC_("documents-action", "_Raise or Open Image"), "",
    NC_("documents-action", "Raise window if already open"),
    G_CALLBACK (documents_raise_or_open_cmd_callback),
    GIMP_HELP_DOCUMENT_OPEN },

  { "documents-file-open-dialog", NULL,
    NC_("documents-action", "File Open _Dialog"), "",
    NC_("documents-action", "Open image dialog"),
    G_CALLBACK (documents_file_open_dialog_cmd_callback),
    GIMP_HELP_DOCUMENT_OPEN },

  { "documents-copy-location", GTK_STOCK_COPY,
    NC_("documents-action", "Copy Image _Location"), "",
    NC_("documents-action", "Copy image location to clipboard"),
    G_CALLBACK (documents_copy_location_cmd_callback),
    GIMP_HELP_DOCUMENT_COPY_LOCATION },

  { "documents-remove", GTK_STOCK_REMOVE,
    NC_("documents-action", "Remove _Entry"), "",
    NC_("documents-action", "Remove the selected entry"),
    G_CALLBACK (documents_remove_cmd_callback),
    GIMP_HELP_DOCUMENT_REMOVE },

  { "documents-clear", GTK_STOCK_CLEAR,
    NC_("documents-action", "_Clear History"), "",
    NC_("documents-action", "Clear the entire document history"),
    G_CALLBACK (documents_clear_cmd_callback),
    GIMP_HELP_DOCUMENT_CLEAR },

  { "documents-recreate-preview", GTK_STOCK_REFRESH,
    NC_("documents-action", "Recreate _Preview"), "",
    NC_("documents-action", "Recreate preview"),
    G_CALLBACK (documents_recreate_preview_cmd_callback),
    GIMP_HELP_DOCUMENT_REFRESH },

  { "documents-reload-previews", NULL,
    NC_("documents-action", "Reload _all Previews"), "",
    NC_("documents-action", "Reload all previews"),
    G_CALLBACK (documents_reload_previews_cmd_callback),
    GIMP_HELP_DOCUMENT_REFRESH },

  { "documents-remove-dangling", NULL,
    NC_("documents-action", "Remove Dangling E_ntries"), "",
    NC_("documents-action",
        "Remove entries for which the corresponding file is not available"),
    G_CALLBACK (documents_remove_dangling_cmd_callback),
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
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("documents-open",             imagefile);
  SET_SENSITIVE ("documents-raise-or-open",    imagefile);
  SET_SENSITIVE ("documents-file-open-dialog", TRUE);
  SET_SENSITIVE ("documents-copy-location",    imagefile);
  SET_SENSITIVE ("documents-remove",           imagefile);
  SET_SENSITIVE ("documents-clear",            TRUE);
  SET_SENSITIVE ("documents-recreate-preview", imagefile);
  SET_SENSITIVE ("documents-reload-previews",  imagefile);
  SET_SENSITIVE ("documents-remove-dangling",  imagefile);

#undef SET_SENSITIVE
}
