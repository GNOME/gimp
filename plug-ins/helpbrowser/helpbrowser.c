/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help Browser
 * Copyright (C) 1999 Sven Neumann <sven@gimp.org>
 *                    Michael Natterer <mitschel@cs.tu-berlin.de>
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

#include <string.h> 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gtk-xmhtml/gtk-xmhtml.h>
#include <gdk/gdkkeysyms.h>
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"
#include "libgimp/stdplugins-intl.h"

#include "queue.h"

/*  pixmaps  */
#include "back.xpm"
#include "forward.xpm"

/*  defines  */

#ifdef __EMX__
#define chdir _chdir2
#endif

#define GIMP_HELP_EXT_NAME      "extension_gimp_help_browser"
#define GIMP_HELP_TEMP_EXT_NAME "extension_gimp_help_browser_temp"

#define GIMP_HELP_PREFIX        "help"

enum {
  CONTENTS,
  INDEX,
  HELP
};

enum {
  URL_UNKNOWN,
  URL_NAMED, /* ??? */
  URL_JUMP,
  URL_FILE_LOCAL,

  /* aliases */
  URL_LAST = URL_FILE_LOCAL
};

/*  structures  */

typedef struct 
{
  gint       index;
  gchar     *label;
  Queue     *queue;
  gchar     *current_ref;
  GtkWidget *html;
  gchar     *home;
} HelpPage;

typedef struct
{
  gchar *title;
  gchar *ref;
  gint   count;
} HistoryItem;

/*  constant strings  */

static char *doc_not_found_format_string =
_("<html><head><title>Document not found</title></head>"
  "<body bgcolor=\"#ffffff\">"
  "<center>"
  "<p>"
  "%s"
  "<h3>Couldn't find document</h3>"
  "<tt>%s</tt>"
  "</center>"
  "<p>"
  "<small>This either means that the help for this topic has not been written "
  "yet or that something is wrong with your installation. "
  "Please check carefully before you report this as a bug.</small>" 
  "</body>"
  "</html>");

static char *dir_not_found_format_string =
_("<html><head><title>Directory not found</title></head>"
  "<body bgcolor=\"#ffffff\">"
  "<center>"
  "<p>"
  "%s"
  "<h3>Couldn't change to directory</h3>"
  "<tt>%s</tt>"
  "<h3>while trying to access</h3>"
  "<tt>%s</tt>"
  "</center>"
  "<p>"
  "<small>This either means that the help for this topic has not been written "
  "yet or that something is wrong with your installation. "
  "Please check carefully before you report this as a bug.</small>" 
  "</body>"
  "</html>");

static gchar *eek_png_tag = "<h1>Eeek!</h1>";


/*  the three help notebook pages  */

static HelpPage pages[] =
{
  {
    CONTENTS,
    N_("Contents"),
    NULL,
    NULL,
    NULL,
    "contents.html"
  },

  {
    INDEX,
    N_("Index"),
    NULL,
    NULL,
    NULL,
    "index.html"
  },

  {
    HELP,
    NULL,
    NULL,
    NULL,
    NULL,
    "welcome.html"
  }
};

static HelpPage  *current_page = &pages[HELP];
static GList     *history = NULL;

static GtkWidget *back_button;
static GtkWidget *forward_button;
static GtkWidget *notebook;
static GtkWidget *combo;

static GtkTargetEntry help_dnd_target_table[] =
{
  { "_NETSCAPE_URL", 0, 0 },
};
static guint n_help_dnd_targets = (sizeof (help_dnd_target_table) /
				   sizeof (help_dnd_target_table[0]));

/*  GIMP plugin stuff  */

static void query (void);
static void run   (char *name, int nparams, GParam *param,
		   int *nreturn_vals, GParam **return_vals);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,   /*  init_proc   */
  NULL,   /*  quit_proc   */
  query,  /*  query_proc  */
  run,    /*  run_proc    */
};

static gboolean temp_proc_installed = FALSE;

