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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __OFFSET_DIALOG_H__
#define __OFFSET_DIALOG_H__


typedef void (* GimpOffsetCallback) (GtkWidget      *dialog,
                                     GimpDrawable   *drawable,
                                     GimpContext    *context,
                                     gboolean        wrap_around,
                                     GimpOffsetType  fill_type,
                                     gint            offset_x,
                                     gint            offset_y);


GtkWidget * offset_dialog_new (GimpDrawable       *drawable,
                               GimpContext        *context,
                               GtkWidget          *parent,
                               GimpOffsetCallback  callback,
                               gpointer            user_data);


#endif  /*  __OFFSET_DIALOG_H__  */
