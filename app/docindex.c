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

#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "dialog_handler.h"
#include "docindexif.h"
#include "docindex.h"
#include "gimpdnd.h"
#include "gimprc.h"
#include "session.h"

#include "libgimp/gimpenv.h"

#include "libgimp/gimpintl.h"


IdeaManager *ideas = NULL;

static GList *idea_list = NULL;   /* of gchar *. */

static GtkTargetEntry drag_types[] =
{
  GIMP_TARGET_URI_LIST
};
static gint n_drag_types = sizeof (drag_types) / sizeof (drag_types[0]);

static void create_idea_list                       (void);
static void docindex_cell_configure_drop_on_widget (GtkWidget * widget);


static void
docindex_dnd_filenames_dropped (GtkWidget        *widget,
				GdkDragContext   *context,
				gint              x,
				gint              y,
				GtkSelectionData *selection_data,
				guint             info,
				guint             time)
{
  gint len;
  gchar *data;
  gchar *end;

  switch (info)
    {
    case GIMP_DND_TYPE_URI_LIST:
      data = (gchar *) selection_data->data;
      len = selection_data->length;
      while (len > 0)
	{
	  end = strstr (data, "\x0D\x0A");
	  if (end != NULL)
	    *end = 0;
	  if (*data != '#')
	    {
	      gchar *filename = strchr (data, ':');
	      if (filename != NULL)
		filename ++;
	      else
		filename = data;
	      open_file_in_position (filename, -1);
	    }
	  if (end)
	    {
	      len -= end - data + 2;
	      data = end + 2;
	    }
	  else
	    len = 0;
	}
      break;
    }
  return;
}

void
docindex_configure_drop_on_widget (GtkWidget * widget)
{
  gtk_drag_dest_set (widget,
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_DROP,
                     drag_types, n_drag_types,
                     GDK_ACTION_COPY);

  gtk_signal_connect (GTK_OBJECT (widget), "drag_data_received",
		      GTK_SIGNAL_FUNC (docindex_dnd_filenames_dropped),
		      NULL);
}

static void
docindex_cell_dnd_filenames_dropped (GtkWidget        *widget,
				     GdkDragContext   *context,
				     gint              x,
				     gint              y,
				     GtkSelectionData *selection_data,
				     guint             info,
				     guint             time)
{
  gint len;
  gchar *data;
  gchar *end;
  gint position = g_list_index (GTK_TREE (ideas->tree)->children, widget);

  switch (info)
    {
    case GIMP_DND_TYPE_URI_LIST:
      data = (gchar *) selection_data->data;
      len = selection_data->length;
      while (len > 0)
	{
	  end = strstr (data, "\x0D\x0A");
	  if (end != NULL)
	    *end = 0;
	  if (*data != '#')
	    {
	      gchar *filename = strchr (data, ':');
	      if (filename != NULL)
		filename ++;
	      else
		filename = data;
	      open_file_in_position (filename, position);
	    }
	  if (end)
	    {
	      len -= end - data + 2;
	      data = end + 2;
	    }
	  else
	    len = 0;
	}
      break;
    }
  return;
}

void
docindex_cell_configure_drop_on_widget (GtkWidget *widget)
{
  gtk_drag_dest_set (widget,
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_DROP,
                     drag_types, n_drag_types,
                     GDK_ACTION_COPY);

  gtk_signal_connect (GTK_OBJECT (widget), "drag_data_received",
		      GTK_SIGNAL_FUNC (docindex_cell_dnd_filenames_dropped),
		      NULL);
}

gboolean
idea_window_delete_event_callback (GtkWidget *widget,
				   GdkEvent  *event,
				   gpointer   data)
{
  save_idea_manager (ideas);
  create_idea_list ();
  g_free (ideas);
  ideas = 0;

  return FALSE;
}

void
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

static void
load_from_list (gpointer data,
		gpointer data_null)
{
  idea_add_in_position ((gchar *) data, -1);
}

