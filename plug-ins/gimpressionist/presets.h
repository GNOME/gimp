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

#ifndef __PRESETS_H
#define __PRESETS_H

enum SELECT_PRESET_RETURN_VALUES
{
    SELECT_PRESET_OK = 0,
    SELECT_PRESET_FILE_NOT_FOUND = -1,
    SELECT_PRESET_LOAD_FAILED = -2,
};

void create_presetpage (GtkNotebook *);
int select_preset (const gchar *preset);
void preset_free (void);
void preset_save_button_set_sensitive (gboolean s);

#endif

