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

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligma.h"
#include "core/ligmaimage.h"
#include "core/ligmatoolinfo.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmatextbuffer.h"
#include "widgets/ligmauimanager.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"

#include "tools/ligmatexttool.h"

#include "dialogs/dialogs.h"

#include "text-tool-commands.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   text_tool_load_dialog_response (GtkWidget    *dialog,
                                              gint          response_id,
                                              LigmaTextTool *tool);


/*  public functions  */

void
text_tool_cut_cmd_callback (LigmaAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  LigmaTextTool *text_tool = LIGMA_TEXT_TOOL (data);

  ligma_text_tool_cut_clipboard (text_tool);
}

void
text_tool_copy_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  LigmaTextTool *text_tool = LIGMA_TEXT_TOOL (data);

  ligma_text_tool_copy_clipboard (text_tool);
}

void
text_tool_paste_cmd_callback (LigmaAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  LigmaTextTool *text_tool = LIGMA_TEXT_TOOL (data);

  ligma_text_tool_paste_clipboard (text_tool);
}

void
text_tool_delete_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaTextTool *text_tool = LIGMA_TEXT_TOOL (data);

  ligma_text_tool_delete_selection (text_tool);
}

void
text_tool_load_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  LigmaTextTool *text_tool = LIGMA_TEXT_TOOL (data);
  GtkWidget    *dialog;

  dialog = dialogs_get_dialog (G_OBJECT (text_tool), "ligma-text-file-dialog");

  if (! dialog)
    {
      GtkWidget *parent = NULL;

      if (LIGMA_TOOL (text_tool)->display)
        {
          LigmaDisplayShell *shell;

          shell = ligma_display_get_shell (LIGMA_TOOL (text_tool)->display);

          parent = gtk_widget_get_toplevel (GTK_WIDGET (shell));
        }

      dialog = gtk_file_chooser_dialog_new (_("Open Text File (UTF-8)"),
                                            parent ? GTK_WINDOW (parent) : NULL,
                                            GTK_FILE_CHOOSER_ACTION_OPEN,

                                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                                            _("_Open"),   GTK_RESPONSE_OK,

                                            NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
      ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      gtk_window_set_role (GTK_WINDOW (dialog), "ligma-text-load-file");
      gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (text_tool_load_dialog_response),
                        text_tool);
      g_signal_connect (dialog, "delete-event",
                        G_CALLBACK (gtk_true),
                        NULL);

      dialogs_attach_dialog (G_OBJECT (text_tool),
                             "ligma-text-file-dialog", dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
text_tool_clear_cmd_callback (LigmaAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  LigmaTextTool  *text_tool = LIGMA_TEXT_TOOL (data);
  GtkTextBuffer *buffer    = GTK_TEXT_BUFFER (text_tool->buffer);
  GtkTextIter    start, end;

  gtk_text_buffer_get_bounds (buffer, &start, &end);
  gtk_text_buffer_select_range (buffer, &start, &end);
  ligma_text_tool_delete_selection (text_tool);
}

void
text_tool_text_to_path_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaTextTool *text_tool = LIGMA_TEXT_TOOL (data);

  ligma_text_tool_create_vectors (text_tool);
}

void
text_tool_text_along_path_cmd_callback (LigmaAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  LigmaTextTool *text_tool = LIGMA_TEXT_TOOL (data);
  GError       *error     = NULL;

  if (! ligma_text_tool_create_vectors_warped (text_tool, &error))
    {
      ligma_message (text_tool->image->ligma, G_OBJECT (text_tool),
                    LIGMA_MESSAGE_ERROR,
                    _("Text along path failed: %s"),
                    error->message);
      g_clear_error (&error);
    }
}

void
text_tool_direction_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaTextTool      *text_tool = LIGMA_TEXT_TOOL (data);
  LigmaTextDirection  direction;

  direction = (LigmaTextDirection) g_variant_get_int32 (value);

  g_object_set (text_tool->proxy,
                "base-direction", direction,
                NULL);
}


/*  private functions  */

static void
text_tool_load_dialog_response (GtkWidget    *dialog,
                                gint          response_id,
                                LigmaTextTool *tool)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GFile  *file  = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
      GError *error = NULL;

      if (! ligma_text_buffer_load (tool->buffer, file, &error))
        {
          ligma_message (LIGMA_TOOL (tool)->tool_info->ligma, G_OBJECT (dialog),
                        LIGMA_MESSAGE_ERROR,
                        _("Could not open '%s' for reading: %s"),
                        ligma_file_get_utf8_name (file),
                        error->message);
          g_clear_error (&error);
          g_object_unref (file);
          return;
        }

      g_object_unref (file);
    }

  gtk_widget_hide (dialog);
}
