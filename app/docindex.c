/* docindex.c - Creates the window used by the document index in gimp.
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
#include <gdk/gdkprivate.h>

#include "gimage.h"
#include "fileops.h"
#include "file_new_dialog.h"
#include "commands.h"
#include "docindex.h"

static void idea_up_callback( GtkWidget *widget, gpointer data );
static void idea_down_callback( GtkWidget *widget, gpointer data );
static void idea_remove_callback( GtkWidget *widget, gpointer data );
static void idea_hide_callback( GtkWidget *widget, gpointer data );
static void load_idea_manager( idea_manager * );
static void save_idea_manager( idea_manager * );

idea_manager *ideas;

static char *image_drop_types[] = {"url:ALL"};

GtkWidget *create_idea_toolbar()
{
  GtkWidget *wpixmap;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkWidget *toolbar;

  toolbar = gtk_toolbar_new( GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
  gtk_widget_show( toolbar );
  gtk_toolbar_set_button_relief( GTK_TOOLBAR( toolbar ), GTK_RELIEF_NONE );
  
  pixmap = gdk_pixmap_create_from_xpm( (GdkWindow *)&gdk_root_parent, &mask,
				       &toolbar->style->bg[GTK_STATE_NORMAL],
				       DATADIR "/go/tb_up_arrow.xpm" );
  wpixmap = gtk_pixmap_new( pixmap, mask );
  
  gtk_toolbar_append_item( GTK_TOOLBAR( toolbar ),
			   "Up", "Move the selected entry up in the index", "Toolbar/Up",
			   wpixmap,
			   (GtkSignalFunc) idea_up_callback, NULL);
  
  pixmap = gdk_pixmap_create_from_xpm( (GdkWindow *)&gdk_root_parent, &mask,
				       &toolbar->style->bg[GTK_STATE_NORMAL],
				       DATADIR "/go/tb_down_arrow.xpm" );
  wpixmap = gtk_pixmap_new( pixmap, mask );
  
  gtk_toolbar_append_item( GTK_TOOLBAR( toolbar ),
			   "Down", "Move the selected entry down in the index", "Toolbar/Down",
			   wpixmap,
			   (GtkSignalFunc) idea_down_callback, NULL );
  
  pixmap = gdk_pixmap_create_from_xpm( (GdkWindow *)&gdk_root_parent, &mask,
				       &toolbar->style->bg[GTK_STATE_NORMAL],
				       DATADIR "/go/cancel.xpm" );
  wpixmap = gtk_pixmap_new( pixmap, mask );
  
  gtk_toolbar_append_item( GTK_TOOLBAR( toolbar ),
			   "Remove", "Remove the selected entry from the index", "Toolbar/Remove",
			   wpixmap,
			   (GtkSignalFunc) idea_remove_callback, NULL );
  
  pixmap = gdk_pixmap_create_from_xpm( (GdkWindow *)&gdk_root_parent, &mask,
				       &toolbar->style->bg[GTK_STATE_NORMAL],
				       DATADIR "/go/tb_exit.xpm" );
  wpixmap = gtk_pixmap_new( pixmap, mask );
  
  gtk_toolbar_append_item( GTK_TOOLBAR( toolbar ),
			   "Hide", "Hide the Document Index", "Toolbar/Hide",
			   wpixmap,
			   (GtkSignalFunc) idea_hide_callback, NULL );
  return toolbar;
}

#define IDEA_FILE_ITEMS (sizeof( idea_menu ) / sizeof( GtkMenuEntry ))

static GtkMenuEntry idea_menu [] =
{
  { "<Main>/File/New", "<control>N", file_new_cmd_callback, NULL },
  { "<Main>/File/Open...", "<control>O", file_open_callback, NULL },
  { "<Main>/File/<separator>", NULL, NULL, NULL },
  { "<Main>/File/Hide Index", "<control>W",idea_hide_callback, NULL },
  { "<Main>/File/Quit", "<control>Q", file_quit_cmd_callback, NULL },
  { "<Main>/Help/About...", NULL, about_dialog_cmd_callback, NULL }
};    

GtkMenuFactory *create_idea_menu()
{
  int i;
  GtkMenuFactory *factory;
  GtkMenuFactory *subfactory;
  GtkMenuEntry *current_menu_bar = g_malloc0( sizeof( idea_menu ) );

  for ( i = 0; i < IDEA_FILE_ITEMS; i++ )
    {
      current_menu_bar[i] = idea_menu[i];
    }

  factory = gtk_menu_factory_new( GTK_MENU_FACTORY_MENU_BAR );
  subfactory = gtk_menu_factory_new( GTK_MENU_FACTORY_MENU_BAR );
  gtk_menu_factory_add_subfactory( factory, subfactory, "<Main>" );
  gtk_menu_factory_add_entries( factory, current_menu_bar, IDEA_FILE_ITEMS );

  return subfactory;
}

gint reset_usize( gpointer data )
{
  gtk_widget_set_usize( GTK_WIDGET( data ), 0, 0 );
  return FALSE;
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
  gchar nextchar;

  while ( isspace( nextchar = fgetc( fp ) ) )
    /* empty statement */ ;
  ungetc( nextchar, fp );
}

