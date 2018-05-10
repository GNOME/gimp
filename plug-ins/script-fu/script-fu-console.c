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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include <gdk/gdkkeysyms.h>

#include "scheme-wrapper.h"
#include "script-fu-console.h"

#include "script-fu-intl.h"


#define TEXT_WIDTH  480
#define TEXT_HEIGHT 400

#define PROC_NAME   "plug-in-script-fu-console"

typedef struct
{
  GtkWidget     *dialog;
  GtkTextBuffer *console;
  GtkWidget     *cc;
  GtkWidget     *text_view;
  GtkWidget     *proc_browser;
  GtkWidget     *save_dialog;

  GList         *history;
  gint           history_len;
  gint           history_cur;
  gint           history_max;
} ConsoleInterface;

enum
{
  RESPONSE_CLEAR,
  RESPONSE_SAVE
};

/*
 *  Local Functions
 */
static void      script_fu_console_interface     (void);
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

static gboolean  script_fu_cc_is_empty           (ConsoleInterface *console);
static gboolean  script_fu_cc_key_function       (GtkWidget        *widget,
                                                  GdkEventKey      *event,
                                                  ConsoleInterface *console);

static void      script_fu_output_to_console     (TsOutputType      type,
                                                  const gchar      *text,
                                                  gint              len,
                                                  gpointer          user_data);

/*
 *  Function definitions
 */

void
script_fu_console_run (const gchar      *name,
                       gint              nparams,
                       const GimpParam  *params,
                       gint             *nreturn_vals,
                       GimpParam       **return_vals)
{
  static GimpParam  values[1];

  ts_set_print_flag (1);
  script_fu_console_interface ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;
}

static void
script_fu_console_interface (void)
{
  ConsoleInterface  console = { 0, };
  GtkWidget        *vbox;
  GtkWidget        *button;
  GtkWidget        *scrolled_window;
  GtkWidget        *hbox;

  gimp_ui_init ("script-fu", FALSE);

  console.history_max = 50;

  console.dialog = gimp_dialog_new (_("Script-Fu Console"),
                                    "gimp-script-fu-console",
                                    NULL, 0,
                                    gimp_standard_help_func, PROC_NAME,

                                    _("_Save"),  RESPONSE_SAVE,
                                    _("C_lear"), RESPONSE_CLEAR,
                                    _("_Close"), GTK_RESPONSE_CLOSE,

                                    NULL);

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

  /*  The output text widget  */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  console.console = gtk_text_buffer_new (NULL);
  console.text_view = gtk_text_view_new_with_buffer (console.console);
  g_object_unref (console.console);

  gtk_text_view_set_editable (GTK_TEXT_VIEW (console.text_view), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (console.text_view),
                               GTK_WRAP_WORD);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (console.text_view), 6);
  gtk_text_view_set_right_margin (GTK_TEXT_VIEW (console.text_view), 6);
  gtk_widget_set_size_request (console.text_view, TEXT_WIDTH, TEXT_HEIGHT);
  gtk_container_add (GTK_CONTAINER (scrolled_window), console.text_view);
  gtk_widget_show (console.text_view);

  gtk_text_buffer_create_tag (console.console, "strong",
                              "weight", PANGO_WEIGHT_BOLD,
                              "scale",  PANGO_SCALE_LARGE,
                              NULL);
  gtk_text_buffer_create_tag (console.console, "emphasis",
                              "style",  PANGO_STYLE_OBLIQUE,
                              NULL);

  {
    const gchar * const greetings[] =
    {
      "strong",   N_("Welcome to TinyScheme"),
      NULL,       "\n",
      NULL,       "Copyright (c) Dimitrios Souflis",
      NULL,       "\n",
      "strong",   N_("Script-Fu Console"),
      NULL,       " - ",
      "emphasis", N_("Interactive Scheme Development"),
      NULL,       "\n"
    };

    GtkTextIter cursor;
    gint        i;

    gtk_text_buffer_get_end_iter (console.console, &cursor);

    for (i = 0; i < G_N_ELEMENTS (greetings); i += 2)
      {
        if (greetings[i])
          gtk_text_buffer_insert_with_tags_by_name (console.console, &cursor,
                                                    gettext (greetings[i + 1]),
                                                    -1, greetings[i],
                                                    NULL);
        else
          gtk_text_buffer_insert (console.console, &cursor,
                                  gettext (greetings[i + 1]), -1);
      }
  }

  /*  The current command  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  console.cc = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), console.cc, TRUE, TRUE, 0);
  gtk_widget_grab_focus (console.cc);
  gtk_widget_show (console.cc);

  g_signal_connect (console.cc, "key-press-event",
                    G_CALLBACK (script_fu_cc_key_function),
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

  /*  Initialize the history  */
  console.history     = g_list_append (console.history, NULL);
  console.history_len = 1;

  gtk_widget_show (console.dialog);

  gtk_main ();

  if (console.save_dialog)
    gtk_widget_destroy (console.save_dialog);

  if (console.dialog)
    gtk_widget_destroy (console.dialog);
}

