/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help Browser
 * Copyright (C) 1999-2002 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *
 * Some code & ideas stolen from the GNOME help browser.
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

#include <string.h> 
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <libgtkhtml/gtkhtml.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "queue.h"
#include "uri.h"

#include "libgimp/stdplugins-intl.h"


/*  defines  */

#ifdef __EMX__
#define chdir _chdir2
#endif

#define GIMP_HELP_EXT_NAME      "extension_gimp_help_browser"
#define GIMP_HELP_TEMP_EXT_NAME "extension_gimp_help_browser_temp"

#define GIMP_HELP_PREFIX        "help"

enum
{
  BUTTON_HOME,
  BUTTON_INDEX,
  BUTTON_BACK,
  BUTTON_FORWARD
};


typedef struct
{
  const gchar *title;
  const gchar *ref;
  gint         count;
} HistoryItem;


static const gchar *eek_png_tag = "<h1>Eeek!</h1>";

static gchar       *gimp_help_root = NULL;

static GList       *history = NULL;
static Queue       *queue;
static gchar       *current_ref;

static GtkWidget   *html;
static GtkWidget   *back_button;
static GtkWidget   *forward_button;
static GtkWidget   *combo;

static GtkTargetEntry help_dnd_target_table[] =
{
  { "_NETSCAPE_URL", 0, 0 },
};


/*  GIMP plugin stuff  */

static void query (void);
static void run   (gchar      *name,
		   gint        nparams,
		   GimpParam  *param,
		   gint       *nreturn_vals,
		   GimpParam **return_vals);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static gboolean temp_proc_installed = FALSE;

/*  forward declaration  */

static void     load_page   (const gchar  *ref,
                             gboolean      add_to_queue);
static void     request_url (HtmlDocument *doc,
                             const gchar  *url,
                             HtmlStream   *stream,
                             gpointer      data);
static gboolean io_handler  (GIOChannel   *io, 
                             GIOCondition  condition, 
                             gpointer      data);


/* Taken from glib/gconvert.c:
 * Test of haystack has the needle prefix, comparing case
 * insensitive. haystack may be UTF-8, but needle must
 * contain only ascii.
 */
static gboolean
has_case_prefix (const gchar *haystack, const gchar *needle)
{
  const gchar *h = haystack;
  const gchar *n = needle;

  while (*n && *h && g_ascii_tolower (*n) == g_ascii_tolower (*h))
    {
      n++;
      h++;
    }
  
  return (*n == '\0');
}

static void
close_callback (GtkWidget *widget,
		gpointer   user_data)
{
  gtk_main_quit ();
}

static void
update_toolbar (void)
{
  if (back_button)
    gtk_widget_set_sensitive (back_button, queue_has_prev (queue));
  if (forward_button)
    gtk_widget_set_sensitive (forward_button, queue_has_next (queue));
}

static void
button_callback (GtkWidget *widget,
                 gpointer   data)
{
  const gchar *ref;

  switch (GPOINTER_TO_INT (data))
    {
    case BUTTON_HOME:
      load_page ("contents.html", TRUE);
      break;

    case BUTTON_INDEX:
      load_page ("index.html", TRUE);
      break;

    case BUTTON_BACK:
      if (!(ref = queue_prev (queue)))
        return;
      load_page (ref, FALSE);
      queue_move_prev (queue);
      break;

    case BUTTON_FORWARD:
      if (!(ref = queue_next (queue)))
        return;
      load_page (ref, FALSE);
      queue_move_next (queue);
      break;

    default:
      return;
    }

  update_toolbar ();
}

static void 
entry_changed_callback (GtkWidget *widget,
			gpointer   data)
{
  GList       *list;
  HistoryItem *item;
  const gchar *entry_text;
  gchar       *compare_text;
  gboolean     found = FALSE;

  entry_text = gtk_entry_get_text (GTK_ENTRY (widget));

  for (list = history; list && !found; list = list->next)
    {
      item = (HistoryItem *) list->data;

      if (item->count)
        {
          compare_text = g_strdup_printf ("%s <%i>",
                                          item->title, item->count + 1);
        }
      else
        {
          compare_text = (gchar *) item->title;
        }

      if (strcmp (compare_text, entry_text) == 0)
	{
	  load_page (item->ref, TRUE);
	  found = TRUE;
	}

      if (item->count)
        {
          g_free (compare_text);
        }
    }
}

