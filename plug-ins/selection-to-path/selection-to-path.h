/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Plugin to convert a selection to a path.
 *
 * Copyright (C) 1999 Andy Thomas  alt@gimp.org
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

#include "gtk/gtk.h"
#include "libgimp/gimp.h"

guchar          sel_pixel_value       (gint, gint);
gint            sel_pixel_is_white    (gint, gint);
gint            sel_get_width         (void);
gint            sel_get_height        (void);
gboolean        sel_valid_pixel       (gint, gint);
void            reset_adv_dialog      (void);


GtkWidget *     dialog_create_selection_area(SELVALS *);

