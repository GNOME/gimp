/* docindex.c - Creates the window used by the document index in go and gimp.
 *
 * Copyright (C) 1998 Chris Lahey.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include "config.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "dialog_handler.h"
#include "docindex.h"
#include "fileops.h"
#include "gimpdnd.h"
#include "session.h"

#include "libgimp/gimpenv.h"

#include "libgimp/gimpintl.h"


typedef struct
{
  GtkWidget *window;
  GtkWidget *list;
} IdeaManager;

typedef struct
{
  gboolean  boole;
  gchar    *string;
  gpointer  data;
} BoolCharPair;


static IdeaManager *ideas     = NULL;
static GList       *idea_list = NULL;


/*  dnd stuff  */

static GtkTargetEntry drag_types[] =
{
  GIMP_TARGET_URI_LIST,
  GIMP_TARGET_TEXT_PLAIN,
  GIMP_TARGET_NETSCAPE_URL
};
static gint n_drag_types = sizeof (drag_types) / sizeof (drag_types[0]);


/*  forward declarations  */

static void      create_idea_list                  (void);
static void      idea_add_in_position              (gchar       *label,
						    gint         position);
static void      open_idea_window                  (void);
static void      open_or_raise                     (gchar       *file_name);

static void      idea_up_callback                  (GtkWidget   *widget,
						    gpointer     data);
static void      idea_down_callback                (GtkWidget   *widget,
						    gpointer     data);
static void      idea_remove_callback              (GtkWidget   *widget,
						    gpointer     data);
static void      idea_hide_callback                (GtkWidget   *widget,
						    gpointer     data);

static gboolean  idea_window_delete_event_callback (GtkWidget   *widget,
						    GdkEvent    *event,
						    gpointer     data);

static void      load_idea_manager                 (IdeaManager *ideas);
static void      save_idea_manager                 (IdeaManager *ideas);

static void      clear_white                       (FILE        *fp);
static gint      getinteger                        (FILE        *fp);



/*  public functions  */

void
document_index_create (void)
{
  if (ideas)
    gdk_window_raise (ideas->window->window);
  else
    open_idea_window ();
}

void
document_index_free (void)
{
  idea_hide_callback (NULL, NULL);
}

void
idea_add (gchar *title)
{
  idea_add_in_position (title, 0);
}

FILE *
idea_manager_parse_init (void)
{
  FILE  *fp;
  gchar *desktopfile;

  desktopfile = gimp_personal_rc_file ("ideas");
  fp = fopen (desktopfile, "r");
  g_free (desktopfile);

  return fp;
}

gchar *
idea_manager_parse_line (FILE * fp)
{
  gint   length;
  gchar *filename;

  length = getinteger (fp);

  if (!feof (fp) && !ferror (fp))
    {
      filename = g_malloc0 (length + 1);
      filename[fread (filename, 1, length, fp)] = 0;
      clear_white (fp);
      return filename;
    }

  return NULL;
}


/*  local functions  */

static void
load_from_list (gpointer data,
		gpointer data_null)
{
  idea_add_in_position ((gchar *) data, -1);
}

static void
load_idea_manager (IdeaManager *ideas)
{
  FILE *fp = NULL;

  if (! idea_list)
    fp = idea_manager_parse_init ();

  if (idea_list || fp)
    {
      gtk_widget_show (ideas->window);

      if (fp)
	{
	  gchar *title;

	  clear_white (fp);

	  while ((title = idea_manager_parse_line (fp)))
	    {
	      idea_add_in_position (title, -1);
	      g_free (title);
	    }
	  fclose (fp);
	}
      else
	{
	  g_list_foreach (idea_list, load_from_list, NULL);
	  g_list_foreach (idea_list, (GFunc) g_free, NULL);
	  g_list_free (idea_list);
	  idea_list = 0;
	}
    }
  else
    {
      gtk_widget_show (ideas->window);
    }
}

static void
save_to_ideas (gpointer data,
	       gpointer user_data)
{
  gchar *title;

  title = GTK_LABEL (GTK_BIN (data)->child)->label;

  fprintf ((FILE *) user_data, "%d %s\n", strlen (title), title);
}