/*  forward declaration  */

static gint load_page (HelpPage *source_page, HelpPage *dest_page,
		       gchar *ref, gint pos, 
		       gboolean add_to_queue, gboolean add_to_history);

/*  functions  */

static void
close_callback (GtkWidget *widget,
		gpointer   user_data)
{
  gtk_main_quit ();
}

static void
update_toolbar (HelpPage *page)
{
  if (back_button)
    gtk_widget_set_sensitive (back_button, queue_isprev (page->queue));
  if (forward_button)
    gtk_widget_set_sensitive (forward_button, queue_isnext (page->queue));
}

static void
jump_to_anchor (HelpPage *page, 
		gchar    *anchor)
{
  gint pos;

  g_return_if_fail (page != NULL && anchor != NULL);

  if (*anchor != '#') 
    {
      gchar *a = g_strconcat ("#", anchor, NULL);
      XmHTMLAnchorScrollToName (page->html, a);
      g_free (a);
    }
  else
    XmHTMLAnchorScrollToName (page->html, anchor);

  pos = gtk_xmhtml_get_topline (GTK_XMHTML (page->html));
  queue_add (page->queue, page->current_ref, pos);

  update_toolbar (page);
}

static void
forward_callback (GtkWidget *widget,
		  gpointer   data)
{
  gchar *ref;
  gint pos;

  if (!(ref = queue_next (current_page->queue, &pos)))
    return;

  load_page (current_page, current_page, ref, pos, FALSE, FALSE);
  queue_move_next (current_page->queue);

  update_toolbar (current_page);
}

static void
back_callback (GtkWidget *widget,
	       gpointer   data)
{
  gchar *ref;
  gint pos;

  if (!(ref = queue_prev (current_page->queue, &pos)))
    return;

  load_page (current_page, current_page, ref, pos, FALSE, FALSE);
  queue_move_prev (current_page->queue);

  update_toolbar (current_page);
}

static void 
entry_changed_callback (GtkWidget *widget,
			gpointer   data)
{
  GList       *list;
  HistoryItem *item;
  gchar       *entry_text;
  gchar       *compare_text;
  gboolean     found = FALSE;

  entry_text = gtk_entry_get_text (GTK_ENTRY (widget));

  for (list = history; list && !found; list = list->next)
    {
      item = (HistoryItem *) list->data;

      if (item->count)
	compare_text = g_strdup_printf ("%s <%i>",
					item->title,
					item->count + 1);
      else
	compare_text = item->title;

      if (strcmp (compare_text, entry_text) == 0)
	{
	  load_page (&pages[HELP], &pages[HELP], item->ref, 0, TRUE, FALSE);
	  found = TRUE;
	}

      if (item->count)
	g_free (compare_text);
    }
}

static void
history_add (gchar *ref,
	     gchar *title)
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
      item->ref = g_strdup (ref);
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

  gtk_signal_handler_block_by_data (GTK_OBJECT (GTK_COMBO (combo)->entry), combo);
  gtk_combo_set_popdown_strings (GTK_COMBO (combo), combo_list);
/*    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry), item->title); */
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (GTK_COMBO (combo)->entry), combo);

  for (list = combo_list; list; list = list->next)
    g_free (list->data);

  g_list_free (combo_list);
}

static void
html_source (HelpPage *page,
	     gchar    *ref,
	     gint      pos,
	     gchar    *source, 
	     gboolean  add_to_queue,
	     gboolean  add_to_history)
{
  gchar *title = NULL;
  
  g_return_if_fail (page != NULL && ref != NULL && source != NULL);
  
  /* Load it up */
  gtk_xmhtml_source (GTK_XMHTML (page->html), source);
  
  gtk_xmhtml_set_topline (GTK_XMHTML(page->html), pos);
  
  if (add_to_queue) 
    queue_add (page->queue, ref, pos);

  if (page->index == HELP)
    {
      title = XmHTMLGetTitle (page->html);
      if (!title)
	title = (_("<Untitled>"));
      
      if (add_to_history)
	history_add (ref, title);

      gtk_signal_handler_block_by_data (GTK_OBJECT (GTK_COMBO (combo)->entry),
					combo);
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry), title);
      gtk_signal_handler_unblock_by_data (GTK_OBJECT (GTK_COMBO (combo)->entry),
					  combo);
    }
      
  update_toolbar (page);
}