static void
history_add (const gchar *ref,
	     const gchar *title)
{
  GList       *list;
  GList       *found = NULL;
  HistoryItem *item;
  GList       *combo_list = NULL;
  gint         title_found_count = 0;

  for (list = history; list && !found; list = list->next)
    {
      item = (HistoryItem *) list->data;

      if (strcmp (item->title, title) == 0)
	{
	  if (strcmp (item->ref, ref) != 0)
	    {
	      title_found_count++;
	      continue;
	    }

	  found = list;
        }
    }

  if (found)
    {
      item = (HistoryItem *) found->data;
      history = g_list_remove_link (history, found);
    }
  else
    {
      item = g_new (HistoryItem, 1);
      item->ref   = g_strdup (ref);
      item->title = g_strdup (title);
      item->count = title_found_count;
    }

  history = g_list_prepend (history, item);

  for (list = history; list; list = list->next)
    {
      gchar* combo_title;

      item = (HistoryItem *) list->data;

      if (item->count)
	combo_title = g_strdup_printf ("%s <%i>",
				       item->title,
				       item->count + 1);
      else
	combo_title = g_strdup (item->title);

      combo_list = g_list_prepend (combo_list, combo_title);
    }

  combo_list = g_list_reverse (combo_list);

  g_signal_handlers_block_by_func (GTK_COMBO (combo)->entry,
                                   entry_changed_callback, combo);
  gtk_combo_set_popdown_strings (GTK_COMBO (combo), combo_list);
  g_signal_handlers_unblock_by_func (GTK_COMBO (combo)->entry,
                                     entry_changed_callback, combo);

  for (list = combo_list; list; list = list->next)
    g_free (list->data);

  g_list_free (combo_list);
}

static void
title_changed (HtmlDocument *doc,
               const gchar  *new_title,
               gpointer      data)
{
  gchar *title;

  if (!new_title)
    new_title = (_("<Untitled>"));
      
  title = g_strstrip (g_strdup (new_title));

  history_add (current_ref, title);

  g_signal_handlers_block_by_func (GTK_COMBO (combo)->entry,
                                   entry_changed_callback, combo);
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry), title);
  g_signal_handlers_unblock_by_func (GTK_COMBO (combo)->entry,
                                     entry_changed_callback, combo);

  g_free (title);
}

static void
load_remote_page (const gchar *ref)
{
  GimpParam *return_vals;
  gint       nreturn_vals;

  /*  try to call netscape through the web_browser interface */
  return_vals = gimp_run_procedure ("extension_web_browser",
                                    &nreturn_vals,
                                    GIMP_PDB_INT32,  GIMP_RUN_NONINTERACTIVE,
                                    GIMP_PDB_STRING, ref,
                                    GIMP_PDB_INT32,  FALSE,
                                    GIMP_PDB_END);
  gimp_destroy_params (return_vals, nreturn_vals);
}

static void
load_page (const gchar *ref,	  
	   gboolean     add_to_queue)
{
  HtmlDocument *doc = HTML_VIEW (html)->document;
  gchar        *abs;
  gchar        *new_ref;
  gchar        *anchor;

  g_return_if_fail (ref != NULL);

  abs = uri_to_abs (ref, current_ref);

  g_return_if_fail (abs != NULL);

  anchor = strchr (ref, '#');
  if (anchor && anchor[0] && anchor[1])
    {
      new_ref = g_strconcat (abs, anchor, NULL);
      anchor += 1;
    }
  else
    {
      new_ref = g_strdup (abs);
      anchor = NULL;
    }

  if (strcmp (current_ref, abs))
    {
      if (!has_case_prefix (abs, "file:/"))
        {
          load_remote_page (ref);
          return;
        }

      html_document_clear (doc);
      html_document_open_stream (doc, "text/html");
      gtk_adjustment_set_value (gtk_layout_get_vadjustment (GTK_LAYOUT (html)),
                                0);
      
      request_url (doc, abs, doc->current_stream, NULL);
    }

  if (anchor)
    html_view_jump_to_anchor (HTML_VIEW (html), anchor);

  g_free (current_ref);
  current_ref = new_ref;

  if (add_to_queue) 
    queue_add (queue, new_ref);
  
  update_toolbar ();
}

static void
link_clicked (HtmlDocument *doc,
              const gchar  *url,
              gpointer      data)
{
  load_page (url, TRUE);
}

