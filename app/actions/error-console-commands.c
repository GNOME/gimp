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

static void   error_console_save_response (GtkWidget        *dialog,
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

  if (selection &&
      ! gtk_text_buffer_get_selection_bounds (console->text_buffer,
                                              NULL, NULL))
    {
      gimp_message_literal (console->gimp,
                            G_OBJECT (console), GIMP_MESSAGE_WARNING,
                            _("Cannot save. Nothing is selected."));
      return;
    }

  if (! console->file_dialog)
    {
      GtkWidget *dialog;

      dialog = console->file_dialog =
        gtk_file_chooser_dialog_new (_("Save Error Log to File"), NULL,
                                     GTK_FILE_CHOOSER_ACTION_SAVE,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Save"),   GTK_RESPONSE_OK,

                                     NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
      gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                                GTK_RESPONSE_OK,
                                                GTK_RESPONSE_CANCEL,
                                                -1);

      console->save_selection = selection;

      g_set_weak_pointer (&console->file_dialog, dialog);

      gtk_window_set_screen (GTK_WINDOW (dialog),
                             gtk_widget_get_screen (GTK_WIDGET (console)));
      gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
      gtk_window_set_role (GTK_WINDOW (dialog), "gimp-save-errors");

      gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                      TRUE);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (error_console_save_response),
                        console);
      g_signal_connect (dialog, "delete-event",
                        G_CALLBACK (gtk_true),
                        NULL);

      gimp_help_connect (dialog, NULL, gimp_standard_help_func,
                         GIMP_HELP_ERRORS_DIALOG, NULL, NULL);
    }

  gtk_window_present (GTK_WINDOW (console->file_dialog));

#ifdef G_OS_WIN32
  gimp_window_set_title_bar_theme (console->gimp, console->file_dialog);
#endif
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
error_console_save_response (GtkWidget        *dialog,
                             gint              response_id,
                             GimpErrorConsole *console)
{
  if (response_id == GTK_RESPONSE_OK)
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

  gtk_widget_destroy (dialog);
}
