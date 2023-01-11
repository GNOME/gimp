/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2003  Simon Budig
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

#ifndef __STROKE_DIALOG_H__
#define __STROKE_DIALOG_H__


typedef void (* GimpStrokeCallback) (GtkWidget         *dialog,
                                     GList             *items,
                                     GList             *drawables,
                                     GimpContext       *context,
                                     GimpStrokeOptions *options,
                                     gpointer           user_data);


GtkWidget * stroke_dialog_new (GList              *items,
                               GList              *drawables,
                               GimpContext        *context,
                               const gchar        *title,
                               const gchar        *icon_name,
                               const gchar        *help_id,
                               GtkWidget          *parent,
                               GimpStrokeOptions  *options,
                               GimpStrokeCallback  callback,
                               gpointer            user_data);


#endif  /*  __STROKE_DIALOG_H__  */