static void
request_url (HtmlDocument *doc,
             const gchar  *url,
             HtmlStream   *stream,
             gpointer      data)
{
  gchar *abs;
  gchar *filename;

  g_return_if_fail (url != NULL);
  g_return_if_fail (stream != NULL);

  abs = uri_to_abs (url, current_ref);
  if (!abs)
    return;

  filename = g_filename_from_uri (abs, NULL, NULL);

  if (filename)
    {
      gint fd;

      fd = open (filename, O_RDONLY);

      if (fd == -1)
        {
          gchar *name;
          gchar *msg;

          name = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);

          msg = g_strdup_printf
            ("<html>"
             "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />"
             "<head><title>%s</title></head>"
             "<body bgcolor=\"white\">"
             "<div align=\"center\">"
             "<div>%s</div>"
             "<h3>%s</h3>"
             "<tt>%s</tt>"
             "</div>"
             "<br /><br />"
             "<div align=\"justify\">%s</div>"
             "</body>"
             "</html>",
             _("Document Not Found"),
             eek_png_tag,
             _("Could not locate help documentation"),
             name,
             _("The requested document could not be found in your GIMP help "
               "path as shown above. This means that the topic has not yet "
               "been written or your installation is not complete. Ensure "
               "that your installation is complete before reporting this "
               "error as a bug."));
          
          html_document_write_stream (doc, msg, strlen (msg));

          g_free (msg);
          g_free (name);
        }
      else
        {
          GIOChannel *io = g_io_channel_unix_new (fd);

          g_io_channel_set_close_on_unref (io, TRUE);
          g_io_channel_set_encoding (io, NULL, NULL);

          g_io_add_watch (io, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, 
                          io_handler, stream);
        }

      g_free (filename);
    }

  g_free (abs);
}

static gboolean
io_handler (GIOChannel   *io,
            GIOCondition  condition, 
            gpointer      data)
{
  HtmlStream *stream;
  gchar       buffer[8192];
  guint       bytes;

  stream = (HtmlStream *) data;

  if (condition & G_IO_IN) 
    {
      if (g_io_channel_read_chars (io, buffer, sizeof (buffer),
                                   &bytes, NULL) != G_IO_STATUS_ERROR
          && bytes > 0)
        {
          html_stream_write (stream, buffer, bytes);
        }
      else
	{
          html_stream_close (stream);
          g_io_channel_unref (io);

	  return FALSE;
	}

      if (condition & G_IO_HUP) 
        {
          while (g_io_channel_read_chars (io, buffer, sizeof (buffer),
                                          &bytes, NULL) != G_IO_STATUS_ERROR
                 && bytes > 0)
            {
              html_stream_write (stream, buffer, bytes);
            }
        }
    }

  if (condition & (G_IO_ERR | G_IO_HUP | G_IO_NVAL)) 
    {
      html_stream_close (stream);
      g_io_channel_unref (io);

      return FALSE;
    }

  return TRUE;
}

static void
drag_begin (GtkWidget      *widget,
            GdkDragContext *context,
            gpointer        data)
{
  gtk_drag_set_icon_stock (context, GTK_STOCK_JUMP_TO, -8, -8);
}

static void
drag_data_get (GtkWidget        *widget, 
               GdkDragContext   *context,
               GtkSelectionData *selection_data,
               guint             info,
               guint             time,
               gpointer          data)
{
  if (! current_ref)
    return;

  gtk_selection_data_set (selection_data,
                          selection_data->target,
                          8, 
                          current_ref, 
                          strlen (current_ref));
}

