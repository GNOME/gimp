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
#include "core/gimpdata.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "data-commands.h"
#include "gradients-actions.h"
#include "gradients-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry gradients_actions[] =
{
  { "gradients-popup", GIMP_STOCK_GRADIENT,
    N_("Gradients Menu"), NULL, NULL, NULL,
    GIMP_HELP_GRADIENT_DIALOG },

  { "gradients-new", GTK_STOCK_NEW,
    N_("_New Gradient"), "",
    N_("New gradient"),
    G_CALLBACK (data_new_cmd_callback),
    GIMP_HELP_GRADIENT_NEW },

  { "gradients-duplicate", GIMP_STOCK_DUPLICATE,
    N_("D_uplicate Gradient"), NULL,
    N_("Duplicate gradient"),
    G_CALLBACK (data_duplicate_cmd_callback),
    GIMP_HELP_GRADIENT_DUPLICATE },

  { "gradients-copy-location", GTK_STOCK_COPY,
    N_("Copy Gradient _Location"), "",
    N_("Copy gradient file location to clipboard"),
    G_CALLBACK (data_copy_location_cmd_callback),
    GIMP_HELP_GRADIENT_COPY_LOCATION },

  { "gradients-save-as-pov", GTK_STOCK_SAVE_AS,
    N_("Save as _POV-Ray..."), "",
    N_("Save gradient as POV-Ray"),
    G_CALLBACK (gradients_save_as_pov_ray_cmd_callback),
    GIMP_HELP_GRADIENT_SAVE_AS_POV },

  { "gradients-delete", GTK_STOCK_DELETE,
    N_("_Delete Gradient"), "",
    N_("Delete gradient"),
    G_CALLBACK (data_delete_cmd_callback),
    GIMP_HELP_GRADIENT_DELETE },

  { "gradients-refresh", GTK_STOCK_REFRESH,
    N_("_Refresh Gradients"), "",
    N_("Refresh gradients"),
    G_CALLBACK (data_refresh_cmd_callback),
    GIMP_HELP_GRADIENT_REFRESH }
};

static const GimpStringActionEntry gradients_edit_actions[] =
{
  { "gradients-edit", GTK_STOCK_EDIT,
    N_("_Edit Gradient..."), NULL,
    N_("Edit gradient"),
    "gimp-gradient-editor",
    GIMP_HELP_GRADIENT_EDIT }
};


void
gradients_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 gradients_actions,
                                 G_N_ELEMENTS (gradients_actions));

  gimp_action_group_add_string_actions (group,
                                        gradients_edit_actions,
                                        G_N_ELEMENTS (gradients_edit_actions),
                                        G_CALLBACK (data_edit_cmd_callback));
}

void
gradients_actions_update (GimpActionGroup *group,
                          gpointer         user_data)
{
  GimpContext  *context  = action_data_get_context (user_data);
  GimpGradient *gradient = NULL;
  GimpData     *data     = NULL;

  if (context)
    {
      gradient = gimp_context_get_gradient (context);

      if (gradient)
        data = GIMP_DATA (gradient);
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("gradients-edit",          gradient);
  SET_SENSITIVE ("gradients-duplicate",     gradient);
  SET_SENSITIVE ("gradients-save-as-pov",   gradient);
  SET_SENSITIVE ("gradients-copy-location", gradient && data->filename);
  SET_SENSITIVE ("gradients-delete",        gradient && data->deletable);

#undef SET_SENSITIVE
}
