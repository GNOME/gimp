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

typedef struct
{
  GtkWidget *window;
  GtkWidget *tree;
} IdeaManager;

extern IdeaManager *ideas;

void document_index_create (void);

void close_idea_window     (void);

void idea_add              (gchar     *label);
void idea_add_in_position  (gchar     *label,
			    gint       position);

void idea_hide_callback    (GtkWidget *widget, gpointer data);
void idea_up_callback      (GtkWidget *widget, gpointer data);
void idea_down_callback    (GtkWidget *widget, gpointer data);
void idea_remove_callback  (GtkWidget *widget, gpointer data);

gboolean idea_window_delete_event_callback (GtkWidget *widget,
					    GdkEvent  *event,
					    gpointer   data);
void docindex_configure_drop_on_widget     (GtkWidget *widget);

FILE  * idea_manager_parse_init (void);
gchar * idea_manager_parse_line (FILE  *fp);

void    load_idea_manager       (IdeaManager *);
void    save_idea_manager       (IdeaManager *);

#endif /* __DOCINDEX_H__ */
