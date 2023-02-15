/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

  gint        nr_off_mru_items;
} Menu_t;

Menu_t    *make_menu              (GimpImap       *imap);
void menu_build_mru_items         (MRU_t          *mru);
void menu_set_zoom_sensitivity    (gpointer        data,
                                   gint            factor);

void menu_set_zoom                (gpointer        data,
                                   gint            factor);
void menu_shapes_selected         (gint            count,
                                   gpointer        imap);

void do_main_popup_menu           (GdkEventButton *event,
                                   gpointer        data);

GtkWidget *make_selection_toolbar (GimpImap       *imap);

#endif /* _IMAP_MENU_H */
