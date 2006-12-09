/* GIMP - The GNU Image Manipulation Program
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

#ifndef __DOCKABLE_COMMANDS_H__
#define __DOCKABLE_COMMANDS_H__


void   dockable_add_tab_cmd_callback         (GtkAction   *action,
                                              const gchar *value,
                                              gpointer     data);
void   dockable_close_tab_cmd_callback       (GtkAction   *action,
                                              gpointer     data);
void   dockable_detach_tab_cmd_callback      (GtkAction   *action,
                                              gpointer     data);

void   dockable_toggle_view_cmd_callback     (GtkAction   *action,
                                              GtkAction   *current,
                                              gpointer     data);
void   dockable_view_size_cmd_callback       (GtkAction   *action,
                                              GtkAction   *current,
                                              gpointer     data);
void   dockable_tab_style_cmd_callback       (GtkAction   *action,
                                              GtkAction   *current,
                                              gpointer     data);
void   dockable_show_button_bar_cmd_callback (GtkAction   *action,
                                              gpointer     data);


#endif /* __DOCKABLE_COMMANDS_H__ */