static gint
load_page (HelpPage *source_page,
	   HelpPage *dest_page,
	   gchar    *ref,	  
	   gint      pos,
	   gboolean  add_to_queue,
	   gboolean  add_to_history)
{
  GString  *file_contents;
  FILE     *afile = NULL;
  char      aline[1024];
  gchar    *old_dir;
  gchar    *new_dir, *new_base;
  gchar    *new_ref;
  gboolean  page_valid = FALSE;

  g_return_val_if_fail (ref != NULL && source_page != NULL && dest_page != NULL, FALSE);

  old_dir  = g_dirname (source_page->current_ref);
  new_dir  = g_dirname (ref);
  new_base = g_basename (ref);

  /* return value is intentionally ignored */
  chdir (old_dir);

  file_contents = g_string_new (NULL);

  if (chdir (new_dir) == -1)
    {
      if (g_path_is_absolute (ref))
	new_ref = g_strdup (ref);
      else
	new_ref = g_strconcat (old_dir, G_DIR_SEPARATOR_S, ref, NULL);

      g_string_sprintf (file_contents, dir_not_found_format_string,
			eek_png_tag, new_dir, new_ref);
      html_source (dest_page, new_ref, 0, file_contents->str, add_to_queue, FALSE);

      goto FINISH;
    }

  g_free (new_dir);
  new_dir = g_get_current_dir ();

  new_ref = g_strconcat (new_dir, G_DIR_SEPARATOR_S, new_base, NULL);

  if (strcmp (dest_page->current_ref, new_ref) == 0)
    {
      gtk_xmhtml_set_topline (GTK_XMHTML (dest_page->html), pos);

      if (add_to_queue)
	queue_add (dest_page->queue, new_ref, pos);

      goto FINISH;
    }

  /*
   *  handle basename like: filename.html#11111 -> filename.html
   */ 
  g_strdelimit (new_base,"#",'\0');

  afile = fopen (new_base, "rt");

  if (afile != NULL)
    {
      while (fgets (aline, sizeof (aline), afile))
	file_contents = g_string_append (file_contents, aline);
      fclose (afile);
    }

  if (strlen (file_contents->str) <= 0)
    {
      chdir (old_dir);
      g_string_sprintf (file_contents, doc_not_found_format_string,
			eek_png_tag, ref);
    }
  else
    page_valid = TRUE;

  html_source (dest_page, new_ref, 0, file_contents->str, 
	       add_to_queue, add_to_history && page_valid);

 FINISH:

  g_free (dest_page->current_ref);
  dest_page->current_ref = new_ref;

  g_string_free (file_contents, TRUE);
  g_free (old_dir);
  g_free (new_dir);

  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), dest_page->index);

  return (page_valid);
}

static void
xmhtml_activate (GtkWidget *html,
		 gpointer   data)
{
  XmHTMLAnchorCallbackStruct *cbs = (XmHTMLAnchorCallbackStruct *) data;

  switch (cbs->url_type)
    {
    case URL_JUMP:
      jump_to_anchor (current_page, cbs->href);
      break;

    case URL_FILE_LOCAL:
      load_page (current_page, &pages[HELP], cbs->href, 0, TRUE, TRUE);
      break;

    default:
      /* should handle http request here (e.g. pass them to netscape) */
      break;
    }
}

