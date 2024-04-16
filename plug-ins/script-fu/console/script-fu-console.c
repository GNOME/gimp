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

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include <gdk/gdkkeysyms.h>

#include "script-fu-console.h"
#include "script-fu-console-editor.h"
#include "script-fu-console-history.h"
#include "script-fu-console-total.h"

#include "script-fu-lib.h"
#include "script-fu-intl.h"


#define TEXT_WIDTH  480
#define TEXT_HEIGHT 400

#define PROC_NAME   "plug-in-script-fu-console"

typedef struct
{
  GtkWidget     *dialog;
  GtkTextBuffer *total_history;
  GtkWidget     *editor;
  GtkWidget     *history_view;
  GtkWidget     *proc_browser;
  GtkWidget     *save_dialog;

  CommandHistory history;
} ConsoleInterface;

enum
{
  RESPONSE_CLEAR,
  RESPONSE_SAVE
};

/*
 *  Local Functions
 */
static void      script_fu_console_response      (GtkWidget        *widget,
                                                  gint              response_id,
                                                  ConsoleInterface *console);
static void      script_fu_console_save_dialog   (ConsoleInterface *console);
static void      script_fu_console_save_response (GtkWidget        *dialog,
                                                  gint              response_id,
                                                  ConsoleInterface *console);

static void      script_fu_browse_callback       (GtkWidget        *widget,
                                                  ConsoleInterface *console);
static void      script_fu_browse_response       (GtkWidget        *widget,
                                                  gint              response_id,
                                                  ConsoleInterface *console);
static void      script_fu_browse_row_activated  (GtkDialog        *dialog);

static gboolean  script_fu_editor_key_function   (GtkWidget        *widget,
                                                  GdkEventKey      *event,
                                                  ConsoleInterface *console);
static void      script_fu_console_scroll_end    (GtkWidget        *view);
static void      script_fu_output_to_console     (gboolean          is_error,
                                                  const gchar      *text,
                                                  gint              len,
                                                  gpointer          user_data);

static void      script_fu_models_from_settings  (ConsoleInterface    *console,
                                                  GimpProcedureConfig *config);
static void      script_fu_command_to_history    (ConsoleInterface    *console,
                                                  const gchar         *command);

/*
 *  Function definitions
 */

GimpValueArray *
script_fu_console_run (GimpProcedure       *procedure,
                       GimpProcedureConfig *config)
{
  ConsoleInterface  console = { 0, };
  GtkWidget        *vbox;
  GtkWidget        *button;
  GtkWidget        *scrolled_window;
  GtkWidget        *hbox;

  script_fu_set_print_flag (1);

  gimp_ui_init ("script-fu");

  /* Create model early so we can fill from settings. */
  console.total_history = console_total_history_new ();
  console_history_init (&console.history);
  console_total_append_welcome (console.total_history);

  script_fu_models_from_settings (&console, config);

  console.dialog = gimp_dialog_new (_("Script Console"),
                                    "gimp-script-fu-console",
                                    NULL, 0,
                                    gimp_standard_help_func, PROC_NAME,

                                    _("_Save"),  RESPONSE_SAVE,
                                    _("C_lear"), RESPONSE_CLEAR,
                                    _("_Close"), GTK_RESPONSE_CLOSE,

                                    NULL);
  gtk_window_set_default_size (GTK_WINDOW (console.dialog), TEXT_WIDTH,
                               TEXT_HEIGHT);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (console.dialog),
                                           GTK_RESPONSE_CLOSE,
                                           RESPONSE_CLEAR,
                                           RESPONSE_SAVE,
                                           -1);

  g_object_add_weak_pointer (G_OBJECT (console.dialog),
                             (gpointer) &console.dialog);

  g_signal_connect (console.dialog, "response",
                    G_CALLBACK (script_fu_console_response),
                    &console);

  /*  The main vbox  */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (console.dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /*  A view of the total history.  */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  console.history_view = gtk_text_view_new_with_buffer (console.total_history);
  /* View keeps reference.  Unref our ref so buffer is destroyed with view. */
  g_object_unref (console.total_history);

  gtk_text_view_set_editable (GTK_TEXT_VIEW (console.history_view), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (console.history_view),
                               GTK_WRAP_WORD);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (console.history_view), 6);
  gtk_text_view_set_right_margin (GTK_TEXT_VIEW (console.history_view), 6);
  gtk_widget_set_size_request (console.history_view, TEXT_WIDTH, TEXT_HEIGHT);
  gtk_container_add (GTK_CONTAINER (scrolled_window), console.history_view);
  gtk_widget_show (console.history_view);

  /*  An editor of a command to be executed. */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  console.editor = console_editor_new ();
  gtk_box_pack_start (GTK_BOX (hbox), console.editor, TRUE, TRUE, 0);
  gtk_widget_grab_focus (console.editor);
  gtk_widget_show (console.editor);

  g_signal_connect (console.editor, "key-press-event",
                    G_CALLBACK (script_fu_editor_key_function),
                    &console);

  button = gtk_button_new_with_mnemonic (_("_Browse..."));
  g_object_set (gtk_bin_get_child (GTK_BIN (button)),
                "margin-start", 2,
                "margin-end",   2,
                NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (script_fu_browse_callback),
                    &console);

  gtk_widget_show (console.dialog);

  /* The history model may fill the view, scroll. */
  script_fu_console_scroll_end (console.history_view);

  gtk_main ();

  if (console.save_dialog)
    gtk_widget_destroy (console.save_dialog);

  if (console.dialog)
    gtk_widget_destroy (console.dialog);

  /* Update config with user's change to history */
  console_history_to_settings (&console.history, config);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static void
