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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#if HAVE_DIRENT_H
#include <dirent.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>

#include "gdk/gdkkeysyms.h"
#include "gtk/gtk.h"

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "script-fu-intl.h"

#include "siod.h"
#include "script-fu-console.h"
#include <plug-ins/dbbrowser/dbbrowser.h>

#ifdef G_OS_WIN32
#include <fcntl.h>
#include <io.h>
#endif

#ifndef G_OS_WIN32

#define TEXT_WIDTH  400
#define TEXT_HEIGHT 400
#define ENTRY_WIDTH 400

#define BUFSIZE 256

typedef struct
{
  GtkWidget *console;
  GtkWidget *cc;
  GtkAdjustment *vadj;

  GdkFont   *font_strong;
  GdkFont   *font_emphasis;
  GdkFont   *font_weak;
  GdkFont   *font;

  gint32     input_id;
} ConsoleInterface;

/*
 *  Local Functions
 */

static void  script_fu_console_interface (void);
static void  script_fu_close_callback    (GtkWidget        *widget,
					  gpointer          data);
static void  script_fu_browse_callback    (GtkWidget        *widget,
					  gpointer          data);
static void  script_fu_siod_read         (gpointer          data,
					  gint              id,
					  GdkInputCondition cond);
static gint  script_fu_cc_is_empty       (void);
static gint  script_fu_cc_key_function   (GtkWidget         *widget,
					  GdkEventKey       *event,
					  gpointer           data);

static FILE *script_fu_open_siod_console (void);
static void  script_fu_close_siod_console(void);

/*
 *  Local variables
 */

static ConsoleInterface cint =
{
  NULL,  /*  console  */
  NULL,  /*  current command  */
  NULL,  /*  vertical adjustment  */

  NULL,  /*  strong font  */
  NULL,  /*  emphasis font  */
  NULL,  /*  weak font  */
  NULL,  /*  normal font  */

  -1     /*  input id  */
};

static char   read_buffer[BUFSIZE];
static GList *history = NULL;
static int    history_len = 0;
static int    history_cur = 0;
static int    history_max = 50;

static int   siod_output_pipe[2];
extern int   siod_verbose_level;
extern char  siod_err_msg[];
extern FILE *siod_output;


#define message(string) printf("(%s): %d ::: %s\n", __PRETTY_FUNCTION__, __LINE__, string)

/*
 *  Function definitions
 */

void
script_fu_console_run (char     *name,
		       int       nparams,
		       GParam   *params,
		       int      *nreturn_vals,
		       GParam  **return_vals)
{
  static GParam values[1];
  GStatusType status = STATUS_SUCCESS;
  GRunModeType run_mode;

  run_mode = params[0].data.d_int32;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /*  Enable SIOD output  */
      script_fu_open_siod_console ();

      /*  Run the interface  */
      script_fu_console_interface ();

      /*  Clean up  */
      script_fu_close_siod_console ();
      break;

    case RUN_WITH_LAST_VALS:
    case RUN_NONINTERACTIVE:
      status = STATUS_CALLING_ERROR;
      gimp_message (_("Script-Fu console mode allows only interactive invocation"));
      break;

    default:
      break;
    }

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}

