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

#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "docindex.h"
#include "docindexif.h"

idea_manager *ideas = NULL;
static GList *idea_list = NULL;   /* of gchar *. */
static gint x = 0, y = 0, width = 0, height = 0;


enum {
  TARGET_URI_LIST
};

static void create_idea_list( void );
static void load_idea_manager( idea_manager * );

static void
docindex_dnd_filenames_dropped( GtkWidget *widget,
				GdkDragContext *context,
				gint x,
				gint y,
				GtkSelectionData *selection_data,
				guint info,
				guint time)
{
  gint len;
  gchar *data;
  gchar *end;

  switch ( info )
    {
    case TARGET_URI_LIST:
      data = (gchar *) selection_data->data;
      len = selection_data->length;
      while( len > 0 )
	{
	  end = strstr( data, "\x13\x10" );
	  if ( end != NULL )
	    *end = 0;
	  if ( *data != '#' )
	    {
	      gchar *filename = strchr( data, ':' );
	      if ( filename != NULL )
		filename ++;
	      else
		filename = data;
	      open_file_in_position( filename, -1 );
	    }
	  if ( end )
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
docindex_configure_drop_on_widget(GtkWidget * widget)
{
  static GtkTargetEntry drag_types[] =
  {
    { "text/uri-list", 0, TARGET_URI_LIST },
  };
  static gint n_drag_types = sizeof(drag_types)/sizeof(drag_types[0]);

  gtk_drag_dest_set (widget,
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_DROP,
                     drag_types, n_drag_types,
                     GDK_ACTION_COPY);

  gtk_signal_connect(GTK_OBJECT(widget), "drag_data_received",
                       GTK_SIGNAL_FUNC(docindex_dnd_filenames_dropped), NULL);
}

static void
docindex_cell_dnd_filenames_dropped( GtkWidget *widget,
				     GdkDragContext *context,
				     gint x,
				     gint y,
				     GtkSelectionData *selection_data,
				     guint info,
				     guint time)
{
  gint len;
  gchar *data;
  gchar *end;
  gint position = g_list_index( GTK_TREE( ideas->tree )->children, widget );

  switch ( info )
    {
    case TARGET_URI_LIST:
      data = (gchar *) selection_data->data;
      len = selection_data->length;
      while( len > 0 )
	{
	  end = strstr( data, "\x13\x10" );
	  if ( end != NULL )
	    *end = 0;
	  if ( *data != '#' )
	    {
	      gchar *filename = strchr( data, ':' );
	      if ( filename != NULL )
		filename ++;
	      else
		filename = data;
	      open_file_in_position( filename, position );
	    }
	  if ( end )
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
docindex_cell_configure_drop_on_widget(GtkWidget * widget)
{
  static GtkTargetEntry drag_types[] =
  {
    { "text/uri-list", 0, TARGET_URI_LIST },
  };
  static gint n_drag_types = sizeof(drag_types)/sizeof(drag_types[0]);

  gtk_drag_dest_set (widget,
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_DROP,
                     drag_types, n_drag_types,
                     GDK_ACTION_COPY);

  gtk_signal_connect(GTK_OBJECT(widget), "drag_data_received",
                       GTK_SIGNAL_FUNC(docindex_cell_dnd_filenames_dropped), NULL);
}

static gboolean
idea_window_delete_event_callback( GtkWidget *widget, GdkEvent *event, gpointer data )
{
  if ( ! exit_from_go() )
    {
      save_idea_manager( ideas );
      create_idea_list();
      free( ideas );
      ideas = 0;
    }
  
  return FALSE;
}

void
idea_hide_callback( GtkWidget *widget, gpointer data )
{
  if ( ideas || idea_list || width || height )
    save_idea_manager( ideas );

  /* False if exitting */
  if( ( ! exit_from_go() ) && ideas )
    {
      create_idea_list();
      gtk_widget_destroy( ideas->window );
      free( ideas );
      ideas = 0;
    }
}

void
open_idea_window( void )
{
  make_idea_window( -1, -1 );
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

static void
load_from_list( gpointer data, gpointer data_null )
{
  idea_add_in_position( (gchar *) data, -1 );
}

static void
load_idea_manager( idea_manager *ideas )
{
  FILE *fp = NULL;
  gchar *desktopfile;
  gchar *home_dir;

  if ( ! idea_list )
    {
      home_dir = getenv( "HOME" );
      
      /* open persistant desktop file. */
      desktopfile = append2( home_dir, FALSE, IDEAPATH, FALSE );
      fp = fopen( desktopfile, "rb" );
      g_free( desktopfile );
      
      /* Read in persistant desktop information. */
      if ( fp )
	{	  
	  x = getinteger( fp );
	  y = getinteger( fp );
	  width = getinteger( fp );
	  height = getinteger( fp );
	}
    }
  
  if ( idea_list || fp )
    {
      gtk_widget_set_usize( ideas->window, width, height );
      gtk_widget_show( ideas->window );
      gtk_widget_set_uposition( ideas->window, x, y );
      gtk_idle_add( reset_usize, ideas->window );
      if( fp )
	{
	  gchar *title;
	  gint length;
	  clear_white( fp );
	  
	  while ( ! feof( fp ) && !ferror( fp ) )
	    {
	      length = getinteger( fp );
	      title = g_malloc0( length + 1 );
	      title[fread( title, 1, length, fp )] = 0;
	      idea_add_in_position( title, -1 );
	      g_free( title );
	      clear_white( fp );
	    }
	  fclose( fp );
	}
      else
	{
	  g_list_foreach( idea_list, load_from_list, NULL );
	  g_list_foreach( idea_list, (GFunc) g_free, NULL );
	  g_list_free( idea_list );
	  idea_list = 0;
	}
    }
  else
    gtk_widget_show( ideas->window );
}

static void
save_to_ideas( gpointer data, gpointer user_data )
{
  gchar *title = GTK_LABEL( GTK_BIN( (GtkWidget *) data )->child )->label;
  
  fprintf( (FILE *) user_data, "%d %s\n", strlen( title ), title );
}

static void
save_list_to_ideas( gpointer data, gpointer user_data )
{
  gchar *title = (gchar *) data;
  
  fprintf( (FILE *) user_data, "%d %s\n", strlen( title ), title );
}

void
save_idea_manager( idea_manager *ideas )
{
  FILE *fp;
  gchar *desktopfile;
  gchar *home_dir;
    
  home_dir = getenv( "HOME" );
  
  /* open persistant desktop file. */
  desktopfile = append2( home_dir, FALSE, IDEAPATH, FALSE );
  fp = fopen( desktopfile, "wb" );
  g_free( desktopfile );

  if ( fp )
    {
      if ( ideas )
	{
	  int x, y, width, height;
	  
	  gdk_window_get_geometry( ideas->window->window, &x, &y, &width, &height, NULL );
	  gdk_window_get_origin( ideas->window->window, &x, &y );
	  
	  fprintf( fp, "%d %d %d %d\n", x, y, width, height );
	  
	  g_list_foreach( GTK_TREE( ideas->tree )->children, save_to_ideas, fp );
	}
      else
	{
	  if ( idea_list )
	    {
	      fprintf( fp, "%d %d %d %d\n", x, y, width, height );
	      
	      g_list_foreach( idea_list, save_list_to_ideas, fp );
	    }
	}
      
      fclose( fp );
    }
}

static void
save_to_list( gpointer data, gpointer null_data )
{
  gchar *title = g_strdup( GTK_LABEL( GTK_BIN( (GtkWidget *) data )->child )->label );
  idea_list = g_list_append( idea_list, title );
}

static void
create_idea_list( void )
{
  gdk_window_get_geometry( ideas->window->window, &x, &y, &width, &height, NULL );
  gdk_window_get_origin( ideas->window->window, &x, &y );

  if( idea_list )
    {
      g_list_foreach( idea_list, (GFunc) g_free, NULL );
      g_list_free( idea_list );
      idea_list = 0;
    }
  
  g_list_foreach( GTK_TREE( ideas->tree )->children, save_to_list, NULL );
}

static gint
open_or_raise_callback( GtkWidget *widget, GdkEventButton *event, gpointer func_data )
{
  if ( GTK_IS_TREE_ITEM( widget ) &&
       event->type==GDK_2BUTTON_PRESS )
    {
      open_or_raise( GTK_LABEL( GTK_BIN( widget )->child )->label );
    }

  return FALSE;
} 

void raise_idea_callback( GtkWidget *widget, gpointer data )
{
  if ( ideas )
    gdk_window_raise( ideas->window->window );
  else
    open_idea_window();
}

static void check_needed( gpointer data, gpointer user_data )
{
  GtkWidget *widget = (GtkWidget *) data;
  struct bool_char_pair *pair = (struct bool_char_pair *) user_data;
  
  if ( strcmp( pair->string, GTK_LABEL( GTK_BIN( widget )->child )->label ) == 0 )
    {
      pair->boole = TRUE;
    }
}

static void check_needed_list( gpointer data, gpointer user_data )
{
  struct bool_char_pair *pair = (struct bool_char_pair *) user_data;
  
  if ( strcmp( pair->string, (gchar *) data ) == 0 )
    {
      pair->boole = TRUE;
    }
}

void idea_add( gchar *title )
{
  idea_add_in_position( title, 0 );
}

static void idea_add_in_position_with_select( gchar *title, gint position, gboolean select )
{
  GtkWidget *treeitem;
  struct bool_char_pair pair;

  pair.boole = FALSE;
  pair.string = title;
      
  if ( ideas )
    {
      g_list_foreach( GTK_TREE( ideas->tree )->children, check_needed, &pair );
      
      if ( ! pair.boole )
	{
	  treeitem = gtk_tree_item_new_with_label( title );
	  if ( position < 0 )
	    gtk_tree_append( GTK_TREE( ideas->tree ), treeitem );
	  else
	    gtk_tree_insert( GTK_TREE( ideas->tree ), treeitem, position );
	  
	  gtk_signal_connect( GTK_OBJECT( treeitem ),
			      "button_press_event",
			      GTK_SIGNAL_FUNC( open_or_raise_callback ),
			      NULL );
	  
	  docindex_cell_configure_drop_on_widget( treeitem );
	  
	  gtk_widget_show( treeitem );
	  
	  if ( select )
	    gtk_tree_select_item( GTK_TREE( ideas->tree ), gtk_tree_child_position( GTK_TREE( ideas->tree ), treeitem ) );
	}
    }
  else
    {
      if( ! idea_list )
	{
	  FILE *fp = NULL;
	  gchar *desktopfile;
	  gchar *home_dir;
	  
	  home_dir = getenv( "HOME" );
	  
	  /* open persistant desktop file. */
	  desktopfile = append2( home_dir, FALSE, IDEAPATH, FALSE );
	  fp = fopen( desktopfile, "rb" );
	  g_free( desktopfile );
	  
	  /* Read in persistant desktop information. */
	  if ( fp )
	    {	  
	      gchar *title;
	      gint length;

	      x = getinteger( fp );
	      y = getinteger( fp );
	      width = getinteger( fp );
	      height = getinteger( fp );

	      clear_white( fp );
	  
	      while ( ! feof( fp ) && !ferror( fp ) )
		{
		  length = getinteger( fp );
		  title = g_malloc0( length + 1 );
		  title[fread( title, 1, length, fp )] = 0;
		  idea_list = g_list_append( idea_list, g_strdup( title ) );
		  g_free( title );
		  clear_white( fp );
		}
	      fclose( fp );
	    }
	}

      g_list_foreach( idea_list, check_needed_list, &pair );
      
      if ( ! pair.boole )
	{
	  if ( position < 0 )
	    idea_list = g_list_append( idea_list, g_strdup( title ) );
	  else
	    idea_list = g_list_insert( idea_list, g_strdup( title ), position );
	}
    }
}

void idea_add_in_position( gchar *title, gint position )
{
  idea_add_in_position_with_select( title, position, TRUE );
}

static gint idea_move( GtkWidget *widget, gint distance, gboolean select )
{
  gint orig_position = g_list_index( GTK_TREE( ideas->tree )->children, widget );
  gint position = orig_position + distance;
  gchar *title;
			  
  if( position < 0 )
    position = 0;
  if ( position >= g_list_length( GTK_TREE( ideas->tree )->children ) )
    position = g_list_length( GTK_TREE( ideas->tree )->children ) - 1;
  if ( position != orig_position )
    {
      title = g_strdup( GTK_LABEL( GTK_BIN( widget )->child )->label );
      gtk_container_remove( GTK_CONTAINER( ideas->tree ), widget );
      idea_add_in_position_with_select( title, position, select );
      g_free( title ); 
   }
  return position - orig_position;
}

static void idea_remove( GtkWidget *widget )
{
  gint position = g_list_index( GTK_TREE( ideas->tree )->children, widget );
  gtk_container_remove( GTK_CONTAINER( ideas->tree ), widget );
  if ( g_list_length( GTK_TREE( ideas->tree )->children ) - 1 < position )
    position = g_list_length( GTK_TREE( ideas->tree )->children ) - 1;
  gtk_tree_select_item( GTK_TREE( ideas->tree ), position );  
}

void idea_up_callback( GtkWidget *widget, gpointer data )
{
  GtkWidget *selected;
  if ( GTK_TREE( ideas->tree )->selection )
    {
      selected = GTK_TREE( ideas->tree )->selection->data;
      if ( idea_move( selected, -1, TRUE ) != -1 )
	gtk_statusbar_push( GTK_STATUSBAR( ideas->status ), ideas->contextid, _("This file cannot be moved up.") );
    }
  else
    gtk_statusbar_push( GTK_STATUSBAR( ideas->status ), ideas->contextid, _("There's no selection to move up.") );
}

void idea_down_callback( GtkWidget *widget, gpointer data )
{
  GtkWidget *selected;
  if ( GTK_TREE( ideas->tree )->selection )
    {
      selected = GTK_TREE( ideas->tree )->selection->data;
      if ( idea_move( selected, 1, TRUE ) != 1 )
	gtk_statusbar_push( GTK_STATUSBAR( ideas->status ), ideas->contextid, _("This file cannot be moved down.") );
    }
  else
    gtk_statusbar_push( GTK_STATUSBAR( ideas->status ), ideas->contextid, _("There's no selection to move down.") );
}

void idea_remove_callback( GtkWidget *widget, gpointer data )
{
  GtkWidget *selected;
  if ( GTK_TREE( ideas->tree )->selection )
    {
      selected = GTK_TREE( ideas->tree )->selection->data;
      idea_remove( selected );
    }
  else
    gtk_statusbar_push( GTK_STATUSBAR( ideas->status ), ideas->contextid, _("There's no selection to remove.") );
}

void
close_idea_window( void )
{
  idea_hide_callback( NULL, NULL );
}