script_fu_console_response (GtkWidget        *widget,
                            gint              response_id,
                            ConsoleInterface *console)
{
  switch (response_id)
    {
    case RESPONSE_CLEAR:
      console_total_history_clear (console->total_history);
      break;

    case RESPONSE_SAVE:
      script_fu_console_save_dialog (console);
      break;

    default:
      gtk_main_quit ();
      break;
    }
}


static void
script_fu_console_save_dialog (ConsoleInterface *console)
{
  if (! console->save_dialog)
    {
      console->save_dialog =
        gtk_file_chooser_dialog_new (_("Save Script-Fu Console Output"),
                                     GTK_WINDOW (console->dialog),
                                     GTK_FILE_CHOOSER_ACTION_SAVE,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Save"),   GTK_RESPONSE_OK,

                                     NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (console->save_dialog),
                                       GTK_RESPONSE_OK);
      gimp_dialog_set_alternative_button_order (GTK_DIALOG (console->save_dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (console->save_dialog),
                                                      TRUE);

      g_object_add_weak_pointer (G_OBJECT (console->save_dialog),
                                 (gpointer) &console->save_dialog);

      g_signal_connect (console->save_dialog, "response",
                        G_CALLBACK (script_fu_console_save_response),
                        console);
    }

  gtk_window_present (GTK_WINDOW (console->save_dialog));
}

static void
script_fu_console_save_response (GtkWidget        *dialog,
                                 gint              response_id,
                                 ConsoleInterface *console)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar *filename;
      gchar *str;
      FILE  *fh;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      fh = g_fopen (filename, "w");

      if (! fh)
        {
          g_message (_("Could not open '%s' for writing: %s"),
                     gimp_filename_to_utf8 (filename),
                     g_strerror (errno));

          g_free (filename);
          return;
        }

      str = console_total_history_get_text (console->total_history);

      fputs (str, fh);
      fclose (fh);

      g_free (str);
    }

  gtk_widget_hide (dialog);
}

static void
script_fu_browse_callback (GtkWidget        *widget,
                           ConsoleInterface *console)
{
  if (! console->proc_browser)
    {
      console->proc_browser =
        gimp_proc_browser_dialog_new (_("Script-Fu Procedure Browser"),
                                      "script-fu-procedure-browser",
                                      gimp_standard_help_func, PROC_NAME,

                                      _("_Apply"), GTK_RESPONSE_APPLY,
                                      _("_Close"), GTK_RESPONSE_CLOSE,

                                      NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (console->proc_browser),
                                       GTK_RESPONSE_APPLY);
      gimp_dialog_set_alternative_button_order (GTK_DIALOG (console->proc_browser),
                                               GTK_RESPONSE_CLOSE,
                                               GTK_RESPONSE_APPLY,
                                               -1);

      g_object_add_weak_pointer (G_OBJECT (console->proc_browser),
                                 (gpointer) &console->proc_browser);

      g_signal_connect (console->proc_browser, "response",
                        G_CALLBACK (script_fu_browse_response),
                        console);
      g_signal_connect (console->proc_browser, "row-activated",
                        G_CALLBACK (script_fu_browse_row_activated),
                        console);
    }

  gtk_window_present (GTK_WINDOW (console->proc_browser));
}

