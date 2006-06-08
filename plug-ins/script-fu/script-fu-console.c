/* The GIMP -- an image manipulation program
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

#include <gdk/gdkkeysyms.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "script-fu-intl.h"

#include "siod-wrapper.h"
#include "script-fu-console.h"


#define TEXT_WIDTH  480
#define TEXT_HEIGHT 400

#define BUFSIZE     256

#define PROC_NAME   "plug-in-script-fu-console"


typedef struct
{
  GtkWidget     *dialog;
  GtkTextBuffer *console;
  GtkWidget     *cc;
  GtkWidget     *text_view;
  GtkWidget     *proc_browser;

  gint32         input_id;

  GList         *history;
  gint           history_len;
  gint           history_cur;
  gint           history_max;
} ConsoleInterface;


/*
 *  Local Functions
 */
static void       script_fu_console_interface    (void);
static void       script_fu_response             (GtkWidget        *widget,
                                                  gint              response_id,
                                                  ConsoleInterface *console);
static void       script_fu_browse_callback      (GtkWidget        *widget,
                                                  ConsoleInterface *console);
static void       script_fu_browse_response      (GtkWidget        *widget,
                                                  gint              response_id,
                                                  ConsoleInterface *console);
static void       script_fu_browse_row_activated (GtkDialog        *dialog);
static gboolean   script_fu_cc_is_empty          (ConsoleInterface *console);
static gboolean   script_fu_cc_key_function      (GtkWidget        *widget,
                                                  GdkEventKey      *event,
                                                  ConsoleInterface *console);

static void       script_fu_open_siod_console    (void);
static void       script_fu_close_siod_console   (void);


/*
 *  Local variables
 */
static ConsoleInterface cint =
{
  NULL,  /*  dialog  */
  NULL,  /*  console  */
  NULL,  /*  current command  */
  NULL,  /*  text view  */
  NULL,  /*  proc browser  */

  -1,    /*  input id  */

  NULL,
  0,
  0,
  50
};


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

  script_fu_open_siod_console ();

  script_fu_console_interface ();

  script_fu_close_siod_console ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;
}

static void
script_fu_console_interface (void)
{
  ConsoleInterface *console;
  GtkWidget        *vbox;
  GtkWidget        *button;
  GtkWidget        *scrolled_window;
  GtkWidget        *hbox;

  gimp_ui_init ("script-fu", FALSE);

  console = &cint;

  console->input_id    = -1;
  console->history     = NULL;
  console->history_len = 0;
  console->history_cur = 0;
  console->history_max = 50;

  console->dialog = gimp_dialog_new (_("Script-Fu Console"),
                                     "script-fu-console",
                                     NULL, 0,
                                     gimp_standard_help_func, PROC_NAME,

                                     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,

                                     NULL);

  g_signal_connect (console->dialog, "response",
		    G_CALLBACK (script_fu_response),
		    console);
  g_signal_connect (console->dialog, "destroy",
		    G_CALLBACK (gtk_widget_destroyed),
		    &console->dialog);

  /*  The main vbox  */
  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (console->dialog)->vbox), vbox,
		      TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /*  The output text widget  */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  console->console = gtk_text_buffer_new (NULL);
  console->text_view = gtk_text_view_new_with_buffer (console->console);
  g_object_unref (console->console);

  gtk_text_view_set_editable (GTK_TEXT_VIEW (console->text_view), FALSE);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (console->text_view), 12);
  gtk_text_view_set_right_margin (GTK_TEXT_VIEW (console->text_view), 12);
  gtk_widget_set_size_request (console->text_view, TEXT_WIDTH, TEXT_HEIGHT);
  gtk_container_add (GTK_CONTAINER (scrolled_window), console->text_view);
  gtk_widget_show (console->text_view);

  gtk_text_buffer_create_tag (console->console, "strong",
			      "weight", PANGO_WEIGHT_BOLD,
			      "size",   12 * PANGO_SCALE,
			      NULL);
  gtk_text_buffer_create_tag (console->console, "emphasis",
			      "style",  PANGO_STYLE_OBLIQUE,
			      "size",   10 * PANGO_SCALE,
			      NULL);
  gtk_text_buffer_create_tag (console->console, "weak",
			      "size",   10 * PANGO_SCALE,
			      NULL);

  {
    const gchar *greeting_texts[] =
    {
      "weak",     "\n",
      "strong",   "Welcome to SIOD, Scheme In One Defun\n",
      "weak",     "(C) Copyright 1988-1994 Paradigm Associates Inc.\n\n",
      "strong",   "Script-Fu Console - ",
      "emphasis", "Interactive Scheme Development\n",
      NULL
    };

    GtkTextIter cursor;
    gint        i;

    gtk_text_buffer_get_end_iter (console->console, &cursor);

    for (i = 0; greeting_texts[i]; i += 2)
      {
	gtk_text_buffer_insert_with_tags_by_name (console->console, &cursor,
						  greeting_texts[i + 1], -1,
						  greeting_texts[i],
						  NULL);
      }
  }

  /*  The current command  */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  console->cc = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), console->cc, TRUE, TRUE, 0);
  gtk_widget_grab_focus (console->cc);
  gtk_widget_show (console->cc);

  g_signal_connect (console->cc, "key-press-event",
		    G_CALLBACK (script_fu_cc_key_function),
		    console);

  button = gtk_button_new_with_mnemonic (_("_Browse..."));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
		    G_CALLBACK (script_fu_browse_callback),
		    console);

  /*  Initialize the history  */
  console->history     = g_list_append (console->history, NULL);
  console->history_len = 1;

  gtk_widget_show (console->dialog);

  gtk_main ();

  g_source_remove (console->input_id);

  if (console->dialog)
    gtk_widget_destroy (console->dialog);
}