static GtkWidget *
pixmap_button_new (gchar     **xpm,
		   gchar      *text,
		   GtkWidget  *parent)
{
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *pixmapwid;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle *style;

  gtk_widget_realize (parent);
  style = gtk_widget_get_style (parent);
  pixmap = gdk_pixmap_create_from_xpm_d (parent->window,
					 &mask,
					 &style->bg[GTK_STATE_NORMAL],
					 xpm);
  pixmapwid = gtk_pixmap_new (pixmap, mask);

  box = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), pixmapwid, FALSE, FALSE, 0);
  gtk_widget_show (pixmapwid);

  label = gtk_label_new (text);
  gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  gtk_widget_show (box);

  button = gtk_button_new ();
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_container_add (GTK_CONTAINER (button), box);

  return (button);
}

static void 
notebook_switch_callback (GtkNotebook     *notebook,
			  GtkNotebookPage *page,
			  gint             page_num,
			  gpointer         user_data)
{
  GtkXmHTML *html;
  gint       i;
  GList     *children;

  g_return_if_fail (page_num >= 0 && page_num < 3);

  html = GTK_XMHTML (current_page->html);

  /*  The html widget fails to do the following by itself  */

  GTK_WIDGET_UNSET_FLAGS (html->html.work_area, GTK_MAPPED);
  GTK_WIDGET_UNSET_FLAGS (html->html.vsb, GTK_MAPPED);
  GTK_WIDGET_UNSET_FLAGS (html->html.hsb, GTK_MAPPED);

  /*  Frames  */
  for (i = 0; i < html->html.nframes; i++)
    GTK_WIDGET_UNSET_FLAGS (html->html.frames[i]->frame, GTK_MAPPED);

  /*  Form widgets  */
  for (children = html->children; children; children = children->next)
    GTK_WIDGET_UNSET_FLAGS (children->data, GTK_MAPPED);

  /*  Set the new page  */
  current_page = &pages[page_num];
}

static void 
notebook_switch_after_callback (GtkNotebook     *notebook,
				GtkNotebookPage *page,
				gint             page_num,
				gpointer         user_data)
{
  GtkAccelGroup *accel_group = gtk_accel_group_get_default ();

  gtk_widget_add_accelerator (GTK_XMHTML (current_page->html)->html.vsb,
			      "page_up", accel_group,
			      'b', 0, 0);
  gtk_widget_add_accelerator (GTK_XMHTML (current_page->html)->html.vsb,
			      "page_down", accel_group,
			      ' ', 0, 0);

  gtk_widget_add_accelerator (GTK_XMHTML (current_page->html)->html.vsb,
			      "page_up", accel_group,
			      GDK_Page_Up, 0, 0);
  gtk_widget_add_accelerator (GTK_XMHTML (current_page->html)->html.vsb,
			      "page_down", accel_group,
			      GDK_Page_Down, 0, 0);

  update_toolbar (current_page);
}


static gint
notebook_label_button_press_callback (GtkWidget *widget,
				      GdkEvent  *event,
				      gpointer   data)
{
  guint i = GPOINTER_TO_UINT (data);

  if (current_page != &pages[i])
    gtk_notebook_set_page (GTK_NOTEBOOK (notebook), i);
  
  return (TRUE);
}

static void
combo_drag_begin (GtkWidget *widget,
		  gpointer   data)
{ 
}

static void
combo_drag_handle (GtkWidget        *widget, 
		   GdkDragContext   *context,
		   GtkSelectionData *selection_data,
		   guint             info,
		   guint             time,
		   gpointer          data)
{
  HelpPage *page = (HelpPage*)data;

  if (page->current_ref != NULL)
    {
      gtk_selection_data_set (selection_data,
			      selection_data->target,
			      8, 
			      page->current_ref, 
			      strlen (page->current_ref));
    }
}

static void
page_up_callback (GtkWidget *widget,
		  GtkWidget *html)
{
  GtkAdjustment *adj;

  adj = GTK_ADJUSTMENT (GTK_XMHTML (html)->vsba);
  gtk_adjustment_set_value (adj, adj->value - (adj->page_size));
}