static void
script_fu_console_response (GtkWidget        *widget,
                            gint              response_id,
                            ConsoleInterface *console)
{
  GtkTextIter start, end;

  switch (response_id)
    {
    case RESPONSE_CLEAR:
      gtk_text_buffer_get_start_iter (console->console, &start);
      gtk_text_buffer_get_end_iter (console->console, &end);
      gtk_text_buffer_delete (console->console, &start, &end);
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
  GtkTextIter start, end;

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

      gtk_text_buffer_get_start_iter (console->console, &start);
      gtk_text_buffer_get_end_iter (console->console, &end);

      str = gtk_text_buffer_get_text (console->console, &start, &end, FALSE);

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
  GimpProcBrowserDialog *dialog = GIMP_PROC_BROWSER_DIALOG (widget);
  gchar                 *proc_name;
  gchar                 *proc_blurb;
  gchar                 *proc_help;
  gchar                 *proc_author;
  gchar                 *proc_copyright;
  gchar                 *proc_date;
  GimpPDBProcType        proc_type;
  gint                   n_params;
  gint                   n_return_vals;
  GimpParamDef          *params;
  GimpParamDef          *return_vals;
  gint                   i;
  GString               *text;

  if (response_id != GTK_RESPONSE_APPLY)
    {
      gtk_widget_destroy (widget);
      return;
    }

  proc_name = gimp_proc_browser_dialog_get_selected (dialog);

  if (proc_name == NULL)
    return;

  gimp_procedural_db_proc_info (proc_name,
                                &proc_blurb,
                                &proc_help,
                                &proc_author,
                                &proc_copyright,
                                &proc_date,
                                &proc_type,
                                &n_params,
                                &n_return_vals,
                                &params,
                                &return_vals);

  text = g_string_new ("(");
  text = g_string_append (text, proc_name);

  for (i = 0; i < n_params; i++)
    {
      text = g_string_append_c (text, ' ');
      text = g_string_append (text, params[i].name);
    }

  text = g_string_append_c (text, ')');

  gtk_window_set_focus (GTK_WINDOW (console->dialog), console->cc);

  gtk_entry_set_text (GTK_ENTRY (console->cc), text->str);
  gtk_editable_set_position (GTK_EDITABLE (console->cc),
                             g_utf8_pointer_to_offset (text->str,
                                                       text->str +
                                                       strlen (proc_name) + 2));

  g_string_free (text, TRUE);

  gtk_window_present (GTK_WINDOW (console->dialog));

  g_free (proc_name);
  g_free (proc_blurb);
  g_free (proc_help);
  g_free (proc_author);
  g_free (proc_copyright);
  g_free (proc_date);

  gimp_destroy_paramdefs (params,      n_params);
  gimp_destroy_paramdefs (return_vals, n_return_vals);
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

static void
script_fu_output_to_console (TsOutputType  type,
                             const gchar  *text,
                             gint          len,
                             gpointer      user_data)
{
  ConsoleInterface *console = user_data;

  if (console && console->text_view)
    {
      GtkTextBuffer *buffer;
      GtkTextIter    cursor;

      buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (console->text_view));

      gtk_text_buffer_get_end_iter (buffer, &cursor);

      if (type == TS_OUTPUT_NORMAL)
        {
          gtk_text_buffer_insert (buffer, &cursor, text, len);
        }
      else
        {
          gtk_text_buffer_insert_with_tags_by_name (console->console, &cursor,
                                                    text, len, "emphasis",
                                                    NULL);
        }

      script_fu_console_scroll_end (console->text_view);
    }
}

static gboolean
script_fu_cc_is_empty (ConsoleInterface *console)
{
  const gchar *str;

  if ((str = gtk_entry_get_text (GTK_ENTRY (console->cc))) == NULL)
    return TRUE;

  while (*str)
    {
      if (*str != ' ' && *str != '\t' && *str != '\n')
        return FALSE;

      str ++;
    }

  return TRUE;
}

static gboolean
script_fu_cc_key_function (GtkWidget        *widget,
                           GdkEventKey      *event,
                           ConsoleInterface *console)
{
  GList       *list;
  gint         direction = 0;
  GtkTextIter  cursor;
  GString     *output;

  switch (event->keyval)
    {
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      if (script_fu_cc_is_empty (console))
        return TRUE;

      list = g_list_nth (console->history,
                         (g_list_length (console->history) - 1));

      if (list->data)
        g_free (list->data);

      list->data = g_strdup (gtk_entry_get_text (GTK_ENTRY (console->cc)));

      gtk_text_buffer_get_end_iter (console->console, &cursor);

      gtk_text_buffer_insert (console->console, &cursor, "\n", 1);
      gtk_text_buffer_insert_with_tags_by_name (console->console, &cursor,
                                                "> ", 2,
                                                "strong",
                                                NULL);

      gtk_text_buffer_insert (console->console, &cursor,
                              gtk_entry_get_text (GTK_ENTRY (console->cc)), -1);
      gtk_text_buffer_insert (console->console, &cursor, "\n", 1);

      script_fu_console_scroll_end (console->text_view);

      gtk_entry_set_text (GTK_ENTRY (console->cc), "");

      output = g_string_new (NULL);
      ts_register_output_func (ts_gstring_output_func, output);

      gimp_plugin_set_pdb_error_handler (GIMP_PDB_ERROR_HANDLER_PLUGIN);

      if (ts_interpret_string (list->data) != 0)
        {
          script_fu_output_to_console (TS_OUTPUT_ERROR,
                                       output->str,
                                       output->len,
                                       console);
        }
      else
        {
          script_fu_output_to_console (TS_OUTPUT_NORMAL,
                                       output->str,
                                       output->len,
                                       console);
        }

      gimp_plugin_set_pdb_error_handler (GIMP_PDB_ERROR_HANDLER_INTERNAL);

      g_string_free (output, TRUE);

      gimp_displays_flush ();

      console->history = g_list_append (console->history, NULL);

      if (console->history_len == console->history_max)
        {
          console->history = g_list_remove (console->history,
                                            console->history->data);
          if (console->history->data)
            g_free (console->history->data);
        }
      else
        {
          console->history_len++;
        }

      console->history_cur = g_list_length (console->history) - 1;

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
      break;
    }

  if (direction)
    {
      /*  Make sure we keep track of the current one  */
      if (console->history_cur == g_list_length (console->history) - 1)
        {
          list = g_list_nth (console->history, console->history_cur);

          g_free (list->data);
          list->data = g_strdup (gtk_entry_get_text (GTK_ENTRY (console->cc)));
        }

      console->history_cur += direction;

      if (console->history_cur < 0)
        console->history_cur = 0;

      if (console->history_cur >= console->history_len)
        console->history_cur = console->history_len - 1;

      gtk_entry_set_text (GTK_ENTRY (console->cc),
                          (gchar *) (g_list_nth (console->history,
                                                 console->history_cur))->data);

      gtk_editable_set_position (GTK_EDITABLE (console->cc), -1);

      return TRUE;
    }

  return FALSE;
}
