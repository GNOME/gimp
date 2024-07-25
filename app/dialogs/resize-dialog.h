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

#ifndef __RESIZE_DIALOG_H__
#define __RESIZE_DIALOG_H__


typedef void (* GimpResizeCallback) (GtkWidget    *dialog,
                                     GimpViewable *viewable,
                                     GimpContext  *context,
                                     gint          width,
                                     gint          height,
                                     GimpUnit     *unit,
                                     gint          offset_x,
                                     gint          offset_y,
                                     gdouble       xres,
                                     gdouble       yres,
                                     GimpUnit     *res_unit,
                                     GimpFillType  fill_type,
                                     GimpItemSet   layer_set,
                                     gboolean      resize_text_layers,
                                     gpointer      user_data);


GtkWidget * resize_dialog_new (GimpViewable       *viewable,
                               GimpContext        *context,
                               const gchar        *title,
                               const gchar        *role,
                               GtkWidget          *parent,
                               GimpHelpFunc        help_func,
                               const gchar        *help_id,
                               GimpUnit           *unit,
                               GimpFillType        fill_type,
                               GimpItemSet         layer_set,
                               gboolean            resize_text_layers,
                               GimpResizeCallback  callback,
                               gpointer            user_data);


#endif  /*  __RESIZE_DIALOG_H__  */
