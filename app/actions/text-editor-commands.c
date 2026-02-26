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

#include "widgets/gimphelp-ids.h"
#include "widgets/gimptextbuffer.h"
#include "widgets/gimptexteditor.h"
#include "widgets/gimpuimanager.h"

#include "text-editor-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   text_editor_load_response (GtkNativeDialog *dialog,
                                         gint             response_id,
                                         GimpTextEditor  *editor);


/*  public functions  */

void
text_editor_load_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpTextEditor *editor = GIMP_TEXT_EDITOR (data);

  if (! editor->file_dialog)
    {
      GtkFileChooserNative *dialog;

      dialog = editor->file_dialog =
        gtk_file_chooser_native_new (_("Open Text File (UTF-8)"),
                                     GTK_WINDOW (editor),
                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                     _("_Open"), _("_Cancel"));

      g_set_weak_pointer (&editor->file_dialog, dialog);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (text_editor_load_response),
                        editor);
    }

  gtk_native_dialog_show (GTK_NATIVE_DIALOG (editor->file_dialog));
}

void
text_editor_clear_cmd_callback (GimpAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GimpTextEditor *editor = GIMP_TEXT_EDITOR (data);
  GtkTextBuffer  *buffer;

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (editor->view));

  gtk_text_buffer_set_text (buffer, "", 0);
}

void
text_editor_direction_cmd_callback (GimpAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  GimpTextEditor    *editor = GIMP_TEXT_EDITOR (data);
  GimpTextDirection  direction;

  direction = (GimpTextDirection) g_variant_get_int32 (value);

  gimp_text_editor_set_direction (editor, direction);
}


/*  private functions  */

static void
text_editor_load_response (GtkNativeDialog *dialog,
                           gint             response_id,
                           GimpTextEditor  *editor)
{
  if (response_id == GTK_RESPONSE_ACCEPT)
    {
      GtkTextBuffer *buffer;
      GFile         *file;
      GError        *error = NULL;

      buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (editor->view));
      file   = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));

      if (! gimp_text_buffer_load (GIMP_TEXT_BUFFER (buffer), file, &error))
        {
          gimp_message (editor->ui_manager->gimp, G_OBJECT (dialog),
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

  gtk_native_dialog_hide (dialog);
}
