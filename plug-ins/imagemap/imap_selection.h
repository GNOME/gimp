/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2004 Maurits Rijk  m.rijk@chello.nl
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

#ifndef _IMAP_SELECTION_H
#define _IMAP_SELECTION_H

#include "imap_command.h"
#include "imap_object.h"

typedef struct {
  GtkListStore          *store;
  GtkTreeSelection      *selection;

  GtkWidget    *container;
  GtkWidget    *list;
  GtkWidget    *selected_child;
  ObjectList_t *object_list;
  gint          selected_row;
  gint          nr_rows;
  gboolean      is_visible;
  gboolean      select_lock;
  gboolean      doubleclick;

  CommandFactory_t cmd_move_up;
  CommandFactory_t cmd_move_down;
  CommandFactory_t cmd_delete;
  CommandFactory_t cmd_edit;
} Selection_t;

Selection_t *make_selection (ObjectList_t *list,
                             GimpImap     *imap);
void selection_toggle_visibility(Selection_t *selection);
void selection_freeze(Selection_t *selection);
void selection_thaw(Selection_t *selection);

#define selection_set_move_up_command(selection, command) \
        ((selection)->cmd_move_up = (command))
#define selection_set_move_down_command(selection, command) \
        ((selection)->cmd_move_down = (command))
#define selection_set_delete_command(selection, command) \
        ((selection)->cmd_delete = (command))
#define selection_set_edit_command(selection, command) \
        ((selection)->cmd_edit = (command))

#endif /* _IMAP_SELECTION_H */

