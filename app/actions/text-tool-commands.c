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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimptextbuffer.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"

#include "text/gimptextlayer.h"

#include "tools/gimptexttool.h"
#include "tools/gimptexttool-editor.h"

#include "dialogs/dialogs.h"

#include "text-tool-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   text_tool_load_dialog_response (GtkNativeDialog *dialog,
                                              gint             response_id,
                                              GimpTextTool    *tool);


/*  public functions  */

void
text_tool_cut_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (data);

  gimp_text_tool_cut_clipboard (text_tool);
}

void
text_tool_copy_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (data);

  gimp_text_tool_copy_clipboard (text_tool);
}

void
text_tool_paste_cmd_callback (GimpAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (data);

  gimp_text_tool_paste_clipboard (text_tool);
}

void
text_tool_paste_unformatted_cmd_callback (GimpAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (data);

  gimp_text_tool_paste_clipboard_unformatted (text_tool);
}

void
text_tool_toggle_bold_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (data);

  gimp_text_tool_toggle_tag (text_tool, text_tool->buffer->bold_tag);
}

void
text_tool_toggle_italic_cmd_callback (GimpAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (data);

  gimp_text_tool_toggle_tag (text_tool, text_tool->buffer->italic_tag);
}

void
text_tool_toggle_underline_cmd_callback (GimpAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (data);

  gimp_text_tool_toggle_tag (text_tool, text_tool->buffer->underline_tag);
}

void
text_tool_delete_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (data);

  gimp_text_tool_delete_selection (text_tool);
}

void
text_tool_load_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpTextTool         *text_tool = GIMP_TEXT_TOOL (data);
  GtkFileChooserNative *dialog;

  dialog =
    GTK_FILE_CHOOSER_NATIVE (dialogs_get_native_dialog (G_OBJECT (text_tool),
                             "gimp-text-file-dialog"));

  if (! dialog)
    {
      GtkWidget *parent = NULL;

      if (GIMP_TOOL (text_tool)->display)
        {
          GimpDisplayShell *shell;

          shell = gimp_display_get_shell (GIMP_TOOL (text_tool)->display);

          parent = gtk_widget_get_toplevel (GTK_WIDGET (shell));
        }

      dialog = gtk_file_chooser_native_new (_("Open Text File (UTF-8)"),
                                            parent ? GTK_WINDOW (parent) : NULL,
                                            GTK_FILE_CHOOSER_ACTION_OPEN,
                                            _("_Open"), _("_Cancel"));

      g_signal_connect (dialog, "response",
                        G_CALLBACK (text_tool_load_dialog_response),
                        text_tool);

      dialogs_attach_native_dialog (G_OBJECT (text_tool),
                                    "gimp-text-file-dialog",
                                    GTK_NATIVE_DIALOG (dialog));
    }

  gtk_native_dialog_show (GTK_NATIVE_DIALOG (dialog));
}

void
text_tool_clear_cmd_callback (GimpAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  GimpTextTool  *text_tool = GIMP_TEXT_TOOL (data);
  GtkTextBuffer *buffer    = GTK_TEXT_BUFFER (text_tool->buffer);
  GtkTextIter    start, end;

  gtk_text_buffer_get_bounds (buffer, &start, &end);
  gtk_text_buffer_select_range (buffer, &start, &end);
  gimp_text_tool_delete_selection (text_tool);
}

void
text_tool_text_to_path_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (data);

  gimp_text_tool_create_path (text_tool);
}

void
text_tool_text_along_path_cmd_callback (GimpAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (data);
  GError       *error     = NULL;

  if (! gimp_text_tool_create_path_warped (text_tool, &error))
    {
      gimp_message (text_tool->image->gimp, G_OBJECT (text_tool),
                    GIMP_MESSAGE_ERROR,
                    _("Text along path failed: %s"),
                    error->message);
      g_clear_error (&error);
    }
}

void
text_tool_direction_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpTextTool      *text_tool = GIMP_TEXT_TOOL (data);
  GimpTextDirection  direction;

  direction = (GimpTextDirection) g_variant_get_int32 (value);

  g_object_set (text_tool->proxy,
                "base-direction", direction,
                NULL);
}

void
text_tool_restore_on_canvas_editor_position_cmd_callback (GimpAction *action,
                                                          GVariant   *value,
                                                          gpointer    data)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (data);

  gimp_text_tool_restore_on_canvas_editor_position (text_tool);
}



/*  private functions  */

static void
text_tool_load_dialog_response (GtkNativeDialog *dialog,
                                gint             response_id,
                                GimpTextTool    *tool)
{
  if (response_id == GTK_RESPONSE_ACCEPT)
    {
      GFile  *file  = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
      GError *error = NULL;

      if (! gimp_text_buffer_load (tool->buffer, file, &error))
        {
          gimp_message (GIMP_TOOL (tool)->tool_info->gimp, G_OBJECT (dialog),
                        GIMP_MESSAGE_ERROR,
                        _("Could not open '%s' for reading: %s"),
                        gimp_file_get_utf8_name (file),
                        error->message);
          g_clear_error (&error);
          g_object_unref (file);
          return;
        }

      g_object_unref (file);
    }

  dialogs_detach_native_dialog (G_OBJECT (tool), dialog);
  gtk_native_dialog_hide (dialog);
}
