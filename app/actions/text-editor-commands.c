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

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmatextbuffer.h"
#include "widgets/ligmatexteditor.h"
#include "widgets/ligmauimanager.h"

#include "text-editor-commands.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   text_editor_load_response (GtkWidget      *dialog,
                                         gint            response_id,
                                         LigmaTextEditor *editor);


/*  public functions  */

void
text_editor_load_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaTextEditor *editor = LIGMA_TEXT_EDITOR (data);

  if (! editor->file_dialog)
    {
      GtkWidget *dialog;

      dialog = editor->file_dialog =
        gtk_file_chooser_dialog_new (_("Open Text File (UTF-8)"),
                                     GTK_WINDOW (editor),
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
      gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

      g_object_add_weak_pointer (G_OBJECT (dialog),
                                 (gpointer) &editor->file_dialog);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (text_editor_load_response),
                        editor);
      g_signal_connect (dialog, "delete-event",
                        G_CALLBACK (gtk_true),
                        NULL);
    }

  gtk_window_present (GTK_WINDOW (editor->file_dialog));
}

void
text_editor_clear_cmd_callback (LigmaAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  LigmaTextEditor *editor = LIGMA_TEXT_EDITOR (data);
  GtkTextBuffer  *buffer;

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (editor->view));

  gtk_text_buffer_set_text (buffer, "", 0);
}

void
text_editor_direction_cmd_callback (LigmaAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  LigmaTextEditor    *editor = LIGMA_TEXT_EDITOR (data);
  LigmaTextDirection  direction;

  direction = (LigmaTextDirection) g_variant_get_int32 (value);

  ligma_text_editor_set_direction (editor, direction);
}


/*  private functions  */

static void
text_editor_load_response (GtkWidget      *dialog,
                           gint            response_id,
                           LigmaTextEditor *editor)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GtkTextBuffer *buffer;
      GFile         *file;
      GError        *error = NULL;

      buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (editor->view));
      file   = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));

      if (! ligma_text_buffer_load (LIGMA_TEXT_BUFFER (buffer), file, &error))
        {
          ligma_message (editor->ui_manager->ligma, G_OBJECT (dialog),
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
