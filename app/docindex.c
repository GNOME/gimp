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
#include "gimpui.h"
#include "gimpdnd.h"
#include "ops_buttons.h"
#include "session.h"

#include "libgimp/gimpenv.h"

#include "libgimp/gimpintl.h"

#include "pixmaps/folder.xpm"
#include "pixmaps/raise.xpm"
#include "pixmaps/lower.xpm"
#include "pixmaps/delete.xpm"


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


/*  forward declarations  */

static void      create_idea_list                  (void);
static void      idea_add_in_position              (gchar       *label,
						    gint         position);
static void      open_idea_window                  (void);
static void      open_or_raise                     (gchar       *file_name,
						    gboolean    try_raise);

static void      idea_open_callback                (GtkWidget   *widget,
						    gpointer     data);
static void      idea_open_or_raise_callback       (GtkWidget   *widget,
						    gpointer     data);
static void      idea_up_callback                  (GtkWidget   *widget,
						    gpointer     data);
static void      idea_to_top_callback              (GtkWidget   *widget,
						    gpointer     data);
static void      idea_down_callback                (GtkWidget   *widget,
						    gpointer     data);
static void      idea_to_bottom_callback           (GtkWidget   *widget,
						    gpointer     data);
static void      idea_remove_callback              (GtkWidget   *widget,
						    gpointer     data);
static void      idea_hide_callback                (GtkWidget   *widget,
						    gpointer     data);

static void      load_idea_manager                 (IdeaManager *ideas);
static void      save_idea_manager                 (IdeaManager *ideas);

static void      clear_white                       (FILE        *fp);
static gint      getinteger                        (FILE        *fp);


/*  local variables  */

static IdeaManager *ideas     = NULL;
static GList       *idea_list = NULL;


/*  the ops buttons  */
static GtkSignalFunc open_ext_callbacks[] = 
{
  idea_open_or_raise_callback, file_open_callback, NULL, NULL
};

static GtkSignalFunc raise_ext_callbacks[] = 
{
  idea_to_top_callback, NULL, NULL, NULL
};

static GtkSignalFunc lower_ext_callbacks[] = 
{
  idea_to_bottom_callback, NULL, NULL, NULL
};

