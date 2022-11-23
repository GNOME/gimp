/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligmaimage.h"

#include "text/ligmatextlayer.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmatexteditor.h"
#include "widgets/ligmahelp-ids.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"

#include "tools/ligmatool.h"
#include "tools/ligmatexttool.h"

#include "text-tool-actions.h"
#include "text-tool-commands.h"

#include "ligma-intl.h"


static const LigmaActionEntry text_tool_actions[] =
{
  { "text-tool-popup", NULL,
    NC_("text-tool-action", "Text Tool Menu"), NULL, NULL, NULL,
    NULL },

  { "text-tool-cut", LIGMA_ICON_EDIT_CUT,
    NC_("text-tool-action", "Cu_t"), NULL, "<primary>X",
    text_tool_cut_cmd_callback,
    NULL },

  { "text-tool-copy", LIGMA_ICON_EDIT_COPY,
    NC_("text-tool-action", "_Copy"), NULL, "<primary>C",
    text_tool_copy_cmd_callback,
    NULL },

  { "text-tool-paste", LIGMA_ICON_EDIT_PASTE,
    NC_("text-tool-action", "_Paste"), NULL, "<primary>V",
    text_tool_paste_cmd_callback,
    NULL },

  { "text-tool-delete", LIGMA_ICON_EDIT_DELETE,
    NC_("text-tool-action", "_Delete"), NULL, NULL,
    text_tool_delete_cmd_callback,
    NULL },

  { "text-tool-load", LIGMA_ICON_DOCUMENT_OPEN,
    NC_("text-tool-action", "_Open text file..."), NULL, NULL,
    text_tool_load_cmd_callback,
    NULL },

  { "text-tool-clear", LIGMA_ICON_EDIT_CLEAR,
    NC_("text-tool-action", "Cl_ear"), NULL,
    NC_("text-tool-action", "Clear all text"),
    text_tool_clear_cmd_callback,
    NULL },

  { "text-tool-text-to-path", LIGMA_ICON_PATH,
    NC_("text-tool-action", "_Path from Text"), "",
    NC_("text-tool-action",
        "Create a path from the outlines of the current text"),
    text_tool_text_to_path_cmd_callback,
    NULL },

  { "text-tool-text-along-path", LIGMA_ICON_PATH,
    NC_("text-tool-action", "Text _along Path"), "",
    NC_("text-tool-action",
        "Bend the text along the currently active path"),
    text_tool_text_along_path_cmd_callback,
    NULL }
};

static const LigmaRadioActionEntry text_tool_direction_actions[] =
{
  { "text-tool-direction-ltr", LIGMA_ICON_FORMAT_TEXT_DIRECTION_LTR,
    NC_("text-tool-action", "From left to right"), NULL, NULL,
    LIGMA_TEXT_DIRECTION_LTR,
    NULL },

  { "text-tool-direction-rtl", LIGMA_ICON_FORMAT_TEXT_DIRECTION_RTL,
    NC_("text-tool-action", "From right to left"), NULL, NULL,
    LIGMA_TEXT_DIRECTION_RTL,
    NULL },

  { "text-tool-direction-ttb-rtl", LIGMA_ICON_FORMAT_TEXT_DIRECTION_TTB_RTL,
    NC_("text-tool-action", "Vertical, right to left (mixed orientation)"), NULL, NULL,
    LIGMA_TEXT_DIRECTION_TTB_RTL,
    NULL },

  { "text-tool-direction-ttb-rtl-upright", LIGMA_ICON_FORMAT_TEXT_DIRECTION_TTB_RTL_UPRIGHT,
    NC_("text-tool-action", "Vertical, right to left (upright orientation)"), NULL, NULL,
    LIGMA_TEXT_DIRECTION_TTB_RTL_UPRIGHT,
    NULL },

  { "text-tool-direction-ttb-ltr", LIGMA_ICON_FORMAT_TEXT_DIRECTION_TTB_LTR,
    NC_("text-tool-action", "Vertical, left to right (mixed orientation)"), NULL, NULL,
    LIGMA_TEXT_DIRECTION_TTB_LTR,
    NULL },

  { "text-tool-direction-ttb-ltr-upright", LIGMA_ICON_FORMAT_TEXT_DIRECTION_TTB_LTR_UPRIGHT,
    NC_("text-tool-action", "Vertical, left to right (upright orientation)"), NULL, NULL,
    LIGMA_TEXT_DIRECTION_TTB_LTR_UPRIGHT,
    NULL }
};


