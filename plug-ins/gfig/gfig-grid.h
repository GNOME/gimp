/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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

#ifndef __GFIG_GRID_H__
#define __GFIG_GRID_H__

#define GFIG_NORMAL_GC -1
#define GFIG_BLACK_GC -2
#define GFIG_WHITE_GC -3
#define GFIG_GREY_GC  -4
#define GFIG_DARKER_GC -5
#define GFIG_LIGHTER_GC -6
#define GFIG_VERY_DARK_GC -7

#define MIN_GRID         10
#define MAX_GRID         50

extern gint grid_gc_type;

void gfig_grid_colors (GtkWidget *widget);
void find_grid_pos     (GdkPoint  *p,
                        GdkPoint  *gp,
                        guint      state);
void draw_grid         (cairo_t *cr);

#endif /* __GFIG_GRID_H__ */