static OpsButton ops_buttons[] =
{
  { folder_xpm, idea_open_callback, open_ext_callbacks,
    N_("Open the selected entry\n"
       "<Shift> Raise window if already open\n"
       "<Ctrl> Load Image dialog"), NULL,
    NULL, 0 },
  { raise_xpm, idea_up_callback, raise_ext_callbacks,
    N_("Move the selected entry up in the index\n"
       "<Shift> To top"), NULL,
    NULL, 0 },
  { lower_xpm, idea_down_callback, lower_ext_callbacks,
    N_("Move the selected entry down in the index\n"
       "<Shift> To bottom"), NULL,
    NULL, 0 },
  { delete_xpm, idea_remove_callback, NULL,
    N_("Remove the selected entry from the index"), NULL,
    NULL, 0 },
  { NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};


/*  dnd stuff  */

static GtkTargetEntry drag_types[] =
{
  GIMP_TARGET_URI_LIST,
  GIMP_TARGET_TEXT_PLAIN,
  GIMP_TARGET_NETSCAPE_URL
};
static gint n_drag_types = sizeof (drag_types) / sizeof (drag_types[0]);


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
document_index_add (gchar *title)
{
  idea_add_in_position (title, 0);
}

FILE *
document_index_parse_init (void)
{
  FILE  *fp;
  gchar *desktopfile;
  gint   dummy;

  desktopfile = gimp_personal_rc_file ("ideas");

  fp = fopen (desktopfile, "r");

  if (fp != NULL)
    {
      /*  eventually strip away the old file format's first line  */
      if (fscanf (fp, "%i %i %i %i", &dummy, &dummy, &dummy, &dummy) != 4)
	{
	  fclose (fp);
	  fp = fopen (desktopfile, "r");
	}
    }

  g_free (desktopfile);

  return fp;
}

gchar *
document_index_parse_line (FILE * fp)
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
    fp = document_index_parse_init ();

  if (idea_list || fp)
    {
      gtk_widget_show (ideas->window);

      if (fp)
	{
	  gchar *title;

	  clear_white (fp);

	  while ((title = document_index_parse_line (fp)))
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
list_item_callback (GtkWidget      *widget,
		    GdkEventButton *event,
		    gpointer        data)
{
  if (GTK_IS_LIST_ITEM (widget) &&
      event->type == GDK_2BUTTON_PRESS)
    {
      open_or_raise (GTK_LABEL (GTK_BIN (widget)->child)->label, FALSE);
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
			      GTK_SIGNAL_FUNC (list_item_callback),
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

	  fp = document_index_parse_init ();

	  if (fp)
	    {  
	      gchar *filename;

	      while ((filename = document_index_parse_line (fp)))
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
open_or_raise (gchar    *file_name,
	       gboolean  try_raise)
{
  BoolCharPair pair;

  pair.boole  = FALSE;
  pair.string = file_name;
  pair.data   = NULL;

  if (try_raise)
    {
      gdisplays_foreach (raise_if_match, &pair);

      if (! pair.boole)
	{
	  file_open (file_name, file_name);
	}
    }
  else
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

  if (!feof(fp))
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

  if (!feof(fp))
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
idea_open_callback (GtkWidget   *widget,
		    gpointer     data)
{
  GtkWidget *selected;

  if (GTK_LIST (ideas->list)->selection)
    {
      selected = GTK_LIST (ideas->list)->selection->data;
      open_or_raise (GTK_LABEL (GTK_BIN (selected)->child)->label, FALSE);
    }
  else
    {
      file_open_callback (widget, data);
    }
}

static void
idea_open_or_raise_callback (GtkWidget   *widget,
			     gpointer     data)
{
  GtkWidget *selected;

  if (GTK_LIST (ideas->list)->selection)
    {
      selected = GTK_LIST (ideas->list)->selection->data;
      open_or_raise (GTK_LABEL (GTK_BIN (selected)->child)->label, TRUE);
    }
  else
    {
      file_open_callback (widget, data);
    }
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
idea_to_top_callback (GtkWidget   *widget,
		      gpointer     data)
{
  GtkWidget *selected;

  if (GTK_LIST (ideas->list)->selection)
    {
      selected = GTK_LIST (ideas->list)->selection->data;
      idea_move (selected, - g_list_length (GTK_LIST (ideas->list)->children),
		 TRUE);
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
idea_to_bottom_callback (GtkWidget   *widget,
			 gpointer     data)
{
  GtkWidget *selected;

  if (GTK_LIST (ideas->list)->selection)
    {
      selected = GTK_LIST (ideas->list)->selection->data;
      idea_move (selected, g_list_length (GTK_LIST (ideas->list)->children),
		 TRUE);
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

  /* False if exiting */
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

static void
ops_buttons_update (GtkWidget *widget,
		    gpointer   data)
{
  GtkWidget *selected = NULL;
  gint       length   = 0;
  gint       index    = -1;

  length = g_list_length (GTK_LIST (ideas->list)->children);

  if (GTK_LIST (ideas->list)->selection)
    {
      selected = GTK_LIST (ideas->list)->selection->data;
      index  = g_list_index  (GTK_LIST (ideas->list)->children, selected);
    }

#define SET_OPS_SENSITIVE(button,condition) \
        gtk_widget_set_sensitive (ops_buttons[(button)].widget, \
                                  (condition) != 0)

  SET_OPS_SENSITIVE (1, selected && index > 0);
  SET_OPS_SENSITIVE (2, selected && index < (length - 1));
  SET_OPS_SENSITIVE (3, selected);

#undef SET_OPS_SENSITIVE
}

static void
open_idea_window (void)
{
  GtkWidget *main_vbox;
  GtkWidget *scrolled_win;
  GtkWidget *abox;
  GtkWidget *button_box;
  gint       i;

  ideas = g_new0 (IdeaManager, 1);

  ideas->window = gimp_dialog_new (_("Document Index"), "docindex",
				   gimp_standard_help_func,
				   "dialogs/document_index.html",
				   GTK_WIN_POS_MOUSE,
				   FALSE, TRUE, FALSE,

				   _("Close"), idea_hide_callback,
				   NULL, NULL, NULL, TRUE, TRUE,

				   NULL);

  gtk_drag_dest_set (ideas->window,
                     GTK_DEST_DEFAULT_ALL,
                     drag_types, n_drag_types,
                     GDK_ACTION_COPY);

  gimp_dnd_file_dest_set (ideas->window);

  dialog_register (ideas->window);
  session_set_window_geometry (ideas->window, &document_index_session_info,
			       TRUE);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (ideas->window)->vbox),
		     main_vbox);
  gtk_widget_show (main_vbox);

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

  /*  The ops buttons  */
  abox = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ideas->window)->action_area), abox,
		      FALSE, FALSE, 4);
  gtk_widget_show (abox);

  button_box = ops_button_box_new (ops_buttons, OPS_BUTTON_NORMAL);
  gtk_container_add (GTK_CONTAINER (abox), button_box);
  gtk_widget_show (button_box);

  for (i = 0; ; i++)
    {
      if (ops_buttons[i].widget == NULL)
	break;

      gtk_misc_set_padding (GTK_MISC (GTK_BIN (ops_buttons[i].widget)->child),
			    12, 0);
    }

  /* Load and Show window */
  load_idea_manager (ideas);

  gtk_signal_connect_after (GTK_OBJECT (ideas->list), "selection_changed",
			    GTK_SIGNAL_FUNC (ops_buttons_update),
			    NULL);
  ops_buttons_update (NULL, NULL);
}
