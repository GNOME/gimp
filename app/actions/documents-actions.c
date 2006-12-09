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

#include "core/gimpcontext.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "documents-actions.h"
#include "documents-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry documents_actions[] =
{
  { "documents-popup", GTK_STOCK_OPEN,
    N_("Documents Menu"), NULL, NULL, NULL,
    GIMP_HELP_DOCUMENT_DIALOG },

  { "documents-open", GTK_STOCK_OPEN,
    N_("_Open Image"), "",
    N_("Open the selected entry"),
    G_CALLBACK (documents_open_cmd_callback),
    GIMP_HELP_DOCUMENT_OPEN },

  { "documents-raise-or-open", GTK_STOCK_OPEN,
    N_("_Raise or Open Image"), "",
    N_("Raise window if already open"),
    G_CALLBACK (documents_raise_or_open_cmd_callback),
    GIMP_HELP_DOCUMENT_OPEN },

  { "documents-file-open-dialog", GTK_STOCK_OPEN,
    N_("File Open _Dialog"), "",
    N_("Open image dialog"),
    G_CALLBACK (documents_file_open_dialog_cmd_callback),
    GIMP_HELP_DOCUMENT_OPEN },

  { "documents-copy-location", GTK_STOCK_COPY,
    N_("Copy Image _Location"), "",
    N_("Copy image location to clipboard"),
    G_CALLBACK (documents_copy_location_cmd_callback),
    GIMP_HELP_DOCUMENT_COPY_LOCATION },

  { "documents-remove", GTK_STOCK_REMOVE,
    N_("Remove _Entry"), "",
    N_("Remove the selected entry"),
    G_CALLBACK (documents_remove_cmd_callback),
    GIMP_HELP_DOCUMENT_REMOVE },

  { "documents-clear", GTK_STOCK_CLEAR,
    N_("_Clear History"), "",
    N_("Clear the entire document history"),
    G_CALLBACK (documents_clear_cmd_callback),
    GIMP_HELP_DOCUMENT_CLEAR },

  { "documents-recreate-preview", GTK_STOCK_REFRESH,
    N_("Recreate _Preview"), "",
    N_("Recreate preview"),
    G_CALLBACK (documents_recreate_preview_cmd_callback),
    GIMP_HELP_DOCUMENT_REFRESH },

  { "documents-reload-previews", GTK_STOCK_REFRESH,
    N_("Reload _all Previews"), "",
    N_("Reload all previews"),
    G_CALLBACK (documents_reload_previews_cmd_callback),
    GIMP_HELP_DOCUMENT_REFRESH },

  { "documents-remove-dangling", GTK_STOCK_REFRESH,
    N_("Remove Dangling E_ntries"), "",
    N_("Remove dangling entries"),
    G_CALLBACK (documents_remove_dangling_cmd_callback),
    GIMP_HELP_DOCUMENT_REFRESH }
};


void
documents_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
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