static void
page_down_callback (GtkWidget *widget,
		    GtkWidget *html)
{
  GtkAdjustment *adj;

  adj = GTK_ADJUSTMENT (GTK_XMHTML (html)->vsba);
  gtk_adjustment_set_value (adj, adj->value + (adj->page_size));
}

static gint
wheel_callback (GtkWidget      *widget,
		GdkEventButton *bevent,
		GtkWidget      *html)
{
  GtkAdjustment *adj;
  gfloat new_value;

  if (! GTK_XMHTML (html)->html.needs_vsb)
    return FALSE;

  adj = GTK_ADJUSTMENT (GTK_XMHTML (html)->vsba);

  switch (bevent->button)
    {
    case 4:
      new_value = adj->value - adj->page_increment / 2;
      break;

    case 5:
      new_value = adj->value + adj->page_increment / 2;
      break;

    default:
      return FALSE;
    }

  new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
  gtk_adjustment_set_value (adj, new_value);

  return TRUE;
}

static gint
set_initial_history (gpointer data)
{
  gint   add_to_history = GPOINTER_TO_INT (data);
  gchar *title;

  title = XmHTMLGetTitle (pages[HELP].html);
  if (add_to_history)
    history_add (pages[HELP].current_ref, title);

  gtk_signal_handler_block_by_data (GTK_OBJECT (GTK_COMBO (combo)->entry), combo);
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry), title);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (GTK_COMBO (combo)->entry), combo);

  return FALSE;
}