#define SET_HIDE_EMPTY(action,condition) \
        ligma_action_group_set_action_hide_empty (group, action, (condition) != 0)

void
text_tool_actions_setup (LigmaActionGroup *group)
{
  ligma_action_group_add_actions (group, "text-tool-action",
                                 text_tool_actions,
                                 G_N_ELEMENTS (text_tool_actions));

  ligma_action_group_add_radio_actions (group, "text-tool-action",
                                       text_tool_direction_actions,
                                       G_N_ELEMENTS (text_tool_direction_actions),
                                       NULL,
                                       LIGMA_TEXT_DIRECTION_LTR,
                                       text_tool_direction_cmd_callback);
}

/* The following code is written on the assumption that this is for a
 * context menu, activated by right-clicking in a text layer.
 * Therefore, the tool must have a display.  If for any reason the
 * code is adapted to a different situation, some existence testing
 * will need to be added.
 */
void
text_tool_actions_update (LigmaActionGroup *group,
                          gpointer         data)
{
  LigmaTextTool     *text_tool  = LIGMA_TEXT_TOOL (data);
  LigmaDisplay      *display    = LIGMA_TOOL (text_tool)->display;
  LigmaImage        *image      = ligma_display_get_image (display);
  GList            *layers;
  GList            *paths;
  LigmaDisplayShell *shell;
  GtkClipboard     *clipboard;
  gboolean          text_layer = FALSE;
  gboolean          text_sel   = FALSE;   /* some text is selected        */
  gboolean          clip       = FALSE;   /* clipboard has text available */
  LigmaTextDirection direction;
  gint              i;

  layers = ligma_image_get_selected_layers (image);

  if (g_list_length (layers) == 1)
    text_layer = ligma_item_is_text_layer (LIGMA_ITEM (layers->data));

  paths = ligma_image_get_selected_vectors (image);

  text_sel = ligma_text_tool_get_has_text_selection (text_tool);

  /* see whether there is text available for pasting */
  shell = ligma_display_get_shell (display);
  clipboard = gtk_widget_get_clipboard (shell->canvas,
                                        GDK_SELECTION_CLIPBOARD);
  clip = gtk_clipboard_wait_is_text_available (clipboard);

#define SET_VISIBLE(action,condition) \
        ligma_action_group_set_action_visible (group, action, (condition) != 0)
#define SET_SENSITIVE(action,condition) \
        ligma_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)
#define SET_ACTIVE(action,condition) \
        ligma_action_group_set_action_active (group, action, (condition) != 0)

  SET_SENSITIVE ("text-tool-cut",             text_sel);
  SET_SENSITIVE ("text-tool-copy",            text_sel);
  SET_SENSITIVE ("text-tool-paste",           clip);
  SET_SENSITIVE ("text-tool-delete",          text_sel);
  SET_SENSITIVE ("text-tool-clear",           text_layer);
  SET_SENSITIVE ("text-tool-load",            image);
  SET_SENSITIVE ("text-tool-text-to-path",    text_layer);
  SET_SENSITIVE ("text-tool-text-along-path", text_layer && g_list_length (paths) == 1);

  direction = ligma_text_tool_get_direction (text_tool);
  for (i = 0; i < G_N_ELEMENTS (text_tool_direction_actions); i++)
    {
      if (direction == text_tool_direction_actions[i].value)
        {
          SET_ACTIVE (text_tool_direction_actions[i].name, TRUE);
          break;
        }
    }
}
