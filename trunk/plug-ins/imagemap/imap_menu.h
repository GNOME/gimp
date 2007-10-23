/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef _IMAP_MENU_H
#define _IMAP_MENU_H

#include "imap_command.h"
#include "imap_mru.h"

typedef struct {
  GtkWidget *file_menu;
  GtkWidget *edit_menu;
  GtkWidget *open_recent;
  GtkWidget *undo;
  GtkWidget *redo;
  GtkWidget *arrow;
  GtkWidget *fuzzy_select;
  GtkWidget *rectangle;
  GtkWidget *circle;
  GtkWidget *polygon;
  GtkWidget *grid;
  GtkWidget *gray;
  GtkWidget *color;
  GtkWidget *zoom[8];
  GtkWidget *zoom_in;
  GtkWidget *zoom_out;

  gint	      nr_off_mru_items;
} Menu_t;

GtkWidget *menu_get_widget(const gchar *path);
Menu_t *make_menu(GtkWidget *main_vbox, GtkWidget *window);
void menu_build_mru_items(MRU_t *mru);
void menu_set_zoom_sensitivity(gint factor);

void menu_set_zoom(gint factor);
void menu_check_grid(gboolean check);
void menu_shapes_selected(gint count);

void do_main_popup_menu(GdkEventButton *event);

GtkWidget *make_toolbar(GtkWidget *main_vbox, GtkWidget *window);
GtkWidget *make_tools(GtkWidget *window);
GtkWidget *make_selection_toolbar(void);

#endif /* _IMAP_MENU_H */