gboolean
open_browser_dialog (gchar *locale,
		     gchar *path)
{
  GtkWidget *window;
  GtkWidget *vbox, *hbox, *bbox, *html_box;
  GtkWidget *button;
  GtkWidget *title;
  GtkWidget *drag_source;
  GtkWidget *label;

  gchar  *initial_dir;
  gchar  *initial_ref;
  gchar  *root_dir;
  gchar  *eek_png_path;
  gint    success;
  guint   i;
  gint    argc;
  gchar **argv;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("webbrowser");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  root_dir = g_strconcat (gimp_data_directory(), G_DIR_SEPARATOR_S, 
			  GIMP_HELP_PREFIX, NULL);

  if (chdir (root_dir) == -1)
    {
      gimp_message (_("GIMP Help Browser Error.\n\n"
		      "Couldn't find my root html directory."));
      return FALSE;
    }

  eek_png_path = g_strconcat (root_dir, G_DIR_SEPARATOR_S,
			      "images", G_DIR_SEPARATOR_S,
			      "eek.png", NULL);
  if (access (eek_png_path, R_OK) == 0)
    eek_png_tag = g_strdup_printf ("<img src=\"%s\">", eek_png_path);

  g_free (eek_png_path);
  g_free (root_dir);
  initial_dir = g_get_current_dir ();

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (close_callback),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (close_callback),
		      NULL);
  gtk_window_set_wmclass (GTK_WINDOW (window), "helpbrowser", "Gimp");
  gtk_window_set_title (GTK_WINDOW (window), _("GIMP Help Browser"));

  gimp_help_connect_help_accel (window, gimp_plugin_help_func,
				"dialogs/help.html");

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  bbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (bbox), 0);
  gtk_box_pack_start (GTK_BOX (hbox), bbox, FALSE, FALSE, 0);

  back_button = pixmap_button_new (back_xpm, _("Back"), window);
  gtk_container_add (GTK_CONTAINER (bbox), back_button);
  gtk_widget_set_sensitive (GTK_WIDGET (back_button), FALSE);
  gtk_signal_connect (GTK_OBJECT (back_button), "clicked",
		      GTK_SIGNAL_FUNC (back_callback),
		      NULL);
  gtk_widget_show (back_button);

  forward_button = pixmap_button_new (forward_xpm, _("Forward"), window);
  gtk_container_add (GTK_CONTAINER (bbox), forward_button);
  gtk_widget_set_sensitive (GTK_WIDGET (forward_button), FALSE);
  gtk_signal_connect (GTK_OBJECT (forward_button), "clicked",
		      GTK_SIGNAL_FUNC (forward_callback),
		      NULL);
  gtk_widget_show (forward_button);

  gtk_widget_show (bbox);

  bbox = gtk_hbutton_box_new ();
  gtk_box_pack_end (GTK_BOX (hbox), bbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_label (_("Close"));
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
 		      GTK_SIGNAL_FUNC (close_callback),
		      NULL);
  gtk_widget_show (button);

  gtk_widget_show (bbox);
  gtk_widget_show (hbox);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_notebook_set_tab_vborder (GTK_NOTEBOOK (notebook), 0);
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);

  for (i = 0; i < 3; i++)
    {
      static guint page_up_signal = 0;
      static guint page_down_signal = 0;

      pages[i].index = i;
      pages[i].html = gtk_xmhtml_new ();
      pages[i].queue = queue_new ();
      pages[i].current_ref = g_strconcat (initial_dir, G_DIR_SEPARATOR_S,
					  locale, G_DIR_SEPARATOR_S,
					  ".", NULL);

      gtk_xmhtml_set_anchor_underline_type (GTK_XMHTML (pages[i].html),
					    GTK_ANCHOR_SINGLE_LINE);
      gtk_xmhtml_set_anchor_buttons (GTK_XMHTML (pages[i].html), FALSE);
      gtk_widget_set_usize (GTK_WIDGET (pages[i].html), -1, 300);

      switch (i)
	{
	case CONTENTS:
	case INDEX:
	  title = drag_source = gtk_event_box_new ();
	  label = gtk_label_new (gettext (pages[i].label));
	  gtk_container_add (GTK_CONTAINER (title), label);
	  gtk_widget_show (label);
	  break;
	case HELP:
	  title = combo = gtk_combo_new ();
	  drag_source = GTK_COMBO (combo)->entry;
	  gtk_widget_set_usize (GTK_WIDGET (combo), 300, -1);
	  gtk_entry_set_editable (GTK_ENTRY (GTK_COMBO (combo)->entry), FALSE); 
	  gtk_combo_set_use_arrows (GTK_COMBO (combo), TRUE);
	  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (combo)->entry), 
			      "changed",
			      GTK_SIGNAL_FUNC (entry_changed_callback), 
			      combo);
	  gtk_widget_show (combo);
	  break;
	default:
	  title = drag_source = NULL;     /* to please the compiler */
	  break;
	}	  

      /*  connect to the button_press signal to make notebook switching work */ 
      gtk_signal_connect (GTK_OBJECT (title), "button_press_event",
			  GTK_SIGNAL_FUNC (notebook_label_button_press_callback), 
			  GUINT_TO_POINTER (i));

      /*  dnd source  */
      gtk_drag_source_set (GTK_WIDGET (drag_source),
			   GDK_BUTTON1_MASK,
			   help_dnd_target_table, n_help_dnd_targets, 
			   GDK_ACTION_MOVE | GDK_ACTION_COPY);
      gtk_signal_connect (GTK_OBJECT (drag_source), "drag_begin",
			  GTK_SIGNAL_FUNC (combo_drag_begin),
			  &pages[i]);
      gtk_signal_connect (GTK_OBJECT (drag_source), "drag_data_get",
			  GTK_SIGNAL_FUNC (combo_drag_handle),
			  &pages[i]);

      html_box = gtk_vbox_new (FALSE, 0);
      gtk_container_add (GTK_CONTAINER (html_box), pages[i].html);

      gtk_notebook_append_page (GTK_NOTEBOOK (notebook), html_box, title);
      gtk_widget_show (title);

      if (i == HELP && path)
	{
	  if (g_path_is_absolute (path))
	    initial_ref = g_strdup (path);
	  else
	    initial_ref = g_strconcat (initial_dir, G_DIR_SEPARATOR_S,
				       locale, G_DIR_SEPARATOR_S,
				       path, NULL);
	}
      else
	initial_ref = g_strconcat (initial_dir, G_DIR_SEPARATOR_S,
				   locale, G_DIR_SEPARATOR_S,
				   pages[i].home, NULL);

      success = load_page (&pages[i], &pages[i], initial_ref, 0, TRUE, FALSE);
      g_free (initial_ref);

      gtk_widget_show (pages[i].html);
      gtk_widget_show (html_box);
      gtk_signal_connect (GTK_OBJECT (pages[i].html), "activate",
			  (GtkSignalFunc) xmhtml_activate,
			  &pages[i]);

      if (! page_up_signal)
	{
	  page_up_signal = gtk_object_class_user_signal_new 
	    (GTK_OBJECT (GTK_XMHTML (pages[i].html)->html.vsb)->klass,
	     "page_up",
	     GTK_RUN_FIRST,
	     gtk_marshal_NONE__NONE,
	     GTK_TYPE_NONE, 0);
	  page_down_signal = gtk_object_class_user_signal_new
	    (GTK_OBJECT (GTK_XMHTML (pages[i].html)->html.vsb)->klass,
	     "page_down",
	     GTK_RUN_FIRST,
	     gtk_marshal_NONE__NONE,
	     GTK_TYPE_NONE, 0);
	}

      gtk_signal_connect (GTK_OBJECT (GTK_XMHTML (pages[i].html)->html.vsb),
			  "page_up",
			  GTK_SIGNAL_FUNC (page_up_callback),
			  pages[i].html);
      gtk_signal_connect (GTK_OBJECT (GTK_XMHTML (pages[i].html)->html.vsb), 
			  "page_down",
			  GTK_SIGNAL_FUNC (page_down_callback),
			  pages[i].html);

      gtk_signal_connect (GTK_OBJECT (GTK_XMHTML (pages[i].html)->html.work_area),
			  "button_press_event",
			  GTK_SIGNAL_FUNC (wheel_callback),
			  pages[i].html);
    }

  gtk_signal_connect (GTK_OBJECT (notebook), "switch-page",
		      GTK_SIGNAL_FUNC (notebook_switch_callback),
		      NULL);
  gtk_signal_connect_after (GTK_OBJECT (notebook), "switch-page",
			    GTK_SIGNAL_FUNC (notebook_switch_after_callback),
			    NULL);

  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), HELP);
  gtk_widget_show (notebook);

  gtk_widget_show (vbox);
  gtk_widget_show (window);

  gtk_idle_add ((GtkFunction) set_initial_history, GINT_TO_POINTER (success));

  g_free (initial_dir);

  return TRUE;
}

