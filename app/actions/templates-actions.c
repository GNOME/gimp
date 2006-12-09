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
#include "templates-actions.h"
#include "templates-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry templates_actions[] =
{
  { "templates-popup", GIMP_STOCK_TEMPLATE,
    N_("Templates Menu"), NULL, NULL, NULL,
    GIMP_HELP_TEMPLATE_DIALOG },

  { "templates-create-image", GIMP_STOCK_IMAGE,
    N_("_Create Image from Template..."), "",
    N_("Create a new image from the selected template"),
    G_CALLBACK (templates_create_image_cmd_callback),
    GIMP_HELP_TEMPLATE_IMAGE_NEW },

  { "templates-new", GTK_STOCK_NEW,
    N_("_New Template..."), "",
    N_("Create a new template"),
    G_CALLBACK (templates_new_cmd_callback),
    GIMP_HELP_TEMPLATE_NEW },

  { "templates-duplicate", GIMP_STOCK_DUPLICATE,
    N_("D_uplicate Template..."), "",
    N_("Duplicate the selected template"),
    G_CALLBACK (templates_duplicate_cmd_callback),
    GIMP_HELP_TEMPLATE_DUPLICATE },

  { "templates-edit", GTK_STOCK_EDIT,
    N_("_Edit Template..."), "",
    N_("Edit the selected template"),
    G_CALLBACK (templates_edit_cmd_callback),
    GIMP_HELP_TEMPLATE_EDIT },

  { "templates-delete", GTK_STOCK_DELETE,
    N_("_Delete Template"), "",
    N_("Delete the selected template"),
    G_CALLBACK (templates_delete_cmd_callback),
    GIMP_HELP_TEMPLATE_DELETE }
};


void
templates_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 templates_actions,
                                 G_N_ELEMENTS (templates_actions));
}

void
templates_actions_update (GimpActionGroup *group,
                          gpointer         data)
{
  GimpContext  *context  = action_data_get_context (data);
  GimpTemplate *template = NULL;

  if (context)
    template = gimp_context_get_template (context);

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("templates-create-image", template);
  SET_SENSITIVE ("templates-new",          context);
  SET_SENSITIVE ("templates-duplicate",    template);
  SET_SENSITIVE ("templates-edit",         template);
  SET_SENSITIVE ("templates-delete",       template);

#undef SET_SENSITIVE
}
