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
  gchar *newstring = g_malloc( strlen( string1 ) + strlen( string2 ) + 1 );
  sprintf( newstring, "%s%s", string1, string2 );
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
