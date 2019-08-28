/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2003 Maurits Rijk  lpeek.mrijk@consunet.nl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.i
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef _IMAP_COMMANDS_H
#define _IMAP_COMMANDS_H

#include "imap_command.h"
#include "imap_object.h"
#include "imap_preview.h"

Command_t *clear_command_new(ObjectList_t *list);
Command_t *copy_command_new(ObjectList_t *list);
Command_t *copy_object_command_new(Object_t *obj);
Command_t *create_command_new(ObjectList_t *list, Object_t *obj);
Command_t *cut_command_new(ObjectList_t *list);
Command_t *cut_object_command_new(Object_t *obj);
Command_t *delete_command_new(ObjectList_t *list, Object_t *obj);
Command_t *delete_point_command_new(Object_t *obj, GdkPoint *point);
Command_t *edit_object_command_new(Object_t *obj);
Command_t *gimp_guides_command_new(ObjectList_t *list,
                                   GimpDrawable *drawable);
Command_t *guides_command_new(ObjectList_t *list);
Command_t *insert_point_command_new(Object_t *obj, gint x, gint y, gint edge);
Command_t *move_down_command_new(ObjectList_t *list);
Command_t *move_command_new(Preview_t *preview, Object_t *obj, gint x, gint y);
Command_t *move_sash_command_new(GtkWidget *widget, Object_t *obj,
                                 gint x, gint y, MoveSashFunc_t sash_func);
Command_t *move_selected_command_new(ObjectList_t *list, gint dx, gint dy);
Command_t *move_to_front_command_new(ObjectList_t *list);
Command_t *move_up_command_new(ObjectList_t *list);
Command_t *object_down_command_new(ObjectList_t *list, Object_t *obj);
Command_t *object_move_command_new(Object_t *obj, gint x, gint y);
Command_t *object_up_command_new(ObjectList_t *list, Object_t *obj);
Command_t *paste_command_new(ObjectList_t *list);
Command_t *select_all_command_new(ObjectList_t *list);
Command_t *select_command_new(Object_t *obj);
Command_t *select_next_command_new(ObjectList_t *list);
Command_t *select_prev_command_new(ObjectList_t *list);
Command_t *select_region_command_new(GtkWidget *widget, ObjectList_t *list,
                                     gint x, gint y);
Command_t *send_to_back_command_new(ObjectList_t *list);
Command_t *unselect_all_command_new(ObjectList_t *list, Object_t *exception);
Command_t *unselect_command_new(Object_t *obj);

#endif /* _IMAP_COMMANDS_H */
