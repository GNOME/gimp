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

#include "core/gimpcontext.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpcontainereditor.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimphelp-ids.h"

#include "templates-actions.h"
#include "gui/templates-commands.h"

#include "gimp-intl.h"


static GimpActionEntry templates_actions[] =
{
  { "templates-create-image", GIMP_STOCK_IMAGE,
    N_("_Create Image from Template..."), "", NULL,
    G_CALLBACK (templates_create_image_cmd_callback),
    GIMP_HELP_TEMPLATE_IMAGE_NEW },

  { "templates-new", GTK_STOCK_NEW,
    N_("_New Template..."), "", NULL,
    G_CALLBACK (templates_new_template_cmd_callback),
    GIMP_HELP_TEMPLATE_NEW },

  { "templates-duplicate", GIMP_STOCK_DUPLICATE,
    N_("D_uplicate Template..."), "", NULL,
    G_CALLBACK (templates_duplicate_template_cmd_callback),
    GIMP_HELP_TEMPLATE_DUPLICATE },

  { "templates-edit", GIMP_STOCK_EDIT,
    N_("_Edit Template..."), "", NULL,
    G_CALLBACK (templates_edit_template_cmd_callback),
    GIMP_HELP_TEMPLATE_EDIT },

  { "templates-delete", GTK_STOCK_DELETE,
    N_("_Delete Template"), "", NULL,
    G_CALLBACK (templates_delete_template_cmd_callback),
    GIMP_HELP_TEMPLATE_DELETE }
};


void
templates_actions_setup (GimpActionGroup *group,
                         gpointer         data)
{
  gimp_action_group_add_actions (group,
                                 templates_actions,
                                 G_N_ELEMENTS (templates_actions),
                                 data);
}

void
templates_actions_update (GimpActionGroup *group,
                          gpointer         data)
{
  GimpContainerEditor *editor;
  GimpTemplate        *template;

  editor = GIMP_CONTAINER_EDITOR (data);

  template = gimp_context_get_template (editor->view->context);

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("templates-create-image", template);
  SET_SENSITIVE ("templates-new",          TRUE);
  SET_SENSITIVE ("templates-duplicate",    template);
  SET_SENSITIVE ("templates-edit",         template);
  SET_SENSITIVE ("templates-delete",       template);

#undef SET_SENSITIVE
}
