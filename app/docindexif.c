/* docindexif.c - Interface file for the docindex for gimp.
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

#include <gtk/gtk.h>

#include "dialog_handler.h"
#include "docindex.h"
#include "docindexif.h"
#include "fileops.h"
#include "gimage.h"
#include "gimphelp.h"
#include "session.h"

#include "libgimp/gimpintl.h"


static void
raise_if_match (gpointer data,
		gpointer user_data)
{
  GimpImage *gimage = GIMP_IMAGE (data);

  struct bool_char_pair *pair = (struct bool_char_pair *) user_data;

  if ((! pair->boole) && gimage->has_filename)
    if (strcmp (pair->string, gimage->filename) == 0)
      {
	pair->boole = TRUE;
	/* gdk_raise_window( NULL, gimage-> ); */  /* FIXME */
      }
}

void
open_or_raise (gchar *file_name)
{
  struct bool_char_pair pair;

  pair.boole  = FALSE;
  pair.string = file_name;

  gimage_foreach (raise_if_match, &pair);
  
  if (! pair.boole)
    {
      file_open (file_name, file_name);
    }
}

void
open_file_in_position (gchar *filename,
		       gint   position)
{
  file_open (filename, filename);
}

static GtkWidget *
create_idea_toolbar (void)
{
  GtkWidget *toolbar;

  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("Open"), _("Open a file"), "Toolbar/Open",
			   NULL,
			   (GtkSignalFunc) file_open_callback, NULL);

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("Up"), _("Move the selected entry up in the index"), "Toolbar/Up",
			   NULL,
			   (GtkSignalFunc) idea_up_callback, NULL);

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("Down"), _("Move the selected entry down in the index"), "Toolbar/Down",
			   NULL,
			   (GtkSignalFunc) idea_down_callback, NULL );

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("Remove"), _("Remove the selected entry from the index"), "Toolbar/Remove",
			   NULL,
			   (GtkSignalFunc) idea_remove_callback, NULL );

  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("Close"), _("Close the Document Index"), "Toolbar/Hide",
			   NULL,
			   (GtkSignalFunc) idea_hide_callback, NULL );

  return toolbar;
}

gint
getinteger (FILE *fp)
{
  gchar nextchar;
  gint response = 0;
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

void
clear_white (FILE *fp)
{
  gint nextchar;

  while (isspace (nextchar = fgetc (fp)))
    /* empty statement */ ;
  if (nextchar != EOF)
    ungetc (nextchar, fp);
}

void
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

  /* Setup tree */
  ideas->tree = gtk_tree_new ();
  gtk_tree_set_selection_mode (GTK_TREE (ideas->tree), GTK_SELECTION_BROWSE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win),
					 ideas->tree);
  gtk_widget_show (ideas->tree);

  docindex_configure_drop_on_widget (ideas->tree);

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
