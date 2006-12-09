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

#include "widgets/gimpactiongroup.h"
#include "widgets/gimptexteditor.h"
#include "widgets/gimphelp-ids.h"

#include "text-editor-actions.h"
#include "text-editor-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry text_editor_actions[] =
{
  { "text-editor-toolbar", GTK_STOCK_EDIT,
    "Text Editor Toolbar", NULL, NULL, NULL,
    GIMP_HELP_TEXT_EDITOR_DIALOG },

  { "text-editor-load", GTK_STOCK_OPEN,
    N_("Open"), "",
    N_("Load text from file"),
    G_CALLBACK (text_editor_load_cmd_callback),
    NULL },

  { "text-editor-clear", GTK_STOCK_CLEAR,
    N_("Clear"), "",
    N_("Clear all text"),
    G_CALLBACK (text_editor_clear_cmd_callback),
    NULL }
};

static const GimpRadioActionEntry text_editor_direction_actions[] =
{
  { "text-editor-direction-ltr", GIMP_STOCK_TEXT_DIR_LTR,
    N_("LTR"), "",
    N_("From left to right"),
    GIMP_TEXT_DIRECTION_LTR,
    NULL },

  { "text-editor-direction-rtl", GIMP_STOCK_TEXT_DIR_RTL,
    N_("RTL"), "",
    N_("From right to left"),
    GIMP_TEXT_DIRECTION_RTL,
    NULL }
};


void
text_editor_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 text_editor_actions,
                                 G_N_ELEMENTS (text_editor_actions));

  gimp_action_group_add_radio_actions (group,
                                       text_editor_direction_actions,
                                       G_N_ELEMENTS (text_editor_direction_actions),
                                       NULL,
                                       GIMP_TEXT_DIRECTION_LTR,
                                       G_CALLBACK (text_editor_direction_cmd_callback));
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
    }

#undef SET_ACTIVE
}