static void
script_fu_browse_response (GtkWidget        *widget,
                           gint              response_id,
                           ConsoleInterface *console)
{
  GimpProcBrowserDialog  *dialog = GIMP_PROC_BROWSER_DIALOG (widget);
  GimpProcedure          *procedure;
  gchar                  *proc_name;
  GParamSpec            **pspecs;
  gint                    n_pspecs;
  gint                    i;
  GString                *text;

  if (response_id != GTK_RESPONSE_APPLY)
    {
      gtk_widget_destroy (widget);
      return;
    }

  proc_name = gimp_proc_browser_dialog_get_selected (dialog);

  if (proc_name == NULL)
    return;

  procedure = gimp_pdb_lookup_procedure (gimp_get_pdb (), proc_name);

  pspecs = gimp_procedure_get_arguments (procedure, &n_pspecs);

  text = g_string_new ("(");
  text = g_string_append (text, proc_name);

  for (i = 0; i < n_pspecs; i++)
    {
      text = g_string_append_c (text, ' ');
      text = g_string_append (text, pspecs[i]->name);
    }

  text = g_string_append_c (text, ')');

  gtk_window_set_focus (GTK_WINDOW (console->dialog), console->editor);

  console_editor_set_text_and_position (console->editor,
                                        text->str,
                                        g_utf8_pointer_to_offset (
                                          text->str,
                                          text->str + strlen (proc_name) + 2));

  g_string_free (text, TRUE);

  gtk_window_present (GTK_WINDOW (console->dialog));

  g_free (proc_name);
}

static void
script_fu_browse_row_activated (GtkDialog *dialog)
{
  gtk_dialog_response (dialog, GTK_RESPONSE_APPLY);
}

static gboolean
script_fu_console_idle_scroll_end (GtkWidget *view)
{
  GtkWidget *parent = gtk_widget_get_parent (view);

  if (parent)
    {
      GtkAdjustment *adj;

      adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (parent));

      gtk_adjustment_set_value (adj,
                                gtk_adjustment_get_upper (adj) -
                                gtk_adjustment_get_page_size (adj));
    }

  g_object_unref (view);

  return FALSE;
}

static void
script_fu_console_scroll_end (GtkWidget *view)
{
  /*  the text view idle updates, so we need to idle scroll too
   */
  g_object_ref (view);

  g_idle_add ((GSourceFunc) script_fu_console_idle_scroll_end, view);
}

/* Write result of eval to the console view.
 * But not put results in the history model.
 */
static void
script_fu_output_to_console (gboolean      is_error_msg,
                             const gchar  *result_text,
                             gint          len,
                             gpointer      user_data)
{
  ConsoleInterface *console = user_data;

  if (console && console->history_view)
    {
      if (! is_error_msg)
        console_total_append_text_normal (console->total_history, result_text, len);
      else
        console_total_append_text_emphasize (console->total_history, result_text, len);

      script_fu_console_scroll_end (console->history_view);
    }
}



