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

#include "text-tool-actions.h"
#include "text-tool-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry text_tool_actions[] =
{
  { "text-tool-popup", NULL,
    N_("Text Tool Popup"), NULL, NULL, NULL,
    GIMP_HELP_TEXT_EDITOR_DIALOG },

  { "text-tool-cut", NULL,
    N_("Cut"), NULL, NULL,
    NULL, NULL },

  { "text-tool-copy", NULL,
    N_("Copy"), NULL, NULL,
    NULL, NULL },

  { "text-tool-paste", NULL,
    N_("Paste"), NULL, NULL,
    NULL, NULL },

  { "text-tool-delete", NULL,
    N_("Delete"), NULL, NULL,
    NULL, NULL },

  { "text-tool-load", NULL,
    N_("Open"), NULL,
    N_("Load text from file"),
    G_CALLBACK (text_tool_load_cmd_callback),
    NULL },

  { "text-tool-clear", NULL,
    N_("Clear"), "",
    N_("Clear all text"),
    G_CALLBACK (text_tool_clear_cmd_callback),
    NULL },

  { "text-tool-input-methods", NULL,
    N_("Input Methods"), NULL, NULL, NULL,
    GIMP_HELP_TEXT_EDITOR_DIALOG }
};

static const GimpRadioActionEntry text_tool_direction_actions[] =
{
  { "text-tool-direction-ltr", GIMP_STOCK_TEXT_DIR_LTR,
    N_("LTR"), "",
    N_("From left to right"),
    GIMP_TEXT_DIRECTION_LTR,
    NULL },

  { "text-tool-direction-rtl", GIMP_STOCK_TEXT_DIR_RTL,
    N_("RTL"), "",
    N_("From right to left"),
    GIMP_TEXT_DIRECTION_RTL,
    NULL }
};


void
text_tool_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 text_tool_actions,
                                 G_N_ELEMENTS (text_tool_actions));

  gimp_action_group_add_radio_actions (group,
                                       text_tool_direction_actions,
                                       G_N_ELEMENTS (text_tool_direction_actions),
                                       NULL,
                                       GIMP_TEXT_DIRECTION_LTR,
                                       G_CALLBACK (text_tool_direction_cmd_callback));
}

void
text_tool_actions_update (GimpActionGroup *group,
                          gpointer         data)
{
  /* Things will be added here soon*/
}
