/* docindex.h - Header file for the document index.
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

#ifndef __DOCINDEX_H__
#define __DOCINDEX_H__

#include <stdio.h>

#include <gtk/gtk.h>

typedef struct idea_manager
{
  /* The scrollbar */
  GtkWidget *vscrollbar,
    /* The GTK window */
    *window;
  GtkWidget *tree;
  /* The menufactory */
  GtkMenuFactory *menubar;
  /* The window menu widget */
  GtkWidget *window_menu;
  /* The status bar widget */
  GtkWidget *status;
  /* The main status context id */
  guint contextid;
  gboolean changed;
  gboolean named;
  gchar *title;
  gint auto_save;
  gint long_term_auto_save;
  gint count;
} idea_manager;

void raise_idea_callback (GtkWidget *widget, gpointer data);

void make_idea_window( gint x, gint y );
void open_idea_window( void );
void close_idea_window( void );
void idea_add( gchar *label );
void idea_add_in_position( gchar *label, gint position );
void idea_hide_callback( GtkWidget *widget, gpointer data );
void idea_up_callback( GtkWidget *widget, gpointer data );
void idea_down_callback( GtkWidget *widget, gpointer data );
void idea_remove_callback( GtkWidget *widget, gpointer data );
gboolean idea_window_delete_event_callback( GtkWidget *widget, GdkEvent *event, gpointer data );
void docindex_configure_drop_on_widget(GtkWidget * widget);

FILE  * idea_manager_parse_init (int  * window_x,
				 int  * window_y,
				 int  * window_width,
				 int  * window_height);
gchar * idea_manager_parse_line (FILE * fp);

void    load_idea_manager       (idea_manager *);
void    save_idea_manager       (idea_manager *);

extern idea_manager *ideas;

#endif /* __DOCINDEX_H__ */