static void
script_fu_console_interface (void)
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *vsb;
  GtkWidget *table;
  GtkWidget *hbox;
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("script-fu");

  INIT_I18N_UI();

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), _("Script-Fu Console"));
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) script_fu_close_callback,
		      NULL);
  gtk_signal_connect (GTK_OBJECT (dlg),
		      "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroyed),
		      &dlg);
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), 2);
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dlg)->action_area), 2);

  /*  Action area  */
  button = gtk_button_new_with_label (_("Close"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) script_fu_close_callback,
                      NULL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /*  The info vbox  */
  label = gtk_label_new (_("SIOD Output"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), label, FALSE, TRUE, 0);
  gtk_widget_show (label);

  /*  The output text widget  */
  cint.vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  vsb = gtk_vscrollbar_new (cint.vadj);
  cint.console = gtk_text_new (NULL, cint.vadj);
  gtk_text_set_editable (GTK_TEXT (cint.console), FALSE);
  gtk_widget_set_usize (cint.console, TEXT_WIDTH, TEXT_HEIGHT);

  table  = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);

  gtk_table_attach (GTK_TABLE (table), vsb, 1, 2, 0, 1,
		    0, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), cint.console, 0, 1, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  gtk_container_border_width (GTK_CONTAINER (table), 2);

  cint.font_strong = gdk_font_load ("-*-helvetica-bold-r-normal-*-*-120-*-*-*-*-*-*");
  cint.font_emphasis = gdk_font_load ("-*-helvetica-medium-o-normal-*-*-100-*-*-*-*-*-*");
  cint.font_weak = gdk_font_load ("-*-helvetica-medium-r-normal-*-*-100-*-*-*-*-*-*");
  cint.font = gdk_font_load ("-*-*-medium-r-normal-*-*-100-*-*-c-*-*-*");

  /*  Realize the widget before allowing new text to be inserted  */
  gtk_widget_realize (cint.console);

  gtk_text_insert (GTK_TEXT (cint.console), cint.font_strong, NULL, NULL,
		   "The GIMP - GNU Image Manipulation Program\n\n", -1);
  gtk_text_insert (GTK_TEXT (cint.console), cint.font_emphasis, NULL, NULL,
		   "Copyright (C) 1995 Spencer Kimball and Peter Mattis\n", -1);
  gtk_text_insert (GTK_TEXT (cint.console), cint.font_weak, NULL, NULL,
		   "\n", -1);
  gtk_text_insert (GTK_TEXT (cint.console), cint.font_weak, NULL, NULL,
		   "This program is free software; you can redistribute it and/or modify\n", -1);
  gtk_text_insert (GTK_TEXT (cint.console), cint.font_weak, NULL, NULL,
		   "it under the terms of the GNU General Public License as published by\n", -1);
  gtk_text_insert (GTK_TEXT (cint.console), cint.font_weak, NULL, NULL,
		   "the Free Software Foundation; either version 2 of the License, or\n", -1);
  gtk_text_insert (GTK_TEXT (cint.console), cint.font_weak, NULL, NULL,
		   "(at your option) any later version.\n", -1);
  gtk_text_insert (GTK_TEXT (cint.console), cint.font_weak, NULL, NULL,
		   "\n", -1);
  gtk_text_insert (GTK_TEXT (cint.console), cint.font_weak, NULL, NULL,
		   "This program is distributed in the hope that it will be useful,\n", -1);
  gtk_text_insert (GTK_TEXT (cint.console), cint.font_weak, NULL, NULL,
		   "but WITHOUT ANY WARRANTY; without even the implied warranty of\n", -1);
  gtk_text_insert (GTK_TEXT (cint.console), cint.font_weak, NULL, NULL,
		   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n", -1);
  gtk_text_insert (GTK_TEXT (cint.console), cint.font_weak, NULL, NULL,
		   "See the GNU General Public License for more details.\n", -1);
  gtk_text_insert (GTK_TEXT (cint.console), cint.font_weak, NULL, NULL,
		   "\n", -1);
  gtk_text_insert (GTK_TEXT (cint.console), cint.font_weak, NULL, NULL,
		   "You should have received a copy of the GNU General Public License\n", -1);
  gtk_text_insert (GTK_TEXT (cint.console), cint.font_weak, NULL, NULL,
		   "along with this program; if not, write to the Free Software\n", -1);
  gtk_text_insert (GTK_TEXT (cint.console), cint.font_weak, NULL, NULL,
		   "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n", -1);
  gtk_text_insert (GTK_TEXT (cint.console), cint.font_weak, NULL, NULL,
		   "\n\n", -1);
  gtk_text_insert (GTK_TEXT (cint.console), cint.font_strong, NULL, NULL,
		   "Script-Fu Console - ", -1);
  gtk_text_insert (GTK_TEXT (cint.console), cint.font_emphasis, NULL, NULL,
		   "Interactive Scheme Development\n\n", -1);

  gtk_widget_show (vsb);
  gtk_widget_show (cint.console);
  gtk_widget_show (table);

  /*  The current command  */
  label = gtk_label_new (_("Current Command"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), label, FALSE, TRUE, 0);
  gtk_widget_show (label);

  hbox = gtk_hbox_new ( FALSE, 0 );
  gtk_widget_set_usize (hbox, ENTRY_WIDTH, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, FALSE, TRUE, 0);
  gtk_widget_show (hbox);
    
  cint.cc = gtk_entry_new ();
  
  gtk_box_pack_start (GTK_BOX (hbox), cint.cc, 
		      TRUE, TRUE, 0);
/*    gtk_widget_set_usize (cint.cc, (ENTRY_WIDTH*5)/6, 0);  */
  GTK_WIDGET_SET_FLAGS (cint.cc, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (cint.cc);
  gtk_signal_connect (GTK_OBJECT (cint.cc), "key_press_event",
		      (GtkSignalFunc) script_fu_cc_key_function,
		      NULL);

  button = gtk_button_new_with_label (_("Browse..."));
/*    gtk_widget_set_usize (button, (ENTRY_WIDTH)/6, 0); */
  gtk_box_pack_start (GTK_BOX (hbox), button, 
		      FALSE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) script_fu_browse_callback,
		      NULL);
  gtk_widget_show (button);
  gtk_widget_show (cint.cc);

  cint.input_id = gdk_input_add (siod_output_pipe[0],
				 GDK_INPUT_READ,
				 script_fu_siod_read,
				 NULL);

  /*  Initialize the history  */
  history = g_list_append (history, NULL);
  history_len = 1;

  gtk_widget_show (dlg);

  gtk_main ();

  gdk_input_remove (cint.input_id);
  if (dlg)
    gtk_widget_destroy (dlg);
  gdk_flush ();
}