static gchar *append2( gchar *string1, gboolean del1, gchar *string2, gboolean del2)
{
  gchar *newstring = g_malloc( strlen( string1 ) + strlen( string2 ) + 1 );
  sprintf( newstring, "%s%s", string1, string2 );
  if ( del1 )
    g_free( string1 );
  if ( del2 )
    g_free( string2 );
  return newstring;
}

static void
idea_dnd_drop_data_available_callback(GtkWidget *widget, GdkEventDropDataAvailable *event, gpointer user_data)
{
  gint len = event->data_numbytes;
  gchar *data = event->data; 
 
  /* Test for the type that was dropped */
  if (strcmp (event->data_type, "url:ALL") == 0)
    {
      while( len > 0 )
	{
	  file_open( data, data );
	  len -= ( strlen( data ) + 1 );
	  data += strlen( data ) + 1;
	}
    }
  return;
}

static void
idea_cell_dnd_drop_data_available_callback(GtkWidget *widget, GdkEventDropDataAvailable *event, gpointer user_data)
{
  gint len = event->data_numbytes;
  gchar *data = event->data;
  
  /* Test for the type that was dropped */
  if (strcmp (event->data_type, "url:ALL") == 0)
    {
      while( len > 0 )
	{
	  file_open( data, data );
	  len -= ( strlen( data ) + 1 );
	  data += strlen( data ) + 1;
	}
    }
  return;
}

static gboolean
idea_window_delete_event_callback( GtkWidget *widget, GdkEvent *event, gpointer data )
{
  save_idea_manager( ideas );
  free( ideas );
  ideas = 0;
  return FALSE;
}

static void
idea_hide_callback( GtkWidget *widget, gpointer data )
{
  save_idea_manager( ideas );
  gtk_widget_destroy( ideas->window );
  free( ideas );
  ideas = 0;
}

void
open_idea_window()
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
  gtk_container_add( GTK_CONTAINER( scrolled_win ), ideas->tree );
  gtk_widget_show( ideas->tree );

  /* allocate the window and attach the menu */
  ideas->window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  ideas->menubar = create_idea_menu( ideas );
  menu = ideas->menubar->widget;
  /* Setup accelerator (hotkey) table */
  accel = ideas->menubar->accel_group;
  /* Setup the window_menu widget for additions and removals */
  ideas->window_menu = GTK_WIDGET( GTK_MENU_ITEM( g_list_nth( GTK_MENU_SHELL( menu )->children, 1 )->data )->submenu );

  /* Add accelerators to window widget */
  gtk_window_add_accel_group( GTK_WINDOW( ideas->window ), accel );

  /* Setup the status bar */
  ideas->status = gtk_statusbar_new();
  ideas->contextid = gtk_statusbar_get_context_id( GTK_STATUSBAR( ideas->status ), "main context" );

  /* Setup the toolbar */
  toolbar = create_idea_toolbar();
  
  /* Setup a vbox to contain the menu */
  main_vbox = gtk_vbox_new( FALSE, 0 );
  gtk_box_pack_start( GTK_BOX( main_vbox ), menu, FALSE, FALSE, 0 );
  gtk_box_pack_start( GTK_BOX( main_vbox ), toolbar, FALSE, FALSE, 0 );
  gtk_box_pack_start( GTK_BOX( main_vbox ), scrolled_win, TRUE, TRUE, 0 ); 
  gtk_box_pack_start( GTK_BOX( main_vbox ), ideas->status, FALSE, FALSE, 0 );
  gtk_widget_show( menu );
  gtk_widget_show( scrolled_win );
  gtk_widget_show( ideas->status );

  /* Set the GOWindow title */
  ideas->title = g_strdup( "Document Index" );

  /* Set the GtkWindow title */
  gtk_window_set_title( GTK_WINDOW( ideas->window ), ideas->title );

  /* Set the initial status message */
  gtk_statusbar_push( GTK_STATUSBAR( ideas->status ), ideas->contextid,  "GTK successfully started" );

  /* Connect the signals */
  gtk_signal_connect( GTK_OBJECT( ideas->window ), "delete_event",
		      GTK_SIGNAL_FUNC( idea_window_delete_event_callback ),
		      NULL );
  
  /* Add the main vbox to the window */
  gtk_container_border_width( GTK_CONTAINER( ideas->window ), 0 );
  gtk_container_add( GTK_CONTAINER( ideas->window ), main_vbox );
  gtk_widget_show( main_vbox );

  gtk_signal_connect( GTK_OBJECT( ideas->tree ),
		      "drop_data_available_event",
		      GTK_SIGNAL_FUNC( idea_dnd_drop_data_available_callback ),
		      NULL );

  /* Load and Show window */
  load_idea_manager( ideas );

  /* Set the position of the window if it was requested */
  if ( x >= 0 && y >= 0 )
    gtk_widget_set_uposition( ideas->window, x, y );

  gtk_widget_dnd_drop_set( ideas->tree, TRUE, image_drop_types, 1, FALSE );
}

