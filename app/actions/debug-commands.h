/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 */

#ifndef __DEBUG_COMMANDS_H__
#define __DEBUG_COMMANDS_H__


#ifdef GIMP_UNSTABLE
#define ENABLE_DEBUG_MENU 1
#endif


#ifdef ENABLE_DEBUG_MENU

void   debug_mem_profile_cmd_callback        (GtkAction *action,
                                              gpointer   data);
void   debug_dump_menus_cmd_callback         (GtkAction *action,
                                              gpointer   data);
void   debug_dump_managers_cmd_callback      (GtkAction *action,
                                              gpointer   data);
void   debug_dump_attached_data_cmd_callback (GtkAction *action,
                                              gpointer   data);

#endif /* ENABLE_DEBUG_MENU */


#endif /* __DEBUG_COMMANDS_H__ */
