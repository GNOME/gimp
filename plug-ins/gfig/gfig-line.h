/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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
 *
 */

#ifndef __GFIG_LINE_H__
#define __GFIG_LINE_H__

void       d_save_line      (Dobject     *obj,
                             GString     *string);

void       d_draw_line      (Dobject     *obj);
void       d_paint_line     (Dobject     *obj);
Dobject   *d_copy_line      (Dobject     *obj);

void       d_update_line    (GdkPoint    *pnt);
void       d_line_start     (GdkPoint    *pnt,
                             gint         shift_down);
void       d_line_end       (GdkPoint    *pnt,
                             gint         shift_down);
void       d_line_object_class_init (void);

#endif /* __GFIG_LINE_H__ */
