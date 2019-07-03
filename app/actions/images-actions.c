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
#include "core/gimpimage.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "images-actions.h"
#include "images-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry images_actions[] =
{
  { "images-popup", GIMP_ICON_DIALOG_IMAGES,
    NC_("images-action", "Images Menu"), NULL, NULL, NULL,
    GIMP_HELP_IMAGE_DIALOG },

  { "images-raise-views", GIMP_ICON_GO_TOP,
    NC_("images-action", "_Raise Views"), NULL,
    NC_("images-action", "Raise this image's displays"),
    images_raise_views_cmd_callback,
    NULL },

  { "images-new-view", GIMP_ICON_DOCUMENT_NEW,
    NC_("images-action", "_New View"), NULL,
    NC_("images-action", "Create a new display for this image"),
    images_new_view_cmd_callback,
    NULL },

  { "images-delete", GIMP_ICON_EDIT_DELETE,
    NC_("images-action", "_Delete Image"), NULL,
    NC_("images-action", "Delete this image"),
    images_delete_image_cmd_callback,
    NULL }
};


void
images_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "images-action",
                                 images_actions,
                                 G_N_ELEMENTS (images_actions));
}

void
images_actions_update (GimpActionGroup *group,
                       gpointer         data)
{
  GimpContext *context    = action_data_get_context (data);
  GimpImage   *image      = NULL;
  gint         disp_count = 0;

  if (context)
    {
      image = gimp_context_get_image (context);

      if (image)
        disp_count = gimp_image_get_display_count (image);
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("images-raise-views", image);
  SET_SENSITIVE ("images-new-view",    image);
  SET_SENSITIVE ("images-delete",      image && disp_count == 0);

#undef SET_SENSITIVE
}
