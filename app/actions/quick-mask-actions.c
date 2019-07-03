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

#include "core/gimpimage.h"
#include "core/gimpimage-quick-mask.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "quick-mask-actions.h"
#include "quick-mask-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry quick_mask_actions[] =
{
  { "quick-mask-popup", NULL,
    NC_("quick-mask-action", "Quick Mask Menu"), NULL, NULL, NULL,
    GIMP_HELP_QUICK_MASK },

  { "quick-mask-configure", NULL,
    NC_("quick-mask-action", "_Configure Color and Opacity..."), NULL, NULL,
    quick_mask_configure_cmd_callback,
    GIMP_HELP_QUICK_MASK_EDIT }
};

static const GimpToggleActionEntry quick_mask_toggle_actions[] =
{
  { "quick-mask-toggle", GIMP_ICON_QUICK_MASK_ON,
    NC_("quick-mask-action", "Toggle _Quick Mask"), "<shift>Q",
    NC_("quick-mask-action", "Toggle Quick Mask on/off"),
    quick_mask_toggle_cmd_callback,
    FALSE,
    GIMP_HELP_QUICK_MASK_TOGGLE }
};

static const GimpRadioActionEntry quick_mask_invert_actions[] =
{
  { "quick-mask-invert-on", NULL,
    NC_("quick-mask-action", "Mask _Selected Areas"), NULL, NULL,
    TRUE,
    GIMP_HELP_QUICK_MASK_INVERT },

  { "quick-mask-invert-off", NULL,
    NC_("quick-mask-action", "Mask _Unselected Areas"), NULL, NULL,
    FALSE,
    GIMP_HELP_QUICK_MASK_INVERT }
};


void
quick_mask_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "quick-mask-action",
                                 quick_mask_actions,
                                 G_N_ELEMENTS (quick_mask_actions));

  gimp_action_group_add_toggle_actions (group, "quick-mask-action",
                                        quick_mask_toggle_actions,
                                        G_N_ELEMENTS (quick_mask_toggle_actions));

  gimp_action_group_add_radio_actions (group, "quick-mask-action",
                                       quick_mask_invert_actions,
                                       G_N_ELEMENTS (quick_mask_invert_actions),
                                       NULL,
                                       FALSE,
                                       quick_mask_invert_cmd_callback);
}

void
quick_mask_actions_update (GimpActionGroup *group,
                           gpointer         data)
{
  GimpImage *image               = action_data_get_image (data);
  gboolean   quick_mask_state    = FALSE;
  gboolean   quick_mask_inverted = FALSE;
  GimpRGB    quick_mask_color;

  if (image)
    {
      quick_mask_state    = gimp_image_get_quick_mask_state (image);
      quick_mask_inverted = gimp_image_get_quick_mask_inverted (image);

      gimp_image_get_quick_mask_color (image, &quick_mask_color);
    }

#define SET_SENSITIVE(action,sensitive) \
        gimp_action_group_set_action_sensitive (group, action, (sensitive) != 0)
#define SET_ACTIVE(action,active) \
        gimp_action_group_set_action_active (group, action, (active) != 0)
#define SET_COLOR(action,color) \
        gimp_action_group_set_action_color (group, action, (color), FALSE)

  SET_SENSITIVE ("quick-mask-toggle", image);
  SET_ACTIVE    ("quick-mask-toggle", quick_mask_state);

  SET_SENSITIVE ("quick-mask-invert-on",  image);
  SET_SENSITIVE ("quick-mask-invert-off", image);

  if (quick_mask_inverted)
    SET_ACTIVE ("quick-mask-invert-on", TRUE);
  else
    SET_ACTIVE ("quick-mask-invert-off", TRUE);

  SET_SENSITIVE ("quick-mask-configure", image);

  if (image)
    SET_COLOR ("quick-mask-configure", &quick_mask_color);

#undef SET_SENSITIVE
#undef SET_ACTIVE
#undef SET_COLOR
}