static void
save_list_to_ideas (gpointer data,
		    gpointer user_data)
{
  gchar *title;

  title = (gchar *) data;

  fprintf ((FILE *) user_data, "%d %s\n", strlen (title), title);
}

static void
save_idea_manager (IdeaManager *ideas)
{
  FILE  *fp;
  gchar *desktopfile;
    
  /* open persistant desktop file. */
  desktopfile = gimp_personal_rc_file ("ideas");
  fp = fopen (desktopfile, "w");
  g_free (desktopfile);

  if (fp)
    {
      if (ideas)
	{
	  g_list_foreach (GTK_LIST (ideas->list)->children, save_to_ideas, fp);
	}
      else if (idea_list)
	{
	  g_list_foreach (idea_list, save_list_to_ideas, fp);
	}

      fclose (fp);
    }
}

static void
save_to_list (gpointer data,
	      gpointer null_data)
{
  gchar *title;

  title = g_strdup (GTK_LABEL (GTK_BIN (data)->child)->label);

  idea_list = g_list_append (idea_list, title);
}

static void
create_idea_list (void)
{
  if (idea_list)
    {
      g_list_foreach (idea_list, (GFunc) g_free, NULL);
      g_list_free (idea_list);
      idea_list = 0;
    }

  g_list_foreach (GTK_LIST (ideas->list)->children, save_to_list, NULL);
}

static gint
open_or_raise_callback (GtkWidget      *widget,
			GdkEventButton *event,
			gpointer        func_data)
{
  if (GTK_IS_LIST_ITEM (widget) &&
      event->type == GDK_2BUTTON_PRESS)
    {
      open_or_raise (GTK_LABEL (GTK_BIN (widget)->child)->label);
    }

  return FALSE;
} 

static void
check_needed (gpointer data,
	      gpointer user_data)
{
  BoolCharPair *pair;

  pair = (BoolCharPair *) user_data;

  if (strcmp (pair->string, GTK_LABEL (GTK_BIN (data)->child)->label) == 0)
    {
      pair->boole = TRUE;
      pair->data  = data;
    }
}

static void
check_needed_list (gpointer data,
		   gpointer user_data)
{
  BoolCharPair *pair;

  pair = (BoolCharPair *) user_data;

  if (strcmp (pair->string, (gchar *) data) == 0)
    {
      pair->boole = TRUE;
      pair->data  = data;
    }
}

static void
idea_add_in_position_with_select (gchar    *title,
				  gint      position,
				  gboolean  select)
{
  BoolCharPair  pair;

  pair.boole  = FALSE;
  pair.string = title;
  pair.data   = NULL;

  if (ideas)
    {
      g_list_foreach (GTK_LIST (ideas->list)->children, check_needed, &pair);

      if (! pair.boole)
	{
	  GtkWidget *listitem;
	  GList     *list = NULL;

	  listitem = gtk_list_item_new_with_label (title);
	  list = g_list_append (list, listitem);

	  if (position < 0)
	    gtk_list_append_items (GTK_LIST (ideas->list), list);
	  else
	    gtk_list_insert_items (GTK_LIST (ideas->list), list, position);

	  gtk_signal_connect (GTK_OBJECT (listitem), "button_press_event",
			      GTK_SIGNAL_FUNC (open_or_raise_callback),
			      NULL);

	  gtk_widget_show (listitem);

	  if (select)
	    gtk_list_item_select (GTK_LIST_ITEM  (listitem));
	}
      else /* move entry to top */
	{
	  gchar *title;

	  title = g_strdup (GTK_LABEL (GTK_BIN (pair.data)->child)->label);
	  gtk_container_remove (GTK_CONTAINER (ideas->list),
				GTK_WIDGET (pair.data));
	  idea_add_in_position_with_select (title, 0, TRUE);
	  g_free (title); 
	}
    }
  else
    {
      if (! idea_list)
	{
	  FILE  *fp;

	  fp = idea_manager_parse_init ();

	  if (fp)
	    {  
	      gchar *filename;

	      while ((filename = idea_manager_parse_line (fp)))
		{
		  idea_list = g_list_append (idea_list, g_strdup (filename));
		  g_free (filename);
		}

	      fclose (fp);
	    }
	}

      g_list_foreach (idea_list, check_needed_list, &pair);

      if (! pair.boole)
	{
	  if (position < 0)
	    idea_list = g_list_prepend (idea_list, g_strdup (title));
	  else
	    idea_list = g_list_insert (idea_list, g_strdup (title), position);
	}
      else /* move entry to top */
	{
	  idea_list = g_list_remove (idea_list, pair.data);
	  g_free (pair.data);
	  idea_list = g_list_prepend (idea_list, g_strdup (title));
	}
    }
}

