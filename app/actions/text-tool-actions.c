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

#include "text/gimptextlayer.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimptexteditor.h"
#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "tools/gimptool.h"
#include "tools/gimptexttool.h"

#include "text-tool-actions.h"
#include "text-tool-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry text_tool_actions[] =
{
  { "text-tool-cut", GIMP_ICON_EDIT_CUT,
    NC_("text-tool-action", "Cu_t"), NULL, { "<primary>X", NULL }, NULL,
    text_tool_cut_cmd_callback,
    GIMP_HELP_TEXT_TOOL_CUT },

  { "text-tool-copy", GIMP_ICON_EDIT_COPY,
    NC_("text-tool-action", "_Copy"), NULL, { "<primary>C", NULL }, NULL,
    text_tool_copy_cmd_callback,
    GIMP_HELP_TEXT_TOOL_COPY },

  { "text-tool-paste", GIMP_ICON_EDIT_PASTE,
    NC_("text-tool-action", "_Paste"), NULL, { "<primary>V", NULL }, NULL,
    text_tool_paste_cmd_callback,
    GIMP_HELP_TEXT_TOOL_PASTE },

  { "text-tool-toggle-bold", GIMP_ICON_FORMAT_TEXT_BOLD,
    NC_("text-tool-action", "_Bold"), NULL, { "<primary>B", NULL }, NULL,
    text_tool_toggle_bold_cmd_callback,
    NULL },

  { "text-tool-toggle-italic", GIMP_ICON_FORMAT_TEXT_ITALIC,
    NC_("text-tool-action", "_Italic"), NULL, { "<primary>I", NULL }, NULL,
    text_tool_toggle_italic_cmd_callback,
    NULL },

   { "text-tool-toggle-underline", GIMP_ICON_FORMAT_TEXT_UNDERLINE,
    NC_("text-tool-action", "_Underline"), NULL, { "<primary>U", NULL }, NULL,
    text_tool_toggle_underline_cmd_callback,
    NULL },

  { "text-tool-delete", GIMP_ICON_EDIT_DELETE,
    NC_("text-tool-action", "_Delete"), NULL, { NULL }, NULL,
    text_tool_delete_cmd_callback,
    GIMP_HELP_TEXT_TOOL_DELETE },

  { "text-tool-load", GIMP_ICON_DOCUMENT_OPEN,
    NC_("text-tool-action", "_Open text file..."), NULL, { NULL }, NULL,
    text_tool_load_cmd_callback,
    GIMP_HELP_TEXT_TOOL_OPEN_TEXT_FILE },

  { "text-tool-clear", GIMP_ICON_EDIT_CLEAR,
    NC_("text-tool-action", "Cl_ear"), NULL, { NULL },
    NC_("text-tool-action", "Clear all text"),
    text_tool_clear_cmd_callback,
    GIMP_HELP_TEXT_TOOL_CLEAR },

  { "text-tool-text-to-path", GIMP_ICON_PATH,
    NC_("text-tool-action", "Text to _Path"), NULL, { NULL },
    NC_("text-tool-action",
        "Create a path from the outlines of the current text"),
    text_tool_text_to_path_cmd_callback,
    GIMP_HELP_TEXT_TOOL_TEXT_TO_PATH },

  { "text-tool-text-along-path", GIMP_ICON_PATH,
    NC_("text-tool-action", "Text _along Path"), NULL, { NULL },
    NC_("text-tool-action",
        "Bend the text along the currently active path"),
    text_tool_text_along_path_cmd_callback,
    GIMP_HELP_TEXT_TOOL_TEXT_ALONG_PATH }
};