FILE *
idea_manager_parse_init (void)
{
  FILE  *fp = NULL;
  gchar *desktopfile;

  desktopfile = gimp_personal_rc_file ("ideas");
  fp = fopen (desktopfile, "r");
  g_free (desktopfile);

  return fp;
}

gchar *
idea_manager_parse_line (FILE * fp)
{
  gint length;
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

void
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
    gtk_widget_show (ideas->window);
}

static void
save_to_ideas (gpointer data,
	       gpointer user_data)
{
  gchar *title = GTK_LABEL (GTK_BIN ((GtkWidget *) data)->child)->label;

  fprintf ((FILE *) user_data, "%d %s\n", strlen (title), title);
}

static void
save_list_to_ideas (gpointer data,
		    gpointer user_data)
{
  gchar *title = (gchar *) data;

  fprintf ((FILE *) user_data, "%d %s\n", strlen (title), title);
}

void
save_idea_manager (IdeaManager *ideas)
{
  FILE *fp;
  gchar *desktopfile;
    
  /* open persistant desktop file. */
  desktopfile = gimp_personal_rc_file ("ideas");
  fp = fopen (desktopfile, "w");
  g_free (desktopfile);

  if (fp)
    {
      if (ideas)
	{
	  g_list_foreach (GTK_TREE (ideas->tree)->children, save_to_ideas, fp);
	}
      else
	{
	  if (idea_list)
	    {
	      g_list_foreach (idea_list, save_list_to_ideas, fp);
	    }
	}

      fclose (fp);
    }
}

static void
save_to_list (gpointer data,
	      gpointer null_data)
{
  gchar *title = g_strdup (GTK_LABEL (GTK_BIN ((GtkWidget *) data)->child)->label);
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

  g_list_foreach (GTK_TREE (ideas->tree)->children, save_to_list, NULL);
}

static gint
open_or_raise_callback (GtkWidget      *widget,
			GdkEventButton *event,
			gpointer        func_data )
{
  if (GTK_IS_TREE_ITEM (widget) &&
      event->type == GDK_2BUTTON_PRESS)
    {
      open_or_raise (GTK_LABEL (GTK_BIN (widget)->child )->label);
    }

  return FALSE;
} 

void
document_index_create (void)
{
  if (ideas)
    gdk_window_raise (ideas->window->window);
  else
    open_idea_window ();
}

static void
check_needed (gpointer data,
	      gpointer user_data)
{
  GtkWidget *widget = (GtkWidget *) data;
  struct bool_char_pair *pair = (struct bool_char_pair *) user_data;

  if (strcmp (pair->string, GTK_LABEL (GTK_BIN (widget)->child )->label) == 0)
    {
      pair->boole = TRUE;
    }
}

static void
check_needed_list (gpointer data,
		   gpointer user_data)
{
  struct bool_char_pair *pair = (struct bool_char_pair *) user_data;

  if (strcmp (pair->string, (gchar *) data ) == 0)
    {
      pair->boole = TRUE;
    }
}

