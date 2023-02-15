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

#ifndef _IMAP_GRID_H
#define _IMAP_GRID_H

void do_grid_settings_dialog (GSimpleAction     *action,
                              GVariant          *parameter,
                              gpointer           user_data);
void draw_grid               (cairo_t           *cr,
                              gint               width,
                              gint               height);
void toggle_grid             (GSimpleAction     *action,
                              GVariant          *new_state,
                              gpointer           user_data);
void round_to_grid           (gint              *x,
                              gint              *y);

gboolean grid_near_x         (gint               x);
gboolean grid_near_y         (gint               y);

#endif /* _IMAP_GRID_H */