static void
script_fu_close_callback (GtkWidget *widget,
			  gpointer   data)
{
  gtk_main_quit ();
}

void 
apply_callback (gchar     *proc_name,
		gchar     *scheme_proc_name,
		gchar     *proc_blurb,
		gchar     *proc_help,
		gchar     *proc_author,
		gchar     *proc_copyright,
		gchar     *proc_date,
		int        proc_type,
		int        nparams,
		int        nreturn_vals,
		GParamDef *params,
		GParamDef *return_vals )
{
  gint i;
  GString *text;

  if (proc_name == NULL) 
    return;
  
  text = g_string_new ("(");
  text = g_string_append (text, scheme_proc_name);
  for (i=0; i<nparams; i++) 
    {
      text = g_string_append_c (text, ' ');
      text = g_string_append (text, params[i].name);
    }
  text = g_string_append_c (text, ')');

  gtk_entry_set_text (GTK_ENTRY (cint.cc), text->str);
  g_string_free (text, TRUE);
}

static void
script_fu_browse_callback (GtkWidget *widget,
			   gpointer   data)
{
  gtk_quit_add_destroy (1, (GtkObject*) gimp_db_browser (apply_callback));
}

static gint
script_fu_console_scroll_end (gpointer data)
{
  /* The Text widget in 1.0.1 doesn't like being scrolled before
   * it is size-allocated, so we wait for it
   */
  if ((cint.console->allocation.width > 1) && 
      (cint.console->allocation.height > 1))
    {
      cint.vadj->value = cint.vadj->upper - cint.vadj->page_size;
      gtk_signal_emit_by_name (GTK_OBJECT (cint.vadj), "changed");
    }
  else
    gtk_idle_add (script_fu_console_scroll_end, NULL);
  
  return FALSE;
}

static void
script_fu_siod_read (gpointer          data,
		     gint              id,
		     GdkInputCondition cond)
{
  int count;
  static int hack = 0;

  if ((count = read (id, read_buffer, BUFSIZE - 1)) != 0)
    {
      if (!hack) /* this is a stupid hack, but as of 10/27/98
		 * the script-fu-console will hang on my system without it.
		 * the real cause of this needs to be tracked down.
		 * posibly a problem with the text widget, or the
		 * signal handlers not getting fully initialized or...
		 * no reports of hangs on other platforms, could just be a
		 * problem with my system.
		 */
      {
	hack = 1;
	return;
      }
      read_buffer[count] = '\0';
      gtk_text_freeze (GTK_TEXT (cint.console));
      gtk_text_insert (GTK_TEXT (cint.console), cint.font_weak, NULL, NULL,
		       read_buffer, -1);
      gtk_text_thaw (GTK_TEXT (cint.console));

      script_fu_console_scroll_end (NULL);
    }
}

static gint
script_fu_cc_is_empty (void)
{
  char *str;

  if ((str = gtk_entry_get_text (GTK_ENTRY (cint.cc))) == NULL)
    return TRUE;

  while (*str)
    {
      if (*str != ' ' && *str != '\t' && *str != '\n')
	return FALSE;

      str ++;
    }

  return TRUE;
}

