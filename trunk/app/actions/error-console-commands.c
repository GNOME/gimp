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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"

#include "widgets/gimperrorconsole.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "error-console-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   error_console_save_response (GtkWidget        *dialog,
                                           gint              response_id,
                                           GimpErrorConsole *console);


/*  public functions  */

void
error_console_clear_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpErrorConsole *console = GIMP_ERROR_CONSOLE (data);
  GtkTextIter       start_iter;
  GtkTextIter       end_iter;

  gtk_text_buffer_get_bounds (console->text_buffer, &start_iter, &end_iter);
  gtk_text_buffer_delete (console->text_buffer, &start_iter, &end_iter);
}

void
error_console_select_all_cmd_callback (GtkAction *action,
                                       gpointer   data)
{
  GimpErrorConsole *console = GIMP_ERROR_CONSOLE (data);
  GtkTextIter       start_iter;
  GtkTextIter       end_iter;

  gtk_text_buffer_get_bounds (console->text_buffer, &start_iter, &end_iter);
  gtk_text_buffer_select_range (console->text_buffer, &start_iter, &end_iter);
}

void
error_console_save_cmd_callback (GtkAction *action,
                                 gint       value,
                                 gpointer   data)
{
  GimpErrorConsole *console = GIMP_ERROR_CONSOLE (data);
  GtkFileChooser   *chooser;

  if (value && ! gtk_text_buffer_get_selection_bounds (console->text_buffer,
                                                       NULL, NULL))
    {
      gimp_message (console->gimp, G_OBJECT (console), GIMP_MESSAGE_WARNING,
                    _("Cannot save. Nothing is selected."));
      return;
    }

  if (console->file_dialog)
    {
      gtk_window_present (GTK_WINDOW (console->file_dialog));
      return;
    }

  console->file_dialog =
    gtk_file_chooser_dialog_new (_("Save Error Log to File"), NULL,
                                 GTK_FILE_CHOOSER_ACTION_SAVE,

                                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                 GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

                                 NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (console->file_dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  console->save_selection = value;

  g_object_add_weak_pointer (G_OBJECT (console->file_dialog),
                             (gpointer) &console->file_dialog);

  chooser = GTK_FILE_CHOOSER (console->file_dialog);

  gtk_window_set_screen (GTK_WINDOW (chooser),
                         gtk_widget_get_screen (GTK_WIDGET (console)));

  gtk_window_set_position (GTK_WINDOW (chooser), GTK_WIN_POS_MOUSE);
  gtk_window_set_role (GTK_WINDOW (chooser), "gimp-save-errors");

  gtk_dialog_set_default_response (GTK_DIALOG (chooser), GTK_RESPONSE_OK);
  gtk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);

  g_signal_connect (chooser, "response",
                    G_CALLBACK (error_console_save_response),
                    console);
  g_signal_connect (chooser, "delete-event",
                    G_CALLBACK (gtk_true),
                    NULL);

  gimp_help_connect (GTK_WIDGET (chooser), gimp_standard_help_func,
                     GIMP_HELP_ERRORS_DIALOG, NULL);

  gtk_widget_show (GTK_WIDGET (chooser));
}


/*  private functions  */

static void
error_console_save_response (GtkWidget        *dialog,
                             gint              response_id,
                             GimpErrorConsole *console)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GError *error = NULL;
      gchar  *filename;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      if (! gimp_text_buffer_save (console->text_buffer, filename,
                                   console->save_selection, &error))
        {
          gimp_message (console->gimp, G_OBJECT (dialog), GIMP_MESSAGE_ERROR,
                        _("Error writing file '%s':\n%s"),
                        gimp_filename_to_utf8 (filename),
                        error->message);
          g_clear_error (&error);
          g_free (filename);
          return;
        }

      g_free (filename);
    }

  gtk_widget_destroy (dialog);
}