static void
open_browser_dialog (const gchar *help_path,
		     const gchar *locale,
		     const gchar *help_file)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *bbox;
  GtkWidget *scroll;
  GtkWidget *button;
  GtkWidget *drag_source;
  GtkWidget *image;
  gchar     *eek_png_path;

  gimp_ui_init ("helpbrowser", TRUE);

  eek_png_path = g_build_filename (gimp_help_root, "images", "eek.png", NULL);

  if (g_file_test (eek_png_path, G_FILE_TEST_EXISTS))
    eek_png_tag = g_strdup_printf ("<img src=\"%s\">", eek_png_path);

  g_free (eek_png_path);

  /*  the dialog window  */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy",
                    G_CALLBACK (close_callback),
                    NULL);
  gtk_window_set_wmclass (GTK_WINDOW (window), "helpbrowser", "Gimp");
  gtk_window_set_title (GTK_WINDOW (window), _("GIMP Help Browser"));

  gimp_help_connect (window, gimp_standard_help_func, "dialogs/help.html");

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  /*  buttons  */
  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_START);
  gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 0);
  gtk_widget_show (bbox);

  button = gtk_button_new_from_stock (GTK_STOCK_HOME);
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (button_callback),
                    GINT_TO_POINTER (BUTTON_HOME));

  button = gtk_button_new_from_stock (GTK_STOCK_INDEX);
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_show (button);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (button_callback),
                    GINT_TO_POINTER (BUTTON_INDEX));

  back_button = button = gtk_button_new_from_stock (GTK_STOCK_GO_BACK);
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (button_callback),
                    GINT_TO_POINTER (BUTTON_BACK));
  gtk_widget_show (button);

  forward_button = button = gtk_button_new_from_stock (GTK_STOCK_GO_FORWARD);
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (button_callback),
                    GINT_TO_POINTER (BUTTON_FORWARD));
  gtk_widget_show (button);

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  the drag source  */
  drag_source = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (hbox), drag_source, FALSE, FALSE, 4);
  gtk_widget_show (drag_source);

  gtk_drag_source_set (GTK_WIDGET (drag_source),
                       GDK_BUTTON1_MASK,
                       help_dnd_target_table,
                       G_N_ELEMENTS (help_dnd_target_table), 
                       GDK_ACTION_MOVE | GDK_ACTION_COPY);
  g_signal_connect (drag_source, "drag_begin",
                    G_CALLBACK (drag_begin),
                    NULL);
  g_signal_connect (drag_source, "drag_data_get",
                    G_CALLBACK (drag_data_get),
                    NULL);
  
  image = gtk_image_new_from_stock (GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (drag_source), image);
  gtk_widget_show (image);

  /*  the title combo  */
  combo = gtk_combo_new ();
  gtk_widget_set_size_request (GTK_WIDGET (combo), 360, -1);
  gtk_combo_set_use_arrows (GTK_COMBO (combo), TRUE);
  g_object_set (GTK_COMBO (combo)->entry, "editable", FALSE, NULL); 
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  g_signal_connect (GTK_COMBO (combo)->entry, "changed",
                    G_CALLBACK (entry_changed_callback), 
                    combo);


  /*  HTML view  */
  html  = html_view_new ();
  queue = queue_new ();

  gtk_widget_set_size_request (GTK_WIDGET (html), -1, 240);

  scroll = 
    gtk_scrolled_window_new (gtk_layout_get_hadjustment (GTK_LAYOUT (html)),
                             gtk_layout_get_vadjustment (GTK_LAYOUT (html)));
  
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);
  gtk_widget_show (scroll);

  gtk_container_add (GTK_CONTAINER (scroll), html);
  gtk_widget_show (html);
  
  html_view_set_document (HTML_VIEW (html), html_document_new ());

  g_signal_connect (HTML_VIEW (html)->document, "title_changed",
                    G_CALLBACK (title_changed),
                    NULL);
  g_signal_connect (HTML_VIEW (html)->document, "link_clicked",
                    G_CALLBACK (link_clicked),
                    NULL);
  g_signal_connect (HTML_VIEW (html)->document, "request_url",
                    G_CALLBACK (request_url),
                    NULL);

  gtk_widget_show (window);

  current_ref = g_strconcat ("file://", help_path, "/", locale, "/", NULL);

  load_page (help_file, TRUE);
}

static gboolean
idle_load_page (gpointer data)
{
  gchar *path = data;

  load_page (path, TRUE);
  g_free (path);

  return FALSE;
}

static void
run_temp_proc (gchar      *name,
	       gint        nparams,
	       GimpParam  *param,
	       gint       *nreturn_vals,
	       GimpParam **return_vals)
{
  static GimpParam  values[1];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gchar *help_path;
  gchar *locale;
  gchar *help_file;
  gchar *path;

  /*  set default values  */
  help_path = g_strdup (gimp_help_root);
  locale    = g_strdup ("C");
  help_file = g_strdup ("introduction.html");

  /*  Make sure all the arguments are there!  */
  if (nparams == 3)
    {
      if (param[0].data.d_string && strlen (param[0].data.d_string))
	{
	  g_free (help_path);
	  help_path = g_strdup (param[0].data.d_string);
	  g_strdelimit (help_path, "/", G_DIR_SEPARATOR);
	}
      if (param[1].data.d_string && strlen (param[1].data.d_string))
	{
	  g_free (locale);
	  locale    = g_strdup (param[1].data.d_string);
	}
      if (param[2].data.d_string && strlen (param[2].data.d_string))
	{
	  g_free (help_file);
	  help_file = g_strdup (param[2].data.d_string);
	  g_strdelimit (help_file, "/", G_DIR_SEPARATOR);
	}
    }

  path = g_build_filename (help_path, locale, help_file, NULL);

  g_free (help_path);
  g_free (locale);
  g_free (help_file);

  g_idle_add (idle_load_page, path);  /* frees path */

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

/*  from libgimp/gimp.c  */
void
gimp_run_temp (void);

static gboolean
input_callback (GIOChannel   *channel,
                GIOCondition  condition,
                gpointer      data)
{
  /* We have some data in the wire - read it */
  /* The below will only ever run a single proc */
  gimp_run_temp ();

  return TRUE;
}

static void
install_temp_proc (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_STRING, "help_path", "" },
    { GIMP_PDB_STRING, "locale",    "Language to use" },
    { GIMP_PDB_STRING, "help_file", "Path of a local document to open. "
                                    "Can be relative to GIMP_HELP_PATH." }
  };

  gimp_install_temp_proc (GIMP_HELP_TEMP_EXT_NAME,
			  "DON'T USE THIS ONE",
			  "(Temporary procedure)",
			  "Sven Neumann <sven@gimp.org>, "
			  "Michael Natterer <mitch@gimp.org>",
			  "Sven Neumann & Michael Natterer",
			  "1999-2002",
			  NULL,
			  "",
			  GIMP_TEMPORARY,
			  G_N_ELEMENTS (args), 0,
			  args, NULL,
			  run_temp_proc);

  /* Tie into the gdk input function */
  g_io_add_watch (_readchannel, G_IO_IN | G_IO_PRI, input_callback, NULL);

  temp_proc_installed = TRUE;
}