static gint
script_fu_cc_key_function (GtkWidget   *widget,
			   GdkEventKey *event,
			   gpointer     data)
{
  GList *list;
  int direction = 0;

  switch (event->keyval)
    {
    case GDK_Return:
      gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "key_press_event");

      if (script_fu_cc_is_empty ())
	return TRUE;

      list = g_list_nth (history, (g_list_length (history) - 1));
      if (list->data)
	g_free (list->data);
      list->data = g_strdup (gtk_entry_get_text (GTK_ENTRY (cint.cc)));

      gtk_text_freeze (GTK_TEXT (cint.console));
      gtk_text_insert (GTK_TEXT (cint.console), cint.font_strong, NULL, NULL, "=> ", -1);
      gtk_text_insert (GTK_TEXT (cint.console), cint.font, NULL, NULL,
		       gtk_entry_get_text (GTK_ENTRY (cint.cc)), -1);
      gtk_text_insert (GTK_TEXT (cint.console), cint.font, NULL, NULL, "\n\n", -1);
      gtk_text_thaw (GTK_TEXT (cint.console));

      cint.vadj->value = cint.vadj->upper - cint.vadj->page_size;
      gtk_signal_emit_by_name (GTK_OBJECT (cint.vadj), "changed");

      gtk_entry_set_text (GTK_ENTRY (cint.cc), "");
      gdk_flush ();

      repl_c_string ((char *) list->data, 0, 0, 1);
      gimp_displays_flush ();

      history = g_list_append (history, NULL);
      if (history_len == history_max)
	{
	  history = g_list_remove (history, history->data);
	  if (history->data)
	    g_free (history->data);
	}
      else
	history_len++;
      history_cur = g_list_length (history) - 1;

      return TRUE;
      break;

    case GDK_KP_Up:
    case GDK_Up:
      gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "key_press_event");
      direction = -1;
      break;

    case GDK_KP_Down:
    case GDK_Down:
      gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "key_press_event");
      direction = 1;
      break;

    case GDK_P:
    case GDK_p:
      if (event->state & GDK_CONTROL_MASK)
	{
	  gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "key_press_event");
	  direction = -1;
	}
      break;

    case GDK_N:
    case GDK_n:
      if (event->state & GDK_CONTROL_MASK)
	{
	  gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "key_press_event");
	  direction = 1;
	}
      break;

    default:
      break;
    }

  if (direction)
    {
      /*  Make sure we keep track of the current one  */
      if (history_cur == g_list_length (history) - 1)
	{
	  list = g_list_nth (history, history_cur);
	  if (list->data)
	    g_free (list->data);
	  list->data = g_strdup (gtk_entry_get_text (GTK_ENTRY (cint.cc)));
	}

      history_cur += direction;
      if (history_cur < 0)
	history_cur = 0;
      if (history_cur >= history_len)
	history_cur = history_len - 1;

      gtk_entry_set_text (GTK_ENTRY (cint.cc), (char *) (g_list_nth (history, history_cur))->data);

      return TRUE;
    }

  return FALSE;
}


static FILE *
script_fu_open_siod_console (void)
{
  if (siod_output == stdout)
    {
      if (pipe (siod_output_pipe))
	{
	  gimp_message (_("Unable to open SIOD output pipe"));
	}
      else if ((siod_output = fdopen (siod_output_pipe [1], "w")) == NULL)
	{
	  gimp_message (_("Unable to open a stream on the SIOD output pipe"));
	  siod_output = stdout;
	}
      else
	{
	  siod_verbose_level = 2;
	  print_welcome ();
	}
    }

  return siod_output;
}

static void
script_fu_close_siod_console (void)
{
  if (siod_output != stdout)
    fclose (siod_output);
  close (siod_output_pipe[0]);
  close (siod_output_pipe[1]);
}

#endif /* !G_OS_WIN32 */

void
script_fu_eval_run (char     *name,
		    int       nparams,
		    GParam   *params,
		    int      *nreturn_vals,
		    GParam  **return_vals)
{
  static GParam values[1];
  GStatusType status = STATUS_SUCCESS;
  GRunModeType run_mode;

  run_mode = params[0].data.d_int32;

  switch (run_mode)
    {
    case RUN_NONINTERACTIVE:
      if (repl_c_string (params[1].data.d_string, 0, 0, 1) != 0)
	status = STATUS_EXECUTION_ERROR;
      break;

    case RUN_INTERACTIVE:
    case RUN_WITH_LAST_VALS:
      status = STATUS_CALLING_ERROR;
      gimp_message (_("Script-Fu evaluate mode allows only noninteractive invocation"));
      break;

    default:
      break;
    }

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}