static gint
idle_load_page (gpointer data)
{
  gchar *path = data;

  load_page (&pages[HELP], &pages[HELP], path, 0, TRUE, TRUE);
  g_free (path);

  return FALSE;
}

static void
run_temp_proc (gchar   *name,
	       gint     nparams,
	       GParam  *param,
	       gint    *nreturn_vals,
	       GParam **return_vals)
{
  static GParam values[1];
  GStatusType status = STATUS_SUCCESS;
  gchar *locale;
  gchar *path;

  /*  Make sure all the arguments are there!  */
  if ((nparams != 2) ||
      !param[0].data.d_string ||
      !strlen (param[0].data.d_string) ||
      !param[1].data.d_string ||
      !strlen (param[1].data.d_string))
    {
      locale = "C";
      path   = "welcome.html";
    }
  else
    {
      locale = param[0].data.d_string;
      path   = param[1].data.d_string;
    }

  g_strdelimit (path, "/", G_DIR_SEPARATOR);

  if (g_path_is_absolute (path))
    path = g_strdup (path);
  else
    path = g_strconcat (gimp_data_directory (), G_DIR_SEPARATOR_S, 
			GIMP_HELP_PREFIX, G_DIR_SEPARATOR_S,
			locale, G_DIR_SEPARATOR_S,
			path, NULL);

  gtk_idle_add (idle_load_page, path);

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
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
  static GParamDef args[] =
  {
    { PARAM_STRING, "locale", "Langusge to use" },
    { PARAM_STRING, "path",   "Path of a local document to open. "
                              "Can be relative to GIMP_HELP_PATH." }
  };
  static int nargs = sizeof (args) / sizeof (args[0]);

  static GParamDef *return_vals = NULL;
  static int nreturn_vals = 0;

  gimp_install_temp_proc (GIMP_HELP_TEMP_EXT_NAME,
			  "DON'T USE THIS ONE",
			  "(Temporary procedure)",
			  "Sven Neumann <sven@gimp.org>, "
			  "Michael Natterer <mitschel@cs.tu-berlin.de>",
			  "Sven Neumann & Michael Natterer",
			  "1999",
			  NULL,
			  "",
			  PROC_TEMPORARY,
			  nargs, nreturn_vals,
			  args, return_vals,
			  run_temp_proc);

  /* Tie into the gdk input function */
  g_io_add_watch (_readchannel, G_IO_IN | G_IO_PRI, input_callback, NULL);

  /* This needed on Win32 */
  gimp_request_wakeups ();

  temp_proc_installed = TRUE;
}