static void
open_url (const gchar *help_path,
	  const gchar *locale,
	  const gchar *help_file)
{
  open_browser_dialog (help_path, locale, help_file);

  install_temp_proc ();
  gtk_main ();
}


MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,  "run_mode",  "Interactive" },
    { GIMP_PDB_STRING, "help_path", "" },
    { GIMP_PDB_STRING, "locale",    "Language to use" },
    { GIMP_PDB_STRING, "help_file", "Path of a local document to open. "
                                    "Can be relative to GIMP_HELP_PATH." }
  };

  gimp_install_procedure (GIMP_HELP_EXT_NAME,
                          "Browse the GIMP help pages",
                          "A small and simple HTML browser optimized for "
			  "browsing the GIMP help pages.",
                          "Sven Neumann <sven@gimp.org>, "
			  "Michael Natterer <mitch@gimp.org>",
			  "Sven Neumann & Michael Natterer",
                          "1999-2002",
                          NULL,
                          "",
                          GIMP_EXTENSION,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);
}

static void
run (gchar      *name,
     gint        nparams,
     GimpParam  *param,
     gint       *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam  values[1];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  const gchar *env_root_dir = NULL;
  gchar       *help_path    = NULL;
  gchar       *locale       = NULL;
  gchar       *help_file    = NULL;

  run_mode = param[0].data.d_int32;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals = values;

  INIT_I18N ();

  if (strcmp (name, GIMP_HELP_EXT_NAME) == 0)
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_NONINTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
	  /*  set default values  */
	  env_root_dir = g_getenv ("GIMP_HELP_ROOT");

	  if (env_root_dir)
	    {
	      if (!g_file_test (env_root_dir, G_FILE_TEST_IS_DIR))
		{
		  g_message (_("GIMP Help Browser Error.\n\n"
			       "Couldn't find GIMP_HELP_ROOT html directory.\n"
			       "(%s)"), env_root_dir);

		  status = GIMP_PDB_EXECUTION_ERROR;
		  break;
		}

	      gimp_help_root = g_strdup (env_root_dir);
	    }
	  else
	    {
	      gimp_help_root = g_build_filename (gimp_data_directory (),
                                                 GIMP_HELP_PREFIX, NULL);
	    }

	  help_path = g_strdup (gimp_help_root);
	  locale    = g_strdup ("C");
	  help_file = g_strdup ("introduction.html");
	  
	  /*  Make sure all the arguments are there!  */
	  if (nparams == 4)
	    {
	      if (param[1].data.d_string && strlen (param[1].data.d_string))
		{
		  g_free (help_path);
		  help_path = g_strdup (param[1].data.d_string);
		}
	      if (param[2].data.d_string && strlen (param[2].data.d_string))
		{
		  g_free (locale);
		  locale = g_strdup (param[2].data.d_string);
		}
	      if (param[3].data.d_string && strlen (param[3].data.d_string))
		{
		  g_free (help_file);
		  help_file = g_strdup (param[3].data.d_string);
		}
	    }
          break;

        default:
	  status = GIMP_PDB_CALLING_ERROR;
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
       	  open_url (help_path, locale, help_file);

	  g_free (help_path);
	  g_free (locale);
	  g_free (help_file);
        }

      values[0].data.d_status = status;
    }
  else
    values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
}
