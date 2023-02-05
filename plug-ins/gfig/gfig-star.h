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

#ifndef __GFIG_STAR_H__
#define __GFIG_STAR_H__

void tool_options_star        (GtkWidget *notebook);

void d_star_object_class_init (void);

void d_star_start             (GdkPoint  *pnt,
                               gboolean   shift_down);
void d_star_end               (GimpGfig  *gfig,
                               GdkPoint  *pnt,
                               gboolean   shift_down);


#endif /* __GFIG_STAR_H__ */
