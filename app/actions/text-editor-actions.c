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

#include "widgets/gimpactiongroup.h"
#include "widgets/gimptexteditor.h"
#include "widgets/gimphelp-ids.h"

#include "text-editor-actions.h"
#include "text-editor-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry text_editor_actions[] =
{
  { "text-editor-load", GIMP_ICON_DOCUMENT_OPEN,
    NC_("text-editor-action", "Open"), NULL, { NULL },
    NC_("text-editor-action", "Load text from file"),
    text_editor_load_cmd_callback,
    NULL },

  { "text-editor-clear", GIMP_ICON_EDIT_CLEAR,
    NC_("text-editor-action", "Clear"), NULL, { NULL },
    NC_("text-editor-action", "Clear all text"),
    text_editor_clear_cmd_callback,
    NULL }
};

static const GimpRadioActionEntry text_editor_direction_actions[] =
{
  { "text-editor-direction-ltr", GIMP_ICON_FORMAT_TEXT_DIRECTION_LTR,
    NC_("text-editor-action", "LTR"), NULL, { NULL },
    NC_("text-editor-action", "From left to right"),
    GIMP_TEXT_DIRECTION_LTR,
    NULL },

  { "text-editor-direction-rtl", GIMP_ICON_FORMAT_TEXT_DIRECTION_RTL,
    NC_("text-editor-action", "RTL"), NULL, { NULL },
    NC_("text-editor-action", "From right to left"),
    GIMP_TEXT_DIRECTION_RTL,
    NULL },

  { "text-editor-direction-ttb-rtl", GIMP_ICON_FORMAT_TEXT_DIRECTION_TTB_RTL,
    NC_("text-editor-action", "TTB-RTL"), NULL, { NULL },
    NC_("text-editor-action", "Vertical, right to left (mixed orientation)"),
    GIMP_TEXT_DIRECTION_TTB_RTL,
    NULL },

  { "text-editor-direction-ttb-rtl-upright", GIMP_ICON_FORMAT_TEXT_DIRECTION_TTB_RTL_UPRIGHT,
    NC_("text-editor-action", "TTB-RTL-UPRIGHT"), NULL, { NULL },
    NC_("text-editor-action", "Vertical, right to left (upright orientation)"),
    GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT,
    NULL },

  { "text-editor-direction-ttb-ltr", GIMP_ICON_FORMAT_TEXT_DIRECTION_TTB_LTR,
    NC_("text-editor-action", "TTB-LTR"), NULL, { NULL },
    NC_("text-editor-action", "Vertical, left to right (mixed orientation)"),
    GIMP_TEXT_DIRECTION_TTB_LTR,
    NULL },

  { "text-editor-direction-ttb-ltr-upright", GIMP_ICON_FORMAT_TEXT_DIRECTION_TTB_LTR_UPRIGHT,
    NC_("text-editor-action", "TTB-LTR-UPRIGHT"), NULL, { NULL },
    NC_("text-editor-action", "Vertical, left to right (upright orientation)"),
    GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT,
    NULL },
};


void
text_editor_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "text-editor-action",
                                 text_editor_actions,
                                 G_N_ELEMENTS (text_editor_actions));

  gimp_action_group_add_radio_actions (group, "text-editor-action",
                                       text_editor_direction_actions,
                                       G_N_ELEMENTS (text_editor_direction_actions),
                                       NULL,
                                       GIMP_TEXT_DIRECTION_LTR,
                                       text_editor_direction_cmd_callback);
}

void
text_editor_actions_update (GimpActionGroup *group,
                            gpointer         data)
{
  GimpTextEditor *editor = GIMP_TEXT_EDITOR (data);

#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

  switch (editor->base_dir)
    {
    case GIMP_TEXT_DIRECTION_LTR:
      SET_ACTIVE ("text-editor-direction-ltr", TRUE);
      break;

    case GIMP_TEXT_DIRECTION_RTL:
      SET_ACTIVE ("text-editor-direction-rtl", TRUE);
      break;

    case GIMP_TEXT_DIRECTION_TTB_RTL:
      SET_ACTIVE ("text-editor-direction-ttb-rtl", TRUE);
      break;

    case GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
      SET_ACTIVE ("text-editor-direction-ttb-rtl-upright", TRUE);
      break;

    case GIMP_TEXT_DIRECTION_TTB_LTR:
      SET_ACTIVE ("text-editor-direction-ttb-ltr", TRUE);
      break;

    case GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
      SET_ACTIVE ("text-editor-direction-ttb-ltr-upright", TRUE);
      break;
    }

#undef SET_ACTIVE
}