static gboolean
open_url (gchar *locale,
	  gchar *path)
{
  if (! open_browser_dialog (locale, path))
    return FALSE;

  install_temp_proc ();
  gtk_main ();

  return TRUE;
}

MAIN ()

static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32,  "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "locale"  , "Language to use" },
    { PARAM_STRING, "path",     "Path of a local document to open. "
                                "Can be relative to GIMP_HELP_PATH." }
  };
  static int nargs = sizeof (args) / sizeof (args[0]);
  static GParamDef *return_vals = NULL;
  static int nreturn_vals = 0;

  gimp_install_procedure (GIMP_HELP_EXT_NAME,
                          _("Browse the GIMP help pages"),
                          _("A small and simple HTML browser optimzed for "
			    "browsing the GIMP help pages."),
                          "Sven Neumann <sven@gimp.org>, "
			  "Michael Natterer <mitschel@cs.tu-berlin.de>",
			  "Sven Neumann & Michael Natterer",
                          "1999",
                          NULL,
                          "",
                          PROC_EXTENSION,
                          nargs, nreturn_vals,
                          args, return_vals);
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  gchar *locale;
  gchar *path;

  run_mode = param[0].data.d_int32;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals = values;

  if (strcmp (name, GIMP_HELP_EXT_NAME) == 0)
    {
      switch (run_mode)
        {
        case RUN_INTERACTIVE:
        case RUN_NONINTERACTIVE:
	case RUN_WITH_LAST_VALS:
         /*  Make sure all the arguments are there!  */
          if ((nparams != 3) ||
	      !param[1].data.d_string ||
	      !strlen (param[1].data.d_string) ||
	      !param[2].data.d_string ||
	      !strlen (param[2].data.d_string))
	    {
	      locale = "C";
	      path   = g_strdup ("welcome.html");
	    }
	  else
	    {
	      locale = g_strdup (param[1].data.d_string);
	      path   = g_strdup (param[2].data.d_string);
	    }
          break;
        default:
	  locale = NULL;
	  path   = NULL;
	  status = STATUS_CALLING_ERROR;
          break;
        }

      if (status == STATUS_SUCCESS)
        {
	  g_strdelimit (path, "/", G_DIR_SEPARATOR);

       	  if (!open_url (locale, path))
            values[0].data.d_status = STATUS_EXECUTION_ERROR;
	  else
	    values[0].data.d_status = STATUS_SUCCESS;

	  g_free (path);
        }
      else
        values[0].data.d_status = status;
    }
  else
    values[0].data.d_status = STATUS_CALLING_ERROR;
}