static gboolean
script_fu_editor_key_function (GtkWidget        *widget,
                               GdkEventKey      *event,
                               ConsoleInterface *console)
{
  gint         direction = 0;
  GString     *output;
  gboolean     is_error;
  const gchar *command;

  switch (event->keyval)
    {
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      if (console_editor_is_empty (console->editor))
        return TRUE;

      command = g_strdup (console_editor_get_text (console->editor));

      script_fu_command_to_history (console, command);
      /* Assert history advanced to new, empty tail. */

      script_fu_console_scroll_end (console->history_view);

      console_editor_clear (console->editor);

      output = g_string_new (NULL);
      script_fu_redirect_output_to_gstr (output);

      gimp_plug_in_set_pdb_error_handler (gimp_get_plug_in (),
                                          GIMP_PDB_ERROR_HANDLER_PLUGIN);

      is_error = script_fu_interpret_string (command);

      /* Send captured stdout to console, w/o emphasis. */
      script_fu_output_to_console (FALSE, output->str, output->len, console);
      /* Assert the output had a trailing newline, possibly an empty line. */

      if (is_error)
        {
          /* Send error text to console, w emphasis. */
          const gchar *text = script_fu_get_error_msg ();

          script_fu_output_to_console (TRUE, text, strlen (text), console);

          g_free ( (gpointer) text);
        }

      gimp_plug_in_set_pdb_error_handler (gimp_get_plug_in (),
                                          GIMP_PDB_ERROR_HANDLER_INTERNAL);

      g_string_free (output, TRUE);

      gimp_displays_flush ();

      return TRUE;
      break;

    case GDK_KEY_KP_Up:
    case GDK_KEY_Up:
      direction = -1;
      break;

    case GDK_KEY_KP_Down:
    case GDK_KEY_Down:
      direction = 1;
      break;

    case GDK_KEY_P:
    case GDK_KEY_p:
      if (event->state & GDK_CONTROL_MASK)
        direction = -1;
      break;

    case GDK_KEY_N:
    case GDK_KEY_n:
      if (event->state & GDK_CONTROL_MASK)
        direction = 1;
      break;

    default:
      /* Any other key is the user editing.
       * Set cursor to tail: user is done scrolling history.
       * Must do this to ensure edited command line is saved in history.
       */
      console_history_cursor_to_tail (&console->history);
      break;
    }

  if (direction)
    {
      /* Tail was preallocated and usually empty.
       * Keep the editor contents in the tail as cursor is moved away from tail.
       * So any edited text is not lost if user moves cursor back to tail.
       */
      command = console_editor_get_text (console->editor);
      /* command can be NULL */
      if (console_history_is_cursor_at_tail (&console->history))
        console_history_set_tail (&console->history, g_strdup (command));

      /* Now move cursor and replace editor contents. */
      console_history_move_cursor (&console->history, direction);
      command = console_history_get_at_cursor (&console->history);
      /* command can be NULL. */
      console_editor_set_text_and_position (console->editor,
                                            command,
                                            -1);

      return TRUE;
    }

  return FALSE;
}

/* Restore models from settings.
 * This understands how to get history as a GStrv from settings
 * and how to put GStrv into both models.
 *
 * Just the model.  The view does not exist yet.
 */
static void
script_fu_models_from_settings (ConsoleInterface    *console,
                                GimpProcedureConfig *config)

{
  GStrv strings_in;

  /* Assert the History model is empty, recently init. */

  strings_in = console_history_from_settings (&console->history, config);

  /* The history setting can be empty, and GStrv can be NULL.
   * !!! But g_strv_length requires its arg!=NULL
   */
  if (strings_in==NULL)
    return;

  /* Adding requires a new tail. */
  console_history_new_tail (&console->history);

  /* Order of the GStrv is earliest command first.
   * Iterate ascending, i.e. earliest command to history first.
   * Not concerned with performance.
   */
  for (gint i = 0; i < g_strv_length (strings_in); i++)
    script_fu_command_to_history (console, g_strdup (strings_in[i]));

  g_strfreev (strings_in);
}


/* Append a command to history.
 *
 * Knows to put to both TotalHistory and History models.
 *
 * Transfers ownership of command to History model.
 * Caller may retain a reference, for a short time.
 *
 * !!! The History model is finite and limits itself.
 * While the TotalHistory model is nearly unlimited.
 * More commands in the view than in the History model.
 */
static void
script_fu_command_to_history (ConsoleInterface *console,
                              const gchar      *command)
{
  /* Require new_tail called previously. */

  /* To History model. */
  console_history_set_tail (&console->history, command);

  /* Advance history, editor wants preallocated tail. */
  console_history_new_tail (&console->history);

  /* Decorated command to TotalHistory model. */
  console_total_append_command (console->total_history, command);

  /* Ensure there is a new tail. */
}
