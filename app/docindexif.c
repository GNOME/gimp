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


#include "docindex.h"
#include "docindexif.h"

void
raise_if_match( gpointer data, gpointer user_data )
{
  GimpImage *gimage = GIMP_IMAGE (data);
  struct bool_char_pair *pair = (struct bool_char_pair *) user_data;
  if ( ( ! pair->boole ) && gimage->has_filename)
    if ( strcmp( pair->string, gimage->filename ) == 0 )
      {
       pair->boole = TRUE;
       /*      gdk_raise_window( NULL, gimage-> ); */  /* FIXME */
      }
}

void
open_or_raise( gchar *file_name )
{
  struct bool_char_pair pair;
  
  pair.boole = FALSE;
  pair.string = file_name;
  
  gimage_foreach( raise_if_match, &pair );
  
  if ( ! pair.boole )
    {
      file_open( file_name, file_name );
    }
}

gboolean
exit_from_go()
{
  return FALSE;
}

void open_file_in_position( gchar *filename, gint position )
{
  file_open( filename, filename );
}

GtkMenuFactory *create_idea_menu()
{
  return NULL;
}

GtkWidget *create_idea_toolbar()
{
  GtkWidget *toolbar;

  toolbar = gtk_toolbar_new( GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
  gtk_widget_show( toolbar );
  gtk_toolbar_set_button_relief( GTK_TOOLBAR( toolbar ), GTK_RELIEF_NONE );
  
  gtk_toolbar_append_item( GTK_TOOLBAR( toolbar ),
			   _("Open"), _("Open a file"), "Toolbar/Open",
			   NULL,
			   (GtkSignalFunc) file_open_callback, NULL);
  
  gtk_toolbar_append_item( GTK_TOOLBAR( toolbar ),
			   _("Up"), _("Move the selected entry up in the index"), "Toolbar/Up",
			   NULL,
			   (GtkSignalFunc) idea_up_callback, NULL);
  
  gtk_toolbar_append_item( GTK_TOOLBAR( toolbar ),
			   _("Down"), _("Move the selected entry down in the index"), "Toolbar/Down",
			   NULL,
			   (GtkSignalFunc) idea_down_callback, NULL );
  
  gtk_toolbar_append_item( GTK_TOOLBAR( toolbar ),
			   _("Remove"), _("Remove the selected entry from the index"), "Toolbar/Remove",
			   NULL,
			   (GtkSignalFunc) idea_remove_callback, NULL );
  
  gtk_toolbar_append_item( GTK_TOOLBAR( toolbar ),
			   _("Close"), _("Close the Document Index"), "Toolbar/Hide",
			   NULL,
			   (GtkSignalFunc) idea_hide_callback, NULL );
  return toolbar;
}

gchar *append2( gchar *string1, gboolean del1, gchar *string2, gboolean del2)
{
  gchar *newstring = g_strconcat( string1, string2, NULL );
  if ( del1 )
    g_free( string1 );
  if ( del2 )
    g_free( string2 );
  return newstring;
}

int getinteger( FILE *fp )
{
  gchar nextchar;
  int response = 0;
  gboolean negative = FALSE;

  while ( isspace( nextchar = fgetc( fp ) ) )
    /* empty statement */ ;

  if ( nextchar == '-' )
    {
      negative = TRUE;
      while ( isspace( nextchar = fgetc( fp ) ) )
	/* empty statement */ ;
    }

  for ( ; '0' <= nextchar && '9' >= nextchar; nextchar = fgetc( fp ) )
    {
      response *= 10;
      response += nextchar - '0';
    }
  for ( ; isspace( nextchar ); nextchar = fgetc( fp ) )
    /* empty statement */ ;
  ungetc( nextchar, fp );
  if ( negative )
    response = -response;
  return response;
}

void clear_white( FILE *fp )
{
  int nextchar;

  while ( isspace( nextchar = fgetc( fp ) ) )
    /* empty statement */ ;
  if (nextchar != EOF)
    ungetc( nextchar, fp );
}

/* reset_usize
 *  A callback so that the window can be resized smaller. */
gint reset_usize( gpointer data )
{
  gtk_widget_set_usize( GTK_WIDGET( data ), 0, 0 );
  return FALSE;
}

void
make_idea_window( int x, int y )
{
  GtkWidget *main_vbox, *menu;
  GtkWidget *scrolled_win;
  GtkWidget *toolbar;
  GtkAccelGroup *accel;

  /* malloc idea_manager */
  ideas = g_malloc0( sizeof( idea_manager ) );

  /* Setup tree */
  ideas->tree = gtk_tree_new();
  gtk_tree_set_selection_mode( GTK_TREE( ideas->tree ), GTK_SELECTION_BROWSE );
  
  /* Setup scrolled window */
  scrolled_win = gtk_scrolled_window_new( NULL, NULL );
  gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolled_win ), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS );
  gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW( scrolled_win ), ideas->tree );
  gtk_widget_show( ideas->tree );

  /* allocate the window and attach the menu */
  ideas->window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  ideas->menubar = create_idea_menu( ideas );
  if( ideas->menubar )
    {
      menu = ideas->menubar->widget;
      /* Setup accelerator (hotkey) table */
      accel = ideas->menubar->accel_group;
      
      /* Add accelerators to window widget */
      gtk_window_add_accel_group( GTK_WINDOW( ideas->window ), accel );
    }
  else menu = NULL;
  
  /* Setup the status bar */
  ideas->status = gtk_statusbar_new();
  ideas->contextid = gtk_statusbar_get_context_id( GTK_STATUSBAR( ideas->status ), "main context" );

  /* Setup the toolbar */
  toolbar = create_idea_toolbar();
  
  /* Setup a vbox to contain the menu */
  main_vbox = gtk_vbox_new( FALSE, 0 );
  if( menu )
    gtk_box_pack_start( GTK_BOX( main_vbox ), menu, FALSE, FALSE, 0 );
  gtk_box_pack_start( GTK_BOX( main_vbox ), toolbar, FALSE, FALSE, 0 );
  gtk_box_pack_start( GTK_BOX( main_vbox ), scrolled_win, TRUE, TRUE, 0 ); 
  gtk_box_pack_start( GTK_BOX( main_vbox ), ideas->status, FALSE, FALSE, 0 );
  if( menu )
    gtk_widget_show( menu );
  gtk_widget_show( scrolled_win );
  gtk_widget_show( ideas->status );

  /* Set the GOWindow title */
  ideas->title = g_strdup( _("Document Index") );

  /* Set the GtkWindow title */
  gtk_window_set_title( GTK_WINDOW( ideas->window ), ideas->title );

  /* Set the initial status message */
  gtk_statusbar_push( GTK_STATUSBAR( ideas->status ), ideas->contextid,  _("GTK successfully started") );

  /* Connect the signals */
  gtk_signal_connect( GTK_OBJECT( ideas->window ), "delete_event",
		      GTK_SIGNAL_FUNC( idea_window_delete_event_callback ),
		      NULL );
  
  /* Add the main vbox to the window */
  gtk_container_set_border_width( GTK_CONTAINER( ideas->window ), 0 );
  gtk_container_add( GTK_CONTAINER( ideas->window ), main_vbox );
  gtk_widget_show( main_vbox );

  docindex_configure_drop_on_widget(ideas->tree);

  /* Load and Show window */
  load_idea_manager( ideas );

  /* Set the position of the window if it was requested */
  if ( x >= 0 && y >= 0 )
    gtk_widget_set_uposition( ideas->window, x, y );
}
