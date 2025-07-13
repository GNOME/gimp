/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 */

#pragma once


extern GimpDialogFactory *global_dialog_factory;
extern GimpContainer     *global_recent_docks;


void        dialogs_init              (Gimp            *gimp);
void        dialogs_exit              (Gimp            *gimp);

void        dialogs_load_recent_docks (Gimp            *gimp);
void        dialogs_save_recent_docks (Gimp            *gimp);

GtkWidget * dialogs_get_toolbox       (void);


/* attaching dialogs to arbitrary objects, and detaching them
 * automatically upon destruction
 */
GtkWidget * dialogs_get_dialog        (GObject         *attach_object,
                                       const gchar     *attach_key);
void        dialogs_attach_dialog     (GObject         *attach_object,
                                       const gchar     *attach_key,
                                       GtkWidget       *dialog);
void        dialogs_detach_dialog     (GObject         *attach_object,
                                       GtkWidget       *dialog);
void        dialogs_destroy_dialog    (GObject         *attach_object,
                                       const gchar     *attach_key);