static void
idea_add_in_position_with_select (gchar    *title,
				  gint      position,
				  gboolean  select )
{
  GtkWidget *treeitem;
  struct bool_char_pair pair;

  pair.boole = FALSE;
  pair.string = title;

  if (ideas)
    {
      g_list_foreach (GTK_TREE (ideas->tree)->children, check_needed, &pair);

      if (! pair.boole)
	{
	  treeitem = gtk_tree_item_new_with_label (title);
	  if (position < 0)
	    gtk_tree_append (GTK_TREE (ideas->tree), treeitem);
	  else
	    gtk_tree_insert (GTK_TREE (ideas->tree), treeitem, position);

	  gtk_signal_connect (GTK_OBJECT (treeitem),
			      "button_press_event",
			      GTK_SIGNAL_FUNC (open_or_raise_callback),
			      NULL);

	  docindex_cell_configure_drop_on_widget (treeitem);

	  gtk_widget_show (treeitem);

	  if (select)
	    gtk_tree_select_item (GTK_TREE (ideas->tree),
				  gtk_tree_child_position (GTK_TREE (ideas->tree ), treeitem));
	}
    }
  else
    {
      if (! idea_list)
	{
	  FILE *fp = NULL;
	  gchar *desktopfile;

	  /* open persistant desktop file. */
	  desktopfile = gimp_personal_rc_file ("ideas");
	  fp = fopen (desktopfile, "r");
	  g_free (desktopfile);

	  /* Read in persistant desktop information. */
	  if (fp)
	    {  
	      gchar *title;
	      gint length;

	      clear_white (fp);

	      while (!feof (fp) && !ferror (fp))
		{
		  length = getinteger (fp);
		  title = g_malloc0 (length + 1);
		  title[fread (title, 1, length, fp)] = 0;
		  idea_list = g_list_append (idea_list, g_strdup (title));
		  g_free (title);
		  clear_white (fp);
		}
	      fclose (fp);
	    }
	}

      g_list_foreach (idea_list, check_needed_list, &pair);

      if (! pair.boole)
	{
	  if (position < 0)
	    idea_list = g_list_append (idea_list, g_strdup (title));
	  else
	    idea_list = g_list_insert (idea_list, g_strdup (title), position);
	}
    }
}

void
idea_add (gchar *title)
{
  idea_add_in_position (title, 0);
}

void
idea_add_in_position (gchar *title,
		      gint   position)
{
  idea_add_in_position_with_select (title, position, TRUE);
}

static gint
idea_move (GtkWidget *widget,
	   gint       distance,
	   gboolean   select)
{
  gint orig_position = g_list_index (GTK_TREE (ideas->tree)->children, widget);
  gint position = orig_position + distance;
  gchar *title;

  if (position < 0)
    position = 0;

  if (position >= g_list_length (GTK_TREE (ideas->tree)->children))
    position = g_list_length (GTK_TREE (ideas->tree)->children) - 1;

  if (position != orig_position)
    {
      title = g_strdup (GTK_LABEL (GTK_BIN (widget)->child )->label);
      gtk_container_remove (GTK_CONTAINER (ideas->tree), widget);
      idea_add_in_position_with_select (title, position, select);
      g_free (title); 
   }

  return position - orig_position;
}

static void
idea_remove (GtkWidget *widget)
{
  gint position = g_list_index (GTK_TREE (ideas->tree )->children, widget);

  gtk_container_remove (GTK_CONTAINER (ideas->tree), widget);

  if (g_list_length (GTK_TREE (ideas->tree)->children) - 1 < position)
    position = g_list_length (GTK_TREE (ideas->tree)->children) - 1;

  gtk_tree_select_item (GTK_TREE (ideas->tree), position);  
}

void
idea_up_callback (GtkWidget *widget,
		  gpointer   data)
{
  GtkWidget *selected;

  if (GTK_TREE (ideas->tree)->selection)
    {
      selected = GTK_TREE (ideas->tree)->selection->data;
      idea_move (selected, -1, TRUE);
    }
}

void
idea_down_callback (GtkWidget *widget,
		    gpointer   data)
{
  GtkWidget *selected;

  if (GTK_TREE (ideas->tree)->selection)
    {
      selected = GTK_TREE (ideas->tree)->selection->data;
      idea_move (selected, 1, TRUE);
    }
}

void
idea_remove_callback (GtkWidget *widget,
		      gpointer   data)
{
  GtkWidget *selected;

  if (GTK_TREE (ideas->tree)->selection)
    {
      selected = GTK_TREE (ideas->tree)->selection->data;
      idea_remove (selected);
    }
}

void
close_idea_window (void)
{
  idea_hide_callback (NULL, NULL);
}