static const GimpRadioActionEntry text_tool_direction_actions[] =
{
  { "text-tool-direction-ltr", GIMP_ICON_FORMAT_TEXT_DIRECTION_LTR,
    NC_("text-tool-action", "From left to right"), NULL, { NULL }, NULL,
    GIMP_TEXT_DIRECTION_LTR,
    GIMP_HELP_TEXT_TOOL_DIRECTION_LTR },

  { "text-tool-direction-rtl", GIMP_ICON_FORMAT_TEXT_DIRECTION_RTL,
    NC_("text-tool-action", "From right to left"), NULL, { NULL }, NULL,
    GIMP_TEXT_DIRECTION_RTL,
    GIMP_HELP_TEXT_TOOL_DIRECTION_RTL },

  { "text-tool-direction-ttb-rtl", GIMP_ICON_FORMAT_TEXT_DIRECTION_TTB_RTL,
    NC_("text-tool-action", "Vertical, right to left (mixed orientation)"), NULL, { NULL }, NULL,
    GIMP_TEXT_DIRECTION_TTB_RTL,
    GIMP_HELP_TEXT_TOOL_DIRECTION_TTB_RTL },

  { "text-tool-direction-ttb-rtl-upright", GIMP_ICON_FORMAT_TEXT_DIRECTION_TTB_RTL_UPRIGHT,
    NC_("text-tool-action", "Vertical, right to left (upright orientation)"), NULL, { NULL }, NULL,
    GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT,
    GIMP_HELP_TEXT_TOOL_DIRECTION_TTB_RTL_UP },

  { "text-tool-direction-ttb-ltr", GIMP_ICON_FORMAT_TEXT_DIRECTION_TTB_LTR,
    NC_("text-tool-action", "Vertical, left to right (mixed orientation)"), NULL, { NULL }, NULL,
    GIMP_TEXT_DIRECTION_TTB_LTR,
    GIMP_HELP_TEXT_TOOL_DIRECTION_TTB_LTR },

  { "text-tool-direction-ttb-ltr-upright", GIMP_ICON_FORMAT_TEXT_DIRECTION_TTB_LTR_UPRIGHT,
    NC_("text-tool-action", "Vertical, left to right (upright orientation)"), NULL, { NULL }, NULL,
    GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT,
    GIMP_HELP_TEXT_TOOL_DIRECTION_TTB_LTR_UP }
};


#define SET_HIDE_EMPTY(action,condition) \
        gimp_action_group_set_action_hide_empty (group, action, (condition) != 0)

void
text_tool_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "text-tool-action",
                                 text_tool_actions,
                                 G_N_ELEMENTS (text_tool_actions));

  gimp_action_group_add_radio_actions (group, "text-tool-action",
                                       text_tool_direction_actions,
                                       G_N_ELEMENTS (text_tool_direction_actions),
                                       NULL,
                                       GIMP_TEXT_DIRECTION_LTR,
                                       text_tool_direction_cmd_callback);
}

/* The following code is written on the assumption that this is for a
 * context menu, activated by right-clicking in a text layer.
 * Therefore, the tool must have a display.  If for any reason the
 * code is adapted to a different situation, some existence testing
 * will need to be added.
 */
void
text_tool_actions_update (GimpActionGroup *group,
                          gpointer         data)
{
  GimpTextTool     *text_tool  = GIMP_TEXT_TOOL (data);
  GimpDisplay      *display    = GIMP_TOOL (text_tool)->display;
  GimpImage        *image      = gimp_display_get_image (display);
  GList            *layers;
  GList            *paths;
  GimpDisplayShell *shell;
  GtkClipboard     *clipboard;
  gboolean          text_layer = FALSE;
  gboolean          text_sel   = FALSE;   /* some text is selected        */
  gboolean          clip       = FALSE;   /* clipboard has text available */
  GimpTextDirection direction;
  gint              i;

  layers = gimp_image_get_selected_layers (image);

  if (g_list_length (layers) == 1)
    text_layer = gimp_item_is_text_layer (GIMP_ITEM (layers->data));

  paths = gimp_image_get_selected_paths (image);

  text_sel = gimp_text_tool_get_has_text_selection (text_tool);

  /* see whether there is text available for pasting */
  shell = gimp_display_get_shell (display);
  clipboard = gtk_widget_get_clipboard (shell->canvas,
                                        GDK_SELECTION_CLIPBOARD);
  clip = gtk_clipboard_wait_is_text_available (clipboard);

#define SET_VISIBLE(action,condition) \
        gimp_action_group_set_action_visible (group, action, (condition) != 0)
#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)
#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

  SET_SENSITIVE ("text-tool-cut",              text_sel);
  SET_SENSITIVE ("text-tool-copy",             text_sel);
  SET_SENSITIVE ("text-tool-paste",            clip);
  SET_SENSITIVE ("text-tool-toggle-bold",      text_sel);
  SET_SENSITIVE ("text-tool-toggle-italic",    text_sel);
  SET_SENSITIVE ("text-tool-toggle-underline", text_sel);
  SET_SENSITIVE ("text-tool-delete",           text_sel);
  SET_SENSITIVE ("text-tool-clear",            text_layer);
  SET_SENSITIVE ("text-tool-load",             image);
  SET_SENSITIVE ("text-tool-text-to-path",     text_layer);
  SET_SENSITIVE ("text-tool-text-along-path",  text_layer && g_list_length (paths) == 1);

  direction = gimp_text_tool_get_direction (text_tool);
  for (i = 0; i < G_N_ELEMENTS (text_tool_direction_actions); i++)
    {
      if (direction == text_tool_direction_actions[i].value)
        {
          SET_ACTIVE (text_tool_direction_actions[i].name, TRUE);
          break;
        }
    }
}