static void
idea_add_in_position (gchar *title,
		      gint   position)
{
  idea_add_in_position_with_select (title, position, TRUE);
}

static void
raise_if_match (gpointer data,
		gpointer user_data)
{
  GDisplay     *gdisp;
  BoolCharPair *pair;

  gdisp = (GDisplay *) data;
  pair  = (BoolCharPair *) user_data;

  if (gdisp->gimage->has_filename &&
      strcmp (pair->string, gdisp->gimage->filename) == 0)
    {
      pair->boole = TRUE;
      gdk_window_raise (gdisp->shell->window);
    }
}

static void
open_or_raise (gchar *file_name)
{
  BoolCharPair pair;

  pair.boole  = FALSE;
  pair.string = file_name;
  pair.data   = NULL;

  gdisplays_foreach (raise_if_match, &pair);
  
  if (! pair.boole)
    {
      file_open (file_name, file_name);
    }
}


/*  file parsing functions  */

static gint
getinteger (FILE *fp)
{
  gchar    nextchar;
  gint     response = 0;
  gboolean negative = FALSE;

  while (isspace (nextchar = fgetc (fp)))
    /* empty statement */ ;

  if (nextchar == '-')
    {
      negative = TRUE;
      while (isspace (nextchar = fgetc (fp)))
	/* empty statement */ ;
    }

  for (; '0' <= nextchar && '9' >= nextchar; nextchar = fgetc (fp))
    {
      response *= 10;
      response += nextchar - '0';
    }
  for (; isspace (nextchar); nextchar = fgetc (fp))
    /* empty statement */ ;

  ungetc (nextchar, fp);
  if (negative)
    response = -response;

  return response;
}

static void
clear_white (FILE *fp)
{
  gint nextchar;

  while (isspace (nextchar = fgetc (fp)))
    /* empty statement */ ;

  if (nextchar != EOF)
    ungetc (nextchar, fp);
}


/*  toolbar / dialog callbacks  */

static gint
idea_move (GtkWidget *widget,
	   gint       distance,
	   gboolean   select)
{
  gint   orig_position;
  gint   position;
  gchar *title;

  orig_position = g_list_index (GTK_LIST (ideas->list)->children, widget);
  position = orig_position + distance;

  if (position < 0)
    position = 0;

  if (position >= g_list_length (GTK_LIST (ideas->list)->children))
    position = g_list_length (GTK_LIST (ideas->list)->children) - 1;

  if (position != orig_position)
    {
      title = g_strdup (GTK_LABEL (GTK_BIN (widget)->child)->label);
      gtk_container_remove (GTK_CONTAINER (ideas->list), widget);
      idea_add_in_position_with_select (title, position, select);
      g_free (title); 
   }

  return position - orig_position;
}

static void
idea_up_callback (GtkWidget *widget,
		  gpointer   data)
{
  GtkWidget *selected;

  if (GTK_LIST (ideas->list)->selection)
    {
      selected = GTK_LIST (ideas->list)->selection->data;
      idea_move (selected, -1, TRUE);
    }
}

static void
idea_down_callback (GtkWidget *widget,
		    gpointer   data)
{
  GtkWidget *selected;

  if (GTK_LIST (ideas->list)->selection)
    {
      selected = GTK_LIST (ideas->list)->selection->data;
      idea_move (selected, 1, TRUE);
    }
}

static void
idea_remove (GtkWidget *widget)
{
  gint position;

  position = g_list_index (GTK_LIST (ideas->list)->children, widget);

  gtk_container_remove (GTK_CONTAINER (ideas->list), widget);

  if (g_list_length (GTK_LIST (ideas->list)->children) - 1 < position)
    position = g_list_length (GTK_LIST (ideas->list)->children) - 1;

  gtk_list_select_item (GTK_LIST (ideas->list), position);  
}