static void
load_idea_manager( idea_manager *ideas )
{
  FILE *fp;
  gchar *desktopfile;
  gchar *home_dir;
    
  home_dir = getenv( "HOME" );
  
  /* open persistant desktop file. */
  desktopfile = append2( home_dir, FALSE, "/" GIMPDIR "/ideas", FALSE );
  fp = fopen( desktopfile, "r" );
  g_free( desktopfile );
  
  /* Read in persistant desktop information. */
  if ( fp )
    {
      gint x, y, width, height, length;
      gchar *title;
      
      x = getinteger( fp );
      y = getinteger( fp );
      width = getinteger( fp );
      height = getinteger( fp );

      gtk_widget_set_usize( ideas->window, width, height );
      gtk_widget_show( ideas->window );
      gtk_widget_set_uposition( ideas->window, x, y );
      gtk_idle_add( reset_usize, ideas->window );
      
      clear_white( fp );

      while ( ! feof( fp ) )
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
      gtk_widget_show( ideas->window );
    }
}

static void
save_to_ideas( gpointer data, gpointer user_data )
{
  gchar *title = GTK_LABEL( GTK_BIN( (GtkWidget *) data )->child )->label;
  
  fprintf( (FILE *) user_data, "%d %s\n", strlen( title ), title );
}

static void
save_idea_manager( idea_manager *ideas )
{
  FILE *fp;
  gchar *desktopfile;
  gchar *home_dir;
  int x, y, width, height;
    
  home_dir = getenv( "HOME" );
  
  /* open persistant desktop file. */
  desktopfile = append2( home_dir, FALSE, "/" GIMPDIR "/ideas", FALSE );
  fp = fopen( desktopfile, "w" );
  g_free( desktopfile );

  gdk_window_get_geometry( ideas->window->window, &x, &y, &width, &height, NULL );
  gdk_window_get_origin( ideas->window->window, &x, &y );
  
  fprintf( fp, "%d %d %d %d\n", x, y, width, height );

  g_list_foreach( GTK_TREE( ideas->tree )->children, save_to_ideas, fp );
  
  fclose( fp );
}

struct bool_char_pair
{
  gboolean boole;
  gchar *string;
};

static void raise_if_match( gpointer data, gpointer user_data )
{
  GimpImage *gimage = GIMP_IMAGE (data);
  struct bool_char_pair *pair = (struct bool_char_pair *) user_data;
  if ( ( ! pair->boole ) && gimage->has_filename)
    if ( strcmp( pair->string, gimage->filename ) == 0 )
      {
	pair->boole = TRUE;
	/*	gdk_raise_window( NULL, gimage-> ); */  /* FIXME */
      }
}

void open_or_raise( gchar *file_name )
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

gint open_or_raise_callback( GtkWidget *widget, GdkEventButton *event, gpointer func_data )
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

void idea_add( gchar *title )
{
  idea_add_in_position( title, 0 );
}

void idea_add_in_position_with_select( gchar *title, gint position, gboolean select )
{
  GtkWidget *treeitem;
  struct bool_char_pair pair;

  if ( ideas )
    {
      pair.boole = FALSE;
      pair.string = title;
      
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
	  
	  gtk_signal_connect( GTK_OBJECT( treeitem ),
			      "drop_data_available_event",
			      GTK_SIGNAL_FUNC( idea_cell_dnd_drop_data_available_callback ),
			      NULL );
	  
	  gtk_widget_show( treeitem );
	  
	  gtk_widget_dnd_drop_set( treeitem, TRUE, image_drop_types, 1, FALSE );
	  
	  if ( select )
	    gtk_tree_select_item( GTK_TREE( ideas->tree ), gtk_tree_child_position( GTK_TREE( ideas->tree ), treeitem ) );
	}
    }
}

void idea_add_in_position( gchar *title, gint position )
{
  idea_add_in_position_with_select( title, position, TRUE );
}

gint idea_move( GtkWidget *widget, gint distance, gboolean select )
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

void idea_remove( GtkWidget *widget )
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
	gtk_statusbar_push( GTK_STATUSBAR( ideas->status ), ideas->contextid, "This file cannot be moved up." );
    }
  else
    gtk_statusbar_push( GTK_STATUSBAR( ideas->status ), ideas->contextid, "There's no selection to move up." );
}

void idea_down_callback( GtkWidget *widget, gpointer data )
{
  GtkWidget *selected;
  if ( GTK_TREE( ideas->tree )->selection )
    {
      selected = GTK_TREE( ideas->tree )->selection->data;
      if ( idea_move( selected, 1, TRUE ) != 1 )
	gtk_statusbar_push( GTK_STATUSBAR( ideas->status ), ideas->contextid, "This file cannot be moved down." );
    }
  else
    gtk_statusbar_push( GTK_STATUSBAR( ideas->status ), ideas->contextid, "There's no selection to move down." );
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
    gtk_statusbar_push( GTK_STATUSBAR( ideas->status ), ideas->contextid, "There's no selection to remove." );
}
