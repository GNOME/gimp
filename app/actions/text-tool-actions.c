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

#include "core/gimpdrawable.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimptexteditor.h"
#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "tools/gimptool.h"
#include "tools/gimptexttool.h"

#include "text-tool-actions.h"
#include "text-tool-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry text_tool_actions[] =
{
  { "text-tool-popup", NULL,
    N_("Text Tool Popup"), NULL, NULL, NULL,
    NULL },

  { "text-tool-cut", GTK_STOCK_CUT,
    N_("Cut"), NULL, NULL,
    G_CALLBACK (text_tool_cut_cmd_callback),
    NULL },

  { "text-tool-copy", GTK_STOCK_COPY,
    N_("Copy"), NULL, NULL,
    G_CALLBACK (text_tool_copy_cmd_callback),
    NULL },

  { "text-tool-paste", GTK_STOCK_PASTE,
    N_("Paste"), NULL, NULL,
    G_CALLBACK (text_tool_paste_cmd_callback),
    NULL },

  { "text-tool-delete", GTK_STOCK_DELETE,
    N_("Delete selected"), NULL, NULL,
    G_CALLBACK (text_tool_delete_cmd_callback),
    NULL },

  { "text-tool-load", GTK_STOCK_OPEN,
    N_("Open"), NULL,
    N_("Load text from file"),
    G_CALLBACK (text_tool_load_cmd_callback),
    NULL },

  { "text-tool-clear", GTK_STOCK_CLEAR,
    N_("Clear"), "",
    N_("Clear all text"),
    G_CALLBACK (text_tool_clear_cmd_callback),
    NULL },

  { "text-tool-path-from-text", GIMP_STOCK_PATH,
    N_("Path from Text"), "",
    N_("Create a path from the outlines of the current text"),
    G_CALLBACK (text_tool_path_from_text_callback),
    NULL },

  { "text-tool-input-methods", NULL,
    N_("Input Methods"), NULL, NULL, NULL,
    NULL }
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

/*
 * The following code is written on the assumption that this is for a context
 * menu, activated by right-clicking in a text layer.  Therefore, the tool
 * must have a display.  If for any reason the code is adapted to a different
 * situation, some existence testing will need to be added.
 */
void
text_tool_actions_update (GimpActionGroup *group,
                          gpointer         data)
{
  GimpTextTool  *text_tool  = GIMP_TEXT_TOOL (data);
  GimpImage     *image      = GIMP_TOOL (text_tool)->display->image;
  GimpLayer     *layer      = NULL;
  gboolean       text_layer = FALSE;
  gboolean       text_sel   = FALSE;   /* some text is selected        */
  gboolean       clip       = FALSE;   /* clipboard has text available */

  layer = gimp_image_get_active_layer (image);

  if (layer)
    text_layer = gimp_drawable_is_text_layer (GIMP_DRAWABLE (layer));

  text_sel = gimp_text_tool_get_has_text_selection (text_tool);

  /*
   * see whether there is text available for pasting
   */
  {
    GtkClipboard  *clipboard;

    clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
    clip = gtk_clipboard_wait_is_text_available (clipboard);
  }

#define SET_VISIBLE(action,condition) \
        gimp_action_group_set_action_visible (group, action, (condition) != 0)
#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)
#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

  SET_SENSITIVE ("text-tool-cut",             text_sel);
  SET_SENSITIVE ("text-tool-copy",            text_sel);
  SET_SENSITIVE ("text-tool-paste",           clip);
  SET_SENSITIVE ("text-tool-delete",          text_sel);
  SET_SENSITIVE ("text-tool-clear",           text_layer);
  SET_SENSITIVE ("text-tool-load",            image);
  SET_SENSITIVE ("text-tool-path-from-text",  text_layer);
}