static void
idea_remove_callback (GtkWidget *widget,
		      gpointer   data)
{
  GtkWidget *selected;

  if (GTK_LIST (ideas->list)->selection)
    {
      selected = GTK_LIST (ideas->list)->selection->data;
      idea_remove (selected);
    }
}

static void
idea_hide_callback (GtkWidget *widget,
		    gpointer   data)
{
  if (ideas || idea_list)
    save_idea_manager (ideas);

  /* False if exitting */
  if (ideas)
    {
      create_idea_list ();
      dialog_unregister (ideas->window);
      session_get_window_info (ideas->window, &document_index_session_info);
      gtk_widget_destroy (ideas->window);
      g_free (ideas);
      ideas = 0;
    }
}

static gboolean
idea_window_delete_event_callback (GtkWidget *widget,
				   GdkEvent  *event,
				   gpointer   data)
{
  idea_hide_callback (NULL, NULL);

  return TRUE;
}

static GtkWidget *
create_idea_toolbar (void)
{
  GtkWidget *toolbar;

  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("Open"),
			   _("Open a file"),
			   "Toolbar/Open",
			   NULL,
			   GTK_SIGNAL_FUNC (file_open_callback),
			   NULL);

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("Up"),
			   _("Move the selected entry up in the index"),
			   "Toolbar/Up",
			   NULL,
			   GTK_SIGNAL_FUNC (idea_up_callback),
			   NULL);

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("Down"),
			   _("Move the selected entry down in the index"),
			   "Toolbar/Down",
			   NULL,
			   GTK_SIGNAL_FUNC (idea_down_callback),
			   NULL);

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("Remove"),
			   _("Remove the selected entry from the index"),
			   "Toolbar/Remove",
			   NULL,
			   GTK_SIGNAL_FUNC (idea_remove_callback),
			   NULL);

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("Close"),
			   _("Close the Document Index"),
			   "Toolbar/Hide",
			   NULL,
			   GTK_SIGNAL_FUNC (idea_hide_callback),
			   NULL);

  return toolbar;
}

static void
open_idea_window (void)
{
  GtkWidget *main_vbox;
  GtkWidget *scrolled_win;
  GtkWidget *toolbar;

  /* alloc idea_manager */
  ideas = g_new0 (IdeaManager, 1);

  ideas->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width (GTK_CONTAINER (ideas->window), 0);
  gtk_window_set_title (GTK_WINDOW (ideas->window), _("Document Index"));

  /* Connect the signals */
  gtk_signal_connect (GTK_OBJECT (ideas->window), "delete_event",
		      GTK_SIGNAL_FUNC (idea_window_delete_event_callback),
		      NULL);

  main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (ideas->window), main_vbox);
  gtk_widget_show (main_vbox);

  /* Toolbar */
  toolbar = create_idea_toolbar ();
  gtk_box_pack_start (GTK_BOX (main_vbox), toolbar, FALSE, FALSE, 0);
  gtk_widget_show (toolbar);

  /* Scrolled window */
  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS );
  gtk_box_pack_start (GTK_BOX (main_vbox), scrolled_win, TRUE, TRUE, 0); 
  gtk_widget_show (scrolled_win);

  /* Setup list */
  ideas->list = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (ideas->list), GTK_SELECTION_BROWSE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win),
					 ideas->list);
  gtk_widget_show (ideas->list);

  gtk_drag_dest_set (ideas->window,
                     GTK_DEST_DEFAULT_ALL,
                     drag_types, n_drag_types,
                     GDK_ACTION_COPY);

  gimp_dnd_file_dest_set (ideas->window);

  dialog_register (ideas->window);
  session_set_window_geometry (ideas->window, &document_index_session_info,
			       TRUE);

  /*  Connect the "F1" help key  */
  gimp_help_connect_help_accel (ideas->window,
				gimp_standard_help_func,
				"dialogs/document_index.html");

  /* Load and Show window */
  load_idea_manager (ideas);
}
