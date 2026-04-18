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

#include "widgets/gimperrorconsole.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimptextbuffer.h"
#include "widgets/gimpwidgets-utils.h"

#include "error-console-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   error_console_save_response (GtkNativeDialog  *dialog,
                                           gint              response_id,
                                           GimpErrorConsole *console);


/*  public functions  */

void
error_console_clear_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpErrorConsole *console = GIMP_ERROR_CONSOLE (data);
  GtkTextIter       start_iter;
  GtkTextIter       end_iter;

  gtk_text_buffer_get_bounds (console->text_buffer, &start_iter, &end_iter);
  gtk_text_buffer_delete (console->text_buffer, &start_iter, &end_iter);
}

void
error_console_select_all_cmd_callback (GimpAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  GimpErrorConsole *console = GIMP_ERROR_CONSOLE (data);
  GtkTextIter       start_iter;
  GtkTextIter       end_iter;

  gtk_text_buffer_get_bounds (console->text_buffer, &start_iter, &end_iter);
  gtk_text_buffer_select_range (console->text_buffer, &start_iter, &end_iter);
}

void
error_console_save_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpErrorConsole *console   = GIMP_ERROR_CONSOLE (data);
  gboolean          selection = (gboolean) g_variant_get_int32 (value);
  GtkFileFilter    *filter;

  if (selection &&
      ! gtk_text_buffer_get_selection_bounds (console->text_buffer,
                                              NULL, NULL))
    {
      gimp_message_literal (console->gimp,
                            G_OBJECT (console), GIMP_MESSAGE_WARNING,
                            _("Cannot save. Nothing is selected."));
      return;
    }

  console->file_dialog =
    gtk_file_chooser_native_new (_("Save Error Log to File"), NULL,
                                 GTK_FILE_CHOOSER_ACTION_SAVE,
                                 _("_Save"), _("_Cancel"));

  console->save_selection = selection;

  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (console->file_dialog),
                                                  TRUE);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Log Files (*.log)"));
  gtk_file_filter_add_pattern (filter, "*.log");
  gtk_file_filter_add_pattern (filter, "*.LOG");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (console->file_dialog), filter);

  g_signal_connect_object (console->file_dialog, "response",
                           G_CALLBACK (error_console_save_response),
                           console, 0);

  gtk_native_dialog_show (GTK_NATIVE_DIALOG (console->file_dialog));
}

void
error_console_highlight_error_cmd_callback (GimpAction *action,
                                            GVariant   *value,
                                            gpointer    data)
{
  GimpErrorConsole *console = GIMP_ERROR_CONSOLE (data);
  gboolean          active  = g_variant_get_boolean (value);

  console->highlight[GIMP_MESSAGE_ERROR] = active;
}

void
error_console_highlight_warning_cmd_callback (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data)
{
  GimpErrorConsole *console = GIMP_ERROR_CONSOLE (data);
  gboolean          active  = g_variant_get_boolean (value);

  console->highlight[GIMP_MESSAGE_WARNING] = active;
}

void
error_console_highlight_info_cmd_callback (GimpAction *action,
                                           GVariant   *value,
                                           gpointer    data)
{
  GimpErrorConsole *console = GIMP_ERROR_CONSOLE (data);
  gboolean          active  = g_variant_get_boolean (value);

  console->highlight[GIMP_MESSAGE_INFO] = active;
}


/*  private functions  */

static void
error_console_save_response (GtkNativeDialog  *dialog,
                             gint              response_id,
                             GimpErrorConsole *console)
{
  if (response_id == GTK_RESPONSE_ACCEPT)
    {
      GFile  *file  = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
      GError *error = NULL;

      if (! gimp_text_buffer_save (GIMP_TEXT_BUFFER (console->text_buffer),
                                   file,
                                   console->save_selection, &error))
        {
          gimp_message (console->gimp, G_OBJECT (dialog), GIMP_MESSAGE_ERROR,
                        _("Error writing file '%s':\n%s"),
                        gimp_file_get_utf8_name (file),
                        error->message);
          g_clear_error (&error);
          g_object_unref (file);
          return;
        }

      g_object_unref (file);
    }

  gtk_native_dialog_destroy (GTK_NATIVE_DIALOG (console->file_dialog));
  console->file_dialog = NULL;
}