static void
script_fu_response (GtkWidget        *widget,
                    gint              response_id,
                    ConsoleInterface *console)
{
  gtk_main_quit ();
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

                                      GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
                                      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,

                                      NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (console->proc_browser),
                                       GTK_RESPONSE_APPLY);

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

  gtk_entry_set_text (GTK_ENTRY (console->cc), text->str);
  g_string_free (text, TRUE);

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
script_fu_console_idle_scroll_end (ConsoleInterface *console)
{
  GtkAdjustment *adj = GTK_TEXT_VIEW (console->text_view)->vadjustment;

  gtk_adjustment_set_value (adj, adj->upper - adj->page_size);

  return FALSE;
}

static void
script_fu_console_scroll_end (ConsoleInterface *console)
{
  /*  the text view idle updates so we need to idle scroll too
   */
  g_idle_add ((GSourceFunc) script_fu_console_idle_scroll_end, console);
}

void
script_fu_output_to_console (gchar *text)
{
  GtkTextIter cursor;

  gtk_text_buffer_get_end_iter (cint.console, &cursor);
  gtk_text_buffer_insert_with_tags_by_name (cint.console, &cursor,
                                            text, -1,
                                            "weak",
                                            NULL);

  script_fu_console_scroll_end (&cint);
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

  switch (event->keyval)
    {
    case GDK_Return:
      if (script_fu_cc_is_empty (console))
	return TRUE;

      list = g_list_nth (console->history,
                         (g_list_length (console->history) - 1));
      if (list->data)
	g_free (list->data);
      list->data = g_strdup (gtk_entry_get_text (GTK_ENTRY (console->cc)));

      gtk_text_buffer_get_end_iter (console->console, &cursor);

      gtk_text_buffer_insert_with_tags_by_name (console->console, &cursor,
						"\n=> ", -1,
						"strong",
						NULL);

      gtk_text_buffer_insert_with_tags_by_name (console->console, &cursor,
                                                gtk_entry_get_text (GTK_ENTRY (console->cc)), -1,
                                                "weak",
                                                NULL);

      gtk_text_buffer_insert_with_tags_by_name (console->console, &cursor,
						"\n", -1,
						"weak",
						NULL);

      script_fu_console_scroll_end (console);

      gtk_entry_set_text (GTK_ENTRY (console->cc), "");

      siod_interpret_string ((const gchar *) list->data);
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
	console->history_len++;
      console->history_cur = g_list_length (console->history) - 1;

      return TRUE;
      break;

    case GDK_KP_Up:
    case GDK_Up:
      direction = -1;
      break;

    case GDK_KP_Down:
    case GDK_Down:
      direction = 1;
      break;

    case GDK_P:
    case GDK_p:
      if (event->state & GDK_CONTROL_MASK)
        direction = -1;
      break;

    case GDK_N:
    case GDK_n:
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
	  if (list->data)
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

static void
script_fu_open_siod_console (void)
{
  siod_set_console_mode (1);
  siod_set_verbose_level (2);
}

static void
script_fu_close_siod_console (void)
{
  FILE *siod_output;

  siod_output = siod_get_output_file ();

  if (siod_output != stdout)
    fclose (siod_output);

  siod_set_console_mode (0);
}

void
script_fu_eval_run (const gchar      *name,
		    gint              nparams,
		    const GimpParam  *params,
		    gint             *nreturn_vals,
		    GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  GimpRunMode       run_mode;

  run_mode = params[0].data.d_int32;

  switch (run_mode)
    {
    case GIMP_RUN_NONINTERACTIVE:
      if (siod_interpret_string (params[1].data.d_string) != 0)
        {
          const gchar *msg = siod_get_error_msg ();

          if (msg)
            g_printerr (msg);

          status = GIMP_PDB_EXECUTION_ERROR;
        }
      break;

    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      status = GIMP_PDB_CALLING_ERROR;
      g_message (_("Script-Fu evaluation mode only allows "
                   "non-interactive invocation"));
      break;

    default:
      break;
    }

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}
